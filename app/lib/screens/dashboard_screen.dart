import 'package:flutter/material.dart';

import '../models.dart';
import '../plant_service.dart';
import '../widgets.dart';

class DashboardScreen extends StatelessWidget {
  const DashboardScreen({super.key, required this.service});

  final PlantService service;

  @override
  Widget build(BuildContext context) {
    final t = service.telemetry;
    final theme = Theme.of(context);
    return RefreshIndicator(
      onRefresh: service.connect,
      child: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          Row(children: [
            Expanded(child: Text('Plant Care', style: theme.textTheme.headlineMedium)),
            _OnlineChip(service: service),
          ]),
          const SizedBox(height: 12),
          if (service.link == LinkState.unconfigured)
            const _Notice(icon: Icons.settings, text: 'Enter the device ID shown on the LCD in Settings.')
          else if (service.link == LinkState.failed)
            const _Notice(icon: Icons.cloud_off, text: "Can't reach the broker. Check the phone's internet; retrying.")
          else if (t != null && !service.hasLiveData)
            const _Notice(icon: Icons.history, text: 'Showing the last known readings. The plant monitor is offline.'),
          if (t != null) _StatusCard(t: t, live: service.hasLiveData),
          const SizedBox(height: 8),
          Row(children: [
            Expanded(
                child: RingGauge(
                    label: 'Soil moisture', value: t?.moisture, icon: Icons.grass, color: _moistureColor(t?.moisture))),
            const SizedBox(width: 8),
            Expanded(
                child: RingGauge(
                    label: 'Water tank', value: t?.tank, icon: Icons.water_drop, color: _tankColor(t?.tank, t?.locked))),
          ]),
          const SizedBox(height: 8),
          Row(children: [
            Expanded(
                child: MetricTile(
                    icon: Icons.thermostat,
                    value: t?.temp == null ? '--' : '${t!.temp!.toStringAsFixed(1)} °C',
                    label: 'Temperature')),
            Expanded(
                child: MetricTile(
                    icon: Icons.water_outlined, value: t?.hum == null ? '--' : '${t!.hum}%', label: 'Humidity')),
            Expanded(
                child: MetricTile(
                    icon: Icons.wb_sunny_outlined, value: t?.light == null ? '--' : '${t!.light}%', label: 'Light')),
          ]),
          const SizedBox(height: 8),
          Card(
            child: ListTile(
              leading: Icon(t?.pump == true ? Icons.opacity : Icons.power_settings_new,
                  color: t?.pump == true ? Colors.blue : null),
              title: Text(t == null
                  ? 'Pump'
                  : t.pump
                      ? 'Pump running'
                      : t.watering
                          ? 'Soaking between pulses'
                          : 'Pump idle'),
              subtitle: Text(t?.locked == true ? 'Locked: tank is low' : 'Waters automatically below 35%'),
            ),
          ),
          const SizedBox(height: 16),
          _WaterButton(service: service),
          if (t != null) ...[
            const SizedBox(height: 16),
            Text(
              'Updated ${ago(t.receivedAt)} · up ${_duration(t.uptime)}${t.rssi == null ? '' : ' · Wi-Fi ${t.rssi} dBm'}',
              textAlign: TextAlign.center,
              style: theme.textTheme.bodySmall,
            ),
          ],
        ],
      ),
    );
  }

  static Color _moistureColor(int? v) => v == null
      ? Colors.grey
      : v < 20
          ? Colors.red
          : v < 35
              ? Colors.orange
              : Colors.green;

  static Color _tankColor(int? v, bool? locked) => v == null
      ? Colors.grey
      : locked == true || v < 15
          ? Colors.red
          : v < 30
              ? Colors.orange
              : Colors.blue;

  static String _duration(int s) {
    final d = Duration(seconds: s);
    if (d.inDays > 0) return '${d.inDays} d ${d.inHours % 24} h';
    if (d.inHours > 0) return '${d.inHours} h ${d.inMinutes % 60} min';
    return '${d.inMinutes} min';
  }
}

class _OnlineChip extends StatelessWidget {
  const _OnlineChip({required this.service});

  final PlantService service;

  @override
  Widget build(BuildContext context) {
    final (label, color) = switch (service.link) {
      LinkState.connected when service.deviceReachable => ('Online', Colors.green),
      LinkState.connected => ('Device offline', Colors.grey),
      LinkState.connecting => ('Connecting', Colors.amber),
      LinkState.failed => ('No internet', Colors.red),
      LinkState.unconfigured => ('Not set up', Colors.grey),
    };
    return Chip(
      avatar: Icon(Icons.circle, size: 12, color: color),
      label: Text(label),
      visualDensity: VisualDensity.compact,
    );
  }
}

class _Notice extends StatelessWidget {
  const _Notice({required this.icon, required this.text});

  final IconData icon;
  final String text;

  @override
  Widget build(BuildContext context) => Card(
        color: Theme.of(context).colorScheme.surfaceContainerHigh,
        child: ListTile(leading: Icon(icon), title: Text(text)),
      );
}

class _StatusCard extends StatelessWidget {
  const _StatusCard({required this.t, required this.live});

  final Telemetry t;
  final bool live;

  @override
  Widget build(BuildContext context) {
    final (color, icon, detail) = switch (t.status) {
      'OK' => (Colors.green, Icons.check_circle, 'Everything looks good.'),
      'WATERING' => (Colors.blue, Icons.water_drop, 'Watering in short pulses, letting each one soak in.'),
      'VERY DRY' => (Colors.orange, Icons.warning_amber, 'Soil is critically dry.'),
      'REFILL TANK' => (Colors.red, Icons.format_color_reset, 'Watering is paused until you refill the tank.'),
      'PUMP FAULT' => (Colors.red, Icons.build, "Watering isn't raising moisture. Check the pump and tubing."),
      'SENSOR FAIL' => (Colors.red, Icons.sensors_off, "A sensor isn't responding. Check its wiring."),
      'TOO HOT' => (Colors.deepOrange, Icons.thermostat, 'Temperature is above the safe range.'),
      'TOO COLD' => (Colors.indigo, Icons.ac_unit, 'Temperature is below the safe range.'),
      _ => (Colors.grey, Icons.info, ''),
    };
    final shade = live ? color : Colors.grey;
    return Card(
      color: shade.withValues(alpha: 0.15),
      child: ListTile(
        leading: Icon(icon, color: shade, size: 32),
        title: Text(t.status, style: TextStyle(color: shade, fontWeight: FontWeight.bold, letterSpacing: 0.5)),
        subtitle: Text([
          detail,
          if (t.alarms.length > 1) 'Also: ${t.alarms.map(alertTitle).toSet().join(', ')}',
        ].where((s) => s.isNotEmpty).join('\n')),
      ),
    );
  }
}

class _WaterButton extends StatelessWidget {
  const _WaterButton({required this.service});

  final PlantService service;

  @override
  Widget build(BuildContext context) {
    final reason = service.waterBlockedReason;
    return Column(children: [
      SizedBox(
        width: double.infinity,
        height: 56,
        child: FilledButton.icon(
          onPressed: reason != null
              ? null
              : () {
                  final sent = service.water();
                  ScaffoldMessenger.of(context).showSnackBar(SnackBar(
                    content: Text(sent ? 'Watering for 5 seconds' : "Couldn't send the command"),
                  ));
                },
          icon: const Icon(Icons.water_drop),
          label: const Text('Water now', style: TextStyle(fontSize: 18)),
        ),
      ),
      if (reason != null) ...[
        const SizedBox(height: 6),
        Text(reason, style: Theme.of(context).textTheme.bodySmall),
      ],
    ]);
  }
}
