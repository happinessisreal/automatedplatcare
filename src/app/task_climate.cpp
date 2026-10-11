#include "alert_gate.h"
#include "plant_config.h"
#include "hysteresis.h"
#include "sensors.h"
#include "state.h"
#include "tasks.h"

void climateTask(void*) {
  HysteresisAlarm hot(TEMP_MAX_C, TEMP_MAX_C - TEMP_HYST_C);
  HysteresisAlarm cold(TEMP_MIN_C, TEMP_MIN_C + TEMP_HYST_C);
  AlertGate hotGate(ALERT_REMINDER_MS);
  AlertGate coldGate(ALERT_REMINDER_MS);
  AlertGate sensorGate(ALERT_REMINDER_MS);
  int badReads = 0;

  for (;;) {
    uint32_t now = millis();
    float t, h;
    bool ok = readClimate(t, h);
    badReads = ok ? 0 : badReads + 1;
    bool sensorBad = badReads >= SENSOR_FAIL_COUNT;
    if (ok) {
      hot.update(t);
      cold.update(t);
    }
    bool isHot = hot.active() && !sensorBad;
    bool isCold = cold.active() && !sensorBad;

    stateUpdate([&](State& s) {
      if (ok) {
        s.tempC = t;
        s.humidity = h;
      } else if (sensorBad) {
        s.tempC = NAN;
        s.humidity = NAN;
      }
      setBit(s.alarms, ALARM_TEMP_HIGH, isHot);
      setBit(s.alarms, ALARM_TEMP_LOW, isCold);
      setBit(s.alarms, ALARM_DHT_SENSOR, sensorBad);
    });
    if (hotGate.update(isHot, now)) alertPost(AlertType::TempHigh);
    if (coldGate.update(isCold, now)) alertPost(AlertType::TempLow);
    if (sensorGate.update(sensorBad, now)) alertPost(AlertType::SensorFault);

    vTaskDelay(pdMS_TO_TICKS(CLIMATE_PERIOD_MS));
  }
}
