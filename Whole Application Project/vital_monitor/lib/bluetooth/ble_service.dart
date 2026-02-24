import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import '../models/sensor_data.dart';

class BleService {

  BluetoothDevice? device;
  BluetoothCharacteristic? characteristic;

  Stream<SensorData>? dataStream;

Future startScan(Function(BluetoothDevice) onFound) async {

  print("STARTING SCAN...");

  await FlutterBluePlus.startScan();

  FlutterBluePlus.scanResults.listen((results) {

    for (var r in results) {

      print("FOUND: ${r.advertisementData.advName}");

      if (r.advertisementData.advName == "ESP32_VITAL") {

        print("ESP FOUND !!!");

        FlutterBluePlus.stopScan();

        onFound(r.device);
        return;
      }
    }
  });
}


  Future connect(BluetoothDevice d) async {

    device = d;
    await d.connect(autoConnect: false);


    var services = await d.discoverServices();

    for (var s in services) {
      if (s.uuid.toString() ==
          "12345678-1234-1234-1234-1234567890ab") {

        characteristic = s.characteristics.first;
        await characteristic!.setNotifyValue(true);

        dataStream =
            characteristic!.onValueReceived.map((event) {

          String raw = String.fromCharCodes(event);

          return SensorData.fromBle(raw);
        });
      }
    }
  }
}
