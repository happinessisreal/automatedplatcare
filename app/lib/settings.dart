import 'package:shared_preferences/shared_preferences.dart';

class ConnectionSettings {
  const ConnectionSettings({required this.host, required this.port, required this.deviceId});

  final String host;
  final int port;
  final String deviceId;

  bool get isComplete => host.isNotEmpty && deviceId.isNotEmpty;
  String get topicBase => 'plantcare/$deviceId';

  /// Build-time defaults from `--dart-define-from-file=env.json`, so the device ID isn't in the source.
  static const defaults = ConnectionSettings(
    host: String.fromEnvironment('MQTT_HOST', defaultValue: 'broker.hivemq.com'),
    port: int.fromEnvironment('MQTT_PORT', defaultValue: 8883),
    deviceId: String.fromEnvironment('DEVICE_ID'),
  );

  static Future<ConnectionSettings> load() async {
    final p = SharedPreferencesAsync();
    return ConnectionSettings(
      host: await p.getString('host') ?? defaults.host,
      port: await p.getInt('port') ?? defaults.port,
      deviceId: await p.getString('deviceId') ?? defaults.deviceId,
    );
  }

  Future<void> save() async {
    final p = SharedPreferencesAsync();
    await p.setString('host', host);
    await p.setInt('port', port);
    await p.setString('deviceId', deviceId);
  }
}
