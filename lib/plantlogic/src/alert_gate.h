#pragma once
#include <stdint.h>

// Turns a condition that stays true into discrete alerts: one when it starts, then a reminder every
// reminderMs while it lasts. Unsigned arithmetic keeps millis() wrap-around harmless.
class AlertGate {
 public:
  explicit AlertGate(uint32_t reminderMs) : reminderMs_(reminderMs) {}

  // Returns true when an alert should be sent now.
  bool update(bool condition, uint32_t nowMs) {
    if (!condition) {
      active_ = false;
      return false;
    }
    if (!active_ || nowMs - lastMs_ >= reminderMs_) {
      active_ = true;
      lastMs_ = nowMs;
      return true;
    }
    return false;
  }

 private:
  uint32_t reminderMs_;
  uint32_t lastMs_ = 0;
  bool active_ = false;
};
