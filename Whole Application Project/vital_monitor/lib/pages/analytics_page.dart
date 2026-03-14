import 'package:flutter/material.dart';
import 'package:fl_chart/fl_chart.dart';
import '../database/history_db.dart';

class AnalyticsPage extends StatefulWidget {
  const AnalyticsPage({super.key});

  @override
  State<AnalyticsPage> createState() => _AnalyticsPageState();
}

class _AnalyticsPageState extends State<AnalyticsPage> {
  List<Map<String, dynamic>> data = [];

  double avgBpm = 0;
  double avgSpo2 = 0;
  double avgTemp = 0;
  double avgVoc = 0;

  int healthScore = 0;

  String recommendation = "";

  String range = "All";

  @override
  void initState() {
    super.initState();
    loadAnalytics();
  }

  Future<void> loadAnalytics() async {
    final d = await HistoryDB.getAll();

    if (d.isEmpty) return;

    data = d;

    avgBpm = d.map((e) => e["bpm"]).reduce((a, b) => a + b) / d.length;
    avgSpo2 = d.map((e) => e["spo2"]).reduce((a, b) => a + b) / d.length;
    avgTemp = d.map((e) => e["bodyTemp"]).reduce((a, b) => a + b) / d.length;
    avgVoc = d.map((e) => e["voc"]).reduce((a, b) => a + b) / d.length;

    healthScore = calculateHealthScore();

    recommendation = generateAdvice();

    setState(() {});
  }

  int calculateHealthScore() {
    int score = 100;

    if (avgBpm > 100) score -= 20;
    if (avgSpo2 < 94) score -= 25;
    if (avgTemp > 37.5) score -= 15;
    if (avgVoc > 400) score -= 10;

    return score.clamp(0, 100);
  }

  String generateAdvice() {
    if (avgSpo2 < 94) {
      return "Oxygen level slightly low. Consider rest and fresh air.";
    }

    if (avgBpm > 100) {
      return "Heart rate elevated. Try relaxing or reducing activity.";
    }

    if (avgTemp > 37.5) {
      return "Body temperature elevated. Monitor for fever symptoms.";
    }

    if (avgVoc > 400) {
      return "Air quality is poor. Move to a well ventilated area.";
    }

    return "Vitals look stable. Maintain healthy activity and hydration.";
  }

  List<FlSpot> getSpots(String field) {
    List<FlSpot> spots = [];

    for (int i = 0; i < data.length; i++) {
      spots.add(FlSpot(i.toDouble(), (data[i][field] as num).toDouble()));
    }

    return spots;
  }

  Widget card(String title, Widget child) {
    return Container(
      padding: const EdgeInsets.all(16),
      margin: const EdgeInsets.only(bottom: 16),

      decoration: BoxDecoration(
        color: Theme.of(context).cardColor,
        borderRadius: BorderRadius.circular(20),
      ),

      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            title,
            style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
          ),

          const SizedBox(height: 12),

          child,
        ],
      ),
    );
  }

  Widget healthSummary() {
    return card(
      "Health Summary",

      Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceAround,
            children: [
              metric("Avg BPM", avgBpm.toStringAsFixed(0)),
              metric("Avg SpO2", "${avgSpo2.toStringAsFixed(0)}%"),
              metric("Avg Temp", "${avgTemp.toStringAsFixed(1)}°C"),
            ],
          ),

          const SizedBox(height: 16),

          Row(
            mainAxisAlignment: MainAxisAlignment.spaceAround,
            children: [
              metric("VOC", avgVoc.toStringAsFixed(0)),
              metric("Health Score", healthScore.toString()),
              metric("Records", data.length.toString()),
            ],
          ),
        ],
      ),
    );
  }

  Widget metric(String label, String value) {
    return Column(
      children: [
        Text(
          value,
          style: const TextStyle(fontSize: 22, fontWeight: FontWeight.bold),
        ),

        const SizedBox(height: 4),

        Text(label, style: const TextStyle(color: Colors.white60)),
      ],
    );
  }

  Widget chart(String title, String field, Color color) {
    return card(
      title,

      SizedBox(
        height: 200,

        child: LineChart(
          LineChartData(
            gridData: const FlGridData(show: true),
            borderData: FlBorderData(show: false),
            titlesData: const FlTitlesData(show: false),

            lineBarsData: [
              LineChartBarData(
                spots: getSpots(field),
                isCurved: true,
                barWidth: 3,
                color: color,
                dotData: const FlDotData(show: false),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget riskPanel() {
    String risk;

    if (healthScore > 85) {
      risk = "LOW";
    } else if (healthScore > 60) {
      risk = "MODERATE";
    } else {
      risk = "HIGH";
    }

    return card(
      "Risk Prediction",

      Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceAround,
            children: [
              riskItem("Heart Risk", avgBpm > 100 ? "Elevated" : "Normal"),
              riskItem("Oxygen Risk", avgSpo2 < 94 ? "Low" : "Normal"),
              riskItem("Environment", avgVoc > 400 ? "Polluted" : "Good"),
            ],
          ),

          const SizedBox(height: 16),

          Text(
            "Overall Risk Level: $risk",
            style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
          ),
        ],
      ),
    );
  }

  Widget riskItem(String label, String value) {
    return Column(
      children: [
        Text(
          value,
          style: const TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
        ),

        const SizedBox(height: 4),

        Text(label, style: const TextStyle(color: Colors.white60)),
      ],
    );
  }

  Widget aiAdvice() {
    return card(
      "AI Recommendations",

      Text(recommendation, style: const TextStyle(fontSize: 16)),
    );
  }

  Widget controls() {
    return card(
      "Analytics Controls",

      Row(
        mainAxisAlignment: MainAxisAlignment.spaceAround,

        children: [
          DropdownButton<String>(
            value: range,

            items: const [
              DropdownMenuItem(value: "All", child: Text("All Data")),
              DropdownMenuItem(value: "1H", child: Text("Last Hour")),
              DropdownMenuItem(value: "10M", child: Text("10 Minutes")),
              DropdownMenuItem(value: "1M", child: Text("1 Minute")),
            ],

            onChanged: (v) {
              setState(() {
                range = v!;
              });
            },
          ),

          ElevatedButton.icon(
            icon: const Icon(Icons.refresh),
            label: const Text("Refresh"),
            onPressed: loadAnalytics,
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("AI Health Analytics")),

      body: data.isEmpty
          ? const Center(child: Text("No analytics data available"))
          : SingleChildScrollView(
              padding: const EdgeInsets.all(16),

              child: Column(
                children: [
                  controls(),

                  healthSummary(),

                  chart("Heart Rate Trend", "bpm", Colors.red),

                  chart("Oxygen Trend", "spo2", Colors.blue),

                  chart("Temperature Trend", "bodyTemp", Colors.orange),

                  riskPanel(),

                  aiAdvice(),
                ],
              ),
            ),
    );
  }
}
