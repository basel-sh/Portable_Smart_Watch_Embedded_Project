import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

class SettingsPage extends StatefulWidget {
  const SettingsPage({super.key});

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  final nameController = TextEditingController();
  final ageController = TextEditingController();
  final heightController = TextEditingController();
  final weightController = TextEditingController();
  final emergencyController = TextEditingController();

  String gender = "Male";

  bool doctorMode = false;
  bool darkTheme = true;

  bool fallAlert = true;
  bool heartAlert = true;

  double bmi = 0;

  @override
  void initState() {
    super.initState();
    loadSettings();
  }

  Future<void> loadSettings() async {
    final prefs = await SharedPreferences.getInstance();

    setState(() {
      nameController.text = prefs.getString("name") ?? "";
      ageController.text = prefs.getString("age") ?? "";
      heightController.text = prefs.getString("height") ?? "";
      weightController.text = prefs.getString("weight") ?? "";
      emergencyController.text = prefs.getString("emergency") ?? "";

      gender = prefs.getString("gender") ?? "Male";

      doctorMode = prefs.getBool("doctorMode") ?? false;
      darkTheme = prefs.getBool("darkTheme") ?? true;

      fallAlert = prefs.getBool("fallAlert") ?? true;
      heartAlert = prefs.getBool("heartAlert") ?? true;

      calculateBMI();
    });
  }

  Future<void> saveSettings() async {
    final prefs = await SharedPreferences.getInstance();

    prefs.setString("name", nameController.text);
    prefs.setString("age", ageController.text);
    prefs.setString("height", heightController.text);
    prefs.setString("weight", weightController.text);
    prefs.setString("emergency", emergencyController.text);

    prefs.setString("gender", gender);

    prefs.setBool("doctorMode", doctorMode);
    prefs.setBool("darkTheme", darkTheme);

    prefs.setBool("fallAlert", fallAlert);
    prefs.setBool("heartAlert", heartAlert);
  }

  void calculateBMI() {
    double h = double.tryParse(heightController.text) ?? 0;
    double w = double.tryParse(weightController.text) ?? 0;

    if (h > 0 && w > 0) {
      bmi = w / ((h / 100) * (h / 100));
    } else {
      bmi = 0;
    }
  }

  Widget sectionTitle(String t) {
    return Padding(
      padding: const EdgeInsets.all(12),
      child: Text(
        t,
        style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
      ),
    );
  }

  Widget inputField(String title, TextEditingController c) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
      child: TextField(
        controller: c,
        decoration: InputDecoration(
          labelText: title,
          border: OutlineInputBorder(borderRadius: BorderRadius.circular(12)),
        ),
        onChanged: (v) {
          calculateBMI();
          saveSettings();
          setState(() {});
        },
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Settings")),

      body: ListView(
        children: [
          /// PERSONAL PROFILE
          sectionTitle("Personal Profile"),

          inputField("Full Name", nameController),

          inputField("Age", ageController),

          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16),
            child: DropdownButtonFormField<String>(
              initialValue: gender,
              items: const [
                DropdownMenuItem(value: "Male", child: Text("Male")),
                DropdownMenuItem(value: "Female", child: Text("Female")),
              ],
              onChanged: (v) {
                setState(() {
                  gender = v.toString();
                });
                saveSettings();
              },
              decoration: InputDecoration(
                labelText: "Gender",
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(12),
                ),
              ),
            ),
          ),

          const SizedBox(height: 10),

          inputField("Height (cm)", heightController),

          inputField("Weight (kg)", weightController),

          ListTile(
            title: const Text("BMI"),
            subtitle: Text(
              bmi == 0 ? "Enter height & weight" : bmi.toStringAsFixed(1),
            ),
            leading: const Icon(Icons.monitor_weight),
          ),

          /// APPEARANCE
          sectionTitle("Appearance"),

          SwitchListTile(
            title: const Text("Dark Theme"),
            value: darkTheme,
            onChanged: (v) {
              setState(() {
                darkTheme = v;
              });
              saveSettings();
            },
          ),

          /// SYSTEM
          sectionTitle("System"),

          SwitchListTile(
            title: const Text("Doctor Mode"),
            value: doctorMode,
            onChanged: (v) {
              setState(() {
                doctorMode = v;
              });
              saveSettings();
            },
          ),

          const ListTile(
            title: Text("Language"),
            subtitle: Text("English (Arabic coming soon)"),
            leading: Icon(Icons.language),
          ),

          /// EMERGENCY
          sectionTitle("Emergency"),

          inputField("Emergency Contact", emergencyController),

          SwitchListTile(
            title: const Text("Fall Detection Alerts"),
            value: fallAlert,
            onChanged: (v) {
              setState(() {
                fallAlert = v;
              });
              saveSettings();
            },
          ),

          SwitchListTile(
            title: const Text("Heart Attack Alerts"),
            value: heartAlert,
            onChanged: (v) {
              setState(() {
                heartAlert = v;
              });
              saveSettings();
            },
          ),

          /// DATA
          sectionTitle("Data"),

          const ListTile(
            leading: Icon(Icons.picture_as_pdf),
            title: Text("Export Medical PDF"),
            subtitle: Text("Future feature"),
          ),

          const ListTile(
            leading: Icon(Icons.cloud),
            title: Text("Cloud Sync"),
            subtitle: Text("Future feature"),
          ),

          ListTile(
            leading: const Icon(Icons.delete),
            title: const Text("Clear History"),
            onTap: () {},
          ),

          const SizedBox(height: 40),
        ],
      ),
    );
  }
}
