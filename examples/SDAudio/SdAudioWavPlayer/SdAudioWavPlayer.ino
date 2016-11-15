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

  Auto Analog Audio Library Information:
  http://github.com/TMRh20
  http://tmrh20.blogspot.com

  SDAudioWavPlayer Example:
  This example demonstrates a more complex method of playing *.wav files using AAAudio using interrupts to load the data
  while other code is able to run.
  Wav files in this example need to be 8 to 12-bit, 16-44khz, Mono or Stereo
  
*/

/******** User Config ************************************/

#define SD_CS_PIN 10
#define AUDIO_DEBUG

/*********************************************************/
#include <SPI.h>
#include <SD.h>
#include <AutoAnalogAudio.h>
/*********************************************************/
AutoAnalog aaAudio;
File myFile;
/*********************************************************/
#include "myWAV.h"
/*********************************************************/

void setup() {
  
  Serial.begin(115200);
  
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD init failed!");
    return;
  }
  Serial.println("SD ok\nAnalog Audio Begin");
  
  aaAudio.begin(0, 1);     // Start AAAudio with only the DAC
  aaAudio.autoAdjust = 0;  // Disable automatic timer adjustment
  
}

/*********************************************************/

void loop() {

  if (Serial.available()) {
    char input = Serial.read();
    switch (input) {

      case '1':  playAudio("M8b24kM.wav");  break; //Play a *.wav file by name - 8bit, 24khz, Mono
      case '2':  playAudio("M8b24kS.wav");  break; //Play  8bit, 24khz, Stereo
      case '3':  playAudio("M16b24kS.wav"); break; //Play 16bit, 24khz, Stereo
      case '4':  playAudio("M8b44kST.wav"); break; //Play  8bit, 44khz, Stereo
      case '5':  channelSelection = 0;      break; //Play the audio on DAC0
      case '6':  channelSelection = 1;      break; //Play the audio on DAC1
      case '7':  channelSelection = 2;      break; //Play the audio on DAC0 & DAC1
      case '8': Serial.println("OK");       break;
    }
  }
}

/*********************************************************/


