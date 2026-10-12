import 'dart:convert';
import 'dart:math';

import 'package:flutter/foundation.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

import 'settings.dart';

/// Connects (TLS unless the port is 1883) with automatic reconnects. Throws if the first attempt fails.
Future<MqttServerClient> connectMqtt(ConnectionSettings s, String role) async {
  final clientId = 'plantcare-$role-${Random().nextInt(0x7fffffff).toRadixString(16)}';
  final client = MqttServerClient.withPort(s.host, clientId, s.port)
    ..secure = s.port != 1883
    ..keepAlivePeriod = 30
    ..connectTimeoutPeriod = 8000
    ..autoReconnect = true;
  client.logging(on: false);
  client.connectionMessage = MqttConnectMessage().withClientIdentifier(clientId).startClean();
  try {
    await client.connect();
  } catch (e) {
    debugPrint('[mqtt] $role connect to ${s.host}:${s.port} failed: $e');
    client.disconnect();
    rethrow;
  }
  return client;
}

void publishText(MqttServerClient client, String topic, String text, {MqttQos qos = MqttQos.atMostOnce}) {
  final builder = MqttClientPayloadBuilder()..addUTF8String(text);
  client.publishMessage(topic, qos, builder.payload!);
}

/// Topic name after `plantcare/<id>/`, payload text, and whether it was a retained copy.
({String name, String text, bool retained}) decodeMessage(MqttReceivedMessage<MqttMessage> m) {
  final msg = m.payload as MqttPublishMessage;
  return (
    name: m.topic.split('/').last,
    text: utf8.decode(msg.payload.message),
    retained: msg.header?.retain ?? false,
  );
}
