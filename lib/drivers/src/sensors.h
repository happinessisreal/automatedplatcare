#pragma once

// Sensor access shared by the firmware and the hardware-test sketches. A bad reading comes back
// as -1 / 0 / false rather than a made-up value, so callers can fail safe.

void sensorsBegin();

int soilRaw();             // average of 16 ADC samples
int soilPercent(int raw);  // -1 if raw is outside the plausible range
int lightRaw();
int lightPercent(int raw);
float tankDistanceCm();    // median of 5 pings; 0 if there's no echo
bool readClimate(float& tempC, float& humidity);
