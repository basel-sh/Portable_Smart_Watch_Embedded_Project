#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {

  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("PHONE CONNECTED");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("PHONE DISCONNECTED");
    delay(200);
    BLEDevice::startAdvertising();
  }
};

int heartRate = 75;
float temp = 36.5;
int spo2 = 98;

void setup() {

  Serial.begin(115200);

  BLEDevice::init("ESP32_VITAL");

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService =
      pServer->createService("12345678-1234-1234-1234-1234567890ab");

  pCharacteristic =
      pService->createCharacteristic(
          "abcd1234-1234-1234-1234-1234567890ab",
          BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  BLEDevice::getAdvertising()->start();

  Serial.println("BLE READY");

  randomSeed(millis());
}

void loop() {

  if (deviceConnected) {

    heartRate += random(-2, 3);
    heartRate = constrain(heartRate, 65, 95);

    temp += random(-2, 3) * 0.05;
    temp = constrain(temp, 35.8, 37.5);

    spo2 += random(-1, 2);
    spo2 = constrain(spo2, 94, 100);

    // SHORT FORMAT (SAFE)
    String data =
      String(heartRate) + "," +
      String(temp,1) + "," +
      String(spo2);

    pCharacteristic->setValue(data.c_str());
    pCharacteristic->notify();

    Serial.println(data);
  }

  delay(1000);
}
