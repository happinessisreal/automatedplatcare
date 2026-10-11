#include <LiquidCrystal.h>

#include "plant_config.h"
#include "secrets.h"
#include "state.h"
#include "tasks.h"

static LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_E, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
static char shown[2][17];

// Writes a row only when its text changes, padded so leftovers from longer text are cleared.
static void showRow(int row, const char* text) {
  char buf[17];
  snprintf(buf, sizeof buf, "%-16.16s", text);
  if (strcmp(buf, shown[row]) == 0) return;
  lcd.setCursor(0, row);
  lcd.print(buf);
  strcpy(shown[row], buf);
}

static void pct3(char (&out)[4], int v) {
  if (v < 0 || v > 100) strcpy(out, " --");
  else snprintf(out, sizeof out, "%3d", v);
}

static void drawLcd(const State& s, bool climatePage) {
  char a[4], b[4], c[4], row[24];
  pct3(a, s.moisture);
  pct3(b, s.tank);
  snprintf(row, sizeof row, "Soil%s%%  Tk%s%%", a, b);  // "Soil 42%  Tk 85%"
  showRow(0, row);

  if (climatePage) {
    char temp[8];
    if (isnan(s.tempC)) strcpy(temp, "--.-");
    else snprintf(temp, sizeof temp, "%4.1f", s.tempC);
    pct3(a, isnan(s.humidity) ? -1 : (int)lroundf(s.humidity));
    pct3(c, s.light);
    snprintf(row, sizeof row, "%sC %s%% L%s%%", temp, a, c);  // "28.4C  61% L 70%"
  } else {
    snprintf(row, sizeof row, "%-12.12s%4s", statusText(s), netOnline ? " NET" : " ---");
  }
  showRow(1, row);
}

// The hardware buzzer is active (beeps on DC); Wokwi's buzzer only sounds with a tone.
static void setBuzzer(bool on) {
  static bool current = false;
  if (on == current) return;
  current = on;
#ifdef SIMULATION
  on ? tone(PIN_BUZZER, 2000) : noTone(PIN_BUZZER);
#else
  digitalWrite(PIN_BUZZER, on);
#endif
}

static void logState(const State& s) {
  Serial.printf("[state] soil=%d tank=%d light=%d temp=%.1f hum=%.0f pump=%d watering=%d locked=%d alarms=0x%02X net=%d status=%s\n",
                s.moisture, s.tank, s.light, s.tempC, s.humidity, s.pumpOn, s.watering, pumpLocked.load(), s.alarms,
                netOnline.load(), statusText(s));
}

void displayTask(void*) {
  const int leds[] = {PIN_LED_GREEN, PIN_LED_BLUE, PIN_LED_RED};
  for (int pin : leds) pinMode(pin, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  lcd.begin(16, 2);

  // Boot screen: all LEDs on as a lamp test, and the device ID the app has to match.
  for (int pin : leds) digitalWrite(pin, HIGH);
  showRow(0, "Plant Care");
  showRow(1, DEVICE_ID);
  vTaskDelay(pdMS_TO_TICKS(4000));

  uint16_t prevBuzz = 0;
  uint32_t beepStart = 0, lastLcd = 0, lastLog = 0;

  for (;;) {
    uint32_t now = millis();
    State s = stateGet();
    bool alarm = s.alarms != 0;

    digitalWrite(PIN_LED_GREEN, !alarm && !s.watering);
    digitalWrite(PIN_LED_BLUE, s.watering);
    digitalWrite(PIN_LED_RED, alarm);

    // Buzzer: 3 short beeps when a new alarm appears, repeated every BEEP_REPEAT_MS while any lasts.
    // A dry plant only alerts the phone and LCD, as in the proposal.
    uint16_t buzz = s.alarms & ~ALARM_DEHYDRATION;
    if ((buzz & ~prevBuzz) || (buzz && now - beepStart >= BEEP_REPEAT_MS)) beepStart = now;
    prevBuzz = buzz;
    uint32_t t = now - beepStart;
    setBuzzer(buzz && t < 900 && (t / 150) % 2 == 0);

    if (now - lastLcd >= LCD_REFRESH_MS) {
      lastLcd = now;
      drawLcd(s, (now / LCD_PAGE_MS) % 2 == 0);
    }
    if (now - lastLog >= SERIAL_LOG_MS) {
      lastLog = now;
      logState(s);
    }

    vTaskDelay(pdMS_TO_TICKS(DISPLAY_TICK_MS));
  }
}
