#include "state.h"

std::atomic<bool> pumpLocked{true};
std::atomic<bool> manualWaterReq{false};
std::atomic<bool> netOnline{false};

SemaphoreHandle_t stateMutex;
State sharedState;
static QueueHandle_t alertQueue;

static const char* const ALARM_NAMES[ALARM_COUNT] = {
    "dehydration", "low_tank", "temp_high", "temp_low", "pump_fault", "soil_sensor", "tank_sensor", "dht_sensor",
};

static const char* const ALERT_NAMES[] = {
    "dehydration", "low_tank", "temp_high", "temp_low", "pump_fault", "manual_refused", "sensor_fault",
};

void stateInit() {
  stateMutex = xSemaphoreCreateMutex();
  alertQueue = xQueueCreate(16, sizeof(AlertType));
}

State stateGet() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  State copy = sharedState;
  xSemaphoreGive(stateMutex);
  return copy;
}

const char* alarmName(int bitIndex) {
  return bitIndex >= 0 && bitIndex < ALARM_COUNT ? ALARM_NAMES[bitIndex] : "unknown";
}

const char* alertTypeName(AlertType type) { return ALERT_NAMES[static_cast<int>(type)]; }

const char* statusText(const State& s) {
  if (s.alarms & ALARM_LOW_TANK) return "REFILL TANK";
  if (s.alarms & ALARM_PUMP_FAULT) return "PUMP FAULT";
  if (s.alarms & SENSOR_ALARMS) return "SENSOR FAIL";
  if (s.alarms & ALARM_TEMP_HIGH) return "TOO HOT";
  if (s.alarms & ALARM_TEMP_LOW) return "TOO COLD";
  if (s.watering) return "WATERING";
  if (s.alarms & ALARM_DEHYDRATION) return "VERY DRY";
  return "OK";
}

void alertPost(AlertType type) {
  Serial.printf("[alert] %s\n", alertTypeName(type));
  xQueueSend(alertQueue, &type, 0);
}

bool alertTake(AlertType& type) { return xQueueReceive(alertQueue, &type, 0) == pdTRUE; }
