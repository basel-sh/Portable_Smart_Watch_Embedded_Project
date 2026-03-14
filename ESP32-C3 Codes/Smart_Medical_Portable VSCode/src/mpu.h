#ifndef MPU_SENSOR_H
#define MPU_SENSOR_H

#include <Wire.h>

#define MPU_ADDR 0x68
#define BUZZER_PIN 10   

int16_t AcX, AcY, AcZ;

// ANC & Fall Detection Variables
volatile bool isUserMoving = false;
volatile float currentAccMag = 1.0f;
#define MOTION_THRESHOLD 0.15f // Deviation from 1.0g to trigger 'Moving' state

bool freeFallDetected = false;
bool impactDetected = false;
unsigned long freeFallTime = 0;
unsigned long impactTime = 0;

const float FREEFALL_THRESHOLD = 0.6;   
const float IMPACT_THRESHOLD = 2.7;     
const float STILL_TOLERANCE = 0.15;     

const int STILL_COUNT_REQUIRED = 10;
int stillCounter = 0;

bool initMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  if (Wire.endTransmission(true) != 0) {
    Serial.println("MPU6050 not found");
    return false;
  }

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);
  Wire.write(0x10); 
  Wire.endTransmission(true);

  pinMode(BUZZER_PIN, OUTPUT);  
  digitalWrite(BUZZER_PIN, LOW);
  
  Serial.println("MPU6050 Initialized");
  return true;
}

bool processFallDetection(SemaphoreHandle_t mutex) {
  if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom((uint16_t)MPU_ADDR, (uint8_t)6, true);

    AcX = Wire.read() << 8 | Wire.read();
    AcY = Wire.read() << 8 | Wire.read();
    AcZ = Wire.read() << 8 | Wire.read();
    xSemaphoreGive(mutex);
  } else {
    return false;
  }

  float ax = AcX / 4096.0;
  float ay = AcY / 4096.0;
  float az = AcZ / 4096.0;
  currentAccMag = sqrt(ax*ax + ay*ay + az*az);

  // --- MOTION ANC GATE ---
  if (abs(currentAccMag - 1.0f) > MOTION_THRESHOLD) {
      isUserMoving = true;
  } else {
      isUserMoving = false;
  }

  unsigned long now = millis();

  if (!freeFallDetected && currentAccMag < FREEFALL_THRESHOLD) {
    freeFallDetected = true;
    freeFallTime = now;
  }

  if (freeFallDetected && !impactDetected && currentAccMag > IMPACT_THRESHOLD) {
    impactDetected = true;
    impactTime = now;
  }

  if (impactDetected) {
    if (abs(currentAccMag - 1.0) < STILL_TOLERANCE) {
      stillCounter++;
    } else {
      stillCounter = 0;
    }

    if (stillCounter > STILL_COUNT_REQUIRED) {
      freeFallDetected = false;
      impactDetected = false;
      stillCounter = 0;
      return true; 
    }

    if (now - impactTime > 3000) {
      freeFallDetected = false;
      impactDetected = false;
      stillCounter = 0;
    }
  }
  return false;
}

#endif