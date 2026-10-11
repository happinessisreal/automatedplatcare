// Prints everything published under plantcare/<DEVICE_ID>/, and can send a watering command.
//
//   dart run bin/watch.dart                 watch until Ctrl+C
//   dart run bin/watch.dart --water         also send {"action":"water"} after subscribing
//   dart run bin/watch.dart --seconds 20    stop after 20 s
//   dart run bin/watch.dart --id <id>       another device ID

import 'dart:async';
import 'dart:io';

import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_tools/mqtt_tools.dart';

Future<void> main(List<String> args) async {
  final base = topicBase(deviceId(args));
  final client = await connect('plantcare-watch');
  print('[${timestamp()}] connected to $brokerHost:$brokerPort, watching $base/#');

  client.updates!.listen((messages) {
    for (final m in messages) {
      final name = m.topic.substring(base.length + 1);
      print('[${timestamp()}] $name: ${payloadText(m)}');
    }
  });
  client.subscribe('$base/#', MqttQos.atLeastOnce);

  if (args.contains('--water')) {
    await Future<void>.delayed(const Duration(seconds: 2));
    publish(client, '$base/cmd', '{"action":"water"}', qos: MqttQos.atLeastOnce);
    print('[${timestamp()}] sent cmd: {"action":"water"}');
  }

  final i = args.indexOf('--seconds');
  if (i >= 0 && i + 1 < args.length) {
    await Future<void>.delayed(Duration(seconds: int.parse(args[i + 1])));
    client.disconnect();
    exit(0);
  }
}
