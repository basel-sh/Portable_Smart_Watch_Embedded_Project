#include <Wire.h>
#include "temp_sensor.h"
#include "heart_sensor.h"
#include "ble_manager.h"

// --- Serial Debug Flags ---
bool MUTE_ALL_SERIAL = true; // Master switch: Set to 'true' to hide absolutely everything
bool DEBUG_HEART     = true;  // Set to 'true' to print Heart Rate and SpO2
bool DEBUG_TEMP      = true;  // Set to 'true' to print Temperature
bool RAW_CSV_MODE    = true; // Set to 'true' to disable text and ONLY print "BPM,SpO2,Temp,Status"

SemaphoreHandle_t i2cMutex;

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
    // Increased stack size to 8192 for the SpO2 algorithm arrays
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
    int timeoutCounter = 0; // Added a timeout tracker

    // Fill the buffer using Mutex Chunking
    while (samplesRead < samplesNeeded) {
      int read = 0;
      if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        read = readFIFOChunks(startIndex + samplesRead, samplesNeeded - samplesRead);
        xSemaphoreGive(i2cMutex);
      }
      
      samplesRead += read;

      // Hardware Failure Detection
      if (read == 0) {
        timeoutCounter++; 
      } else {
        timeoutCounter = 0; // Reset if we successfully read data
      }

      // If we wait 1 full second (100 loops * 10ms) with NO data, the sensor is dead
      if (timeoutCounter > 100) {
        break; // Break out of the infinite while loop
      }

      vTaskDelay(pdMS_TO_TICKS(10)); 
    }

    // Handle the Failure State
    if (timeoutCounter > 100) {
      // Sensor disconnected or failed! Force everything to 0.
      sharedBPM = 0;
      sharedSpO2 = 0;
      sharedStatus = 0;
      
      firstFill = true; // Force it to try a full 100-sample reboot when it reconnects
      vTaskDelay(pdMS_TO_TICKS(1000)); // Wait a second before polling the dead bus again
      continue; // Skip the heavy SpO2 math and restart the for(;;) loop
    }

    // If we reach here, hardware is healthy. Run the SpO2 Math!
    calculateSpO2();

    if (validHeartRate == 1 && validSPO2 == 1 && irBuffer[50] > 50000) {
      sharedBPM = heartRate;
      sharedSpO2 = spo2;
      sharedStatus = 1;
    } else {
      sharedBPM = 0;
      sharedSpO2 = 0;
      sharedStatus = 0; // Finger removed (but hardware is healthy)
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

    // Hardware Failure Detection for MLX90614
    if (isnan(objectTemp)) {
      objectTemp = 0.0; 
    }

    // --- Smart Serial Output Logic ---
    if (!MUTE_ALL_SERIAL) {
      
      if (RAW_CSV_MODE) {
        // App-Mode: Strict CSV format
        Serial.print(sharedBPM); Serial.print(",");
        Serial.print(sharedSpO2); Serial.print(",");
        Serial.print(objectTemp); Serial.print(",");
        Serial.println(sharedStatus);
        
      } else {
        // Debug-Mode: Human readable text
        if (DEBUG_HEART) {
          Serial.print("BPM: "); Serial.print(sharedBPM);
          Serial.print(" | SpO2: "); Serial.print(sharedSpO2);
          Serial.print("% | Status: "); Serial.print(sharedStatus);
        }
        
        if (DEBUG_HEART && DEBUG_TEMP) {
          Serial.print("  ||  "); // Separator if both are printing
        }
        
        if (DEBUG_TEMP) {
          Serial.print("Temp: "); Serial.print(objectTemp); Serial.print("C");
        }
        
        if (DEBUG_HEART || DEBUG_TEMP) {
          Serial.println(); // End the line properly
        }
      }
    }

    // Send payload to the phone via BLE
    sendBLEData(sharedBPM, objectTemp, sharedSpO2);

    vTaskDelay(pdMS_TO_TICKS(1000)); 
  }
}