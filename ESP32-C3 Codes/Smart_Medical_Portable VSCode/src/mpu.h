#ifndef MPU_SENSOR_H
#define MPU_SENSOR_H

#include <Wire.h>

#define MPU_ADDR 0x68
#define BUZZER_PIN 10   // Ensure GPIO 10 is free on your ESP32

int16_t AcX, AcY, AcZ;

bool freeFallDetected = false;
bool impactDetected = false;

unsigned long freeFallTime = 0;
unsigned long impactTime = 0;

// thresholds
const float FREEFALL_THRESHOLD = 0.6;   
const float IMPACT_THRESHOLD = 2.7;     
const float STILL_TOLERANCE = 0.15;     

const int STILL_COUNT_REQUIRED = 10;
int stillCounter = 0;

bool initMPU() {
  // Wake MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  if (Wire.endTransmission(true) != 0) {
    Serial.println("MPU6050 not found");
    return false;
  }

  // Configure Accelerometer to ±8g range
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);
  Wire.write(0x10); 
  Wire.endTransmission(true);

  pinMode(BUZZER_PIN, OUTPUT);  
  digitalWrite(BUZZER_PIN, LOW);
  
  Serial.println("MPU6050 Initialized");
  return true;
}

// Pass the mutex from main.cpp into this function so it plays nice with other sensors
void processFallDetection(SemaphoreHandle_t mutex) {
  
  // 1. Thread-Safe I2C Read
  if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom((uint16_t)MPU_ADDR, (uint8_t)6, true);

    AcX = Wire.read() << 8 | Wire.read();
    AcY = Wire.read() << 8 | Wire.read();
    AcZ = Wire.read() << 8 | Wire.read();
    
    xSemaphoreGive(mutex); // Release bus for Heart/Temp sensors
  } else {
    return; // Could not get the I2C bus, skip this cycle
  }

  // 2. Math & Scale (Divisor 4096.0 for ±8g range)
  float ax = AcX / 4096.0;
  float ay = AcY / 4096.0;
  float az = AcZ / 4096.0;
  float accMag = sqrt(ax*ax + ay*ay + az*az);

  unsigned long now = millis();

  // Stage 1: Free fall
  if (!freeFallDetected && accMag < FREEFALL_THRESHOLD) {
    freeFallDetected = true;
    freeFallTime = now;
    Serial.println("Free fall detected");
  }

  // Stage 2: Impact
  if (freeFallDetected && !impactDetected && accMag > IMPACT_THRESHOLD) {
    impactDetected = true;
    impactTime = now;
    Serial.println("Impact detected");
  }

  // Stage 3: Motionless confirmation
  if (impactDetected) {
    if (abs(accMag - 1.0) < STILL_TOLERANCE) {
      stillCounter++;
    } else {
      stillCounter = 0;
    }

    if (stillCounter > STILL_COUNT_REQUIRED) {
      Serial.println("FALL DETECTED!");
      
      // Activate buzzer (using RTOS delay so heart rate keeps running in background)
      digitalWrite(BUZZER_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(2000)); 
      digitalWrite(BUZZER_PIN, LOW);

      // Reset states
      freeFallDetected = false;
      impactDetected = false;
      stillCounter = 0;
    }

    // Timeout
    if (now - impactTime > 3000) {
      freeFallDetected = false;
      impactDetected = false;
      stillCounter = 0;
    }
  }
}

#endif