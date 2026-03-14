import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'connect_page.dart';

class OnboardingPage extends StatefulWidget {
  const OnboardingPage({super.key});

  @override
  State<OnboardingPage> createState() => _OnboardingPageState();
}

class _OnboardingPageState extends State<OnboardingPage> {
  final nameController = TextEditingController();
  final ageController = TextEditingController();
  final heightController = TextEditingController();
  final weightController = TextEditingController();

  String gender = "Male";

  double bmi = 0;

  void calculateBMI() {
    double h = double.tryParse(heightController.text) ?? 0;
    double w = double.tryParse(weightController.text) ?? 0;

    if (h > 0 && w > 0) {
      bmi = w / ((h / 100) * (h / 100));
    } else {
      bmi = 0;
    }
  }

  Future<void> finishSetup() async {
    if (nameController.text.isEmpty ||
        ageController.text.isEmpty ||
        heightController.text.isEmpty ||
        weightController.text.isEmpty) {
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(const SnackBar(content: Text("Please fill all fields")));

      return;
    }

    final prefs = await SharedPreferences.getInstance();

    prefs.setBool("firstRun", false);

    prefs.setString("name", nameController.text);
    prefs.setString("age", ageController.text);
    prefs.setString("height", heightController.text);
    prefs.setString("weight", weightController.text);
    prefs.setString("gender", gender);

    if (!mounted) return;

    Navigator.pushReplacement(
      context,
      MaterialPageRoute(builder: (_) => const ConnectPage()),
    );
  }

  Widget input(String label, TextEditingController c) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8),
      child: TextField(
        controller: c,
        keyboardType: TextInputType.number,
        decoration: InputDecoration(
          labelText: label,
          border: OutlineInputBorder(borderRadius: BorderRadius.circular(12)),
        ),
        onChanged: (v) {
          calculateBMI();
          setState(() {});
        },
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Medical Profile Setup")),

      body: Padding(
        padding: const EdgeInsets.all(20),

        child: ListView(
          children: [
            const Text(
              "Important Personal Data",
              style: TextStyle(fontSize: 22, fontWeight: FontWeight.bold),
            ),

            const SizedBox(height: 10),

            const Text(
              "This information helps the system calculate health metrics like BMI and improve monitoring accuracy.",
            ),

            const SizedBox(height: 20),

            TextField(
              controller: nameController,
              decoration: InputDecoration(
                labelText: "Full Name",
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(12),
                ),
              ),
            ),

            const SizedBox(height: 10),

            input("Age", ageController),

            DropdownButtonFormField<String>(
              initialValue: gender,
              items: const [
                DropdownMenuItem(value: "Male", child: Text("Male")),
                DropdownMenuItem(value: "Female", child: Text("Female")),
              ],
              onChanged: (v) {
                setState(() {
                  gender = v.toString();
                });
              },
              decoration: InputDecoration(
                labelText: "Gender",
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(12),
                ),
              ),
            ),

            const SizedBox(height: 10),

            input("Height (cm)", heightController),

            input("Weight (kg)", weightController),

            const SizedBox(height: 10),

            ListTile(
              leading: const Icon(Icons.monitor_weight),
              title: const Text("Calculated BMI"),
              subtitle: Text(
                bmi == 0 ? "Enter height and weight" : bmi.toStringAsFixed(1),
              ),
            ),

            const SizedBox(height: 30),

            ElevatedButton(
              onPressed: finishSetup,
              style: ElevatedButton.styleFrom(
                padding: const EdgeInsets.all(16),
              ),
              child: const Text("Finish Setup", style: TextStyle(fontSize: 18)),
            ),
          ],
        ),
      ),
    );
  }
}
