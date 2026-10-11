#include <unity.h>

#include "alert_gate.h"
#include "calibration.h"
#include "hysteresis.h"
#include "irrigation.h"

void setUp() {}
void tearDown() {}

// ---- calibration ----

void test_raw_to_percent_inverted_sensor() {
  // Soil sensor: 3000 raw = dry (0 %), 1300 raw = wet (100 %)
  TEST_ASSERT_EQUAL_INT(0, rawToPercent(3000, 3000, 1300));
  TEST_ASSERT_EQUAL_INT(100, rawToPercent(1300, 3000, 1300));
  TEST_ASSERT_EQUAL_INT(50, rawToPercent(2150, 3000, 1300));
}

void test_raw_to_percent_clamps_and_guards() {
  TEST_ASSERT_EQUAL_INT(0, rawToPercent(3500, 3000, 1300));
  TEST_ASSERT_EQUAL_INT(100, rawToPercent(900, 3000, 1300));
  TEST_ASSERT_EQUAL_INT(0, rawToPercent(1234, 2000, 2000));
}

void test_tank_percent() {
  TEST_ASSERT_EQUAL_INT(100, distanceToTankPct(5.0f, 20.0f, 5.0f));
  TEST_ASSERT_EQUAL_INT(0, distanceToTankPct(20.0f, 20.0f, 5.0f));
  TEST_ASSERT_EQUAL_INT(50, distanceToTankPct(12.5f, 20.0f, 5.0f));
  TEST_ASSERT_EQUAL_INT(100, distanceToTankPct(3.0f, 20.0f, 5.0f));
  TEST_ASSERT_EQUAL_INT(0, distanceToTankPct(25.0f, 20.0f, 5.0f));
}

void test_tank_percent_invalid() {
  TEST_ASSERT_EQUAL_INT(-1, distanceToTankPct(0.0f, 20.0f, 5.0f));
  TEST_ASSERT_EQUAL_INT(-1, distanceToTankPct(10.0f, 5.0f, 20.0f));
}

void test_median5() {
  const float a[5] = {9, 1, 5, 3, 7};
  TEST_ASSERT_EQUAL_FLOAT(5, median5(a));
  // Two missed echoes (0) don't drag the median down; three do, so the caller sees "invalid".
  const float twoMissed[5] = {0, 10, 0, 11, 12};
  TEST_ASSERT_EQUAL_FLOAT(10, median5(twoMissed));
  const float threeMissed[5] = {0, 10, 0, 0, 12};
  TEST_ASSERT_EQUAL_FLOAT(0, median5(threeMissed));
  TEST_ASSERT_EQUAL_FLOAT(9, a[0]);  // input untouched
}

// ---- hysteresis ----

void test_high_alarm_hysteresis() {
  HysteresisAlarm hot(35.0f, 34.0f);
  TEST_ASSERT_FALSE(hot.update(30.0f));
  TEST_ASSERT_FALSE(hot.active());
  TEST_ASSERT_TRUE(hot.update(35.5f));
  TEST_ASSERT_TRUE(hot.active());
  TEST_ASSERT_FALSE(hot.update(34.5f));  // between the points: stays on
  TEST_ASSERT_TRUE(hot.active());
  TEST_ASSERT_TRUE(hot.update(33.9f));
  TEST_ASSERT_FALSE(hot.active());
}

void test_low_alarm_hysteresis() {
  HysteresisAlarm tankLow(15.0f, 25.0f);
  TEST_ASSERT_FALSE(tankLow.update(50.0f));
  TEST_ASSERT_TRUE(tankLow.update(10.0f));
  TEST_ASSERT_FALSE(tankLow.update(20.0f));  // refilled a little: still locked
  TEST_ASSERT_TRUE(tankLow.active());
  TEST_ASSERT_TRUE(tankLow.update(30.0f));
  TEST_ASSERT_FALSE(tankLow.active());
}

// ---- alert gate ----

void test_alert_fires_once_then_reminds() {
  AlertGate gate(1000);
  TEST_ASSERT_TRUE(gate.update(true, 0));
  TEST_ASSERT_FALSE(gate.update(true, 500));
  TEST_ASSERT_FALSE(gate.update(true, 999));
  TEST_ASSERT_TRUE(gate.update(true, 1000));
  TEST_ASSERT_FALSE(gate.update(true, 1500));
}

void test_alert_rearms_after_clear() {
  AlertGate gate(1000);
  TEST_ASSERT_TRUE(gate.update(true, 0));
  TEST_ASSERT_FALSE(gate.update(false, 100));
  TEST_ASSERT_TRUE(gate.update(true, 200));
}

void test_alert_survives_millis_wraparound() {
  AlertGate gate(1000);
  TEST_ASSERT_TRUE(gate.update(true, 0xFFFFFF00u));
  TEST_ASSERT_FALSE(gate.update(true, 0x00000100u));  // 512 ms later
  TEST_ASSERT_TRUE(gate.update(true, 0x00000400u));   // 1280 ms later
}

// ---- irrigation ----

static const IrrigationParams P = {
    35,       // lowPct
    60,       // highPct
    4000,     // pulseMs
    45000,    // soakMs
    3,        // maxPulses
    5,        // minRisePct
    1800000,  // faultBackoffMs
    5000,     // manualPulseMs
};

// Runs one full pulse + soak starting at t, returning the time the soak ends.
static uint32_t pulseAndSoak(IrrigationController& c, uint32_t t, int moisture) {
  c.update(t + P.pulseMs, moisture, false, false);  // pulse ends
  return t + P.pulseMs + P.soakMs;
}

void test_idle_when_moist() {
  IrrigationController c(P);
  TEST_ASSERT_EQUAL(IrrigationEvent::None, c.update(0, 50, false, false));
  TEST_ASSERT_FALSE(c.pumpOn());
  TEST_ASSERT_FALSE(c.watering());
}

void test_starts_below_low_threshold() {
  IrrigationController c(P);
  TEST_ASSERT_EQUAL(IrrigationEvent::CycleStarted, c.update(0, 30, false, false));
  TEST_ASSERT_TRUE(c.pumpOn());
}

void test_pulse_then_soak() {
  IrrigationController c(P);
  c.update(0, 30, false, false);
  c.update(3999, 30, false, false);
  TEST_ASSERT_TRUE(c.pumpOn());
  c.update(4000, 30, false, false);
  TEST_ASSERT_FALSE(c.pumpOn());
  TEST_ASSERT_TRUE(c.watering());
  TEST_ASSERT_EQUAL(IrrigationState::Soak, c.state());
}

void test_cycle_done_once_moist() {
  IrrigationController c(P);
  c.update(0, 30, false, false);
  uint32_t t = pulseAndSoak(c, 0, 30);
  TEST_ASSERT_EQUAL(IrrigationEvent::CycleDone, c.update(t, 65, false, false));
  TEST_ASSERT_EQUAL(IrrigationState::Idle, c.state());
  TEST_ASSERT_FALSE(c.pumpOn());
}

void test_pulses_again_while_still_dry() {
  IrrigationController c(P);
  c.update(0, 30, false, false);
  uint32_t t = pulseAndSoak(c, 0, 30);
  TEST_ASSERT_EQUAL(IrrigationEvent::None, c.update(t, 40, false, false));
  TEST_ASSERT_TRUE(c.pumpOn());
  TEST_ASSERT_EQUAL(IrrigationState::Pulse, c.state());
}

void test_fault_when_moisture_does_not_rise() {
  IrrigationController c(P);
  c.update(0, 30, false, false);
  uint32_t t = 0;
  for (int i = 0; i < P.maxPulses - 1; i++) {
    t = pulseAndSoak(c, t, 31);
    c.update(t, 31, false, false);  // next pulse starts
  }
  t = pulseAndSoak(c, t, 31);
  TEST_ASSERT_EQUAL(IrrigationEvent::PumpFault, c.update(t, 32, false, false));
  TEST_ASSERT_TRUE(c.faulted());
  TEST_ASSERT_FALSE(c.pumpOn());

  // No automatic watering during the backoff, then it tries again.
  TEST_ASSERT_EQUAL(IrrigationEvent::None, c.update(t + P.faultBackoffMs - 1, 30, false, false));
  TEST_ASSERT_TRUE(c.faulted());
  c.update(t + P.faultBackoffMs, 30, false, false);
  TEST_ASSERT_EQUAL(IrrigationState::Idle, c.state());
  TEST_ASSERT_EQUAL(IrrigationEvent::CycleStarted, c.update(t + P.faultBackoffMs + 1, 30, false, false));
}

void test_no_fault_when_moisture_rises_slowly() {
  IrrigationController c(P);
  c.update(0, 20, false, false);
  uint32_t t = 0;
  for (int i = 0; i < P.maxPulses - 1; i++) {
    t = pulseAndSoak(c, t, 22);
    c.update(t, 22 + 4 * i, false, false);
  }
  t = pulseAndSoak(c, t, 30);
  TEST_ASSERT_EQUAL(IrrigationEvent::CycleDone, c.update(t, 30, false, false));
  TEST_ASSERT_FALSE(c.faulted());
}

void test_lock_stops_running_pulse() {
  IrrigationController c(P);
  c.update(0, 30, false, false);
  TEST_ASSERT_EQUAL(IrrigationEvent::Interrupted, c.update(100, 30, true, false));
  TEST_ASSERT_FALSE(c.pumpOn());
  TEST_ASSERT_EQUAL(IrrigationState::Idle, c.state());
}

void test_no_start_when_locked() {
  IrrigationController c(P);
  TEST_ASSERT_EQUAL(IrrigationEvent::None, c.update(0, 10, true, false));
  TEST_ASSERT_FALSE(c.pumpOn());
}

void test_invalid_reading_never_starts_and_stops_pulse() {
  IrrigationController c(P);
  TEST_ASSERT_EQUAL(IrrigationEvent::None, c.update(0, -1, false, false));
  c.update(10, 30, false, false);
  TEST_ASSERT_EQUAL(IrrigationEvent::Interrupted, c.update(20, -1, false, false));
  TEST_ASSERT_FALSE(c.pumpOn());
}

void test_manual_refused_when_locked() {
  IrrigationController c(P);
  TEST_ASSERT_EQUAL(IrrigationEvent::ManualRefused, c.update(0, 50, true, true));
  TEST_ASSERT_FALSE(c.pumpOn());
}

void test_manual_runs_fixed_pulse_even_when_moist() {
  IrrigationController c(P);
  TEST_ASSERT_EQUAL(IrrigationEvent::ManualStarted, c.update(0, 80, false, true));
  TEST_ASSERT_TRUE(c.pumpOn());
  c.update(4999, 80, false, false);
  TEST_ASSERT_TRUE(c.pumpOn());
  TEST_ASSERT_EQUAL(IrrigationEvent::CycleDone, c.update(5000, 80, false, false));
  TEST_ASSERT_FALSE(c.pumpOn());
}

void test_manual_stopped_by_lock() {
  IrrigationController c(P);
  c.update(0, 80, false, true);
  TEST_ASSERT_EQUAL(IrrigationEvent::Interrupted, c.update(1000, 80, true, false));
  TEST_ASSERT_FALSE(c.pumpOn());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_raw_to_percent_inverted_sensor);
  RUN_TEST(test_raw_to_percent_clamps_and_guards);
  RUN_TEST(test_tank_percent);
  RUN_TEST(test_tank_percent_invalid);
  RUN_TEST(test_median5);
  RUN_TEST(test_high_alarm_hysteresis);
  RUN_TEST(test_low_alarm_hysteresis);
  RUN_TEST(test_alert_fires_once_then_reminds);
  RUN_TEST(test_alert_rearms_after_clear);
  RUN_TEST(test_alert_survives_millis_wraparound);
  RUN_TEST(test_idle_when_moist);
  RUN_TEST(test_starts_below_low_threshold);
  RUN_TEST(test_pulse_then_soak);
  RUN_TEST(test_cycle_done_once_moist);
  RUN_TEST(test_pulses_again_while_still_dry);
  RUN_TEST(test_fault_when_moisture_does_not_rise);
  RUN_TEST(test_no_fault_when_moisture_rises_slowly);
  RUN_TEST(test_lock_stops_running_pulse);
  RUN_TEST(test_no_start_when_locked);
  RUN_TEST(test_invalid_reading_never_starts_and_stops_pulse);
  RUN_TEST(test_manual_refused_when_locked);
  RUN_TEST(test_manual_runs_fixed_pulse_even_when_moist);
  RUN_TEST(test_manual_stopped_by_lock);
  return UNITY_END();
}
