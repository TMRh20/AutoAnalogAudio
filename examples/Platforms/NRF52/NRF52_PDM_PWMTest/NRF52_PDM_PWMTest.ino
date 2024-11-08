#include <AutoAnalogAudio.h>
AutoAnalog aaAudio;

#if defined(USE_TINYUSB)
// Needed for Serial.print on non-MBED enabled or adafruit-based nRF52 cores
#include "Adafruit_TinyUSB.h"
#endif

void setup() {
  Serial.begin(115200);
  aaAudio.begin(1, 1);  //Startup the PDM Microphone and PWM(pseudo DAC) on pin 5
  aaAudio.adcBitsPerSample = 16;  // 16-bit audio at 16khz is the default on NRF52 and cannot be modified currently (in progress)
  aaAudio.dacBitsPerSample = 16;
  aaAudio.setSampleRate(16000);
}

void loop() {

  aaAudio.getADC(320);                        // Get 320 16-bit signed samples from the ADC
  for (int i = 0; i < 320; i++) {             // Copy them into the DAC Buffer. The dac buffer is 16-bits unsigned
    int16_t sample = aaAudio.adcBuffer16[i];  // Copy to signed 16-bit
    sample += 0x8000;                         // Convert to unsigned 16-bit
    aaAudio.dacBuffer16[i] = sample;          // Copy back to unsigned 16-bit buffer
    aaAudio.dacBuffer16[i] >>= 6;             // Downsample to 10-bits for PWM output. With higher sample rates PWM can only handle 8-bits
  }
  aaAudio.feedDAC(0, 320);  // Feed the DAC with the ADC data
}
