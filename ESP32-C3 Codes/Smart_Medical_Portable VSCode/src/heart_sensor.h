#ifndef HEART_SENSOR_H
#define HEART_SENSOR_H

#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h" // Swapped to the clinical SpO2 library

MAX30105 particleSensor;

// The Maxim algorithm requires exactly 100 samples
#define SPO2_BUFFER_SIZE 100
uint32_t irBuffer[SPO2_BUFFER_SIZE];
uint32_t redBuffer[SPO2_BUFFER_SIZE];

// Algorithm Output Variables
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

bool initHeartSensor() {
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 not found");
    return false;
  }

  // Critical SpO2 Configuration:
  // LED Power: 0x3F (~12.8mA - Good for penetrating tissue)
  // Sample Average: 4
  // LED Mode: 2 (Red + IR)
  // Sample Rate: 100 (100Hz / 4 avg = 25Hz effective output)
  // Pulse Width: 411 (18-bit resolution)
  // ADC Range: 4096
  particleSensor.setup(0x3F, 4, 2, 100, 411, 4096); 
  
  return true;
}

// RTOS-Safe Chunk Reader: Grabs whatever is in the FIFO right now
int readFIFOChunks(int startIndex, int maxSamplesToRead) {
  particleSensor.check(); // Tell sensor to ready its data
  int available = particleSensor.available();
  int samplesRead = 0;

  while (available > 0 && samplesRead < maxSamplesToRead) {
    redBuffer[startIndex + samplesRead] = particleSensor.getFIFORed();
    irBuffer[startIndex + samplesRead] = particleSensor.getFIFOIR();
    particleSensor.nextSample(); // Clear the space in the sensor's memory
    
    available--;
    samplesRead++;
  }
  return samplesRead;
}

// Wrapper to trigger the heavy DSP math
void calculateSpO2() {
  maxim_heart_rate_and_oxygen_saturation(
    irBuffer, SPO2_BUFFER_SIZE, 
    redBuffer, 
    &spo2, &validSPO2, 
    &heartRate, &validHeartRate
  );
}

// The Sliding Window: Shift the newest 75 samples down to make room for 25 new ones
void shiftBufferDown() {
  for (byte i = 25; i < SPO2_BUFFER_SIZE; i++) {
    redBuffer[i - 25] = redBuffer[i];
    irBuffer[i - 25] = irBuffer[i];
  }
}

#endif