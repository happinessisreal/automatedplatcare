import 'dart:async';
import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

import 'alert_store.dart';
import 'models.dart';
import 'mqtt_link.dart';
import 'settings.dart';

enum LinkState { unconfigured, connecting, connected, failed }

/// The app's live view of the plant: its own MQTT connection while the UI is open.
class PlantService extends ChangeNotifier {
  static const staleAfter = Duration(seconds: 30);

  ConnectionSettings settings = ConnectionSettings.defaults;
  LinkState link = LinkState.connecting;
  Telemetry? telemetry;
  bool? deviceOnline;
  List<PlantAlert> alerts = [];

  /// Last alert received while the app is open, for an in-app banner.
  PlantAlert? liveAlert;

  final _store = AlertStore();
  MqttServerClient? _client;
  Timer? _retry, _ticker;

  Future<void> start() async {
    settings = await ConnectionSettings.load();
    alerts = await _store.load();
    // Re-render every few seconds so "last seen" and staleness stay current.
    _ticker = Timer.periodic(const Duration(seconds: 5), (_) => notifyListeners());
    await connect();
  }

  /// Fresh data that isn't just the broker's retained copy.
  bool get hasLiveData =>
      telemetry != null && !telemetry!.retained && DateTime.now().difference(telemetry!.receivedAt) < staleAfter;

  bool get deviceReachable => link == LinkState.connected && deviceOnline != false && hasLiveData;

  /// Why "Water now" is unavailable, or null if it can be pressed.
  String? get waterBlockedReason {
    if (link != LinkState.connected) return 'Not connected to the broker';
    if (!deviceReachable) return 'The plant monitor is offline';
    if (telemetry!.locked) return 'Tank is low: refill it first';
    if (telemetry!.watering) return 'Already watering';
    return null;
  }

  Future<void> connect() async {
    _retry?.cancel();
    _client?.disconnect();
    _client = null;
    telemetry = null;
    deviceOnline = null;
    if (!settings.isComplete) {
      link = LinkState.unconfigured;
      notifyListeners();
      return;
    }
    link = LinkState.connecting;
    notifyListeners();
    try {
      final client = await connectMqtt(settings, 'app');
      client.onAutoReconnect = () => _setLink(client, LinkState.connecting);
      client.onAutoReconnected = () => _setLink(client, LinkState.connected);
      client.updates!.listen(_onMessages);
      client.subscribe('${settings.topicBase}/#', MqttQos.atLeastOnce);
      _client = client;
      link = LinkState.connected;
    } catch (_) {
      link = LinkState.failed;
      _retry = Timer(const Duration(seconds: 10), connect);
    }
    notifyListeners();
  }

  void _setLink(MqttServerClient client, LinkState state) {
    if (!identical(client, _client)) return;
    link = state;
    notifyListeners();
  }

  void _onMessages(List<MqttReceivedMessage<MqttMessage>> messages) {
    for (final m in messages) {
      final msg = decodeMessage(m);
      try {
        switch (msg.name) {
          case 'telemetry':
            telemetry = Telemetry.fromJson(jsonDecode(msg.text) as Map<String, dynamic>,
                receivedAt: DateTime.now(), retained: msg.retained);
          case 'online':
            deviceOnline = msg.text == '1';
          case 'alert':
            final alert = PlantAlert.fromJson(jsonDecode(msg.text) as Map<String, dynamic>, receivedAt: DateTime.now());
            if (!alerts.any((a) => a.key == alert.key)) alerts = [alert, ...alerts.take(AlertStore.maxAlerts - 1)];
            liveAlert = alert;
            _store.add(alert);
        }
      } on FormatException {
        // Not valid JSON: ignore.
      }
    }
    notifyListeners();
  }

  /// Sends a manual watering command. Returns false if it couldn't be sent.
  bool water() {
    final client = _client;
    if (client == null || waterBlockedReason != null) return false;
    publishText(client, '${settings.topicBase}/cmd', jsonEncode({'action': 'water'}), qos: MqttQos.atLeastOnce);
    return true;
  }

  Future<void> reloadAlerts() async {
    alerts = await _store.load();
    notifyListeners();
  }

  Future<void> clearAlerts() async {
    await _store.clear();
    alerts = [];
    notifyListeners();
  }

  Future<void> updateSettings(ConnectionSettings next) async {
    await next.save();
    settings = next;
    await connect();
  }

  @override
  void dispose() {
    _retry?.cancel();
    _ticker?.cancel();
    _client?.disconnect();
    super.dispose();
  }
}
