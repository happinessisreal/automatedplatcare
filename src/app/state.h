#pragma once
#include <Arduino.h>

#include <atomic>

// Level conditions, as bits in State::alarms. alarmName() gives the MQTT name of each bit.
enum Alarm : uint16_t {
  ALARM_DEHYDRATION = 1 << 0,
  ALARM_LOW_TANK = 1 << 1,
  ALARM_TEMP_HIGH = 1 << 2,
  ALARM_TEMP_LOW = 1 << 3,
  ALARM_PUMP_FAULT = 1 << 4,
  ALARM_SOIL_SENSOR = 1 << 5,
  ALARM_TANK_SENSOR = 1 << 6,
  ALARM_DHT_SENSOR = 1 << 7,
};
constexpr int ALARM_COUNT = 8;
constexpr uint16_t SENSOR_ALARMS = ALARM_SOIL_SENSOR | ALARM_TANK_SENSOR | ALARM_DHT_SENSOR;

// One-off notifications sent to the phone. alertTypeName() gives the MQTT name.
enum class AlertType : uint8_t { Dehydration, LowTank, TempHigh, TempLow, PumpFault, ManualRefused, SensorFault };

// Latest readings and status shared by all tasks. Invalid readings are -1 (NAN for floats).
struct State {
  int moisture = -1;
  int tank = -1;
  int light = -1;
  float tempC = NAN;
  float humidity = NAN;
  bool pumpOn = false;
  bool watering = false;  // a watering cycle is running (pump on, or soaking between pulses)
  uint16_t alarms = 0;
};

extern std::atomic<bool> pumpLocked;      // tank -> irrigation; locked until the first good tank reading
extern std::atomic<bool> manualWaterReq;  // net -> irrigation
extern std::atomic<bool> netOnline;       // net -> display: MQTT connected

extern SemaphoreHandle_t stateMutex;
extern State sharedState;

void stateInit();
State stateGet();

// Runs fn(State&) while holding the state mutex.
template <typename F>
void stateUpdate(F fn) {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  fn(sharedState);
  xSemaphoreGive(stateMutex);
}

inline void setBit(uint16_t& alarms, uint16_t bit, bool on) { alarms = on ? (alarms | bit) : (alarms & ~bit); }

const char* alarmName(int bitIndex);
const char* alertTypeName(AlertType type);
const char* statusText(const State& s);  // highest-priority condition, max 11 chars for the LCD

void alertPost(AlertType type);   // never blocks; drops the alert if the queue is full
bool alertTake(AlertType& type);  // never blocks
