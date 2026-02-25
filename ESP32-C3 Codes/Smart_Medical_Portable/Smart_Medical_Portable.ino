#include <Wire.h>
#include "temp_sensor.h"
#include "heart_sensor.h"

// 1. Declare the I2C Mutex
SemaphoreHandle_t i2cMutex;

// 2. Cross-Task Communication Variables
// "volatile" tells the compiler these might change at any time from a different core
volatile long sharedIrValue = 0;
volatile int sharedBPM = 0;
volatile int sharedAvgBPM = 0;
volatile int sharedStatus = 0;

// Task Function Prototypes
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
    // Standard xTaskCreate for Single-Core ESP32-C3
    xTaskCreate(
      TaskHeartRate,    // Function to run
      "HeartTask",      // Task name
      4096,             // Stack size
      NULL,             // Parameters
      1,                // Priority (Lower)
      NULL              // Task handle
    );

    xTaskCreate(
      TaskTemperature,  
      "TempTask",       
      2048,             
      NULL,             
      2,                // Priority (Higher, ensures strict 200ms timing)
      NULL              
    );
  } else {
    Serial.println("Error: Failed to create Mutex");
  }
}

void loop() {
  // In an RTOS architecture, the main loop is left empty.
  // The FreeRTOS scheduler takes over completely.
  vTaskDelay(portMAX_DELAY); 
}

// --- TASK DEFINITIONS ---

void TaskHeartRate(void *pvParameters) {
  for (;;) {
    // Wait up to max time to grab the I2C bus token
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
      
      // We hold the token! Safe to talk to MAX30105
      long irValue = updateHeartRate(); 
      
      // Done talking, give the token back immediately
      xSemaphoreGive(i2cMutex);

      // Process the logic locally
      int status = 0;
      int currentBPM = 0;
      int avgBPM = 0;

      if (irValue > 50000) {
        status = 1;
        currentBPM = (int)getBPM();
        avgBPM = getAvgBPM();
      }

      // Update the global variables for the Temp task to read
      sharedIrValue = irValue;
      sharedBPM = currentBPM;
      sharedAvgBPM = avgBPM;
      sharedStatus = status;
    }
    
    // Yield for 10ms to prevent the task from locking up Core 0
    vTaskDelay(pdMS_TO_TICKS(10)); 
  }
}

void TaskTemperature(void *pvParameters) {
  for (;;) {
    float objectTemp = 0.0;

    // Grab the I2C bus token to read the MLX90614
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
      objectTemp = readTemperature();
      xSemaphoreGive(i2cMutex);
    }

    // --- Timed Printing (Strict Integer CSV Format) ---
    // Serial printing does not require the I2C Mutex
    Serial.print(sharedIrValue);
    Serial.print(",");
    Serial.print(sharedBPM);
    Serial.print(",");
    Serial.print(sharedAvgBPM);
    Serial.print(",");
    
    if (isnan(objectTemp)) {
      Serial.print("0.00"); 
    } else {
      Serial.print(objectTemp);
    }
    
    Serial.print(",");
    Serial.println(sharedStatus); // Terminates the packet

    // Block this task for EXACTLY 200ms. FreeRTOS guarantees this timing.
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}