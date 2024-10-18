#include <AutoAnalogAudio.h>

AutoAnalog aaAudio;

#if defined(USE_TINYUSB)
    // Needed for Serial.print on non-MBED enabled or adafruit-based nRF52 cores
    #include "Adafruit_TinyUSB.h"
#endif

/*********************************************************/
/* Tested with MAX98357A I2S breakout
/* BCLK connected to Arduino D1 (p0.03)
/* LRCK connected to Arduino D3 (p0.29)
/* DIN  connected to Arduino D5 (p0.05)
/* SD   connected to Arduino D6 (p1.11)
/*********************************************************/

void setup() {


  Serial.begin(115200);
  aaAudio.begin(1, 1, 1);  //Setup aaAudio using DAC & I2S
  aaAudio.adcBitsPerSample = 16; // 16-bit samples input from PDM
  aaAudio.dacBitsPerSample = 16; // 16-bit samples output to I2S
  aaAudio.setSampleRate(32000);  // 32kHz
  
  pinMode(6, OUTPUT);  //Connected to SD pin of MAX98357A
  digitalWrite(6,HIGH);

}

void loop() {

  aaAudio.getADC(6400); // Get data from PDM microphone

  memcpy(aaAudio.dacBuffer16, aaAudio.adcBuffer16, 12800); // Copy data into output buffer from input buffer
 
  aaAudio.feedDAC(0,6400); // Feed the data to I2S output
}
