import 'package:flutter/material.dart';
import '../bluetooth/ble_service.dart';
import '../models/sensor_data.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'dart:async';
import '../database/history_db.dart';

class DashboardPage extends StatefulWidget {
  final BleService ble;

  const DashboardPage({super.key, required this.ble});

  @override
  State<DashboardPage> createState() => _DashboardPageState();
}

class _DashboardPageState extends State<DashboardPage> {
  SensorData? data;
  StreamSubscription? sub;
  String name = "";
  String age = "";
  String gender = "";
  String height = "";
  String weight = "";
  double bmi = 0;
  bool recording = true;
  int recordInterval = 5; // seconds
  int lastRecordTime = 0;

  Future<void> loadRecordingSettings() async {
    final prefs = await SharedPreferences.getInstance();

    recording = prefs.getBool("recording") ?? true;
    recordInterval = prefs.getInt("recordInterval") ?? 5;
  }

  Future<void> recordData(SensorData d) async {
    if (!recording) return;

    int now = DateTime.now().millisecondsSinceEpoch;

    if (now - lastRecordTime < recordInterval * 1000) return;

    lastRecordTime = now;

    await HistoryDB.insert({
      "time": now,
      "bpm": d.bpm,
      "spo2": d.spo2,
      "bodyTemp": d.bodyTemp,
      "airTemp": d.airTemp,
      "humidity": d.humidity,
      "voc": d.voc,
    });
  }

  Future<void> loadProfile() async {
    final prefs = await SharedPreferences.getInstance();

    setState(() {
      name = prefs.getString("name") ?? "";
      age = prefs.getString("age") ?? "";
      gender = prefs.getString("gender") ?? "";
      height = prefs.getString("height") ?? "";
      weight = prefs.getString("weight") ?? "";

      double h = double.tryParse(height) ?? 0;
      double w = double.tryParse(weight) ?? 0;

      if (h > 0 && w > 0) {
        bmi = w / ((h / 100) * (h / 100));
      }
    });
  }

  @override
  void initState() {
    super.initState();

    loadProfile();
    loadRecordingSettings();

    sub = widget.ble.dataStream?.listen((d) {
      if (!mounted) return;

      recordData(d);

      setState(() {
        data = d;
      });
    });
  }

  @override
  void dispose() {
    sub?.cancel();

    super.dispose();
  }

  int calculateHealthScore() {
    if (data == null) return 0;

    int score = 100;

    if (data!.bpm > 110) score -= 20;
    if (data!.spo2 < 94) score -= 25;
    if (data!.bodyTemp > 38) score -= 20;
    if (data!.airPolluted) score -= 10;
    if (data!.fallDetected) score -= 40;

    return score.clamp(0, 100);
  }

  // Widget recordingControls() {
  //   return Container(
  //     width: double.infinity,
  //     padding: const EdgeInsets.all(16),
  //     decoration: BoxDecoration(
  //       color: Theme.of(context).cardColor,
  //       borderRadius: BorderRadius.circular(20),
  //     ),
  //     child: Column(
  //       children: [
  //         const Text(
  //           "Data Recorder",
  //           style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
  //         ),

  //         const SizedBox(height: 10),

  //         Row(
  //           mainAxisAlignment: MainAxisAlignment.spaceAround,
  //           children: [
  //             ElevatedButton.icon(
  //               icon: Icon(recording ? Icons.pause : Icons.play_arrow),
  //               label: Text(recording ? "Pause" : "Record"),
  //               onPressed: () {
  //                 setState(() {
  //                   recording = !recording;
  //                 });
  //               },
  //             ),

  //             ElevatedButton.icon(
  //               icon: const Icon(Icons.delete),
  //               label: const Text("Clear History"),
  //               onPressed: () async {
  //                 await HistoryDB.clear();
  //               },
  //             ),
  //           ],
  //         ),

  //         const SizedBox(height: 10),

  //         Row(
  //           mainAxisAlignment: MainAxisAlignment.center,
  //           children: [
  //             const Text("Record Interval: "),

  //             DropdownButton<int>(
  //               value: recordInterval,
  //               items: const [
  //                 DropdownMenuItem(value: 2, child: Text("2 sec")),
  //                 DropdownMenuItem(value: 5, child: Text("5 sec")),
  //                 DropdownMenuItem(value: 10, child: Text("10 sec")),
  //                 DropdownMenuItem(value: 30, child: Text("30 sec")),
  //               ],
  //               onChanged: (v) {
  //                 setState(() {
  //                   recordInterval = v!;
  //                 });
  //               },
  //             ),
  //           ],
  //         ),
  //       ],
  //     ),
  //   );
  // }

  Widget patientCard() {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: Colors.green.withValues(alpha: 0.15),
        borderRadius: BorderRadius.circular(20),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              const Icon(Icons.person, size: 30),
              const SizedBox(width: 10),

              Text(
                name.isEmpty ? "Patient Profile" : name,
                style: const TextStyle(
                  fontSize: 20,
                  fontWeight: FontWeight.bold,
                ),
              ),
            ],
          ),

          const SizedBox(height: 10),

          Text("Age: $age"),
          Text("Gender: $gender"),
          Text("Height: $height cm"),
          Text("Weight: $weight kg"),

          const SizedBox(height: 6),

          Text(
            "BMI: ${bmi == 0 ? "-" : bmi.toStringAsFixed(1)}",
            style: const TextStyle(fontWeight: FontWeight.bold),
          ),
        ],
      ),
    );
  }

  Widget sensorCard(String title, String value, IconData icon, Color c) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Theme.of(context).cardColor,
        borderRadius: BorderRadius.circular(20),
      ),
      child: Column(
        children: [
          Icon(icon, color: c, size: 28),
          const SizedBox(height: 6),
          Text(title, style: const TextStyle(color: Colors.white70)),
          const SizedBox(height: 4),
          Text(
            value,
            style: const TextStyle(
              color: Colors.white,
              fontSize: 20,
              fontWeight: FontWeight.bold,
            ),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final d = data;

    return Scaffold(
      appBar: AppBar(
        title: const Text("Live Monitoring"),

        actions: [
          IconButton(
            icon: const Icon(Icons.settings),

            onPressed: () {
              Navigator.pushNamed(context, "/settings");
            },
          ),
        ],
      ),

      body: d == null
          ? const Center(child: CircularProgressIndicator())
          : SingleChildScrollView(
              padding: const EdgeInsets.all(16),

              child: Column(
                children: [
                  /// PATIENT PROFILE
                  patientCard(),
                  const SizedBox(height: 20),

                  /// HEALTH SCORE
                  Container(
                    width: double.infinity,
                    padding: const EdgeInsets.all(20),
                    decoration: BoxDecoration(
                      color: Colors.blue.withValues(alpha: 0.2),
                      borderRadius: BorderRadius.circular(20),
                    ),
                    child: Column(
                      children: [
                        const Text(
                          "Health Score",
                          style: TextStyle(color: Colors.white70),
                        ),

                        const SizedBox(height: 8),

                        Text(
                          "${calculateHealthScore()}",
                          style: const TextStyle(
                            fontSize: 40,
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                      ],
                    ),
                  ),
                  const SizedBox(height: 20),

                  /// ALERT PANEL
                  Container(
                    width: double.infinity,
                    padding: const EdgeInsets.all(16),
                    decoration: BoxDecoration(
                      color: Colors.red.withValues(alpha: 0.2),
                      borderRadius: BorderRadius.circular(20),
                    ),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,

                      children: [
                        const Text(
                          "Risk Alerts",
                          style: TextStyle(
                            fontSize: 18,
                            fontWeight: FontWeight.bold,
                          ),
                        ),

                        const SizedBox(height: 8),

                        if (d.fallDetected) const Text("⚠ Fall detected"),

                        if (d.heartAttack) const Text("⚠ Heart attack risk"),

                        if (d.airPolluted) const Text("⚠ Air polluted"),

                        if (d.handRemoved) const Text("⚠ Sensor removed"),

                        if (!d.fallDetected &&
                            !d.heartAttack &&
                            !d.airPolluted &&
                            !d.handRemoved)
                          const Text("All vitals normal"),
                      ],
                    ),
                  ),

                  const SizedBox(height: 20),

                  /// SENSOR GRID
                  GridView.count(
                    shrinkWrap: true,
                    crossAxisCount: 2,
                    mainAxisSpacing: 12,
                    crossAxisSpacing: 12,
                    physics: const NeverScrollableScrollPhysics(),
                    children: [
                      sensorCard(
                        "BPM",
                        d.bpm.toStringAsFixed(0),
                        Icons.favorite,
                        Colors.red,
                      ),

                      sensorCard(
                        "SpO2",
                        "${d.spo2.toStringAsFixed(0)} %",
                        Icons.water_drop,
                        Colors.blue,
                      ),

                      sensorCard(
                        "Body Temp",
                        "${d.bodyTemp.toStringAsFixed(1)} °C",
                        Icons.thermostat,
                        Colors.orange,
                      ),

                      sensorCard(
                        "Air Temp",
                        "${d.airTemp.toStringAsFixed(1)} °C",
                        Icons.air,
                        Colors.green,
                      ),

                      sensorCard(
                        "Humidity",
                        "${d.humidity.toStringAsFixed(0)} %",
                        Icons.water,
                        Colors.cyan,
                      ),

                      sensorCard(
                        "VOC",
                        d.voc.toStringAsFixed(0),
                        Icons.cloud,
                        Colors.purple,
                      ),
                    ],
                  ),

                  const SizedBox(height: 20),
                ],
              ),
            ),
    );
  }
}
