#include "alert_gate.h"
#include "calibration.h"
#include "plant_config.h"
#include "hysteresis.h"
#include "sensors.h"
#include "state.h"
#include "tasks.h"

void tankTask(void*) {
  HysteresisAlarm low(TANK_LOCK_PCT, TANK_UNLOCK_PCT);
  AlertGate lowGate(ALERT_REMINDER_MS);
  AlertGate sensorGate(ALERT_REMINDER_MS);
  int badReads = 0;

  for (;;) {
    uint32_t now = millis();
    int pct = distanceToTankPct(tankDistanceCm(), TANK_DIST_EMPTY_CM, TANK_DIST_FULL_CM);
    badReads = pct < 0 ? badReads + 1 : 0;
    bool sensorBad = badReads >= SENSOR_FAIL_COUNT;
    if (pct >= 0) low.update(pct);
    bool isLow = low.active();

    // Fail safe: any bad reading locks the pump at once; the sensor alarm waits for several in a row.
    bool locked = pct < 0 || isLow;
    if (locked != pumpLocked.load()) Serial.printf("[tank] pump %s (tank %d%%)\n", locked ? "LOCKED" : "unlocked", pct);
    pumpLocked = locked;

    stateUpdate([&](State& s) {
      if (pct >= 0) s.tank = pct;
      else if (sensorBad) s.tank = -1;
      setBit(s.alarms, ALARM_LOW_TANK, isLow);
      setBit(s.alarms, ALARM_TANK_SENSOR, sensorBad);
    });
    if (lowGate.update(isLow, now)) alertPost(AlertType::LowTank);
    if (sensorGate.update(sensorBad, now)) alertPost(AlertType::SensorFault);

    vTaskDelay(pdMS_TO_TICKS(TANK_PERIOD_MS));
  }
}
