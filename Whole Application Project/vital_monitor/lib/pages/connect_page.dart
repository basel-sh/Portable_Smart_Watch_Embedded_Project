import 'package:flutter/material.dart';
import '../bluetooth/ble_service.dart';
import 'home_page.dart';

class ConnectPage extends StatefulWidget {
  const ConnectPage({super.key});

  @override
  State<ConnectPage> createState() => _ConnectPageState();
}

class _ConnectPageState extends State<ConnectPage> {
  final ble = BleService();

  @override
  void initState() {
    super.initState();

    Future.delayed(const Duration(seconds: 1), () {
      if (!mounted) return;

      Navigator.pushReplacement(
        context,
        MaterialPageRoute(builder: (_) => HomePage(ble: ble)),
      );
    });
  }

  @override
  Widget build(BuildContext context) {
    return const Scaffold(
      body: Center(
        child: Text("Opening Dashboard...", style: TextStyle(fontSize: 22)),
      ),
    );
  }
}
