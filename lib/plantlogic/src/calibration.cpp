#include "calibration.h"

static int clampPct(float pct) {
  if (pct < 0) return 0;
  if (pct > 100) return 100;
  return (int)(pct + 0.5f);
}

int rawToPercent(int raw, int rawAt0, int rawAt100) {
  if (rawAt0 == rawAt100) return 0;
  return clampPct(100.0f * (raw - rawAt0) / (rawAt100 - rawAt0));
}

int distanceToTankPct(float distCm, float emptyCm, float fullCm) {
  if (distCm <= 0 || emptyCm <= fullCm) return -1;
  return clampPct(100.0f * (emptyCm - distCm) / (emptyCm - fullCm));
}

float median5(const float samples[5]) {
  float v[5];
  for (int i = 0; i < 5; i++) v[i] = samples[i];
  for (int i = 1; i < 5; i++) {
    float x = v[i];
    int j = i - 1;
    while (j >= 0 && v[j] > x) {
      v[j + 1] = v[j];
      j--;
    }
    v[j + 1] = x;
  }
  return v[2];
}
