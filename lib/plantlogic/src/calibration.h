#pragma once

// Maps a raw sensor reading onto 0–100 %. rawAt0 maps to 0 % and rawAt100 to 100 %, so an inverted
// sensor (higher raw = lower %) just passes its endpoints the other way round. Clamped to 0–100.
int rawToPercent(int raw, int rawAt0, int rawAt100);

// Tank fill level from the sensor-to-water distance. Returns -1 when the distance is invalid
// (no echo) so callers can fail safe.
int distanceToTankPct(float distCm, float emptyCm, float fullCm);

// Median of five samples; the input is left untouched.
float median5(const float samples[5]);
