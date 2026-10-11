#pragma once

// Each task owns one job and talks to the others only through state.h.
void irrigationTask(void*);  // soil moisture -> pump (pulse and soak)
void tankTask(void*);        // reservoir level -> pump lock
void climateTask(void*);     // temperature/humidity alarms
void lightTask(void*);       // light level
void displayTask(void*);     // LCD, LEDs, buzzer, serial log
void netTask(void*);         // Wi-Fi + MQTT: telemetry, alerts, commands
