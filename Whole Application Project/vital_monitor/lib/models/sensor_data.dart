class SensorData {

  final double bpm;
  final double spo2;
  final double bodyTemp;
  final double airTemp;
  final double humidity;
  final double voc;

  final bool fallDetected;
  final bool heartAttack;
  final bool airPolluted;
  final bool handRemoved;

  final int time;

  SensorData({
    required this.bpm,
    required this.spo2,
    required this.bodyTemp,
    required this.airTemp,
    required this.humidity,
    required this.voc,
    required this.fallDetected,
    required this.heartAttack,
    required this.airPolluted,
    required this.handRemoved,
    required this.time,
  });

  factory SensorData.fromBle(String raw) {

    final p = raw.split(',');

    return SensorData(
      bpm: double.parse(p[0]),
      spo2: double.parse(p[1]),
      bodyTemp: double.parse(p[2]),
      airTemp: double.parse(p[3]),
      humidity: double.parse(p[4]),
      voc: double.parse(p[5]),

      fallDetected: p[6] == "1",
      heartAttack: p[7] == "1",
      airPolluted: p[8] == "1",
      handRemoved: p[9] == "1",

      time: DateTime.now().millisecondsSinceEpoch,
    );
  }
}