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
  void initState() {
    super.initState();

    widget.ble.startFakeStream();
  }

  @override
  Widget build(BuildContext context) {
    final pages = [
      DashboardPage(ble: widget.ble),
      const HistoryPage(),
      const AnalyticsPage(),
      const SettingsPage(),
    ];

    return Scaffold(
      body: pages[index],

      bottomNavigationBar: Container(
        decoration: BoxDecoration(
          color: Theme.of(context).cardColor,
          boxShadow: [
            BoxShadow(
              color: Colors.black.withValues(alpha: 0.4),
              blurRadius: 10,
            ),
          ],
        ),
        child: BottomNavigationBar(
          currentIndex: index,
          onTap: (i) => setState(() => index = i),
          type: BottomNavigationBarType.fixed,

          selectedFontSize: 12,
          unselectedFontSize: 11,

          items: const [
            BottomNavigationBarItem(icon: Icon(Icons.favorite), label: "Live"),
            BottomNavigationBarItem(
              icon: Icon(Icons.history),
              label: "History",
            ),
            BottomNavigationBarItem(
              icon: Icon(Icons.bar_chart),
              label: "Analytics",
            ),
            BottomNavigationBarItem(
              icon: Icon(Icons.settings),
              label: "Settings",
            ),
          ],
        ),
      ),
    );
  }
}
