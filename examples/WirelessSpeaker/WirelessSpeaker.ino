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

  1. This example uses the onboard DAC to play the incoming audio data via DAC0
  2. The incoming audio format is 16bit mono
  3. The nrf24l01+ radios can support around 16-44khz sample rate w/16-bit samples, 88khz+ with 8-bit samples

*/


#include <RF24.h>
#include <AutoAnalogAudio.h>
#include "myRadio.h"

AutoAnalog aaAudio;

/*********************************************************/

void DACC_Handler(void) {
  aaAudio.dacHandler();   //Link the DAC ISR/IRQ library. Called by the MCU when DAC is ready for data
}

/*********************************************************/

void setup() {

  Serial.begin(115200);
  Serial.println("Analog Audio Begin");

  aaAudio.begin(0, 1);  //Setup aaAudio using both DAC and ADC
  aaAudio.autoAdjust = 1;
  aaAudio.dacBitsPerSample = 12;
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

  // Send serial commands to control which channel the audio is played on
  // DAC Channel 0, Channel 1, or both
  if (Serial.available()) {
    char dat = Serial.read();
    switch (dat) {
      case '0': channelSelection = 0; break; // Use DAC0 for output
      case '1': channelSelection = 1; break; // Use DAC1 for output
      case '2': channelSelection = 2; break; // Use both DAC0 and DAC1

    }
  }
}

/*********************************************************/

uint32_t dynSampleRate = 0;

// See myRadio.h: Function is attached to an interrupt triggered by radio RX/TX
void RX() {

  while (radio.available(&pipeNo)) {                          // Check for data and get the pipe number

    if (pipeNo == 2) {
      radio.read(&dynSampleRate, 4);                          // Receive commands using pipe #2
      aaAudio.setSampleRate(dynSampleRate);                   // Pipe 2 is being used for command data, pipe 1 & others for audio data
    } else {

      radio.read(&aaAudio.dacBuffer16, 32);                   // Read the available radio data
      
      for (int i = 0; i < 16; i++) {                          //Convert signed 16-bit variables into unsigned 12-bit
        aaAudio.dacBuffer16[i] += 0x8000;
        aaAudio.dacBuffer16[i] = aaAudio.dacBuffer16[i] >> 4;
      }

      //Received 32 bytes: 16 samples of 16-bits each
      //Send them to the DAC using the channel selected via Serial command
      aaAudio.feedDAC(channelSelection, 16);
    }
  }
}

/*********************************************************/

