import 'dart:async';
import 'dart:math';
import '../models/sensor_data.dart';

class BleService {
  Stream<SensorData>? dataStream;

  void startFakeStream() {
    final r = Random();

    dataStream = Stream.periodic(const Duration(seconds: 1), (_) {
      double bpm = 60 + r.nextInt(40).toDouble();
      double spo2 = 95 + r.nextInt(4).toDouble();
      double bodyTemp = 36 + r.nextDouble();
      double airTemp = 20 + r.nextDouble() * 10;
      double humidity = 40 + r.nextDouble() * 40;
      double voc = r.nextDouble() * 500;

      bool fall = r.nextInt(100) > 70;
      bool heartAttack = bpm > 100;
      bool polluted = voc > 300;
      bool handRemoved = r.nextInt(100) > 80;

      return SensorData(
        bpm: bpm,
        spo2: spo2,
        bodyTemp: bodyTemp,
        airTemp: airTemp,
        humidity: humidity,
        voc: voc,
        fallDetected: fall,
        heartAttack: heartAttack,
        airPolluted: polluted,
        handRemoved: handRemoved,
        time: DateTime.now().millisecondsSinceEpoch,
      );
    }).asBroadcastStream(); // ← important fix
  }
}
