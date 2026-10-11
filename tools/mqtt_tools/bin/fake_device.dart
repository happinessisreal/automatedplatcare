// Pretends to be the ESP32: publishes telemetry and alerts per docs/mqtt-contract.md and obeys
// {"action":"water"}. Time runs fast (short pulses and soaks) so a whole cycle takes seconds.
//
//   dart run bin/fake_device.dart [--id <id>]
//
// Type a command and Enter to force a situation:
//   dry, wet, empty, refill, hot, cold, normal, soilfail, dhtfail, quit, crash

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:math';

import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'package:mqtt_tools/mqtt_tools.dart';

const pulseSec = 3, soakSec = 5, manualSec = 3;

class FakeDevice {
  FakeDevice(this.client, this.base);

  final MqttServerClient client;
  final String base;
  final rnd = Random();

  double moisture = 48, tank = 80, temp = 27.0, tempTarget = 27.0, hum = 60, light = 65;
  bool soilFail = false, dhtFail = false;
  bool locked = false;
  String phase = 'idle'; // idle | pulse | soak | manual
  int phaseLeft = 0, uptime = 0;
  bool manualRequest = false;
  final alarms = <String>{};
  String lastSent = '';
  int lastTelemetry = -100;

  bool get pump => phase == 'pulse' || phase == 'manual';
  bool get watering => phase != 'idle';

  String get status {
    if (alarms.contains('low_tank')) return 'REFILL TANK';
    if (alarms.contains('pump_fault')) return 'PUMP FAULT';
    if (alarms.any((a) => a.endsWith('_sensor'))) return 'SENSOR FAIL';
    if (alarms.contains('temp_high')) return 'TOO HOT';
    if (alarms.contains('temp_low')) return 'TOO COLD';
    if (watering) return 'WATERING';
    if (alarms.contains('dehydration')) return 'VERY DRY';
    return 'OK';
  }

  void tick() {
    uptime++;

    // Environment drift
    if (!watering) moisture = max(0, moisture - 0.6);
    temp += (tempTarget - temp) * 0.3 + (rnd.nextDouble() - 0.5) * 0.2;
    hum = (hum + (rnd.nextDouble() - 0.5) * 1.5).clamp(30, 90);
    light = (light + (rnd.nextDouble() - 0.5) * 4).clamp(0, 100);

    // Tank lock with the same 15/25 % on/off points as the firmware
    if (tank < 15) locked = true;
    if (tank > 25) locked = false;

    // Pulse-and-soak irrigation, as in lib/plantlogic/src/irrigation.cpp but faster
    if (manualRequest) {
      manualRequest = false;
      if (locked) {
        alert('manual_refused', 'Manual watering refused: the tank is low (${tank.round()}%)');
      } else if (phase == 'idle') {
        phase = 'manual';
        phaseLeft = manualSec;
        log('manual pulse started');
      }
    }
    if (pump && locked) {
      phase = 'idle';
      log('watering interrupted: tank low');
    }
    switch (phase) {
      case 'idle':
        if (!soilFail && !locked && moisture < 35) {
          phase = 'pulse';
          phaseLeft = pulseSec;
          log('watering cycle started (moisture ${moisture.round()}%)');
        }
      case 'pulse':
      case 'manual':
        tank = max(0, tank - 2.5);
        if (--phaseLeft <= 0) {
          phase = phase == 'manual' ? 'idle' : 'soak';
          phaseLeft = soakSec;
          if (phase == 'idle') moisture = min(100, moisture + 8);
        }
      case 'soak':
        moisture = min(100, moisture + 1.6);
        if (--phaseLeft <= 0) {
          if (moisture >= 60) {
            phase = 'idle';
            log('watering done (moisture ${moisture.round()}%)');
          } else {
            phase = 'pulse';
            phaseLeft = pulseSec;
          }
        }
    }

    // Alarms (level) and alerts (edges)
    setAlarm('dehydration', !soilFail && moisture < 20, !soilFail && moisture > 25,
        () => 'Soil is critically dry (${moisture.round()}%)');
    setAlarm('low_tank', tank < 15, tank > 25,
        () => 'Water tank is low (${tank.round()}%). Refill it; watering is paused.');
    setAlarm('temp_high', !dhtFail && temp > 35, dhtFail || temp < 34,
        () => 'Too hot for the plant: ${temp.toStringAsFixed(1)} C');
    setAlarm('temp_low', !dhtFail && temp < 15, dhtFail || temp > 16,
        () => 'Too cold for the plant: ${temp.toStringAsFixed(1)} C');
    setAlarm('soil_sensor', soilFail, !soilFail, () => 'Sensor not responding: soil', alertType: 'sensor_fault');
    setAlarm('dht_sensor', dhtFail, !dhtFail, () => 'Sensor not responding: DHT22', alertType: 'sensor_fault');

    final changed = '$pump$watering${alarms.join()}' != lastSent;
    if (changed || uptime - lastTelemetry >= 5) sendTelemetry();
  }

  void setAlarm(String name, bool on, bool off, String Function() msg, {String? alertType}) {
    if (on && alarms.add(name)) alert(alertType ?? name, msg());
    if (off) alarms.remove(name);
  }

  void sendTelemetry() {
    final t = {
      'moisture': soilFail ? null : moisture.round(),
      'tank': tank.round(),
      'light': light.round(),
      'temp': dhtFail ? null : (temp * 10).round() / 10,
      'hum': dhtFail ? null : hum.round(),
      'pump': pump,
      'watering': watering,
      'locked': locked,
      'status': status,
      'alarms': alarms.toList(),
      'uptime': uptime,
      'rssi': -55 - rnd.nextInt(10),
    };
    publish(client, '$base/telemetry', jsonEncode(t), retain: true);
    lastSent = '$pump$watering${alarms.join()}';
    lastTelemetry = uptime;
    log('telemetry ${jsonEncode(t)}');
  }

  void alert(String type, String msg) {
    publish(client, '$base/alert', jsonEncode({'type': type, 'msg': msg, 'uptime': uptime}));
    log('ALERT $type: $msg');
  }

  void command(String line) {
    switch (line.trim()) {
      case 'dry': moisture = 15;
      case 'wet': moisture = 70;
      case 'empty': tank = 5;
      case 'refill': tank = 95;
      case 'hot': tempTarget = 38;
      case 'cold': tempTarget = 12;
      case 'normal': tempTarget = 27;
      case 'soilfail': soilFail = !soilFail;
      case 'dhtfail': dhtFail = !dhtFail;
      case 'quit':
        publish(client, '$base/online', '0', retain: true, qos: MqttQos.atLeastOnce);
        Future<void>.delayed(const Duration(milliseconds: 500), () {
          client.disconnect();
          exit(0);
        });
        return;
      case 'crash':
        log('exiting without disconnecting: the broker should publish online=0 (last will)');
        exit(1);
      case '':
        return;
      default:
        log('unknown command "$line"');
        return;
    }
    log('command: ${line.trim()}');
    sendTelemetry();
  }

  void log(String s) => print('[${timestamp()}] $s');
}

Future<void> main(List<String> args) async {
  final base = topicBase(deviceId(args));
  final client = await connect('plantcare-fake', willTopic: '$base/online');
  final dev = FakeDevice(client, base);
  dev.log('fake device online at $brokerHost:$brokerPort as $base');

  publish(client, '$base/online', '1', retain: true, qos: MqttQos.atLeastOnce);
  client.subscribe('$base/cmd', MqttQos.atLeastOnce);
  client.updates!.listen((messages) {
    for (final m in messages) {
      final text = payloadText(m);
      dev.log('cmd received: $text');
      try {
        if ((jsonDecode(text) as Map)['action'] == 'water') dev.manualRequest = true;
      } on FormatException {
        dev.log('ignoring malformed command');
      }
    }
  });

  stdin.transform(utf8.decoder).transform(const LineSplitter()).listen(dev.command);
  Timer.periodic(const Duration(seconds: 1), (_) => dev.tick());
}
