#include "alert_gate.h"
#include "plant_config.h"
#include "hysteresis.h"
#include "irrigation.h"
#include "sensors.h"
#include "state.h"
#include "tasks.h"

static const char* eventName(IrrigationEvent ev) {
  switch (ev) {
    case IrrigationEvent::CycleStarted: return "cycle started";
    case IrrigationEvent::CycleDone: return "done";
    case IrrigationEvent::Interrupted: return "interrupted (tank low or sensor fault)";
    case IrrigationEvent::PumpFault: return "PUMP FAULT: moisture not rising";
    case IrrigationEvent::ManualStarted: return "manual pulse started";
    case IrrigationEvent::ManualRefused: return "manual refused: tank low";
    default: return "";
  }
}

void irrigationTask(void*) {
  IrrigationController ctl({MOISTURE_LOW_PCT, MOISTURE_HIGH_PCT, PUMP_PULSE_MS, SOAK_MS, MAX_PULSES, MIN_RISE_PCT,
                            PUMP_FAULT_BACKOFF_MS, MANUAL_PULSE_MS});
  HysteresisAlarm critical(MOISTURE_CRITICAL_PCT, MOISTURE_CRITICAL_CLEAR_PCT);
  AlertGate dryGate(ALERT_REMINDER_MS);
  AlertGate sensorGate(ALERT_REMINDER_MS);

  int moisture = -1;
  int badReads = 0;
  bool pumpWasOn = false;
  uint32_t lastRead = 0;
  bool firstRead = true;

  for (;;) {
    uint32_t now = millis();

    if (firstRead || now - lastRead >= SOIL_READ_MS) {
      firstRead = false;
      lastRead = now;
      int pct = soilPercent(soilRaw());
      // One odd reading keeps the last good value; several in a row mean the sensor has failed.
      badReads = pct < 0 ? badReads + 1 : 0;
      bool sensorBad = badReads >= SENSOR_FAIL_COUNT;
      if (pct >= 0) moisture = pct;
      else if (sensorBad) moisture = -1;
      if (moisture >= 0) critical.update(moisture);
      bool dry = moisture >= 0 && critical.active();

      stateUpdate([&](State& s) {
        s.moisture = moisture;
        setBit(s.alarms, ALARM_DEHYDRATION, dry);
        setBit(s.alarms, ALARM_SOIL_SENSOR, sensorBad);
      });
      if (dryGate.update(dry, now)) alertPost(AlertType::Dehydration);
      if (sensorGate.update(sensorBad, now)) alertPost(AlertType::SensorFault);
    }

    IrrigationEvent ev = ctl.update(now, moisture, pumpLocked.load(), manualWaterReq.exchange(false));
    if (ev == IrrigationEvent::ManualRefused) alertPost(AlertType::ManualRefused);
    if (ev == IrrigationEvent::PumpFault) alertPost(AlertType::PumpFault);
    if (ev != IrrigationEvent::None) Serial.printf("[irrigation] %s (moisture %d%%)\n", eventName(ev), moisture);

    bool on = ctl.pumpOn();
    if (on != pumpWasOn) {
      digitalWrite(PIN_RELAY, on ? RELAY_ON : RELAY_OFF);
      pumpWasOn = on;
    }
    stateUpdate([&](State& s) {
      s.pumpOn = on;
      s.watering = ctl.watering();
      setBit(s.alarms, ALARM_PUMP_FAULT, ctl.faulted());
    });

    vTaskDelay(pdMS_TO_TICKS(IRRIGATION_TICK_MS));
  }
}
