# SERVO CONSOLE

**Handheld Arduino Nano game console & servo controller**  
MECHATRONICS Club · Jadavpur University

---

## What is this?

The SERVO Console is a pocket-sized handheld device built around an Arduino Nano, combining a game console and a multi-servo controller. It was designed and built by Md Touhid Alam (3rd year Electrical Engineering, Jadavpur University) as a personal embedded systems project, with resources and support from the Mechatronics Club. The project demonstrates practical firmware design under tight hardware constraints — 30 KB Flash and 2 KB SRAM on the ATmega328P.
---

## Hardware

| Component | Detail |
|---|---|
| Microcontroller | Arduino Nano (ATmega328P) |
| Display | 0.96" SSD1306 OLED, 128×64, I2C |
| Buttons | UP / DOWN / LEFT / RIGHT / SLIDE — D2–D6 |
| Potentiometer | A0 |
| Servos | 4× SG90-class — D9, D10, D11, D12 |
| Battery | 2× 18650 Li-ion (2S pack) |
| Battery monitor | Voltage divider (100kΩ:100kΩ) on A1, 100nF filter cap |

---

## Features

### Games
- **Flappy Bird** — variable speed via POT, 3-2-1 countdown, high score
- **Snake** — 32×16 grid, POT-controlled speed, high score

### Servo Tools
- **Servo Control (POT mode)** — direct potentiometer angle control with optional auto-sweep
- **Servo Control (BTN mode)** — button stepping (+1°/+10° or −1°/−10° via SLIDE)
- **Servo Sequence Recorder** — stamp up to 8 keyframes across all 4 servos, then play back in a loop

### Utilities
- **Battery Monitor** — oversampled ADC with hysteresis, shown as % in the menu title bar
- **Boot Splash** — MC8RNX Club logo bitmap (PROGMEM)

---

## Controls

| Button | Function |
|---|---|
| UP / DOWN | Navigate menus, adjust values |
| LEFT | Back / cancel |
| RIGHT | Select / confirm / apply |
| SLIDE switch | Mode toggle (context-dependent) |
| POT | Angle control / game speed |

---

## Firmware

**File:** `firmware/jumtc_console_v13.ino`

**Libraries required (Arduino Library Manager):**
- Adafruit SSD1306
- Adafruit GFX

**Memory usage (approximate):**

| Resource | Budget | Usage |
|---|---|---|
| Flash | 30,720 B | ~28 KB |
| SRAM | 2,048 B | ~1.6 KB |

Key techniques used:
- `F()` macro on all string literals to keep them in Flash
- PROGMEM for the 1,024-byte logo bitmap
- Non-blocking architecture — all timing via `millis()`, zero `delay()` calls in gameplay
- Oversampled + hysteresis-filtered battery ADC

---

## Wiring

```
Arduino Nano
  D2  →  SLIDE switch (to GND)
  D3  →  RIGHT button (to GND)
  D4  →  UP button    (to GND)
  D5  →  DOWN button  (to GND)
  D6  →  LEFT button  (to GND)
  D9  →  Servo 4 signal
  D10 →  Servo 3 signal
  D11 →  Servo 2 signal
  D12 →  Servo 1 signal
  A0  →  Potentiometer wiper
  A1  →  Battery voltage divider midpoint
  A4  →  OLED SDA
  A5  →  OLED SCL
```

Battery monitor voltage divider: 100kΩ from VBAT to A1, 100kΩ from A1 to GND, 100nF cap across lower resistor.  
Formula: `V_bat = (ADC / 1023.0) × 5.0 × 2.0`  
Range: 6.8V (0%) → 8.0V (100%)

---

## Project history

| Version | Description |
|---|---|
| Early (GAMIVO) | Original prototype, different button layout |
| v6 | Flappy Bird + Calculator + Servo Control |
| v3 (snake/pong) | Flappy + Snake + Pong + Servo Control |
| **v13 (current)** | Flappy + Snake + Servo Recorder + Servo Ctrl + Battery monitor + MC8RNX logo splash |

---

## License

MIT — see [LICENSE](LICENSE)

---

*Built by Md Touhid Alam — 3rd year Electrical Engineering, Jadavpur University*  
*Electronics Team Lead, Mechatronics Club · Resources & support provided by the club*
