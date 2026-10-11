#include <Arduino.h>

#include "calibration.h"
#include "plant_config.h"
#include "sensors.h"

// Calibration: note the distance with the tank empty (TANK_DIST_EMPTY_CM) and with water at the
// fill line (TANK_DIST_FULL_CM). Mount the sensor at least 4 cm above the full water line.

void setup() {
  digitalWrite(PIN_RELAY, RELAY_OFF);
  pinMode(PIN_RELAY, OUTPUT);
  Serial.begin(115200);
  sensorsBegin();
  Serial.println("\n[tank] distance and tank level");
}

void loop() {
  float cm = tankDistanceCm();
  if (cm <= 0) Serial.println("[tank] no echo: check trig GPIO18, echo GPIO19 via 1k/2k divider, 5V and GND");
  else Serial.printf("[tank] distance=%.1f cm level=%d%%\n", cm, distanceToTankPct(cm, TANK_DIST_EMPTY_CM, TANK_DIST_FULL_CM));
  delay(200);
}
