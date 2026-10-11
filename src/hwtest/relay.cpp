#include <Arduino.h>

#include "plant_config.h"

// First checks what's on the relay's IN wire, then switches the relay on for 2 s every 7 s.
// Listen for the click; with the pump attached, keep its intake under water. If the relay never
// switches off, flip RELAY_ON/RELAY_OFF in plant_config.h, or the module can't be driven from
// 3.3V (see docs/wiring.md).

// Reads D23 with the internal pull-down, then the pull-up. A module's input circuit is much
// stronger than these ~45k pulls, so it holds the pin at its own level; a loose wire just follows them.
// Both pulls are too weak to switch the relay, so it stays off during the probe.
static void probeRelayInput() {
  pinMode(PIN_RELAY, INPUT_PULLDOWN);
  delay(5);
  bool withPulldown = digitalRead(PIN_RELAY);
  pinMode(PIN_RELAY, INPUT_PULLUP);
  delay(5);
  bool withPullup = digitalRead(PIN_RELAY);

  if (withPulldown && withPullup) {
    Serial.println("[relay] DETECTED: IN is held high by the module -> active-LOW relay module, powered.");
    if (RELAY_ON != LOW) Serial.println("[relay] config says active-HIGH: set RELAY_ON LOW / RELAY_OFF HIGH");
  } else if (!withPulldown && !withPullup) {
    Serial.println("[relay] DETECTED: IN is held low by the module -> active-HIGH relay module.");
    if (RELAY_ON != HIGH) Serial.println("[relay] config says active-LOW: set RELAY_ON HIGH / RELAY_OFF LOW (or move the jumper to L)");
  } else {
    Serial.println("[relay] NOT DETECTED: IN (D23) is floating. Check the IN wire, and that VCC has 5V and GND is connected.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[relay] probing D23...");
  probeRelayInput();
  digitalWrite(PIN_RELAY, RELAY_OFF);
  pinMode(PIN_RELAY, OUTPUT);
  Serial.println("[relay] 2 s on / 5 s off");
}

void loop() {
  Serial.println("[relay] ON");
  digitalWrite(PIN_RELAY, RELAY_ON);
  delay(2000);
  Serial.println("[relay] off");
  digitalWrite(PIN_RELAY, RELAY_OFF);
  delay(5000);
}
