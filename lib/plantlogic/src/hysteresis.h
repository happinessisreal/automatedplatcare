#pragma once

// Alarm with separate switch-on and switch-off points, so a value hovering near a threshold doesn't
// make it flicker. onAt > offAt gives a "too high" alarm; onAt < offAt gives a "too low" alarm.
class HysteresisAlarm {
 public:
  HysteresisAlarm(float onAt, float offAt) : onAt_(onAt), offAt_(offAt) {}

  // Feeds a new value; returns true if the alarm switched on or off.
  bool update(float value) {
    bool high = onAt_ > offAt_;
    bool next = active_;
    if (!active_ && (high ? value > onAt_ : value < onAt_)) next = true;
    if (active_ && (high ? value < offAt_ : value > offAt_)) next = false;
    bool changed = next != active_;
    active_ = next;
    return changed;
  }

  bool active() const { return active_; }

 private:
  float onAt_, offAt_;
  bool active_ = false;
};
