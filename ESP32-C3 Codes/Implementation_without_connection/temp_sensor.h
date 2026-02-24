#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <Adafruit_MLX90614.h>

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

bool initTempSensor() {
  if (!mlx.begin()) {
    Serial.println("MLX90614 not found");
    return false;
  }
  return true;
}

float readTemperature() {
  return mlx.readObjectTempC();
}

#endif