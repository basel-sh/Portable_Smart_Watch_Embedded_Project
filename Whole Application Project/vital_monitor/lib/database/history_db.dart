import 'package:sqflite/sqflite.dart';
import 'package:path/path.dart';

class HistoryDB {
  static Database? _db;

  static Future<Database> get db async {
    if (_db != null) return _db!;

    final path = join(await getDatabasesPath(), "vital_history.db");

    _db = await openDatabase(
      path,
      version: 1,
      onCreate: (db, version) async {
        await db.execute('''
        CREATE TABLE history(
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          time INTEGER,
          bpm REAL,
          spo2 REAL,
          bodyTemp REAL,
          airTemp REAL,
          humidity REAL,
          voc REAL
        )
        ''');
      },
    );

    return _db!;
  }

  static Future<void> insert(Map<String, dynamic> data) async {
    final database = await db;

    await database.insert("history", data);
  }

  static Future<List<Map<String, dynamic>>> getAll() async {
    final database = await db;

    return database.query("history", orderBy: "time ASC");
  }

  static Future<void> clear() async {
    final database = await db;

    await database.delete("history");
  }
}
