#ifndef HEART_SENSOR_H
#define HEART_SENSOR_H

#include <Wire.h>
#include "MAX30105.h"
#include "mpu.h" // Access currentAccMag

MAX30105 particleSensor;

volatile int heartRate = 0;
volatile int spo2 = 0;
volatile int validHeartRate = 0;
volatile int validSPO2 = 0;

// DSP BPM Variables
unsigned long lastBeat = 0;
float prevInput = 0, prevOutput = 0, prevValue = 0, prevDerivative = 0;
unsigned long lastPeakTime = 0;
const float HP_ALPHA = 0.90f; 
const int BLANKING_WINDOW = 350; 

// ANC Tuning
const float ANC_BETA = 450.0f; // Tuning constant: Motion Magnitude -> PPG Noise

// SpO2 Variables
float dcRed = 0.0, dcIR = 0.0;
const float DC_ALPHA = 0.95f;
float acSqSumRed = 0.0, acSqSumIR = 0.0;
int sampleCount = 0;
const int SPO2_WINDOW = 100;

float highPassFilter(float input) {
    float output = HP_ALPHA * (prevOutput + input - prevInput);
    prevInput = input; prevOutput = output;
    return output;
}

float smoothSignal(float input) {
    static float buffer[5];
    static int index = 0;
    buffer[index] = input;
    index = (index + 1) % 5;
    float sum = 0;
    for(int i=0; i<5; i++) sum += buffer[i];
    return sum / 5.0f;
}

bool detectBeat(float signal) {
    float derivative = signal - prevValue;
    bool peakDetected = false;
    if(prevDerivative > 0 && derivative < 0 && signal > 35.0f) {
        unsigned long now = millis();
        if(now - lastPeakTime > BLANKING_WINDOW) {
            peakDetected = true;
            lastPeakTime = now;
        }
    }
    prevDerivative = derivative;
    prevValue = signal;
    return peakDetected;
}

bool initHeartSensor() {
    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) return false;
    particleSensor.setup(0x24, 4, 2, 100, 411, 4096);
    return true;
}

void processHeartData(SemaphoreHandle_t mutex, float currentAccMag){
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        particleSensor.check();
        xSemaphoreGive(mutex);
    }
    while (particleSensor.available()) {
        uint32_t irValue = particleSensor.getFIFOIR();
        uint32_t redValue = particleSensor.getFIFORed();
        particleSensor.nextSample();

        if(irValue < 30000) { 
            validHeartRate = 0; validSPO2 = 0; heartRate = 0; spo2 = 0;
            dcRed = 0; dcIR = 0; acSqSumRed = 0; acSqSumIR = 0;
            sampleCount = 0; lastBeat = millis();
            prevInput = 0; prevOutput = 0;
            continue;
        }

        // --- APPLY ANC MODIFICATION ---
        float motionNoise = abs(currentAccMag - 1.0f);
        float hpSignal = highPassFilter((float)irValue);
        
        // Subtract estimated motion noise from the AC signal
        float cleanSignal = hpSignal - (motionNoise * ANC_BETA);
        
        float filtered = smoothSignal(cleanSignal);
        
        if(detectBeat(filtered)) {
            long delta = millis() - lastBeat;
            lastBeat = millis();
            float rawBpm = 60000.0f / (float)delta;
            
            if(rawBpm > 45 && rawBpm < 160) {
                heartRate = (heartRate == 0) ? (int)rawBpm : (int)(0.2f * rawBpm + 0.8f * heartRate);
                validHeartRate = 1;
            }
        }

        if(dcRed == 0) { dcRed = redValue; dcIR = irValue; }
        dcRed = (DC_ALPHA * dcRed) + ((1.0f - DC_ALPHA) * redValue);
        dcIR = (DC_ALPHA * dcIR) + ((1.0f - DC_ALPHA) * irValue);
        float acRed = (float)redValue - dcRed;
        float acIR = (float)irValue - dcIR;
        acSqSumRed += acRed * acRed;
        acSqSumIR += acIR * acIR;
        sampleCount++;

        if(sampleCount >= SPO2_WINDOW) {
            float rmsRed = sqrt(acSqSumRed / SPO2_WINDOW);
            float rmsIR = sqrt(acSqSumIR / SPO2_WINDOW);
            float ratio = (rmsRed / dcRed) / (rmsIR / dcIR);
            float calc = 104.0f - (17.0f * ratio);
            if(calc > 85 && calc <= 100) { spo2 = (int)calc; validSPO2 = 1; }
            acSqSumRed = 0; acSqSumIR = 0; sampleCount = 0;
        }
    }
}
#endif