import 'dart:convert';

import 'package:shared_preferences/shared_preferences.dart';

import 'models.dart';

/// Alert history on the phone. The app and the background service both write to it; an alert
/// they both received has the same key, so it's only stored once.
class AlertStore {
  static const maxAlerts = 50;
  static const _key = 'alerts';

  final _prefs = SharedPreferencesAsync();

  Future<List<PlantAlert>> load() async {
    final raw = await _prefs.getStringList(_key) ?? const [];
    return [for (final s in raw) PlantAlert.fromJson(jsonDecode(s) as Map<String, dynamic>)];
  }

  /// Returns false if the alert was already stored.
  Future<bool> add(PlantAlert alert) async {
    final list = await load();
    if (list.any((a) => a.key == alert.key)) return false;
    list.insert(0, alert);
    await _prefs.setStringList(_key, [for (final a in list.take(maxAlerts)) jsonEncode(a.toJson())]);
    return true;
  }

  Future<void> clear() => _prefs.remove(_key);
}
