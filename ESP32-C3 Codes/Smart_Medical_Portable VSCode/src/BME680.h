#ifndef BME_SENSOR_H
#define BME_SENSOR_H

#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

// --- ESP32-C3 Mini Hardware SPI Pin ---
#define BME_CS 7 
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme(BME_CS); 

// --- Global Variables for main.cpp ---
volatile float sharedTemp = 0.0;
volatile float sharedHumidity = 0.0;
volatile float sharedPressure = 0.0;
volatile float sharedAltitude = 0.0;
volatile float sharedGas = 0.0; 
volatile float sharedIAQ = 0.0; 

// --- Fall Detection Delta Altitude ---
volatile float sharedAltitudeDrop = 0.0; 

#define ALT_HISTORY_SIZE 10 // 10 samples @ 100ms = 1 second history
float altHistory[ALT_HISTORY_SIZE] = {0};
int altHistoryIndex = 0;
unsigned long lastAltHistoryTime = 0;

// --- High-Speed EMA Filter ---
float smoothedAltitude = 0.0;
// 0.8 = Extremely fast response, minimal lag. Trusts new data 80%.
#define ALTITUDE_EMA_ALPHA 0.8f

// --- Async & Hybrid Timer Variables ---
unsigned long bmeEndTime = 0;
bool isReadingBME = false;
const unsigned long GAS_INTERVAL_MS = 30000; // 30 seconds
unsigned long lastGasTime = 0;
bool isGasCycle = false;

bool initBME680() {
    SPI.begin();

    if (!bme.begin()) {
        Serial.println("Could not find a valid BME680 sensor, check wiring!");
        return false;
    }

    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    
    // Start with heater OFF for high-speed altimeter polling
    bme.setGasHeater(0, 0); 

    Serial.println("BME680 Hybrid Initialized (Fast EMA + Watchdog Active)");
    return true;
}

void processBME680() {
    // STATE 1: Trigger the measurement
    if (!isReadingBME) {
        if (millis() - lastGasTime >= GAS_INTERVAL_MS) {
            bme.setGasHeater(320, 150); // Heater ON
            isGasCycle = true;
            lastGasTime = millis();
        } else if (isGasCycle) {
            bme.setGasHeater(0, 0); // Heater OFF
            isGasCycle = false;
        }
        
        bmeEndTime = bme.beginReading();
        if (bmeEndTime != 0) {
            isReadingBME = true;
        }
        return; 
    }

    // STATE 2: Read the measurement
    if (isReadingBME && millis() >= bmeEndTime) {
        if (bme.endReading()) {
            sharedTemp = bme.temperature;
            sharedPressure = bme.pressure / 100.0; 
            sharedHumidity = bme.humidity;
            
            if (isGasCycle) {
                sharedGas = bme.gas_resistance / 1000.0; 
                sharedIAQ = sharedGas; 
            }
            
            float rawAlt = bme.readAltitude(SEALEVELPRESSURE_HPA);
            
            // ==========================================
            // --- HARDWARE CRASH WATCHDOG ---
            // ==========================================
            // Catch the 3000m+ brownout bug or NaN errors instantly
            if (isnan(rawAlt) || rawAlt > 2000.0 || rawAlt < -500.0) {
                Serial.println("BME680 BROWNOUT DETECTED! Rebooting silicon...");
                // Force a hardware re-init to restore the factory calibration registers
                bme.begin(); 
                bme.setTemperatureOversampling(BME680_OS_8X);
                bme.setHumidityOversampling(BME680_OS_2X);
                bme.setPressureOversampling(BME680_OS_4X);
                bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
                bme.setGasHeater(0, 0);
                
                isReadingBME = false;
                isGasCycle = false;
                return; // Abort this loop
            }
            // ==========================================

            // High-Speed EMA Filter
            if (smoothedAltitude == 0.0) {
                smoothedAltitude = rawAlt;
                // Fill the buffer so we don't trigger a fake fall on boot
                for(int i=0; i<ALT_HISTORY_SIZE; i++) altHistory[i] = smoothedAltitude; 
            } else {
                smoothedAltitude = (ALTITUDE_EMA_ALPHA * rawAlt) + ((1.0 - ALTITUDE_EMA_ALPHA) * smoothedAltitude);
            }
            
            // --- 1-Second Rolling Delta Logic ---
            if (millis() - lastAltHistoryTime >= 100) {
                altHistory[altHistoryIndex] = smoothedAltitude;
                altHistoryIndex = (altHistoryIndex + 1) % ALT_HISTORY_SIZE; 
                lastAltHistoryTime = millis();
            }
            
            float oldestAltitude = altHistory[altHistoryIndex];
            
            // Calculate the drop (Negative = falling down)
            sharedAltitudeDrop = smoothedAltitude - oldestAltitude;
            sharedAltitude = smoothedAltitude;
        }
        isReadingBME = false; 
    }
}

#endif