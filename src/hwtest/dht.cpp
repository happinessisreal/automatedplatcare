#include <Arduino.h>

#include "plant_config.h"
#include "sensors.h"

// Prints temperature and humidity. Breathe on the sensor: humidity should jump within a few seconds.

void setup() {
  digitalWrite(PIN_RELAY, RELAY_OFF);
  pinMode(PIN_RELAY, OUTPUT);
  Serial.begin(115200);
  sensorsBegin();
  Serial.println("\n[dht] reading DHT22 every 2.5 s");
}

void loop() {
  float t, h;
  if (readClimate(t, h)) Serial.printf("[dht] temp=%.1f C hum=%.1f %%\n", t, h);
  else Serial.println("[dht] read failed: check data wire on GPIO4, 3.3V and GND");
  delay(CLIMATE_PERIOD_MS);
}
