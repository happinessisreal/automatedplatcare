# Wiring guide

Build the circuit in **stages**, one component at a time. After each stage there's a test sketch that shows whether that part works before you add the next. This way a mistake is always in the last thing you wired.

The pin numbers come from [`include/plant_config.h`](../include/plant_config.h). If you change a wire, change it there too.

---

## Before you start

### Rules that prevent damage
1. **Unplug the USB cable and the 5 V adapter before changing any wire.**
2. **ESP32 pins are 3.3 V only.** Never connect 5 V to a GPIO or to the `3V3` pin. The only 5 V signal coming back to the ESP32 (the HC-SR04 echo) goes through the 1k/2k divider described below.
3. **All grounds are connected together**: ESP32 GND, both breadboard GND rails, the adapter's −, and every module's GND.
4. **Water stays below and away from the electronics.** Put the reservoir lower than the breadboard, and route tubing so drips can't run along wires.
5. **Never run the pump dry.** Its intake must be under water whenever it switches on.

### Wire colours (makes checking easier)
| Colour | Use |
|---|---|
| Red | 5 V |
| Orange (or yellow) | 3.3 V |
| Black | GND |
| Any other colour | signals |

### Parts for this guide
From the proposal: ESP32 DevKit V1, capacitive soil sensor v1.2, DHT22, LDR module, HC-SR04, 1-channel 5 V relay module, mini pump + tubing, 16×2 LCD (plain 16-pin, wired directly), buzzer, green/blue/red LEDs, breadboard and jumpers, 5 V 2 A adapter.

Extras:
| Part | Qty | For |
|---|---|---|
| 220 Ω resistor | 3 | LEDs (100 Ω if blue/green are too dim) |
| 1 kΩ resistor | 2 | buzzer transistor base, echo divider |
| 2 kΩ resistor (or two 1 kΩ in series) | 1 | echo divider |
| NPN transistor (2N2222 or BC547) | 1 | buzzer |
| 10 kΩ potentiometer | 1 | LCD contrast |
| 220 Ω resistor (extra) | 1 | LCD backlight |
| 1N4007 diode | 1 | across the pump |
| 100 nF ceramic capacitor | 1 | across the pump |
| 470–1000 µF electrolytic capacitor (10 V or more) | 1 | 5 V rail, stops resets when the pump starts |
| DC barrel-jack breakout | 1 | 5 V adapter onto the breadboard |
| 10 kΩ resistor | 1–2 | only for a bare 4-pin DHT22 or a bare LDR |
| Female-to-male jumpers | ~10 | sensors on long leads |

---

## The board

Common **30-pin** DevKit V1, seen from the top with the antenna end up and USB at the bottom. Each pin's label is printed next to it. **If your board's layout differs, go by the printed labels** (`D25` = GPIO25).

```
                          ┌──────────────────┐
                          │     antenna      │
                     EN ──┤                  ├── D23 ──► Relay IN
                     VP ──┤                  ├── D22 ──► LCD RS (pin 4)
                     VN ──┤                  ├── TX0
   Soil sensor ──►  D34 ──┤                  ├── RX0
           LDR ──►  D35 ──┤                  ├── D21 ──► LCD E (pin 6)
  LCD D4 (11) ◄──  D32 ──┤                  ├── D19 ◄── HC-SR04 Echo (through 1k/2k divider)
  LCD D5 (12) ◄──  D33 ──┤      ESP32       ├── D18 ──► HC-SR04 Trig
     Green LED ◄──  D25 ──┤    DevKit V1     ├── D5
      Blue LED ◄──  D26 ──┤                  ├── TX2 ──► LCD D7 (pin 14)
       Red LED ◄──  D27 ──┤                  ├── RX2 ──► LCD D6 (pin 13)
                    D14 ──┤                  ├── D4  ◄─► DHT22 data
                    D12 ──┤                  ├── D2
        Buzzer ◄──  D13 ──┤                  ├── D15
                    GND ──┤                  ├── GND
                    VIN ──┤       USB        ├── 3V3
                          └──────────────────┘
```

| ESP32 pin | Goes to | Notes |
|---|---|---|
| D34 | soil sensor AOUT | analog input |
| D35 | LDR module AO | analog input |
| D4 | DHT22 data | |
| D18 | HC-SR04 Trig | |
| D19 | HC-SR04 Echo | **through 1k/2k divider** |
| D22 | LCD RS (pin 4) | |
| D21 | LCD E (pin 6) | |
| D32 / D33 | LCD D4 / D5 (pins 11 / 12) | |
| RX2 / TX2 (GPIO16 / 17) | LCD D6 / D7 (pins 13 / 14) | |
| D23 | relay IN | |
| D25 / D26 / D27 | green / blue / red LED | each through 220 Ω |
| D13 | buzzer | through NPN transistor |
| 3V3 | 3.3 V rail | |
| VIN | 5 V rail | see power stages below |
| GND (both) | GND rails | |

These pins avoid the ones that can stop the ESP32 from booting (0, 2, 5, 12, 15). The analog sensors use pins that keep working while Wi-Fi is on.

### Breadboard layout tip
The DevKit is wide. On a standard breadboard it leaves only one free hole beside each pin on one side. Either:
- put two breadboards side by side and straddle the gap between them, or
- push the ESP32 into one breadboard and use female-to-male jumpers on the side that has no free holes.

### Rails
Use one rail pair for **5 V + GND** and the other pair for **3.3 V + GND**. Label them with tape. Join the two GND rails with a black wire.

```
  ┌─ 5V rail ─────── red ────────────────────┐
  └─ GND rail ────── black ──────────────────┘   ← join both GND rails
  ┌─ 3.3V rail ───── orange ─────────────────┐
  └─ GND rail ────── black ──────────────────┘
```

Every module's VCC and GND wire goes to one of these rails, never straight to the ESP32. The rails get their power from the ESP32's power pins at the bottom of the board: `VIN` + `GND` on the left, `3V3` + `GND` on the right.

On long breadboards the rails are often **split in the middle** (a gap in the red/blue line). Bridge the gap with a short jumper, or power only the half you use.

- ESP32 **3V3 → 3.3 V rail**
- ESP32 **GND → GND rail**
- **5 V rail, stages 1–6 (USB power):** ESP32 **VIN → 5 V rail**. VIN carries the laptop's USB 5 V. That's enough for the LCD, HC-SR04 and relay coil, **but not the pump**.
- **5 V rail, stage 7 onward:** the 5 V adapter feeds the rail instead. Instructions are in stage 7.

---

## Stage 1: LEDs and buzzer
**Test:** `test_outputs`, already on the board. The LEDs light green → blue → red, then the buzzer beeps, about once a second.

### LEDs
The long leg is **+ (anode)**. The short leg, on the flat side of the rim, is **− (cathode)**.

```
D25 ──[220Ω]──►|── GND      green
D26 ──[220Ω]──►|── GND      blue
D27 ──[220Ω]──►|── GND      red
              anode  cathode
```

### Buzzer
The firmware switches the buzzer on and off, so it must be an **active** buzzer. Check by touching it straight to 5 V and GND for a moment: an active buzzer beeps, a passive one only clicks.

It draws too much current for a GPIO pin, so a transistor switches it:

```
5V rail ─── Buzzer (+)
            Buzzer (−) ─── Collector ┐
D13 ──[1kΩ]─────────────── Base      ├ NPN
GND rail ───────────────── Emitter   ┘
```

Transistor legs, with the flat face towards you and legs down:
- **2N2222 (TO-92):** E – B – C, left to right
- **BC547:** C – B – E, left to right (the opposite order)

**If you have a 3-pin buzzer module** (VCC, GND, I/O), it already has a transistor. Wire VCC → 5 V, GND → GND, I/O → D13 and skip the transistor. If it beeps whenever D13 is *low*, it's a "low-level trigger" module. Tell me and I'll invert it in the firmware.

**Tell me "stage 1 done".** If blue or green is very dim, swap its 220 Ω for 100 Ω.

---

## Stage 2: LCD (16×2, wired directly)
**Test:** `test_lcd`, already flashed. Row 1 shows `LCD OK  16x2` and row 2 counts seconds as soon as the wiring and contrast are right.

The LCD has 16 pins in a row. **Pin 1 is at the end nearest the corner**, and is usually marked "1" or "VSS".

| LCD pin | Name | Connect to |
|---|---|---|
| 1 | VSS | GND rail |
| 2 | VDD | 5 V rail |
| 3 | V0 (contrast) | **middle** leg of the 10 kΩ potentiometer |
| 4 | RS | ESP32 **D22** |
| 5 | RW | **GND rail** |
| 6 | E | ESP32 **D21** |
| 7, 8, 9, 10 | D0–D3 | nothing |
| 11 | D4 | ESP32 **D32** |
| 12 | D5 | ESP32 **D33** |
| 13 | D6 | ESP32 **RX2** (GPIO16) |
| 14 | D7 | ESP32 **TX2** (GPIO17) |
| 15 | A (backlight +) | 220 Ω resistor → 5 V rail |
| 16 | K (backlight −) | GND rail |

**Potentiometer:** the two **outer** legs go to 5 V and GND (either way round), and the **middle** leg goes to LCD pin 3.

```
5V rail ──┐
          [10kΩ pot]──── middle leg ──► LCD pin 3 (V0)
GND rail ─┘
```

- **RW (pin 5) must go to GND.** Then the LCD only ever receives data, so it never puts 5 V onto an ESP32 pin. That's also why this LCD needs no level shifter.
- D0–D3 (pins 7–10) stay unconnected; the LCD runs in 4-bit mode.
- With power on, **turn the potentiometer slowly** until the text is sharp. At one end of its range you'll see nothing; at the other, a row of solid blocks.
- **No text at any pot position, but the backlight is on?** Check RS, E and D4–D7 against the table. The most common mistake is D4–D7 in the wrong order.

**Tell me "stage 2 done"**, and what the screen shows.

---

## Stage 3: DHT22 (temperature and humidity)
**Test:** `test_dht`. It prints temperature and humidity every 2.5 s. Breathe on the sensor and the humidity should jump.

**DHT22 module (3 pins, `+ out −`):**
```
+   ── 3.3V rail
out ── D4
−   ── GND rail
```

**Bare DHT22 (4 pins):** grille facing you, pins down, numbered 1–4 left to right.
```
pin 1 (VCC)  ── 3.3V rail
pin 2 (DATA) ── D4, and a 10kΩ resistor from DATA to 3.3V
pin 3        ── not connected
pin 4 (GND)  ── GND rail
```

Power it from **3.3 V**, not 5 V, so the data line never goes above 3.3 V.

---

## Stage 4: LDR light sensor
**Test:** `test_ldr`. You calibrate it while I watch the readings:
1. Cover it with your finger. This is "dark".
2. Shine your phone torch on it. This is "bright".

**LDR module with 4 pins (`VCC GND DO AO`):**
```
VCC ── 3.3V rail     (not 5V: the AO output would go up to 5V)
GND ── GND rail
AO  ── D35
DO  ── not connected
```

**Only 3 pins (`VCC GND DO`)?** That module only gives on/off. Use a bare LDR with a divider instead:
```
3.3V rail ── LDR ──┬── D35
                   └──[10kΩ]── GND rail
```

---

## Stage 5: Capacitive soil moisture sensor
**Test:** `test_soil`. You calibrate it while I watch the readings:
1. Hold it in dry air. This is 0 %.
2. Stand it in a glass of water **up to the white line, no deeper**. This is 100 %.

```
VCC  ── 3.3V rail
GND  ── GND rail
AOUT ── D34
```

- **Check the small chip on the sensor.** If it reads **TLC555**, 3.3 V is correct. If it reads **NE555**, it needs 5 V. Power it from the 5 V rail and tell me, so I can check its output stays under the ESP32's limit.
- **Seal the top edge** of the board and the cut edges (nail polish or hot glue). Never push it into the soil past the line, because the electronics above it aren't waterproof.
- Use a long female-to-male lead so the sensor can reach the pot.

---

## Stage 6: HC-SR04 tank level sensor
**Test:** `test_tank`. You calibrate it while I watch the readings:
1. With the tank empty, note the distance.
2. With water at the fill line, note it again.

The Echo pin outputs 5 V, so it goes through a **voltage divider** that brings it down to 3.3 V:

```
VCC  ── 5V rail
GND  ── GND rail
Trig ── D18
Echo ──[1kΩ]──┬── D19
              │
            [2kΩ]          (or two 1kΩ in series)
              │
          GND rail
```

**Mounting:**
- Above the tank, pointing **straight down** at the water.
- **At least 4 cm above the highest water level.** It can't measure closer than about 2–3 cm.
- The tank opening should be wide (about 8 cm or more). A narrow bottle makes the sound bounce off the walls.
- Keep it dry: no splashes, and not inside a sealed lid where condensation forms.

---

## Stage 7: Relay, then the pump
Do this in two steps.

### 7a: Relay alone (no pump yet)
**Test:** `test_relay`. You should hear a click on, 2 s later a click off, then a 5 s pause, repeating. The relay's LED follows it.

```
Relay module      ESP32 / rails
VCC (DC+)   ──── 5V rail
GND (DC−)   ──── GND rail
IN          ──── D23
```

- If the module has a **trigger jumper (H / L)**, set it to **L** (low-level trigger). The firmware assumes this.
- If it clicks **on but never off**, the module can't be switched off from a 3.3 V signal. Tell me; there are two simple fixes (set it to high-level trigger and flip `RELAY_ON`, or drive IN through a transistor).

### 7b: Switch to the 5 V adapter, then add the pump
The pump draws more current than the laptop's USB port can supply.

**Power change, with everything unplugged:**
1. **Remove the VIN → 5 V rail wire.**
2. Adapter barrel jack → breakout: **+ → 5 V rail**, **− → GND rail**.
3. Put the **470–1000 µF capacitor** across the 5 V rail and GND, close to the relay. The long leg (+) goes to 5 V; the leg next to the stripe (−) goes to GND. **Polarity matters**: reversed, it can burst.

**Pump wiring:**
```
5V rail ───────── Relay COM
Relay NO ──────── Pump + (red)
Pump − (black) ── GND rail

Across the pump's two terminals:
  1N4007 diode:   striped end → pump +,  other end → pump −
  100nF capacitor (no polarity)
```

- Use **NO** (normally open), never NC. Then the pump stays off whenever the board is unpowered, booting or crashed.
- The diode absorbs the voltage spike when the pump switches off. Without it, the ESP32 can reset or the relay contacts wear out.
- Put the pump **at the bottom of the reservoir, under water**, before running `test_relay` again. Run tubing from the pump outlet to the drip nozzle in the pot.
- **Siphoning:** if the reservoir is higher than the pot outlet, water keeps flowing after the pump stops. Keep the reservoir lower, or make a small hole in the tubing just above the water line.

**How to power things from now on:**

| Situation | USB cable | Adapter | VIN → 5 V rail wire |
|---|---|---|---|
| Uploading or testing with the laptop | plugged in | on | **removed** |
| Running on its own | unplugged | on | **connected** |

Never have the USB cable plugged in **and** the VIN wire connected while the adapter is on. Some boards would then push the adapter's 5 V back into the laptop's USB port.

---

## Stage 8: Full firmware
1. Put your Wi-Fi name and password in `include/secrets.h`. It must be a **2.4 GHz** network with a normal password; a phone hotspot is easiest. Campus Wi-Fi with a login page won't work.
2. Tell me. I'll flash the full firmware (`pio run -t upload`) and watch its log while you check:
   - The LCD shows `Plant Care` and the device ID, then live readings.
   - The green LED is on when everything is OK.
   - The Android app shows the same readings.

Then we run the test list from the proposal: dry soil, empty tank, hair dryer on the DHT22, covered LDR, several at once, and Wi-Fi off.

---

## Quick reference: what goes where

| Component | Pin | Power |
|---|---|---|
| Green / blue / red LED | D25 / D26 / D27 via 220 Ω | — |
| Buzzer | D13 via 1 kΩ → NPN base | 5 V |
| LCD RS / E / D4 / D5 / D6 / D7 | D22 / D21 / D32 / D33 / RX2 / TX2 | 5 V, RW → GND, contrast pot on V0 |
| DHT22 data | D4 | 3.3 V |
| LDR AO | D35 | 3.3 V |
| Soil sensor AOUT | D34 | 3.3 V (5 V if NE555) |
| HC-SR04 Trig / Echo | D18 / D19 (Echo via 1k/2k divider) | 5 V |
| Relay IN | D23 | 5 V |
| Pump | relay COM ← 5 V, NO → pump + | 5 V adapter |

## Troubleshooting
| Symptom | Likely cause |
|---|---|
| ESP32 restarts when the pump starts | Missing big capacitor or diode, or the pump running from USB power |
| Relay clicks on and never off | Module can't be switched by 3.3 V: see stage 7a |
| LCD lit but blank, or a row of solid blocks | Turn the contrast potentiometer slowly through its whole range |
| LCD shows random characters | D4–D7 in the wrong order, RW not on GND, or a loose data wire |
| LCD backlight off | Pins 15/16 (A/K) not powered, or the 220 Ω missing on pin 15 |
| DHT22 "read failed" | Data wire not on D4, no pull-up on a bare sensor, or powered from the wrong rail |
| Soil reads 0 % or INVALID all the time | AOUT not on D34, or sensor not powered |
| Tank "no echo" | Echo divider wired wrong, sensor not powered from 5 V, or nothing within 4 m |
| Upload fails with "Connecting…" forever | Hold the **BOOT** button on the ESP32 until the upload starts |
