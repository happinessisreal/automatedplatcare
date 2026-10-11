#include <Arduino.h>

#include "plant_config.h"
#include "sensors.h"
#include "state.h"
#include "tasks.h"

void setup() {
  // Pump off before anything else, so a reset can never leave it running.
  digitalWrite(PIN_RELAY, RELAY_OFF);
  pinMode(PIN_RELAY, OUTPUT);

  Serial.begin(115200);
  Serial.println("\n[boot] Plant Care starting");

  stateInit();
  sensorsBegin();

  // Sensor, pump and display tasks on core 1; Wi-Fi/MQTT on core 0 next to the Wi-Fi stack.
  // The tank task runs highest so its echo timing isn't disturbed and a low tank locks the pump fast.
  xTaskCreatePinnedToCore(tankTask, "tank", 4096, nullptr, 4, nullptr, 1);
  xTaskCreatePinnedToCore(irrigationTask, "irrigation", 4096, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(climateTask, "climate", 4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(lightTask, "light", 3072, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(displayTask, "display", 4096, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(netTask, "net", 10240, nullptr, 1, nullptr, 0);
}

void loop() { vTaskDelete(nullptr); }
