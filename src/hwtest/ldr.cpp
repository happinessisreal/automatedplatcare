#include <Arduino.h>

#include "plant_config.h"
#include "sensors.h"

// Calibration: cover the LDR and note raw (LDR_RAW_DARK), then shine a phone torch on it (LDR_RAW_BRIGHT).

void setup() {
  digitalWrite(PIN_RELAY, RELAY_OFF);
  pinMode(PIN_RELAY, OUTPUT);
  Serial.begin(115200);
  sensorsBegin();
  Serial.println("\n[ldr] raw ADC and light %");
}

void loop() {
  int raw = lightRaw();
  Serial.printf("[ldr] raw=%d light=%d%%\n", raw, lightPercent(raw));
  delay(500);
}
