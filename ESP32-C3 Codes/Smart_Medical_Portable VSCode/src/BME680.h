#ifndef BME_SENSOR_H
#define BME_SENSOR_H

#include <SPI.h>
#include <bsec2.h>
#include <math.h>

// Define Hardware VSPI Pins for ESP32-C3
#define BME_CS 7 
#define SEALEVELPRESSURE_HPA (1013.25) // Update this daily for accurate absolute altitude

Bsec2 envSensor;

// Global variables to share data with main.cpp
volatile float sharedTemp = 0.0;
volatile float sharedHumidity = 0.0;
volatile float sharedPressure = 0.0;
volatile float sharedAltitude = 0.0;
// --- NEW: Altitude Smoothing ---
float smoothedAltitude = 0.0;
const float ALTITUDE_ALPHA = 0.1;
volatile float sharedIAQ = 0.0;
volatile float sharedeCO2 = 0.0;
volatile float sharedbVOC = 0.0;

// BSEC2 uses a callback function to hand us the data when the DSP math is finished
void newDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec) {
    if (!outputs.nOutputs) {
        return;
    }

    for (uint8_t i = 0; i < outputs.nOutputs; i++) {
        const bsecData output = outputs.output[i];
        
        switch (output.sensor_id) {
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
                sharedTemp = output.signal;
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
                sharedHumidity = output.signal;
                break;
          case BSEC_OUTPUT_RAW_PRESSURE:{
                sharedPressure = output.signal; // hPa
                
                // 1. Calculate the raw, noisy altitude
                float rawAltitude = 44330.0f * (1.0f - pow(sharedPressure / SEALEVELPRESSURE_HPA, 0.1903f));
                
                // 2. Apply the EMA Low-Pass Filter
                if (smoothedAltitude == 0.0) {
                    smoothedAltitude = rawAltitude; // Seed filter on first boot
                } else {
                    smoothedAltitude = (ALTITUDE_ALPHA * rawAltitude) + ((1.0f - ALTITUDE_ALPHA) * smoothedAltitude);
                }
                
                // 3. Output the rock-solid value
                sharedAltitude = smoothedAltitude;
                break;}
            case BSEC_OUTPUT_IAQ:
                sharedIAQ = output.signal;
                break;
            case BSEC_OUTPUT_CO2_EQUIVALENT:
                sharedeCO2 = output.signal;
                break;
            case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
                sharedbVOC = output.signal;
                break;
        }
    }
}

// Helper to check for hardware/library faults
void checkIaqSensorStatus(void) {
    if (envSensor.status < BSEC_OK) {
        Serial.print("BSEC fatal error code : ");
        Serial.println(envSensor.status);
    } 
    if (envSensor.sensor.status < BME68X_OK) {
        Serial.print("BME68X fatal error code : ");
        Serial.println(envSensor.sensor.status);
    }
}

bool initBME680() {
    // 1. Initialize the Hardware SPI bus
    SPI.begin(); 

    // 2. Attach our data callback
    envSensor.attachCallback(newDataCallback);

    // 3. Initialize BSEC2 using the Hardware SPI interface
    if (!envSensor.begin(BME_CS, SPI)) {
        checkIaqSensorStatus();
        return false;
    }

    // 4. Define the virtual sensors we want the DSP to calculate
    bsecSensor sensorList[] = {
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
        BSEC_OUTPUT_RAW_PRESSURE,
        BSEC_OUTPUT_IAQ,                      
        BSEC_OUTPUT_CO2_EQUIVALENT,           
        BSEC_OUTPUT_BREATH_VOC_EQUIVALENT     
    };

    // 5. Subscribe to sensors in Low Power mode (1 sample every 3 seconds)
    if (!envSensor.updateSubscription(sensorList, 6, BSEC_SAMPLE_RATE_LP)) {
        checkIaqSensorStatus();
        return false;
    }

    Serial.println("BME680 (SPI) Initialized via BSEC2");
    return true;
}

// RTOS Task for the BME680
void processBME680() {
    // Calling run() checks the clock. If 3 seconds have passed, it executes the 
    // SPI read, runs the DSP math, and fires the newDataCallback(). If not, it returns instantly.
    if (!envSensor.run()) {
        checkIaqSensorStatus();
    }
}

#endif