#pragma once

// ---- Pin map ----
// Avoids strapping pins (0, 2, 5, 12, 15). Analog inputs are on ADC1, since ADC2 is unusable with Wi-Fi on.
#define PIN_SOIL        34   // capacitive soil sensor AO (3.3V supply)
#define PIN_LDR         35   // LDR module AO (3.3V supply)
#define PIN_DHT         4    // DHT22 data (3.3V supply)
#define PIN_US_TRIG     18   // HC-SR04 trig
#define PIN_US_ECHO     19   // HC-SR04 echo, through 1k/2k divider
#define PIN_RELAY       23   // relay IN
// 16x2 LCD wired directly in 4-bit mode (RW tied to GND, so the LCD never drives these pins)
#define PIN_LCD_RS      22
#define PIN_LCD_E       21
#define PIN_LCD_D4      32
#define PIN_LCD_D5      33
#define PIN_LCD_D6      16   // labelled RX2 on the board
#define PIN_LCD_D7      17   // labelled TX2 on the board
#define PIN_LED_GREEN   25   // normal
#define PIN_LED_BLUE    26   // watering
#define PIN_LED_RED     27   // alert
#define PIN_BUZZER      13   // through NPN transistor

// Most 1-channel relay modules are active-LOW; swap these if yours is active-HIGH.
#define RELAY_ON        LOW
#define RELAY_OFF       HIGH

// ---- Calibration (placeholders: replace with the hardware-test readings) ----
#define SOIL_RAW_DRY        3000  // test_soil, sensor in dry air
#define SOIL_RAW_WET        1300  // test_soil, sensor in water up to the line
#define SOIL_RAW_MIN_VALID  50    // below this the sensor output is shorted or unplugged
#define SOIL_RAW_MAX_VALID  4090  // ADC saturated: sensor miswired
#define LDR_RAW_DARK        4095  // test_ldr, LDR covered
#define LDR_RAW_BRIGHT      0     // test_ldr, phone torch on the LDR
#define TANK_DIST_EMPTY_CM  20.0  // test_tank, tank empty
#define TANK_DIST_FULL_CM   5.0   // test_tank, water at the fill line

// ---- Thresholds (proposal starting values; tune during testing) ----
#define MOISTURE_CRITICAL_PCT        20   // dehydration alert below this
#define MOISTURE_CRITICAL_CLEAR_PCT  25
#define MOISTURE_LOW_PCT             35   // start watering
#define MOISTURE_HIGH_PCT            60   // stop watering
#define TANK_LOCK_PCT                15   // lock pump below this; keep it above the pump intake
#define TANK_UNLOCK_PCT              25
#define TEMP_MIN_C                   15.0
#define TEMP_MAX_C                   35.0
#define TEMP_HYST_C                  1.0

// ---- Watering: pulse and soak ----
#define PUMP_PULSE_MS          4000
#define SOAK_MS                45000
#define MAX_PULSES             6      // pulses per cycle before judging progress
#define MIN_RISE_PCT           5      // less rise than this over a full cycle = pump/sensor fault
#define PUMP_FAULT_BACKOFF_MS  (30UL * 60 * 1000)
#define MANUAL_PULSE_MS        5000

// ---- Alerts ----
#define ALERT_REMINDER_MS   (30UL * 60 * 1000)  // repeat an alert this often while it lasts
#define SENSOR_FAIL_COUNT   5                   // bad reads in a row before a sensor counts as failed
#define BEEP_REPEAT_MS      30000

// ---- Task timing ----
#define IRRIGATION_TICK_MS  100   // how quickly a pump lock stops a running pulse
#define SOIL_READ_MS        1000
#define TANK_PERIOD_MS      500
#define CLIMATE_PERIOD_MS   2500  // DHT22 needs >= 2 s between reads
#define LIGHT_PERIOD_MS     1000
#define DISPLAY_TICK_MS     50
#define LCD_REFRESH_MS      500
#define LCD_PAGE_MS         2500
#define SERIAL_LOG_MS       2000

// ---- MQTT (topics are plantcare/<DEVICE_ID>/<name>; see docs/mqtt-contract.md) ----
#define MQTT_HOST            "broker.emqx.io"
#define MQTT_PORT            8883
#define MQTT_TOPIC_PREFIX    "plantcare/"
#define TELEMETRY_PERIOD_MS  5000
