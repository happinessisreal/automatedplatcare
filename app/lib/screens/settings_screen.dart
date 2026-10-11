import 'package:flutter/material.dart';
import 'package:flutter_foreground_task/flutter_foreground_task.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../background.dart';
import '../plant_service.dart';
import '../settings.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({super.key, required this.service});

  final PlantService service;

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  final _host = TextEditingController();
  final _port = TextEditingController();
  final _deviceId = TextEditingController();
  bool _serviceRunning = false;
  bool _batteryExempt = false;
  bool _notificationsAllowed = false;
  ConnectionSettings? _shown;

  @override
  void initState() {
    super.initState();
    _refreshStatus();
  }

  /// Fills the fields whenever the service's settings object changes (loaded at startup, or saved).
  void _syncFields() {
    final s = widget.service.settings;
    if (identical(s, _shown)) return;
    _shown = s;
    _host.text = s.host;
    _port.text = '${s.port}';
    _deviceId.text = s.deviceId;
  }

  @override
  void dispose() {
    _host.dispose();
    _port.dispose();
    _deviceId.dispose();
    super.dispose();
  }

  Future<void> _refreshStatus() async {
    final running = await FlutterForegroundTask.isRunningService;
    final exempt = await FlutterForegroundTask.isIgnoringBatteryOptimizations;
    final allowed = await FlutterForegroundTask.checkNotificationPermission() == NotificationPermission.granted;
    if (!mounted) return;
    setState(() {
      _serviceRunning = running;
      _batteryExempt = exempt;
      _notificationsAllowed = allowed;
    });
  }

  Future<void> _save() async {
    final next = ConnectionSettings(
      host: _host.text.trim(),
      port: int.tryParse(_port.text.trim()) ?? 8883,
      deviceId: _deviceId.text.trim().toLowerCase(),
    );
    await widget.service.updateSettings(next);
    FlutterForegroundTask.sendDataToTask(reconnectMessage);
    if (mounted) ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text('Saved. Reconnecting…')));
  }

  Future<void> _toggleService(bool on) async {
    await SharedPreferencesAsync().setBool('backgroundAlerts', on);
    on ? await startBackgroundService() : await stopBackgroundService();
    await _refreshStatus();
  }

  @override
  Widget build(BuildContext context) {
    _syncFields();
    final theme = Theme.of(context);
    return ListView(padding: const EdgeInsets.all(16), children: [
      Text('Settings', style: theme.textTheme.headlineMedium),
      const SizedBox(height: 16),
      Text('Connection', style: theme.textTheme.titleMedium),
      const SizedBox(height: 8),
      TextField(
        controller: _deviceId,
        decoration: const InputDecoration(
          labelText: 'Device ID',
          helperText: 'Shown on the LCD when the plant monitor starts',
          border: OutlineInputBorder(),
        ),
      ),
      const SizedBox(height: 12),
      Row(children: [
        Expanded(
          flex: 3,
          child: TextField(
              controller: _host, decoration: const InputDecoration(labelText: 'Broker', border: OutlineInputBorder())),
        ),
        const SizedBox(width: 8),
        Expanded(
          child: TextField(
            controller: _port,
            keyboardType: TextInputType.number,
            decoration: const InputDecoration(labelText: 'Port', border: OutlineInputBorder()),
          ),
        ),
      ]),
      const SizedBox(height: 12),
      FilledButton(onPressed: _save, child: const Text('Save and reconnect')),
      const SizedBox(height: 24),
      Text('Alerts while the app is closed', style: theme.textTheme.titleMedium),
      SwitchListTile(
        contentPadding: EdgeInsets.zero,
        title: const Text('Background alerts'),
        subtitle: const Text('Keeps a connection open and shows a small permanent notification'),
        value: _serviceRunning,
        onChanged: _toggleService,
      ),
      ListTile(
        contentPadding: EdgeInsets.zero,
        leading: Icon(_notificationsAllowed ? Icons.check_circle : Icons.error_outline,
            color: _notificationsAllowed ? Colors.green : Colors.orange),
        title: const Text('Notification permission'),
        subtitle: Text(_notificationsAllowed ? 'Allowed' : 'Not allowed: alerts will be silent'),
        trailing: _notificationsAllowed
            ? null
            : TextButton(
                onPressed: () async {
                  await FlutterForegroundTask.requestNotificationPermission();
                  await _refreshStatus();
                },
                child: const Text('Allow'),
              ),
      ),
      ListTile(
        contentPadding: EdgeInsets.zero,
        leading: Icon(_batteryExempt ? Icons.check_circle : Icons.battery_alert,
            color: _batteryExempt ? Colors.green : Colors.orange),
        title: const Text('Battery optimisation'),
        subtitle: Text(_batteryExempt ? 'Exempt: alerts stay reliable' : 'Android may delay or stop alerts'),
        trailing: _batteryExempt
            ? null
            : TextButton(
                onPressed: () async {
                  await FlutterForegroundTask.requestIgnoreBatteryOptimization();
                  await _refreshStatus();
                },
                child: const Text('Exempt'),
              ),
      ),
      const SizedBox(height: 24),
      Text(
        'Automated Indoor Plant Care System · CSE 4102 Group 09\n'
        'MQTT over TLS: ${widget.service.settings.host}:${widget.service.settings.port}',
        style: theme.textTheme.bodySmall,
      ),
    ]);
  }
}
