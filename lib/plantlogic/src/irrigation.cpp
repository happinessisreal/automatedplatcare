#include "irrigation.h"

IrrigationEvent IrrigationController::update(uint32_t nowMs, int moisturePct, bool pumpLocked, bool manualRequest) {
  uint32_t elapsed = nowMs - sinceMs_;
  bool valid = moisturePct >= 0;

  switch (state_) {
    case IrrigationState::Idle:
    case IrrigationState::Fault:
      // Manual watering is the owner's explicit call, so it's allowed during a fault backoff too.
      if (manualRequest) {
        if (pumpLocked) return IrrigationEvent::ManualRefused;
        enter(IrrigationState::Manual, nowMs);
        return IrrigationEvent::ManualStarted;
      }
      if (state_ == IrrigationState::Fault) {
        if (elapsed >= p_.faultBackoffMs) enter(IrrigationState::Idle, nowMs);
        return IrrigationEvent::None;
      }
      if (valid && moisturePct < p_.lowPct && !pumpLocked) {
        pulses_ = 1;
        startMoisture_ = moisturePct;
        enter(IrrigationState::Pulse, nowMs);
        return IrrigationEvent::CycleStarted;
      }
      return IrrigationEvent::None;

    case IrrigationState::Pulse:
      if (pumpLocked || !valid) {
        enter(IrrigationState::Idle, nowMs);
        return IrrigationEvent::Interrupted;
      }
      if (elapsed >= p_.pulseMs) enter(IrrigationState::Soak, nowMs);
      return IrrigationEvent::None;

    case IrrigationState::Soak:
      if (!valid) {
        enter(IrrigationState::Idle, nowMs);
        return IrrigationEvent::Interrupted;
      }
      if (elapsed < p_.soakMs) return IrrigationEvent::None;
      if (moisturePct >= p_.highPct) {
        enter(IrrigationState::Idle, nowMs);
        return IrrigationEvent::CycleDone;
      }
      if (pulses_ >= p_.maxPulses) {
        // Still dry after a full cycle: if moisture barely moved, the pump, tubing or sensor is broken.
        if (moisturePct - startMoisture_ < p_.minRisePct) {
          enter(IrrigationState::Fault, nowMs);
          return IrrigationEvent::PumpFault;
        }
        enter(IrrigationState::Idle, nowMs);
        return IrrigationEvent::CycleDone;
      }
      if (pumpLocked) {
        enter(IrrigationState::Idle, nowMs);
        return IrrigationEvent::Interrupted;
      }
      pulses_++;
      enter(IrrigationState::Pulse, nowMs);
      return IrrigationEvent::None;

    case IrrigationState::Manual:
      if (pumpLocked) {
        enter(IrrigationState::Idle, nowMs);
        return IrrigationEvent::Interrupted;
      }
      if (elapsed >= p_.manualPulseMs) {
        enter(IrrigationState::Idle, nowMs);
        return IrrigationEvent::CycleDone;
      }
      return IrrigationEvent::None;
  }
  return IrrigationEvent::None;
}
