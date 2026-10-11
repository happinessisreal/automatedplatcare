import 'package:flutter/material.dart';

/// Big ring showing a 0–100 % reading, or "--" when the sensor has no value.
class RingGauge extends StatelessWidget {
  const RingGauge({super.key, required this.label, required this.value, required this.icon, required this.color});

  final String label;
  final int? value;
  final IconData icon;
  final Color color;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return Card(
      child: Padding(
        padding: const EdgeInsets.symmetric(vertical: 20, horizontal: 12),
        child: Column(children: [
          SizedBox.square(
            dimension: 112,
            child: Stack(fit: StackFit.expand, children: [
              CircularProgressIndicator(
                value: (value ?? 0) / 100,
                strokeWidth: 10,
                strokeCap: StrokeCap.round,
                color: color,
                backgroundColor: theme.colorScheme.surfaceContainerHighest,
              ),
              Center(
                child: Column(mainAxisSize: MainAxisSize.min, children: [
                  Icon(icon, color: color, size: 22),
                  Text(value == null ? '--' : '$value%',
                      style: theme.textTheme.headlineSmall?.copyWith(fontWeight: FontWeight.w600)),
                ]),
              ),
            ]),
          ),
          const SizedBox(height: 12),
          Text(label, style: theme.textTheme.titleSmall),
        ]),
      ),
    );
  }
}

/// Small card with an icon, a value and a label.
class MetricTile extends StatelessWidget {
  const MetricTile({super.key, required this.icon, required this.value, required this.label});

  final IconData icon;
  final String value;
  final String label;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return Card(
      child: Padding(
        padding: const EdgeInsets.symmetric(vertical: 14, horizontal: 8),
        child: Column(children: [
          Icon(icon, color: theme.colorScheme.primary),
          const SizedBox(height: 6),
          Text(value, style: theme.textTheme.titleMedium?.copyWith(fontWeight: FontWeight.w600)),
          Text(label, style: theme.textTheme.bodySmall),
        ]),
      ),
    );
  }
}
