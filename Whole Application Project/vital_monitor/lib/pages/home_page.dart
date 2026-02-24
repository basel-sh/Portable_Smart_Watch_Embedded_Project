import 'package:flutter/material.dart';
import '../bluetooth/ble_service.dart';
import 'dashboard_page.dart';
import 'history_page.dart';
import 'analytics_page.dart';
import 'settings_page.dart';

class HomePage extends StatefulWidget {

  final BleService ble;
  const HomePage({super.key, required this.ble});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {

  int index = 0;

  @override
  Widget build(BuildContext context) {

    final pages = [
      DashboardPage(ble: widget.ble),
      HistoryPage(),
      AnalyticsPage(),
      SettingsPage(),
    ];

    return Scaffold(
      body: pages[index],

      bottomNavigationBar: BottomNavigationBar(
        currentIndex: index,
        onTap: (i) => setState(() => index = i),
        items: const [
          BottomNavigationBarItem(icon: Icon(Icons.favorite), label: "Live"),
          BottomNavigationBarItem(icon: Icon(Icons.history), label: "History"),
          BottomNavigationBarItem(icon: Icon(Icons.bar_chart), label: "Analytics"),
          BottomNavigationBarItem(icon: Icon(Icons.settings), label: "Settings"),
        ],
      ),
    );
  }
}