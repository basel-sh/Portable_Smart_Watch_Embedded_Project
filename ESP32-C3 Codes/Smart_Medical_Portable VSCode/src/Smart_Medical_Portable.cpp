#include <Arduino.h>
#include <Wire.h>
#include "temp_sensor.h"
#include "heart_sensor.h"
#include "ble_manager.h"
#include "mpu.h"
#include "BME680.h"


#define altitudediff -0.55f
#define fallValidationStart 2000
#define BUZZER_PIN 10

// --- Serial Debug Flags ---
bool MUTE_ALL_SERIAL = false; 
bool DEBUG_HEART     = true;  
bool DEBUG_TEMP      = true;  
bool RAW_CSV_MODE    = false; 

SemaphoreHandle_t i2cMutex;

// --- BPM Smoothing Global Variables ---
float smoothedBPM = 0.0;
const float BPM_ALPHA = 0.3; 
int finalDisplayBPM = 0; 

// --- SpO2 Smoothing Global Variables ---
float smoothedSpO2 = 0.0;
// A very low alpha (0.1) because blood oxygen changes very slowly in reality
const float SPO2_ALPHA = 0.1; 
int finalDisplaySpO2 = 0;

// Cross-Task Communication Variables
volatile int sharedBPM = 0;
volatile int sharedSpO2 = 0;
volatile int sharedStatus = 0;

void TaskHeartRate(void *pvParameters);
void TaskTemperature(void *pvParameters);
void TaskFallDetection(void *pvParameters);
void TaskEnvironment(void *pvParameters);

void setup() {
  Serial.begin(115200);

  Wire.begin(8, 9);
  Wire.setClock(100000);

  initTempSensor();
  initHeartSensor();
  initMPU();
  initBME680();

  // --- Initialize Buzzer ---
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure it starts off

  i2cMutex = xSemaphoreCreateMutex();

  if (i2cMutex != NULL) {
    xTaskCreate(TaskFallDetection, "FallTask", 2048, NULL, 3, NULL);
    xTaskCreate(TaskHeartRate, "HeartTask", 8192, NULL, 1, NULL);
    xTaskCreate(TaskTemperature, "TempTask", 2048, NULL, 2, NULL);
    xTaskCreate(TaskEnvironment, "EnvTask", 8192, NULL, 1, NULL); 
  } else {
    Serial.println("Error: Failed to create Mutex");
  }
  initBLE();
}

void loop() {
  vTaskDelay(portMAX_DELAY); 
}

// --- TASK DEFINITIONS ---

void TaskHeartRate(void *pvParameters) {
  for (;;) {
    // Pass the current acceleration magnitude into the ANC filter
    processHeartData(i2cMutex, currentAccMag); 

    if (validHeartRate || validSPO2) {
      sharedBPM = heartRate; 
      sharedSpO2 = spo2;
      sharedStatus = (isUserMoving) ? 2 : 1; // 2 still means 'motion detected' but we are still calculating
    } else {
      sharedBPM = 0; sharedSpO2 = 0; sharedStatus = 0; 
    }
    vTaskDelay(pdMS_TO_TICKS(20)); 
  }
}

void TaskTemperature(void *pvParameters) {
  for (;;) {
    float objectTemp = 0.0;

    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
      objectTemp = readTemperature();
      xSemaphoreGive(i2cMutex);
    }

    if (isnan(objectTemp)) {
      objectTemp = 0.0; 
    }

    // --- UPDATED TELEMETRY PRINTING ---
    if (!MUTE_ALL_SERIAL) {
      if (RAW_CSV_MODE) {
        // CSV Format: BPM, SpO2, SkinTemp, Status, IAQ, Humidity, Gas
        Serial.printf("%d,%d,%.2f,%d,%.2f,%.2f,%.2f\n", 
                      sharedBPM, sharedSpO2, objectTemp, sharedStatus, 
                      sharedIAQ, sharedHumidity, sharedGas);
      } else {
        if (DEBUG_HEART) {
          Serial.printf("BPM: %d | SpO2: %d%% | Status: %d", 
                        sharedBPM, sharedSpO2, sharedStatus);
        }
        
        if (DEBUG_HEART && DEBUG_TEMP) Serial.print("  ||  "); 
        
        if (DEBUG_TEMP) {
          Serial.printf("Skin Temp: %.2fC", objectTemp);
        }
        
        // --- NEW: Humidity and Gas/IAQ display ---
        Serial.printf("  ||  Hum: %.1f%% | IAQ: %.1f\n", 
                      sharedHumidity, sharedIAQ);
      }
    }
    
    // Broadcast to BLE (Passing 4 arguments)
    sendBLEData(sharedBPM, objectTemp, sharedSpO2);

    vTaskDelay(pdMS_TO_TICKS(1000)); 
  }
}

// --- Add these variables above your task ---
unsigned long fallValidationStartTime = 0; // Renamed to avoid mpu.h collision
bool pendingFallValidation = false;

void TaskFallDetection(void *pvParameters) {
  for (;;) {
    // 1. Check for instantaneous physical impact (Now returning a bool!)
    bool mpuDetectedImpact = processFallDetection(i2cMutex); 

    // 2. If we feel an impact, open the validation window
    if (mpuDetectedImpact && !pendingFallValidation) {
        fallValidationStartTime = millis();
        pendingFallValidation = true;
        Serial.println("MPU Impact Detected! Waiting for Barometric validation...");
    }

    // 3. The Validation Window (Check for the next 1000ms after impact)
    if (pendingFallValidation) {
        
        // Did the altitude drop confirm it?
       if (sharedAltitudeDrop < altitudediff) {
            Serial.println("=========================================");
            Serial.println("!!! CRITICAL FALL CONFIRMED !!!");
            Serial.print("Altitude Drop: ");
            Serial.print(sharedAltitudeDrop);
            Serial.println(" meters");
            Serial.println("=========================================");
            
            // --- Activate the Buzzer ---
            digitalWrite(BUZZER_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(2000)); // Ring for 2 seconds
            digitalWrite(BUZZER_PIN, LOW);
            
            // --- TODO: Trigger BLE Emergency Alert Here ---
            
            pendingFallValidation = false; // Reset state machine
        }
        
        // Timeout: If 1 second passes and the altitude never dropped
        else if (millis() - fallValidationStartTime > fallValidationStart) {
            Serial.println("False Alarm Rejected: Impact felt, but no altitude drop.");
            pendingFallValidation = false; // Reset state machine
        }
    }
    
    vTaskDelay(pdMS_TO_TICKS(40)); 
  }
}

void TaskEnvironment(void *pvParameters) {
  for (;;) {
    // Triggers the BSEC2 state machine. Uses SPI, so no Mutex needed!
    processBME680(); 
    
    // Yield the CPU. BSEC2 only outputs new data every 3 seconds, 
    // but polling it every 50ms ensures the DSP timing loop stays perfectly aligned.
    vTaskDelay(pdMS_TO_TICKS(50)); 
  }
}