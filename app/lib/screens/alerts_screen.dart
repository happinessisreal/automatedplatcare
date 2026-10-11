import 'package:flutter/material.dart';

import '../models.dart';
import '../plant_service.dart';

class AlertsScreen extends StatelessWidget {
  const AlertsScreen({super.key, required this.service});

  final PlantService service;

  static IconData _icon(String type) => switch (type) {
        'dehydration' => Icons.grass,
        'low_tank' || 'manual_refused' => Icons.water_drop,
        'temp_high' || 'temp_low' => Icons.thermostat,
        'pump_fault' => Icons.build,
        'sensor_fault' => Icons.sensors_off,
        _ => Icons.notifications,
      };

  @override
  Widget build(BuildContext context) {
    final alerts = service.alerts;
    final theme = Theme.of(context);
    return Column(children: [
      Padding(
        padding: const EdgeInsets.fromLTRB(16, 16, 8, 8),
        child: Row(children: [
          Expanded(child: Text('Alerts', style: theme.textTheme.headlineMedium)),
          if (alerts.isNotEmpty)
            TextButton.icon(
              onPressed: service.clearAlerts,
              icon: const Icon(Icons.delete_sweep_outlined),
              label: const Text('Clear'),
            ),
        ]),
      ),
      Expanded(
        child: alerts.isEmpty
            ? Center(
                child: Column(mainAxisSize: MainAxisSize.min, children: [
                  Icon(Icons.notifications_none, size: 48, color: theme.colorScheme.outline),
                  const SizedBox(height: 8),
                  const Text('No alerts yet'),
                ]),
              )
            : ListView.separated(
                itemCount: alerts.length,
                separatorBuilder: (_, _) => const Divider(height: 1),
                itemBuilder: (context, i) {
                  final a = alerts[i];
                  return ListTile(
                    leading: CircleAvatar(child: Icon(_icon(a.type), size: 20)),
                    title: Text(alertTitle(a.type)),
                    subtitle: Text(a.msg),
                    trailing: Text(ago(a.receivedAt), style: theme.textTheme.bodySmall),
                  );
                },
              ),
      ),
    ]);
  }
}
