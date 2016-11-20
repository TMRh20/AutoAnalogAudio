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

  SDAudioWavRecorder Example: (Higher quality recording requires Arduino Due or better)
  This example demonstrates a simple method to record WAV format files that will play back on any PC or audio device
  The default format is 8-bit, 11khz, Mono and can be adjusted as desired
  
*/

/******** User Config ************************************/

#define SD_CS_PIN 10
#define AUDIO_DEBUG
#define RECORD_DEBUG

const char newWavFile[] = "test.wav";

/*********************************************************/
#include <SPI.h>
#include <SD.h>
#include <AutoAnalogAudio.h>
/*********************************************************/
AutoAnalog aaAudio;
File myFile;
File recFile;
/*********************************************************/
#include "myWAV.h"
#include "myRecording.h"
/*********************************************************/

void setup() {
  
  Serial.begin(115200);
  
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD init failed!");
    return;
  }
  Serial.println("SD ok\nAnalog Audio Begin");
  
  aaAudio.begin(1, 1);     // Start AAAudio with ADC & DAC
  aaAudio.autoAdjust = 0;  // Disable automatic timer adjustment

}

/*********************************************************/
uint32_t displayTimer = 0;

void loop() {

  if(millis()-displayTimer>1000){
    displayTimer = millis();
    if(counter){
      Serial.print("Samples per Second: ");
      Serial.println(counter * MAX_BUFFER_SIZE);
    }
    counter = 0;
  }

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
      case '8':  Serial.println("OK");      break;
      case '9':  startRecording(newWavFile,11000); break; //Start recording @11khz,8-bit,Mono
      case '0':  stopRecording(newWavFile,11000);  break; //Stop the recording and finalize the file
      case 'p':  playAudio(newWavFile);      break; //Play back the recorded audio
      case 'D':  SD.remove(newWavFile);      break; //Delete the file and start fresh
    }
  }  
}

/*********************************************************/


