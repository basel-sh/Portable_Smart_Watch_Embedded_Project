#include <Wire.h>
#include "temp_sensor.h"
#include "heart_sensor.h"

unsigned long lastTempRequest = 0;
const long tempInterval = 200;

void setup() {
  Serial.begin(115200);

  Wire.begin(8, 9);
  Wire.setClock(100000);

  initTempSensor();
  initHeartSensor();
}

void loop() {

  // High-frequency heart update
  long irValue = updateHeartRate();

  // Timed printing
  if (millis() - lastTempRequest > tempInterval) {

    float objectTemp = readTemperature();

    // Heart rate output
    if (irValue < 50000) {
      Serial.print("IR=");
      Serial.print(irValue);
      Serial.print(" | [ No finger ]");
    } else {
      Serial.print("IR=");
      Serial.print(irValue);
      Serial.print(" | BPM=");
      Serial.print(getBPM());
      Serial.print(" | Avg BPM=");
      Serial.print(getAvgBPM());
    }

    // Temperature output
    Serial.print(" | Temp=");
    if (isnan(objectTemp)) {
      Serial.println(" Error");
    } else {
      Serial.print(objectTemp);
      Serial.println(" C");
    }

    lastTempRequest = millis();
  }
}