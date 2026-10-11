#include <Arduino.h>

#include "plant_config.h"

// Lights each LED and sounds the buzzer in turn, so every output can be checked by eye and ear.
// The relay stays off; it has its own test (test_relay).

struct Output {
  int pin;
  const char* name;
  int onMs;
};

static const Output OUTPUTS[] = {
    {PIN_LED_GREEN, "green LED", 1000},
    {PIN_LED_BLUE, "blue LED", 1000},
    {PIN_LED_RED, "red LED", 1000},
    {PIN_BUZZER, "buzzer", 300},
};

void setup() {
  digitalWrite(PIN_RELAY, RELAY_OFF);
  pinMode(PIN_RELAY, OUTPUT);
  Serial.begin(115200);
  for (const Output& o : OUTPUTS) {
    pinMode(o.pin, OUTPUT);
    digitalWrite(o.pin, LOW);
  }
  Serial.println("\n[outputs] cycling green, blue, red, buzzer");
}

void loop() {
  for (const Output& o : OUTPUTS) {
    Serial.printf("[outputs] %s on (GPIO%d)\n", o.name, o.pin);
    digitalWrite(o.pin, HIGH);
    delay(o.onMs);
    digitalWrite(o.pin, LOW);
    delay(300);
  }
}
