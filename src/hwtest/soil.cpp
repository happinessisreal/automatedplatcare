#include <Arduino.h>

#include "plant_config.h"
#include "sensors.h"

// Calibration: hold the sensor in dry air and note raw (SOIL_RAW_DRY), then stand it in water up to
// the line (SOIL_RAW_WET). Raw goes DOWN as the soil gets wetter.

void setup() {
  digitalWrite(PIN_RELAY, RELAY_OFF);
  pinMode(PIN_RELAY, OUTPUT);
  Serial.begin(115200);
  sensorsBegin();
  Serial.println("\n[soil] raw ADC and moisture %");
}

void loop() {
  int raw = soilRaw();
  int pct = soilPercent(raw);
  if (pct < 0) Serial.printf("[soil] raw=%d INVALID: check AO on GPIO34, 3.3V and GND\n", raw);
  else Serial.printf("[soil] raw=%d moisture=%d%%\n", raw, pct);
  delay(500);
}
