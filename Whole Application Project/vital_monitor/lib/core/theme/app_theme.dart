import 'package:flutter/material.dart';

class AppTheme {
  static ThemeData dark = ThemeData(
    brightness: Brightness.dark,

    scaffoldBackgroundColor: const Color(0xFF0F172A),

    primaryColor: const Color(0xFF3B82F6),

    cardColor: const Color(0xFF1E293B),

    dividerColor: Colors.white12,

    appBarTheme: const AppBarTheme(
      backgroundColor: Color(0xFF0F172A),
      elevation: 0,
      centerTitle: true,
    ),

    bottomNavigationBarTheme: const BottomNavigationBarThemeData(
      backgroundColor: Color(0xFF1E293B),

      selectedItemColor: Color(0xFF3B82F6),

      unselectedItemColor: Colors.grey,

      type: BottomNavigationBarType.fixed,

      elevation: 8,
    ),

    cardTheme: CardThemeData(
      color: const Color(0xFF1E293B),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(18)),
    ),

    textTheme: const TextTheme(
      headlineLarge: TextStyle(fontSize: 30, fontWeight: FontWeight.bold),

      bodyLarge: TextStyle(fontSize: 18),
    ),
  );
}
