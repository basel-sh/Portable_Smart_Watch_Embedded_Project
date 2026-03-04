#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

static BLECharacteristic *pCharacteristic;
static bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks
{

  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    Serial.println("PHONE CONNECTED");
  }

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    Serial.println("PHONE DISCONNECTED");
    BLEDevice::startAdvertising();
  }
};

void initBLE()
{

  BLEDevice::init("ESP32_VITAL");

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService =
      pServer->createService("12345678-1234-1234-1234-1234567890ab");

  pCharacteristic =
      pService->createCharacteristic(
          "abcd1234-1234-1234-1234-1234567890ab",
          BLECharacteristic::PROPERTY_NOTIFY |
              BLECharacteristic::PROPERTY_READ);

  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  BLEDevice::getAdvertising()->start();

  Serial.println("BLE READY");
}

void sendBLEData(int bpm, float temp, int spo2)
{
  if (!deviceConnected)
    return;

  // Create a character array to hold the formatted data
  char bleData[32];

  // Format the data into the array (e.g., "75,36.5,98")
  snprintf(bleData, sizeof(bleData), "%d,%.1f,%d", bpm, temp, spo2);

  // Send the array
  pCharacteristic->setValue(bleData);
  pCharacteristic->notify();

  Serial.print("BLE -> ");
  Serial.println(bleData);
}

#endif