#include <AutoAnalogAudio.h>

AutoAnalog aaAudio;

#if defined(USE_TINYUSB)
// Needed for Serial.print on non-MBED enabled or adafruit-based nRF52 cores
#include "Adafruit_TinyUSB.h"
#endif

#define bufferSize 320
float volumeControl = 0.2;

/*********************************************************/
/* Tested with MAX98357A I2S breakout on XIAO 52840 Sense
/* BCLK connected to Arduino D1 (p0.03)
/* LRCK connected to Arduino D3 (p0.29)
/* DIN  connected to Arduino D5 (p0.05)
/* SD   connected to Arduino D6
/*********************************************************/
/* Tested with INMP441 I2S breakout on XIAO 52840 Sense
/* SCK connected to Arduino D1 (p0.03)
/* WS  connected to Arduino D3 (p0.29)
/* SD  connected to Arduino D4 (p0.04)
*/


void setup() {


  Serial.begin(115200);
  aaAudio.maxBufferSize = bufferSize;
  aaAudio.begin(1, 1, 3);           //Setup aaAudio using I2s for Mic and Output
  aaAudio.adcBitsPerSample = 24;    // 24-bit samples input from I2S
  aaAudio.dacBitsPerSample = 24;    // 24-bit samples output to I2S
  aaAudio.setSampleRate(44000, 0);  // 44kHz Mono

  pinMode(6, OUTPUT);  //Connected to SD pin of MAX98357A
  digitalWrite(6, HIGH);
}

void loop() {

  if (Serial.available()) {
    char c = Serial.read();
    if (c == '=') {
      volumeControl += 0.1;
    } else if (c == '-') {
      volumeControl -= 0.1;
      volumeControl = max(0.0, volumeControl);
    }
    Serial.println(volumeControl);
  }


  // For 24 bit samples
  if (aaAudio.dacBitsPerSample == 24) {
    aaAudio.getADC(bufferSize / 2);                                    // Get data from I2S microphone
    memcpy(aaAudio.dacBuffer16, aaAudio.adcBuffer16, bufferSize * 2);  // Copy data into output buffer from input buffer
    for (int i = 0; i < bufferSize; i += 2) {
      int32_t test;
      memcpy(&test, &aaAudio.dacBuffer16[i], 4);
      test *= volumeControl;
      memcpy(&aaAudio.dacBuffer16[i], &test, 4);
    }
    aaAudio.feedDAC(0, bufferSize / 2);  // Feed the data to I2S output

    // For 16-bit samples
  } else {
    aaAudio.getADC(bufferSize);
    memcpy(aaAudio.dacBuffer16, aaAudio.adcBuffer16, bufferSize * 2);
    for (int i = 0; i < bufferSize; i++) {
      int16_t test = aaAudio.dacBuffer16[i];
      test *= volumeControl;
      aaAudio.dacBuffer16[i] = test;
    }
    aaAudio.feedDAC(0, bufferSize);  // Feed the data to I2S output
  }
}



