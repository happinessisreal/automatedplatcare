// Message shapes from docs/mqtt-contract.md.

class Telemetry {
  const Telemetry({
    this.moisture,
    this.tank,
    this.light,
    this.temp,
    this.hum,
    this.pump = false,
    this.watering = false,
    this.locked = false,
    this.status = 'OK',
    this.alarms = const [],
    this.uptime = 0,
    this.rssi,
    required this.receivedAt,
    this.retained = false,
  });

  final int? moisture, tank, light, hum;
  final double? temp;
  final bool pump, watering, locked;
  final String status;
  final List<String> alarms;
  final int uptime;
  final int? rssi;
  final DateTime receivedAt;

  /// Delivered from the broker's retained copy on subscribe: it could be hours old.
  final bool retained;

  factory Telemetry.fromJson(Map<String, dynamic> j, {required DateTime receivedAt, bool retained = false}) =>
      Telemetry(
        moisture: (j['moisture'] as num?)?.round(),
        tank: (j['tank'] as num?)?.round(),
        light: (j['light'] as num?)?.round(),
        temp: (j['temp'] as num?)?.toDouble(),
        hum: (j['hum'] as num?)?.round(),
        pump: j['pump'] == true,
        watering: j['watering'] == true,
        locked: j['locked'] == true,
        status: j['status'] as String? ?? 'OK',
        alarms: (j['alarms'] as List?)?.whereType<String>().toList() ?? const [],
        uptime: (j['uptime'] as num?)?.toInt() ?? 0,
        rssi: (j['rssi'] as num?)?.toInt(),
        receivedAt: receivedAt,
        retained: retained,
      );
}

class PlantAlert {
  const PlantAlert({required this.type, required this.msg, required this.uptime, required this.receivedAt});

  final String type, msg;
  final int uptime;
  final DateTime receivedAt;

  /// Same alert seen by both the app and the background service gets the same key.
  String get key => '$type|$uptime|$msg';

  factory PlantAlert.fromJson(Map<String, dynamic> j, {DateTime? receivedAt}) => PlantAlert(
        type: j['type'] as String? ?? 'unknown',
        msg: j['msg'] as String? ?? '',
        uptime: (j['uptime'] as num?)?.toInt() ?? 0,
        receivedAt: receivedAt ?? DateTime.tryParse(j['receivedAt'] as String? ?? '') ?? DateTime.now(),
      );

  Map<String, dynamic> toJson() =>
      {'type': type, 'msg': msg, 'uptime': uptime, 'receivedAt': receivedAt.toIso8601String()};
}

/// Short title for an alert type or alarm name.
String alertTitle(String type) => switch (type) {
      'dehydration' => 'Soil critically dry',
      'low_tank' => 'Water tank low',
      'temp_high' => 'Too hot',
      'temp_low' => 'Too cold',
      'pump_fault' => 'Pump fault',
      'manual_refused' => 'Watering refused',
      'sensor_fault' || 'soil_sensor' || 'tank_sensor' || 'dht_sensor' => 'Sensor not responding',
      'offline' => 'Plant monitor offline',
      _ => 'Plant alert',
    };

/// Message for an alarm that was already active when we connected (no alert text to show).
String alarmMessage(String alarm, Telemetry t) => switch (alarm) {
      'dehydration' => 'Soil moisture is ${t.moisture ?? '--'}%.',
      'low_tank' => 'Tank is at ${t.tank ?? '--'}%. Watering is paused until you refill it.',
      'temp_high' || 'temp_low' => 'Temperature is ${t.temp?.toStringAsFixed(1) ?? '--'} °C.',
      'pump_fault' => "Watering isn't raising soil moisture. Check the pump, tubing and soil sensor.",
      'soil_sensor' => "The soil moisture sensor isn't responding.",
      'tank_sensor' => "The tank level sensor isn't responding. Watering is paused.",
      'dht_sensor' => "The temperature/humidity sensor isn't responding.",
      _ => 'Status: ${t.status}',
    };

String ago(DateTime t, [DateTime? now]) {
  final d = (now ?? DateTime.now()).difference(t);
  if (d.inSeconds < 10) return 'just now';
  if (d.inMinutes < 1) return '${d.inSeconds} s ago';
  if (d.inHours < 1) return '${d.inMinutes} min ago';
  if (d.inDays < 1) return '${d.inHours} h ago';
  return '${d.inDays} d ago';
}
