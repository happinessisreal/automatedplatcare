#pragma once
#include <stdint.h>

struct IrrigationParams {
  int lowPct;               // start a watering cycle below this moisture
  int highPct;              // end the cycle once moisture reaches this
  uint32_t pulseMs;         // pump on-time per pulse
  uint32_t soakMs;          // wait after each pulse for the water to reach the sensor
  int maxPulses;            // pulses per cycle before judging progress
  int minRisePct;           // a full cycle must raise moisture at least this much, or it's a fault
  uint32_t faultBackoffMs;  // pause automatic watering this long after a fault
  uint32_t manualPulseMs;   // pump on-time for a manual request
};

enum class IrrigationState { Idle, Pulse, Soak, Manual, Fault };

enum class IrrigationEvent { None, CycleStarted, CycleDone, Interrupted, PumpFault, ManualStarted, ManualRefused };

// Pulse-and-soak watering state machine. Pure logic: the caller reads the sensor, drives the relay
// from pumpOn() and passes the time in, so it behaves the same on the ESP32 and in host tests.
class IrrigationController {
 public:
  explicit IrrigationController(const IrrigationParams& p) : p_(p) {}

  // moisturePct < 0 means the reading is invalid. manualRequest is a one-shot: pass true once.
  IrrigationEvent update(uint32_t nowMs, int moisturePct, bool pumpLocked, bool manualRequest);

  bool pumpOn() const { return state_ == IrrigationState::Pulse || state_ == IrrigationState::Manual; }
  bool watering() const { return pumpOn() || state_ == IrrigationState::Soak; }
  bool faulted() const { return state_ == IrrigationState::Fault; }
  IrrigationState state() const { return state_; }

 private:
  void enter(IrrigationState s, uint32_t nowMs) {
    state_ = s;
    sinceMs_ = nowMs;
  }

  IrrigationParams p_;
  IrrigationState state_ = IrrigationState::Idle;
  uint32_t sinceMs_ = 0;
  int pulses_ = 0;
  int startMoisture_ = 0;
};
