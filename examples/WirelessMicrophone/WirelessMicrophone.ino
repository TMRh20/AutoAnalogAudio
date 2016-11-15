/*
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


  Auto Analog Audio (Automatic DAC, ADC & Timer) library

  Features:
  1. Very simple user interface to Arduino DUE DAC and ADC
  2. PCM/WAV Audio/Analog Data playback using Arduino Due DAC
  3. PCM/WAV Audio/Analog Data recording using Arduino Due ADC
  4. Onboard timers drive the DAC & ADC automatically
  5. Automatic sample rate/timer adjustment based on rate of user-driven data requests/input
  6. Uses DMA (Direct Memory Access) to buffer DAC & ADC data

  Auto Analog Audio Library Information:
  http://github.com/TMRh20
  http://tmrh20.blogspot.com

  Audio Relay & Peripheral Test Example:
  This example demonstrates how to manage incoming and outgoing audio streams using
  the AAAudio library and nrf24l01+ radio modules on Arduino Due.

  1. This example uses the onboard ADC to sample audio/analog data via pin A0
  2. The data is then sent via radio to another device
  3. Audio is captured in 8-bit,mono,16khz. 
  
  Library supports 8,10,12-bit sampling at various sample rates.

*/


#include <AutoAnalogAudio.h>
#include "myRadio.h"

AutoAnalog aaAudio;

/*********************************************************/

void setup() {

  Serial.begin(115200);
  Serial.println("Analog Audio Begin");

  aaAudio.begin(1,0);  //Setup aaAudio using ADC only
  aaAudio.autoAdjust = 0;
  aaAudio.adcBitsPerSample = 8;
  aaAudio.setSampleRate(16050);
  setupRadio();
}

/*********************************************************/

uint32_t dispTimer = 0;
uint8_t channelSelection = 0;

void loop() {

  //Display the timer period variable for each channel every 3 seconds
  if (millis() - dispTimer > 3000) {
    dispTimer = millis();

    TcChannel * t = &(TC0->TC_CHANNEL)[0];
    TcChannel * tt = &(TC0->TC_CHANNEL)[1];

    Serial.print("Ch0:");
    Serial.println(t->TC_RC);
    Serial.print("Ch1:");
    Serial.println(tt->TC_RC);
  }

  // With autoAdjust disabled, getADC() will block until the ADC data is ready
  aaAudio.getADC(32);
  radio.writeFast(&aaAudio.adcBuffer,32);
}

/*********************************************************/

