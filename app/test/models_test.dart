import 'dart:convert';

import 'package:flutter_test/flutter_test.dart';
import 'package:plantcare/alert_store.dart';
import 'package:plantcare/models.dart';
import 'package:shared_preferences_platform_interface/in_memory_shared_preferences_async.dart';
import 'package:shared_preferences_platform_interface/shared_preferences_async_platform_interface.dart';

final now = DateTime(2026, 10, 8, 12);

void main() {
  group('Telemetry', () {
    test('parses a full firmware message', () {
      // Exactly what src/app/task_net.cpp publishes
      const json = '{"moisture":42,"tank":85,"light":70,"temp":28.4,"hum":61,"pump":false,"watering":true,'
          '"locked":false,"status":"WATERING","alarms":["dehydration"],"uptime":3605,"rssi":-58}';
      final t = Telemetry.fromJson(jsonDecode(json) as Map<String, dynamic>, receivedAt: now);
      expect(t.moisture, 42);
      expect(t.tank, 85);
      expect(t.light, 70);
      expect(t.temp, 28.4);
      expect(t.hum, 61);
      expect(t.pump, isFalse);
      expect(t.watering, isTrue);
      expect(t.locked, isFalse);
      expect(t.status, 'WATERING');
      expect(t.alarms, ['dehydration']);
      expect(t.uptime, 3605);
      expect(t.rssi, -58);
      expect(t.retained, isFalse);
    });

    test('failed sensors come through as null', () {
      const json = '{"moisture":null,"tank":null,"light":12,"temp":null,"hum":null,"pump":false,"watering":false,'
          '"locked":true,"status":"SENSOR FAIL","alarms":["tank_sensor","dht_sensor"],"uptime":5}';
      final t = Telemetry.fromJson(jsonDecode(json) as Map<String, dynamic>, receivedAt: now, retained: true);
      expect(t.moisture, isNull);
      expect(t.tank, isNull);
      expect(t.temp, isNull);
      expect(t.hum, isNull);
      expect(t.locked, isTrue);
      expect(t.alarms, hasLength(2));
      expect(t.rssi, isNull);
      expect(t.retained, isTrue);
    });

    test('tolerates missing fields', () {
      final t = Telemetry.fromJson({}, receivedAt: now);
      expect(t.status, 'OK');
      expect(t.alarms, isEmpty);
      expect(t.pump, isFalse);
    });
  });

  group('PlantAlert', () {
    test('round-trips through storage JSON', () {
      final a = PlantAlert.fromJson(
          {'type': 'low_tank', 'msg': 'Water tank is low (12%).', 'uptime': 99}, receivedAt: now);
      final b = PlantAlert.fromJson(a.toJson());
      expect(b.key, a.key);
      expect(b.receivedAt, now);
    });

    test('titles cover every alert type and alarm name in the contract', () {
      for (final type in [
        'dehydration', 'low_tank', 'temp_high', 'temp_low', 'pump_fault', 'manual_refused', 'sensor_fault',
        'soil_sensor', 'tank_sensor', 'dht_sensor', 'offline',
      ]) {
        expect(alertTitle(type), isNot('Plant alert'), reason: type);
      }
    });
  });

  test('ago', () {
    expect(ago(now.subtract(const Duration(seconds: 3)), now), 'just now');
    expect(ago(now.subtract(const Duration(seconds: 42)), now), '42 s ago');
    expect(ago(now.subtract(const Duration(minutes: 5)), now), '5 min ago');
    expect(ago(now.subtract(const Duration(hours: 3)), now), '3 h ago');
    expect(ago(now.subtract(const Duration(days: 2)), now), '2 d ago');
  });

  group('AlertStore', () {
    setUp(() => SharedPreferencesAsyncPlatform.instance = InMemorySharedPreferencesAsync.empty());

    PlantAlert alert(int uptime) =>
        PlantAlert(type: 'temp_high', msg: 'Too hot: 36.0 C', uptime: uptime, receivedAt: now);

    test('stores an alert once even if both isolates add it', () async {
      final store = AlertStore();
      expect(await store.add(alert(10)), isTrue);
      expect(await AlertStore().add(alert(10)), isFalse);
      expect(await store.load(), hasLength(1));
    });

    test('keeps the newest alerts first, up to the limit', () async {
      final store = AlertStore();
      for (var i = 0; i < AlertStore.maxAlerts + 5; i++) {
        await store.add(alert(i));
      }
      final list = await store.load();
      expect(list, hasLength(AlertStore.maxAlerts));
      expect(list.first.uptime, AlertStore.maxAlerts + 4);
    });

    test('clear', () async {
      final store = AlertStore();
      await store.add(alert(1));
      await store.clear();
      expect(await store.load(), isEmpty);
    });
  });
}
