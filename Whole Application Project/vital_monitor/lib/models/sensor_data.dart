class SensorData {
  final int hr;
  final double temp;
  final int spo2;
  final int time;

  SensorData({
    required this.hr,
    required this.temp,
    required this.spo2,
    required this.time,
  });

  factory SensorData.fromBle(String data) {
    final parts = data.split(',');

    return SensorData(
      hr: int.parse(parts[0]),
      temp: double.parse(parts[1]),
      spo2: int.parse(parts[2]),
      time: DateTime.now().millisecondsSinceEpoch,
    );
  }
}
