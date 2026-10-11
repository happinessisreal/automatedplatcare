#include "sensors.h"

#include <Arduino.h>
#include <DHT.h>

#include "calibration.h"
#include "plant_config.h"

static DHT dht(PIN_DHT, DHT22);

void sensorsBegin() {
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_SOIL, ADC_11db);
  analogSetPinAttenuation(PIN_LDR, ADC_11db);
  pinMode(PIN_US_TRIG, OUTPUT);
  digitalWrite(PIN_US_TRIG, LOW);
  pinMode(PIN_US_ECHO, INPUT);
  dht.begin();
}

static int averageAdc(int pin) {
  long sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(pin);
  return sum / 16;
}

int soilRaw() { return averageAdc(PIN_SOIL); }

int soilPercent(int raw) {
  if (raw < SOIL_RAW_MIN_VALID || raw > SOIL_RAW_MAX_VALID) return -1;
  return rawToPercent(raw, SOIL_RAW_DRY, SOIL_RAW_WET);
}

int lightRaw() { return averageAdc(PIN_LDR); }

int lightPercent(int raw) { return rawToPercent(raw, LDR_RAW_DARK, LDR_RAW_BRIGHT); }

static float pingCm() {
  digitalWrite(PIN_US_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_US_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_US_TRIG, LOW);
  unsigned long us = pulseIn(PIN_US_ECHO, HIGH, 30000);  // 30 ms covers ~5 m; 0 = no echo
  return us / 58.0f;
}

float tankDistanceCm() {
  float samples[5];
  for (int i = 0; i < 5; i++) {
    samples[i] = pingCm();
    delay(60);  // let the previous echo die out
  }
  return median5(samples);
}

bool readClimate(float& tempC, float& humidity) {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) return false;
  tempC = t;
  humidity = h;
  return true;
}
