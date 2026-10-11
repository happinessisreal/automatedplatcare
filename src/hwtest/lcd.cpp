#include <Arduino.h>
#include <LiquidCrystal.h>

#include "plant_config.h"

// Writes a test pattern to the directly wired 16x2 LCD. The LCD can't answer back, so the check is
// by eye: row 1 shows "LCD OK", row 2 counts seconds.
// Backlight on but no text, or a row of solid blocks: turn the contrast potentiometer slowly.

static LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_E, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

void setup() {
  digitalWrite(PIN_RELAY, RELAY_OFF);
  pinMode(PIN_RELAY, OUTPUT);
  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.print("LCD OK  16x2");
  Serial.println("\n[lcd] PASS if row 1 shows \"LCD OK  16x2\" and row 2 counts up every second");
  Serial.println("[lcd] blank or solid blocks: turn the contrast pot; garbage: check D4-D7 order and RW on GND");
}

void loop() {
  lcd.setCursor(0, 1);
  lcd.printf("uptime %6lus", millis() / 1000);
  Serial.printf("[lcd] wrote uptime %lus\n", millis() / 1000);
  delay(1000);
}
