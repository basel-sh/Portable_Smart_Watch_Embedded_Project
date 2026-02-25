#ifndef HEART_SENSOR_H
#define HEART_SENSOR_H

#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
unsigned long lastBeat = 0; // Fixed: unsigned long for millis() math
float beatsPerMinute = 0;
int beatAvg = 0;

bool initHeartSensor() {
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 not found");
    return false;
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A); // Required for SpO2 later
  particleSensor.setPulseAmplitudeGreen(0);
  particleSensor.setPulseAmplitudeIR(0x0A);  // Explicitly define IR power

  return true;
}

long updateHeartRate() {
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue)) {
    unsigned long delta = millis() - lastBeat;
    lastBeat = millis();
    
    beatsPerMinute = 60000.0 / (float)delta; // Fixed: Optimized division

    if (beatsPerMinute > 20 && beatsPerMinute < 255) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++) {
        beatAvg += rates[x];
      }
      beatAvg /= RATE_SIZE;
    }
  }

  return irValue;
}

float getBPM() {
  return beatsPerMinute;
}

int getAvgBPM() {
  return beatAvg;
}

#endif