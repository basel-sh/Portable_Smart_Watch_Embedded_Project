import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import '../database/history_db.dart';
import 'package:shared_preferences/shared_preferences.dart';

class HistoryPage extends StatefulWidget {
  const HistoryPage({super.key});

  @override
  State<HistoryPage> createState() => _HistoryPageState();
}

class _HistoryPageState extends State<HistoryPage> {
  List<Map<String, dynamic>> data = [];

  bool recording = true;
  int recordInterval = 5;

  @override
  void initState() {
    super.initState();
    loadData();
    loadSettings();
  }

  Future<void> loadSettings() async {
    final prefs = await SharedPreferences.getInstance();

    setState(() {
      recording = prefs.getBool("recording") ?? true;
      recordInterval = prefs.getInt("recordInterval") ?? 5;
    });
  }

  Future<void> saveSettings() async {
    final prefs = await SharedPreferences.getInstance();

    await prefs.setBool("recording", recording);
    await prefs.setInt("recordInterval", recordInterval);
  }

  Future<void> loadData() async {
    final d = await HistoryDB.getAll();

    setState(() {
      data = d;
    });
  }

  List<FlSpot> getSpots(String field) {
    List<FlSpot> spots = [];

    for (int i = 0; i < data.length; i++) {
      spots.add(FlSpot(i.toDouble(), (data[i][field] as num).toDouble()));
    }

    return spots;
  }

  Widget chart(String title, String field, Color color) {
    return Container(
      height: 220,
      padding: const EdgeInsets.all(16),
      margin: const EdgeInsets.only(bottom: 16),

      decoration: BoxDecoration(
        color: Theme.of(context).cardColor,
        borderRadius: BorderRadius.circular(20),
      ),

      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                title,
                style: const TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                ),
              ),

              IconButton(
                icon: const Icon(Icons.fullscreen),
                onPressed: () {
                  Navigator.push(
                    context,
                    MaterialPageRoute(
                      builder: (_) => FullChartPage(
                        title: title,
                        spots: getSpots(field),
                        color: color,
                      ),
                    ),
                  );
                },
              ),
            ],
          ),

          const SizedBox(height: 10),

          Expanded(
            child: LineChart(
              LineChartData(
                gridData: const FlGridData(show: true),

                titlesData: const FlTitlesData(show: false),

                borderData: FlBorderData(show: false),

                lineBarsData: [
                  LineChartBarData(
                    spots: getSpots(field),
                    isCurved: true,
                    barWidth: 3,
                    dotData: const FlDotData(show: false),
                    color: color,
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget controlPanel() {
    return Container(
      padding: const EdgeInsets.all(16),
      margin: const EdgeInsets.only(bottom: 20),
      decoration: BoxDecoration(
        color: Theme.of(context).cardColor,
        borderRadius: BorderRadius.circular(20),
      ),
      child: Column(
        children: [
          const Text(
            "Data Recorder",
            style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
          ),

          const SizedBox(height: 10),

          Row(
            mainAxisAlignment: MainAxisAlignment.spaceAround,
            children: [
              ElevatedButton.icon(
                icon: Icon(recording ? Icons.pause : Icons.play_arrow),
                label: Text(recording ? "Pause" : "Record"),
                onPressed: () {
                  setState(() {
                    recording = !recording;
                  });
                  saveSettings();
                },
              ),

              ElevatedButton.icon(
                icon: const Icon(Icons.delete),
                label: const Text("Clear DB"),
                onPressed: () async {
                  await HistoryDB.clear();
                  loadData();
                },
              ),

              ElevatedButton.icon(
                icon: const Icon(Icons.refresh),
                label: const Text("Reload"),
                onPressed: loadData,
              ),
            ],
          ),

          const SizedBox(height: 12),

          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              const Text("Record Interval: "),

              DropdownButton<int>(
                value: recordInterval,
                items: const [
                  DropdownMenuItem(value: 2, child: Text("2 sec")),
                  DropdownMenuItem(value: 5, child: Text("5 sec")),
                  DropdownMenuItem(value: 10, child: Text("10 sec")),
                  DropdownMenuItem(value: 30, child: Text("30 sec")),
                ],
                onChanged: (v) {
                  setState(() {
                    recordInterval = v!;
                  });
                  saveSettings();
                },
              ),
            ],
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("History & Analytics")),

      body: data.isEmpty
          ? const Center(child: Text("No data recorded yet"))
          : SingleChildScrollView(
              padding: const EdgeInsets.all(16),

              child: Column(
                children: [
                  controlPanel(),

                  chart("Heart Rate (BPM)", "bpm", Colors.red),

                  chart("SpO2 (%)", "spo2", Colors.blue),

                  chart("Body Temperature", "bodyTemp", Colors.orange),

                  chart("Air Quality VOC", "voc", Colors.purple),
                ],
              ),
            ),
    );
  }
}

class FullChartPage extends StatelessWidget {
  final String title;
  final List<FlSpot> spots;
  final Color color;

  const FullChartPage({
    super.key,
    required this.title,
    required this.spots,
    required this.color,
  });

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text(title)),

      body: Padding(
        padding: const EdgeInsets.all(20),

        child: LineChart(
          LineChartData(
            gridData: const FlGridData(show: true),

            borderData: FlBorderData(show: false),

            lineBarsData: [
              LineChartBarData(
                spots: spots,
                isCurved: true,
                barWidth: 4,
                color: color,
                dotData: const FlDotData(show: false),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
