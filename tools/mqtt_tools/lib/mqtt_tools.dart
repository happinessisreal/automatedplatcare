import 'dart:convert';
import 'dart:io';
import 'dart:math';

import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

// Must match include/plant_config.h and docs/mqtt-contract.md.
const brokerHost = 'broker.emqx.io';
const brokerPort = 8883;

/// The device ID from `--id <id>`, or else from the firmware's include/secrets.h.
String deviceId(List<String> args) {
  final i = args.indexOf('--id');
  if (i >= 0 && i + 1 < args.length) return args[i + 1];
  for (var dir = Directory.current; ; dir = dir.parent) {
    final secrets = File('${dir.path}/include/secrets.h');
    if (secrets.existsSync()) {
      final m = RegExp(r'#define\s+DEVICE_ID\s+"([0-9a-fA-F]+)"').firstMatch(secrets.readAsStringSync());
      if (m != null) return m.group(1)!;
    }
    if (dir.parent.path == dir.path) break;
  }
  throw StateError('No DEVICE_ID: pass --id <id> or run inside the project');
}

String topicBase(String id) => 'plantcare/$id';

/// Connects over TLS. With [willTopic], the broker publishes a retained "0" there if we vanish.
Future<MqttServerClient> connect(String clientPrefix, {String? willTopic}) async {
  final clientId = '$clientPrefix-${Random().nextInt(0x7fffffff).toRadixString(16)}';
  final client = MqttServerClient.withPort(brokerHost, clientId, brokerPort)
    ..secure = true
    ..keepAlivePeriod = 30
    ..autoReconnect = true;
  client.logging(on: false);
  var msg = MqttConnectMessage().withClientIdentifier(clientId).startClean();
  if (willTopic != null) {
    msg = msg.withWillTopic(willTopic).withWillMessage('0').withWillRetain().withWillQos(MqttQos.atLeastOnce);
  }
  client.connectionMessage = msg;
  await client.connect();
  return client;
}

void publish(MqttServerClient client, String topic, String payload,
    {bool retain = false, MqttQos qos = MqttQos.atMostOnce}) {
  final builder = MqttClientPayloadBuilder()..addUTF8String(payload);
  client.publishMessage(topic, qos, builder.payload!, retain: retain);
}

String payloadText(MqttReceivedMessage<MqttMessage> m) =>
    utf8.decode((m.payload as MqttPublishMessage).payload.message);

String timestamp() => DateTime.now().toIso8601String().substring(11, 19);
