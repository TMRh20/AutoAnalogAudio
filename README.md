# AutoAnalogAudio

**Automated analog audio input & output (streaming) for Arduino, by TMRh20**

AutoAnalogAudio provides a simple, unified API for streaming audio through a microcontroller's
DAC (or PWM / I2S), ADC (or PDM / I2S / SAADC), timers and DMA. It is designed with radio and
wireless communication in mind, making it easy to build a wide variety of audio applications:
wireless speakers and microphones, SD card playback, audio relays, signal generators and more.

- Plays and records analog (WAV/PCM) audio data
- Timers are adjusted automatically based on the rate of data delivery, for smooth playback
- DMA transfers are used where available to maximize efficiency

**Documentation:** http://TMRh20.github.io/AutoAnalogAudio/
**See also:** [RF24Audio](http://nRF24.github.io/RF24Audio/)

---

## Supported Devices

| Platform | Example Boards | Output | Input | Status |
|---|---|---|---|---|
| **SAM (Arduino Due)** | Arduino Due | Onboard 12-bit DAC (DAC0/DAC1) | Onboard ADC (multi-channel) | ✅ Supported |
| **AVR** | Uno, Nano, Mega, etc. | Pseudo-DAC via PWM (pins 9 & 10) | Onboard ADC | ✅ Supported |
| **Renesas RA4M1** | Arduino Uno R4 Minima, Uno R4 WiFi | Onboard DAC | Onboard ADC | ✅ Supported |
| **nRF52840 / nRF52** | Adafruit Feather nRF52840, XIAO nRF52840, Nano 33 BLE (Mbed & non-Mbed cores) | PWM, I2S | PDM microphone, I2S, SAADC | ✅ Supported |
| **nRF54L15** | Seeed XIAO nRF54L15 | PWM, I2S | PDM microphone, I2S, SAADC | ✅ Supported |
| **ESP32** | ESP32 dev boards | (I2S / internal DAC, partial) | (partial) | ⚠️ **Not really supported yet** – see below |

### ⚠️ A note on ESP32

ESP32 support is **incomplete and experimental**. Code exists in the library (`src/esp32`) along with
a single test example, but it was written against the older ESP-IDF legacy I2S driver and has not
been kept up to date or fully tested. It should **not** be considered a working, supported
platform at this time.

Contributions and pull requests to improve ESP32 support are very welcome!

---

## Installation

Install via the Arduino Library Manager by searching for **AutoAnalogAudio**, or download/clone this
repository into your Arduino `libraries` folder.

## Basic Usage

```cpp
#include <AutoAnalogAudio.h>

AutoAnalog aaAudio;

#define BUFFER_SIZE 32   // Samples per transfer. Must not exceed MAX_BUFFER_SIZE (see below)

void setup() {

  // Select pins and buffer size BEFORE calling begin() (see "Selecting Pins" below)
#if defined(ARDUINO_ARCH_NRF52) || defined(ARDUINO_ARCH_NRF52840) || defined(ARDUINO_NRF54L15) || defined(ARDUINO_ARCH_RENESAS)
  aaAudio.maxBufferSize = BUFFER_SIZE;  // Internal buffers are allocated dynamically at this size
#endif

  // begin(enADC, enDAC) – the meaning of each value depends on the platform
  aaAudio.begin(0, 1);         // Enable output only
  aaAudio.setSampleRate(16000);
}

void loop() {
  for (int i = 0; i < BUFFER_SIZE; i++) {
    aaAudio.dacBuffer[i] = /* your 8-bit sample */ 0;
  }
  aaAudio.feedDAC(0, BUFFER_SIZE);
}
```

On nRF52/nRF54 devices the `begin()` parameters select the peripheral used:

- `enADC`: `0` = Disabled, `1` = PDM, `2` = I2S, `3` = SAADC
- `enDAC`: `0` = Disabled, `1` = PWM, `2` = I2S

On the Arduino Uno R4 (Renesas), use `3` to select the onboard ADC/DAC, e.g. `aaAudio.begin(3, 3);`

### Buffer Size

`MAX_BUFFER_SIZE` sets the largest number of samples that can be passed to `feedDAC()` or `getADC()`
in one call. It is defined per platform in [`src/AutoAnalog_config.h`](src/AutoAnalog_config.h):

| Platform | Default `MAX_BUFFER_SIZE` | Runtime override |
|---|---|---|
| AVR | 32 | No – edit `AutoAnalog_config.h` |
| SAM (Due) | 256 | No – edit `AutoAnalog_config.h` |
| Renesas (Uno R4) | 512 | `aaAudio.maxBufferSize = n;` before `begin()` |
| nRF52 / nRF54L15 | 6400 | `aaAudio.maxBufferSize = n;` before `begin()` |
| ESP32 | 6400 | No – edit `AutoAnalog_config.h` |

On nRF52, nRF54L15 and Renesas boards the internal DMA buffers are allocated dynamically.
Set `maxBufferSize` to the buffer size your sketch actually uses to save RAM, and keep it no
larger than `MAX_BUFFER_SIZE`. If you leave it unset, it defaults to `MAX_BUFFER_SIZE`.

## Selecting Pins

Configure pins **before** calling `begin()`.

### Arduino Due (SAM)
- **Output:** the onboard DAC pins **DAC0** and **DAC1**. Select them with the first argument of
  `feedDAC()`: `0` = DAC0, `1` = DAC1, `2` = alternating DAC0/DAC1 (stereo).
- **Input:** A0 is enabled by default. Use `aaAudio.enableAdcChannel(n)` / `disableAdcChannel(n)`
  to read other pins, where the channel number matches the pin (A1 = `1`, ... up to A6).

### AVR (Uno, Nano, etc.)
- **Output:** PWM on Timer1, pins **9** (`DAC0_PIN`) and **10** (`DAC1_PIN`), defined in
  `AutoAnalog_config.h`. Use the first argument of `feedDAC()` to pick the channel, as on the Due.
  On boards where Timer1's outputs are on other pins (e.g. Mega: 11 & 12), adjust these defines.
  A simple RC low-pass filter on the output is recommended.
- **Input:** `aaAudio.enableAdcChannel(n)` selects the analog input pin.

### Arduino Uno R4 (Renesas)
- **Output:** the onboard DAC on pin **A0**.
- **Input:** `aaAudio.enableAdcChannel(n)` selects the ADC channel. Channel `0` is pin **A1**,
  because A0 is used by the DAC.

### nRF52840 / nRF54L15
Assign pins through these public members:

| Peripheral | Members | Pin numbering |
|---|---|---|
| PDM microphone | `clkPin`, `dinPin`, `pwrPin` (`-1` = none) | Arduino pin numbers. Defaults come from the board's `PIN_PDM_CLK` / `PIN_PDM_DIN` / `PIN_PDM_PWR` |
| SAADC | `dinPin` | Analog input index: AIN0 = `1`, AIN1 = `2`, etc. |
| I2S | `I2S_PIN_xxx` + `I2S_PORT_xxx` for `MCK`, `SCK` (BCLK), `LRCK`, `SDOUT`, `SDIN` | nRF GPIO **pin and port** numbers (e.g. P1.11 → pin 11, port 1), not Arduino pin numbers |
| PWM output | `DEFAULT_PWM_PIN` / `DEFAULT_PWM_PORT` | Compile-time defines at the top of the platform `.cpp` file |

Default I2S pins:

| Signal | nRF52840 (pin, port) | nRF54L15 (pin, port) |
|---|---|---|
| MCK | 2, 0 | 8, 2 |
| SCK / BCLK | 3, 0 | 5, 1 |
| LRCK | 29, 0 | 7, 1 |
| SDOUT | 5, 0 | 11, 1 |
| SDIN | 4, 0 | 10, 1 |

The default PWM output is P0.05 on the nRF52840 and P1.11 on the nRF54L15.

Example: I2S output to a MAX98357A amplifier on an nRF52840.
The MAX98357A doesn't use MCK, and its SD pin must be pulled HIGH.

```cpp
aaAudio.I2S_PIN_SCK   = 3;  aaAudio.I2S_PORT_SCK   = 0;  // BCLK
aaAudio.I2S_PIN_LRCK  = 29; aaAudio.I2S_PORT_LRCK  = 0;  // LRC
aaAudio.I2S_PIN_SDOUT = 5;  aaAudio.I2S_PORT_SDOUT = 0;  // DIN
aaAudio.maxBufferSize = BUFFER_SIZE;
aaAudio.begin(0, 2);  // No input, I2S output
```

Example: PDM microphone with PWM output.

```cpp
aaAudio.clkPin = PIN_PDM_CLK;
aaAudio.dinPin = PIN_PDM_DIN;
aaAudio.pwrPin = PIN_PDM_PWR;  // or -1 if the mic is always powered
aaAudio.gain   = 40;
aaAudio.maxBufferSize = BUFFER_SIZE;
aaAudio.begin(1, 1);  // PDM input, PWM output
```

See the [examples](examples/Platforms) folder for complete, platform-specific sketches
(Due, Uno R4, NRF52, ESP32), and the [documentation](http://TMRh20.github.io/AutoAnalogAudio/)
for the full API.

---

## License

    AutoAnalogAudio streaming via DAC & ADC by TMRh20
    Copyright (C) 2016  TMRh20 - tmrh20@gmail.com, github.com/TMRh20

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
