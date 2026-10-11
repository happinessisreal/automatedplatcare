#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "plant_config.h"
#include "root_ca.h"
#include "secrets.h"
#include "state.h"
#include "tasks.h"

#define TOPIC(name) MQTT_TOPIC_PREFIX DEVICE_ID "/" name

static WiFiClientSecure tls;
static PubSubClient mqtt(tls);

static void onMessage(char* topic, byte* payload, unsigned int len) {
  JsonDocument doc;
  if (deserializeJson(doc, payload, len)) {
    Serial.println("[net] ignoring malformed command");
    return;
  }
  if (doc["action"] == "water") {
    Serial.println("[net] manual watering requested");
    manualWaterReq = true;
  }
}

static bool connectMqtt() {
  char clientId[48];
  snprintf(clientId, sizeof clientId, "plantcare-%s-%08lx", DEVICE_ID, (unsigned long)esp_random());
  // Last will: the broker marks the device offline if the connection drops without a goodbye.
  if (!mqtt.connect(clientId, nullptr, nullptr, TOPIC("online"), 1, true, "0")) return false;
  mqtt.publish(TOPIC("online"), "1", true);
  mqtt.subscribe(TOPIC("cmd"), 1);
  return true;
}

static void publishJson(const char* topic, JsonDocument& doc, bool retained) {
  char buf[512];
  size_t n = serializeJson(doc, buf, sizeof buf);
  mqtt.publish(topic, reinterpret_cast<const uint8_t*>(buf), n, retained);
}

static void publishTelemetry(const State& s) {
  JsonDocument doc;
  if (s.moisture >= 0) doc["moisture"] = s.moisture; else doc["moisture"] = nullptr;
  if (s.tank >= 0) doc["tank"] = s.tank; else doc["tank"] = nullptr;
  if (s.light >= 0) doc["light"] = s.light; else doc["light"] = nullptr;
  if (!isnan(s.tempC)) doc["temp"] = roundf(s.tempC * 10) / 10; else doc["temp"] = nullptr;
  if (!isnan(s.humidity)) doc["hum"] = lroundf(s.humidity); else doc["hum"] = nullptr;
  doc["pump"] = s.pumpOn;
  doc["watering"] = s.watering;
  doc["locked"] = pumpLocked.load();
  doc["status"] = statusText(s);
  JsonArray alarms = doc["alarms"].to<JsonArray>();
  for (int i = 0; i < ALARM_COUNT; i++)
    if (s.alarms & (1 << i)) alarms.add(alarmName(i));
  doc["uptime"] = millis() / 1000;
  doc["rssi"] = WiFi.RSSI();
  publishJson(TOPIC("telemetry"), doc, true);
}

static void sensorList(char* out, size_t n, uint16_t alarms) {
  snprintf(out, n, "%s%s%s", alarms & ALARM_SOIL_SENSOR ? " soil" : "", alarms & ALARM_TANK_SENSOR ? " tank-level" : "",
           alarms & ALARM_DHT_SENSOR ? " DHT22" : "");
}

static void publishAlert(AlertType type, const State& s) {
  char msg[128], list[40];
  switch (type) {
    case AlertType::Dehydration:
      snprintf(msg, sizeof msg, "Soil is critically dry (%d%%)", s.moisture);
      break;
    case AlertType::LowTank:
      snprintf(msg, sizeof msg, "Water tank is low (%d%%). Refill it; watering is paused.", s.tank);
      break;
    case AlertType::TempHigh:
      snprintf(msg, sizeof msg, "Too hot for the plant: %.1f C", s.tempC);
      break;
    case AlertType::TempLow:
      snprintf(msg, sizeof msg, "Too cold for the plant: %.1f C", s.tempC);
      break;
    case AlertType::PumpFault:
      snprintf(msg, sizeof msg, "Watering isn't raising soil moisture. Check the pump, tubing and soil sensor.");
      break;
    case AlertType::ManualRefused:
      snprintf(msg, sizeof msg, "Manual watering refused: the tank is low (%d%%)", s.tank);
      break;
    case AlertType::SensorFault:
      sensorList(list, sizeof list, s.alarms);
      snprintf(msg, sizeof msg, "Sensor not responding:%s", list[0] ? list : " unknown");
      break;
  }
  JsonDocument doc;
  doc["type"] = alertTypeName(type);
  doc["msg"] = msg;
  doc["uptime"] = millis() / 1000;
  publishJson(TOPIC("alert"), doc, false);
  Serial.printf("[net] sent alert %s: %s\n", alertTypeName(type), msg);
}

void netTask(void*) {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  tls.setCACert(ROOT_CA);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setBufferSize(640);
  mqtt.setKeepAlive(30);
  mqtt.setSocketTimeout(10);

  uint32_t retryAt = 0, backoff = 2000, lastTelemetry = 0, lastWifiLog = 0;
  State published;
  bool forcePublish = true;

  for (;;) {
    uint32_t now = millis();

    // Sensor tasks never wait on this one, so watering carries on with no network at all.
    if (WiFi.status() != WL_CONNECTED) {
      netOnline = false;
      if (now - lastWifiLog >= 10000) {
        lastWifiLog = now;
        Serial.printf("[net] waiting for Wi-Fi \"%s\"\n", WIFI_SSID);
      }
      vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }

    if (!mqtt.connected()) {
      netOnline = false;
      if ((int32_t)(now - retryAt) >= 0) {
        Serial.printf("[net] Wi-Fi ok (%s), connecting to %s:%d\n", WiFi.localIP().toString().c_str(), MQTT_HOST, MQTT_PORT);
        if (connectMqtt()) {
          Serial.println("[net] MQTT connected");
          backoff = 2000;
          forcePublish = true;
        } else {
          Serial.printf("[net] MQTT connect failed (state %d), retry in %lu s\n", mqtt.state(), backoff / 1000);
          retryAt = millis() + backoff;
          backoff = min<uint32_t>(backoff * 2, 60000);
        }
      }
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    netOnline = true;
    mqtt.loop();

    State s = stateGet();
    AlertType alert;
    while (alertTake(alert)) publishAlert(alert, s);

    bool changed = s.pumpOn != published.pumpOn || s.watering != published.watering || s.alarms != published.alarms;
    if (forcePublish || changed || now - lastTelemetry >= TELEMETRY_PERIOD_MS) {
      publishTelemetry(s);
      published = s;
      lastTelemetry = now;
      forcePublish = false;
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
