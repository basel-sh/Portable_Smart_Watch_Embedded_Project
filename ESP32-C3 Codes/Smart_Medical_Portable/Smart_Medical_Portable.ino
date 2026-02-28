
#include <Wire.h>
#include "temp_sensor.h"
#include "heart_sensor.h"
#include "ble_manager.h"

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

// Cross-Task Communication Variables
volatile int sharedBPM = 0;
volatile int sharedSpO2 = 0;
volatile int sharedStatus = 0;

void TaskHeartRate(void *pvParameters);
void TaskTemperature(void *pvParameters);

void setup() {
  Serial.begin(115200);

  Wire.begin(8, 9);
  Wire.setClock(100000);

  initTempSensor();
  initHeartSensor();

  i2cMutex = xSemaphoreCreateMutex();

  if (i2cMutex != NULL) {
    xTaskCreate(TaskHeartRate, "HeartTask", 8192, NULL, 1, NULL);
    xTaskCreate(TaskTemperature, "TempTask", 2048, NULL, 2, NULL);
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
  bool firstFill = true;

  for (;;) {
    int samplesNeeded = firstFill ? 100 : 25;
    int startIndex = firstFill ? 0 : 75;
    int samplesRead = 0;
    int timeoutCounter = 0; 

    while (samplesRead < samplesNeeded) {
      int read = 0;
      if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        read = readFIFOChunks(startIndex + samplesRead, samplesNeeded - samplesRead);
        xSemaphoreGive(i2cMutex);
      }
      
      samplesRead += read;

      if (read == 0) {
        timeoutCounter++; 
      } else {
        timeoutCounter = 0; 
      }

      if (timeoutCounter > 100) {
        break; 
      }

      vTaskDelay(pdMS_TO_TICKS(10)); 
    }

    if (timeoutCounter > 100) {
      sharedBPM = 0;
      sharedSpO2 = 0;
      sharedStatus = 0;
      smoothedBPM = 0.0; // Reset filter memory on hardware crash
      
      firstFill = true; 
      vTaskDelay(pdMS_TO_TICKS(1000)); 
      continue; 
    }

    // Run the SpO2 Math
    calculateSpO2();
    
    // 1. Silent BPM Smoothing Filter
    if (validHeartRate == 1) { 
      if (heartRate > 40 && heartRate < 160) {
        if (smoothedBPM == 0.0) {
          smoothedBPM = heartRate; 
        } else {
          smoothedBPM = (BPM_ALPHA * heartRate) + ((1.0 - BPM_ALPHA) * smoothedBPM);
        }
        finalDisplayBPM = (int)smoothedBPM; 
      }
    }
    
    // 2. Global Variable Assignment
    if (validHeartRate == 1 && validSPO2 == 1 && irBuffer[50] > 50000) {
      sharedBPM = finalDisplayBPM; // FIXED: Now routing the smooth data to BLE/CSV!
      sharedSpO2 = spo2;
      sharedStatus = 1;
    } else {
      sharedBPM = 0;
      sharedSpO2 = 0;
      sharedStatus = 0; 
      smoothedBPM = 0.0;     // FIXED: Erase filter history when finger is removed
      finalDisplayBPM = 0;
    }

    shiftBufferDown();
    firstFill = false;
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

    if (!MUTE_ALL_SERIAL) {
      if (RAW_CSV_MODE) {
        Serial.print(sharedBPM); Serial.print(",");
        Serial.print(sharedSpO2); Serial.print(",");
        Serial.print(objectTemp); Serial.print(",");
        Serial.println(sharedStatus);
      } else {
        if (DEBUG_HEART) {
          // Both finalDisplayBPM and sharedBPM are now identical, so either works here
          Serial.print("BPM: "); Serial.print(sharedBPM);
          Serial.print(" | SpO2: "); Serial.print(sharedSpO2);
          Serial.print("% | Status: "); Serial.print(sharedStatus);
        }
        
        if (DEBUG_HEART && DEBUG_TEMP) {
          Serial.print("  ||  "); 
        }
        
        if (DEBUG_TEMP) {
          Serial.print("Temp: "); Serial.print(objectTemp); Serial.print("C");
        }
        
        if (DEBUG_HEART || DEBUG_TEMP) {
          Serial.println(); 
        }
      }
    }

    // FIXED: Added sharedStatus as the 4th argument to match ble_manager.h
    sendBLEData(sharedBPM, objectTemp, sharedSpO2);

    vTaskDelay(pdMS_TO_TICKS(1000)); 
  }
}