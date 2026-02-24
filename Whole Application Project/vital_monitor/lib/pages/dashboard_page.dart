import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import '../bluetooth/ble_service.dart';
import '../models/sensor_data.dart';

class DashboardPage extends StatefulWidget {
  final BleService ble;
  const DashboardPage({super.key, required this.ble});

  @override
  State<DashboardPage> createState() => _DashboardPageState();
}

class _DashboardPageState extends State<DashboardPage>
    with SingleTickerProviderStateMixin {

  SensorData? data;
  List<FlSpot> hrSpots = [];
  int x = 0;

  late AnimationController pulseController;

  @override
  void initState() {
    super.initState();

    pulseController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 800),
      lowerBound: 0.9,
      upperBound: 1.05,
    )..repeat(reverse: true);

    widget.ble.dataStream?.listen((d) {
      setState(() {
        data = d;

        hrSpots.add(FlSpot(x.toDouble(), d.hr.toDouble()));
        x++;

        if (hrSpots.length > 20) {
          hrSpots.removeAt(0);
        }
      });
    });
  }

  @override
  void dispose() {
    pulseController.dispose();
    super.dispose();
  }

  Widget buildGlassCard(String title, String value, IconData icon, Color color) {
    return Container(
      padding: const EdgeInsets.all(18),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.06),
        borderRadius: BorderRadius.circular(24),
        border: Border.all(color: Colors.white24),
      ),
      child: Column(
        children: [
          Icon(icon, color: color, size: 30),
          const SizedBox(height: 8),
          Text(title, style: const TextStyle(color: Colors.white70)),
          const SizedBox(height: 5),
          Text(
            value,
            style: const TextStyle(
                fontSize: 24,
                color: Colors.white,
                fontWeight: FontWeight.bold),
          )
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {

    final hr = data?.hr ?? 0;
    final temp = data?.temp ?? 0;
    final spo2 = data?.spo2 ?? 0;

    return Scaffold(
      backgroundColor: const Color(0xFF090B10),

      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        title: const Text("Live Dashboard"),
      ),

      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [

            /// ❤️ PULSING HEART
            ScaleTransition(
              scale: pulseController,
              child: Container(
                width: 200,
                height: 200,
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  color: Colors.red.withOpacity(0.12),
                ),
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    const Icon(Icons.favorite,
                        color: Colors.red, size: 45),
                    Text(
                      "$hr",
                      style: const TextStyle(
                        fontSize: 52,
                        color: Colors.white,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const Text(
                      "BPM",
                      style: TextStyle(color: Colors.white70),
                    )
                  ],
                ),
              ),
            ),

            const SizedBox(height: 25),

            /// SMALL CARDS
            Row(
              children: [
                Expanded(
                  child: buildGlassCard(
                    "Temp",
                    "$temp °C",
                    Icons.thermostat,
                    Colors.orange,
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: buildGlassCard(
                    "SpO₂",
                    "$spo2 %",
                    Icons.water_drop,
                    Colors.blue,
                  ),
                ),
              ],
            ),

            const SizedBox(height: 25),

            /// LIVE HEART CHART
            Container(
              height: 220,
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: Colors.white.withOpacity(0.06),
                borderRadius: BorderRadius.circular(24),
                border: Border.all(color: Colors.white24),
              ),
              child: LineChart(
                LineChartData(
                  gridData: FlGridData(show: false),
                  titlesData: FlTitlesData(show: false),
                  borderData: FlBorderData(show: false),
                  lineBarsData: [
                    LineChartBarData(
                      spots: hrSpots,
                      isCurved: true,
                      dotData: FlDotData(show: false),
                      barWidth: 4,
                      color: Colors.redAccent,
                    ),
                  ],
                ),
              ),
            ),

            const SizedBox(height: 20),

            /// STATUS
            Container(
              width: double.infinity,
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: Colors.white.withOpacity(0.06),
                borderRadius: BorderRadius.circular(24),
              ),
              child: Text(
                "Status: ${hr > 95 ? "High Heart Rate" : "Normal"}",
                style: const TextStyle(
                  color: Colors.white,
                  fontSize: 18,
                ),
              ),
            )
          ],
        ),
      ),
    );
  }
}