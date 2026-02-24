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

    delay(200); // small safety delay
    BLEDevice::startAdvertising();   // ⭐ restart advertising
    Serial.println("Advertising Restarted");
  }
};


int counter = 0;

void setup() {

  Serial.begin(115200);
  delay(2000);

  Serial.println("START BLE");

  BLEDevice::init("ESP32_TEST");

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

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();

  Serial.println("BLE READY");
}

void loop() {

  if (deviceConnected) {

    String value = "Counter: " + String(counter++);

    pCharacteristic->setValue(value.c_str());
    pCharacteristic->notify();

    Serial.println(value);
  }

  delay(1000);
}
