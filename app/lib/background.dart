import 'dart:async';
import 'dart:convert';

import 'package:flutter_foreground_task/flutter_foreground_task.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'alert_store.dart';
import 'models.dart';
import 'mqtt_link.dart';
import 'notifications.dart';
import 'settings.dart';

/// Messages from this service to the app UI (FlutterForegroundTask.sendDataToMain).
const alertsChangedMessage = 'alerts';

/// Message from the app UI to this service, after the connection settings change.
const reconnectMessage = 'reconnect';

const _serviceId = 300;
const _notifiedKey = 'notifiedAlarms';

void initBackgroundService() {
  FlutterForegroundTask.init(
    androidNotificationOptions: AndroidNotificationOptions(
      channelId: 'plant_monitor',
      channelName: 'Plant monitor connection',
      channelDescription: 'Keeps the connection to your plant open so alerts arrive while the app is closed',
      channelImportance: NotificationChannelImportance.LOW,
      priority: NotificationPriority.LOW,
      onlyAlertOnce: true,
    ),
    iosNotificationOptions: const IOSNotificationOptions(),
    foregroundTaskOptions: ForegroundTaskOptions(
      eventAction: ForegroundTaskEventAction.repeat(15000),
      autoRunOnBoot: true,
      autoRunOnMyPackageReplaced: true,
      allowWakeLock: true,
      allowWifiLock: true,
    ),
  );
}

Future<bool> startBackgroundService() async {
  if (await FlutterForegroundTask.isRunningService) return true;
  final result = await FlutterForegroundTask.startService(
    serviceId: _serviceId,
    serviceTypes: [ForegroundServiceTypes.specialUse],
    notificationTitle: 'Plant Care',
    notificationText: 'Connecting…',
    notificationIcon: const NotificationIcon(metaDataName: 'com.group09.plantcare.NOTIFICATION_ICON'),
    callback: _startAlertWatcher,
  );
  return result is ServiceRequestSuccess;
}

Future<void> stopBackgroundService() => FlutterForegroundTask.stopService();

@pragma('vm:entry-point')
void _startAlertWatcher() => FlutterForegroundTask.setTaskHandler(_AlertWatcher());

/// Runs in the foreground service's isolate: turns MQTT alerts into phone notifications.
class _AlertWatcher extends TaskHandler {
  final _store = AlertStore();
  final _prefs = SharedPreferencesAsync();
  MqttServerClient? _client;
  bool _connecting = false;
  bool _caughtUp = false;
  bool? _deviceOnline;
  Set<String> _notifiedAlarms = {};
  String _serviceText = '';

  @override
  Future<void> onStart(DateTime timestamp, TaskStarter starter) async {
    await initNotifications();
    _notifiedAlarms = (await _prefs.getStringList(_notifiedKey) ?? const []).toSet();
    await _connect();
  }

  @override
  void onRepeatEvent(DateTime timestamp) {
    if (_client?.connectionStatus?.state != MqttConnectionState.connected) _connect();
  }

  @override
  Future<void> onDestroy(DateTime timestamp, bool isTimeout) async => _client?.disconnect();

  @override
  void onReceiveData(Object data) {
    if (data == reconnectMessage) {
      _client?.disconnect();
      _client = null;
      _connect();
    }
  }

  @override
  void onNotificationPressed() => FlutterForegroundTask.launchApp();

  Future<void> _connect() async {
    if (_connecting) return;
    _connecting = true;
    try {
      final settings = await ConnectionSettings.load();
      if (!settings.isComplete) {
        _setServiceText('Set the device ID in the app');
        return;
      }
      final client = await connectMqtt(settings, 'bg');
      // After any reconnect, re-check the retained telemetry for alarms we haven't told the user about.
      client.onAutoReconnected = () => _caughtUp = false;
      client.onAutoReconnect = () => _setServiceText('Reconnecting…');
      client.updates!.listen(_onMessages);
      client.subscribe('${settings.topicBase}/#', MqttQos.atLeastOnce);
      _client = client;
      _caughtUp = false;
      _setServiceText('Watching your plant');
    } catch (_) {
      _setServiceText('No connection, retrying');
    } finally {
      _connecting = false;
    }
  }

  Future<void> _onMessages(List<MqttReceivedMessage<MqttMessage>> messages) async {
    for (final m in messages) {
      final msg = decodeMessage(m);
      try {
        switch (msg.name) {
          case 'alert':
            await _onAlert(PlantAlert.fromJson(jsonDecode(msg.text) as Map<String, dynamic>, receivedAt: DateTime.now()));
          case 'telemetry':
            await _onTelemetry(
                Telemetry.fromJson(jsonDecode(msg.text) as Map<String, dynamic>, receivedAt: DateTime.now()));
          case 'online':
            await _onOnline(msg.text == '1');
        }
      } on FormatException {
        // Not valid JSON: ignore.
      }
    }
  }

  Future<void> _onAlert(PlantAlert alert) async {
    await _store.add(alert);
    await showPlantNotification(alert.type, alertTitle(alert.type), alert.msg);
    _notifiedAlarms.add(alert.type);
    FlutterForegroundTask.sendDataToMain(alertsChangedMessage);
  }

  Future<void> _onTelemetry(Telemetry t) async {
    if (!_caughtUp) {
      _caughtUp = true;
      for (final alarm in t.alarms.where((a) => !_notifiedAlarms.contains(a))) {
        await showPlantNotification(alarm, alertTitle(alarm), alarmMessage(alarm, t));
      }
    }
    // Remember what's active, so a cleared alarm that comes back is notified again.
    final active = t.alarms.toSet();
    if (active.length != _notifiedAlarms.length || !active.containsAll(_notifiedAlarms)) {
      _notifiedAlarms = active;
      await _prefs.setStringList(_notifiedKey, active.toList());
    }
    _setServiceText('Soil ${t.moisture ?? '--'}% · Tank ${t.tank ?? '--'}% · ${t.status}');
  }

  Future<void> _onOnline(bool online) async {
    if (_deviceOnline == true && !online) {
      await showPlantNotification(
          'offline', alertTitle('offline'), 'The ESP32 lost its connection. Check its power and Wi-Fi.');
    }
    if (!online) _setServiceText('Plant monitor offline');
    _deviceOnline = online;
  }

  void _setServiceText(String text) {
    if (text == _serviceText) return;
    _serviceText = text;
    FlutterForegroundTask.updateService(notificationTitle: 'Plant Care', notificationText: text);
  }
}
