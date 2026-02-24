import 'package:flutter/material.dart';

class AppTheme {

  static ThemeData dark = ThemeData(
    brightness: Brightness.dark,
    scaffoldBackgroundColor: const Color(0xFF0E0E12),

    colorScheme: const ColorScheme.dark(
      primary: Color(0xFFFF4D6D),
      secondary: Color(0xFF6C63FF),
    ),

    textTheme: const TextTheme(
      headlineLarge: TextStyle(fontSize: 32, fontWeight: FontWeight.bold),
      bodyLarge: TextStyle(fontSize: 18),
    ),
  );
}