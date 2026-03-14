import 'package:flutter/material.dart';
import 'core/theme/app_theme.dart';
import 'pages/splash_page.dart';
import 'pages/settings_page.dart';

void main() {
  runApp(const VitalApp());
}

class VitalApp extends StatelessWidget {
  const VitalApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,

      theme: AppTheme.dark,

      routes: {"/settings": (context) => const SettingsPage()},

      home: const SplashPage(),
    );
  }
}
