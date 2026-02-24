import 'package:flutter/material.dart';
import '../bluetooth/ble_service.dart';
import 'home_page.dart';
import 'package:permission_handler/permission_handler.dart';

class ConnectPage extends StatefulWidget {
  @override
  State<ConnectPage> createState() => _ConnectPageState();
}

class _ConnectPageState extends State<ConnectPage> {

  final ble = BleService();
  
Future requestPermissions() async {
  await Permission.location.request();
  await Permission.bluetoothScan.request();
  await Permission.bluetoothConnect.request();
}


@override
void initState() {
  super.initState();

  requestPermissions().then((_) {

    ble.startScan((device) async {

      await ble.connect(device);

      Navigator.pushReplacement(
        context,
        MaterialPageRoute(
          builder: (_) => HomePage(ble: ble),
        ),
      );

    });

  });
}


  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Center(child: Text("Scanning ESP32...")),
    );
  }
}
