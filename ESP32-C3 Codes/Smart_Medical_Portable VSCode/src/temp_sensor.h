#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <Adafruit_MLX90614.h>

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// EMA Filter Variables
float smoothedTemp = 0.0;
const float ALPHA = 0.8; // Smoothing factor (0.0 to 1.0). Lower = smoother.
bool firstRead = true;
float estimatedCoreTemp =0.0;
bool initTempSensor() {
  if (!mlx.begin()) {
    Serial.println("MLX90614 not found");
    return false;
  }
  return true;
}

float readTemperature() {
  // 1. Read Raw Data
  float skinTemp = mlx.readObjectTempC();
  float ambientTemp = mlx.readAmbientTempC();

  // If sensor is erroring out, return NaN immediately
  if (isnan(skinTemp) || isnan(ambientTemp)) {
    return NAN;
  }

  // 2. Emissivity Correction (Default is 1.0, Skin is 0.98)
  // Simplified software approximation for slight emissivity mismatch
  skinTemp = skinTemp / 0.98; 

  // 3. Clinical Core Temperature Estimation
  // A basic linear compensation model. 
  // You will need to tune the 'offset' based on where the watch sits on the body.
  float coreTempOffset = 2;
  
  if (skinTemp > 29.0 && skinTemp < 38.0) {
    // Normal human range: skin is usually 2-4 degrees cooler than core depending on ambient
  coreTempOffset = (37.0 - skinTemp) * 1 + ((25.0 - ambientTemp) * 0.2);
  }
  
  float estimatedCoreTemp = skinTemp + coreTempOffset;

  // 4. Exponential Moving Average (EMA) Filter
  if (firstRead) {
    smoothedTemp = estimatedCoreTemp; // Seed the filter
    firstRead = false;
  } else {
    smoothedTemp = (ALPHA * estimatedCoreTemp) + ((1.0 - ALPHA) * smoothedTemp);
  }

  return smoothedTemp;
}

#endif