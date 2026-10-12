# MQTT contract

The ESP32 firmware, the Android app and the fake device in `tools/mqtt_tools` all follow this contract.

- **Broker:** `broker.hivemq.com`, port **8883** (TLS). The certificate chains to Amazon Root CA 1.
- **Topics:** `plantcare/<DEVICE_ID>/<name>`. `DEVICE_ID` is 16 random hex characters. It lives in `include/secrets.h` (firmware) and `app/env.json` (app), and the LCD shows it at boot.
- **Payloads:** UTF-8 JSON, except `online`.

The broker is public. The random device ID is the only thing that keeps other people off these topics, so don't publish it.

| Topic | Direction | Retained | QoS | When |
|---|---|---|---|---|
| `telemetry` | ESP32 → app | yes | 0 | every 5 s, immediately when pump/watering/alarms change, and on (re)connect |
| `alert` | ESP32 → app | no | 0 | when a condition starts, then every 30 min while it lasts |
| `cmd` | app → ESP32 | no | 1 | when the user taps **Water now** |
| `online` | ESP32 → app | yes | 1 | `"1"` after connecting. The broker publishes `"0"` (last will) if the ESP32 drops off. |

## `telemetry`
```json
{
  "moisture": 42, "tank": 85, "light": 70, "temp": 28.4, "hum": 61,
  "pump": false, "watering": true, "locked": false,
  "status": "WATERING",
  "alarms": ["dehydration"],
  "uptime": 3605, "rssi": -58
}
```
| Field | Type | Meaning |
|---|---|---|
| `moisture`, `tank`, `light` | int 0–100 or `null` | percent; `null` = sensor failed |
| `temp` | number or `null` | °C, one decimal |
| `hum` | int or `null` | % relative humidity |
| `pump` | bool | relay on right now |
| `watering` | bool | a watering cycle is running: pump on, or soaking between pulses |
| `locked` | bool | pump locked because the tank is low or its sensor failed |
| `status` | string | highest-priority condition (below); also shown on the LCD |
| `alarms` | string[] | every active condition (below) |
| `uptime` | int | seconds since boot |
| `rssi` | int | Wi-Fi signal, dBm |

`status` values, highest priority first: `REFILL TANK`, `PUMP FAULT`, `SENSOR FAIL`, `TOO HOT`, `TOO COLD`, `WATERING`, `VERY DRY`, `OK`.

`alarms` values: `dehydration`, `low_tank`, `temp_high`, `temp_low`, `pump_fault`, `soil_sensor`, `tank_sensor`, `dht_sensor`.

## `alert`
```json
{ "type": "low_tank", "msg": "Water tank is low (12%). Refill it; watering is paused.", "uptime": 3605 }
```
`type` values: `dehydration`, `low_tank`, `temp_high`, `temp_low`, `pump_fault`, `manual_refused`, `sensor_fault`. Show `msg` to the user as-is.

Alerts aren't retained. A phone that was offline catches up from `telemetry.alarms`, which is retained.

## `cmd`
```json
{ "action": "water" }
```
Runs one fixed watering pulse (5 s), whatever the moisture level. If the tank is low the ESP32 refuses and sends a `manual_refused` alert. Anything else is ignored.
