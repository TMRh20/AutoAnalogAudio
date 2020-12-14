/*
    AutoAnalogAudio streaming via DAC & ADC by TMRh20
    Copyright (C) 2016-2020  TMRh20 - tmrh20@gmail.com, github.com/TMRh20

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
*/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "AutoAnalogAudio.h"

/******** User Config ************************************/

#define SD_CS_PIN 17
#define AUDIO_DEBUG

//Use FreeRTOS Tasks to load data and feed into the DAC, false to handle from main loop
bool useTasks = true;
//Number of ADC readings to sample in a single request. Set to MAX_BUFFER_SIZE or a number < 512
uint32_t adcReadings = MAX_BUFFER_SIZE;

/*********************************************************/
AutoAnalog aaAudio;
File myFile;
/*********************************************************/
#include "volume.h"
#include "myWAV.h"
#include "mySine.h"
/*********************************************************/

bool doADC = false;
bool firstADC = false;

void setup() {

  Serial.begin(57600);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD init failed!");
    //return;
  }
  Serial.println("SD ok\nAnalog Audio Begin");

  aaAudio.begin(1, 1);     // Start AAAudio with only the DAC (ADC,DAC,External ADC)
  aaAudio.autoAdjust = 0;  // Disable automatic timer adjustment
  aaAudio.setSampleRate(80000);
  aaAudio.dacBitsPerSample = 8;

}

/*********************************************************/
uint8_t count = 0;
uint32_t printTimer = millis();

uint32_t adcTimer = 0;
bool doExtADC = false;
bool doSine = false;

/*********************************************************/
uint32_t testCounter = 0;

void loop() {


  if (doADC) {
    aaAudio.getADC(adcReadings);
    //The first call to getADC will not return proper values, so do not write samples to the DAC
    if (!firstADC) {
      for (int i = 0; i < adcReadings; i++) {
        //Volume control:
        //a: Convert unsigned 12-bit ADC output to signed
        //b: Divide by volume modifier, convert back to unsigned
        //c: Downshift to 8-bits
        //d: Feed into the DAC

        int16_t tmpVar = (int16_t)aaAudio.adcBuffer16[i] - 0x800;
        tmpVar = (tmpVar / volumeVar) + 0x800;
        aaAudio.dacBuffer[i] = (uint8_t)(tmpVar >> 4);
        //aaAudio.dacBuffer[i] = aaAudio.adcBuffer16[i];

      }
      aaAudio.feedDAC(0, adcReadings);

    } else {
      firstADC = false;
      memset(aaAudio.dacBuffer, 0, MAX_BUFFER_SIZE);
      aaAudio.feedDAC(0, MAX_BUFFER_SIZE);
      aaAudio.feedDAC(0, MAX_BUFFER_SIZE);

    }
  }

  if (doSine) {
    aaAudio.feedDAC(0, 32);
  }

  if (Serial.available()) {
    char input = Serial.read();
    switch (input) {

      case '1':  playAudio("/M8b24kM.wav");  break; //Play a *.wav file by name - 16bit, 32khz, Mono
      case '2':  playAudio("/M8b24kS.wav");  break; //Play  16bit, 32khz, Stereo
      case '3':  playAudio("/M8b44kST.wav"); break; //Play 16bit, 44khz, Mono
      case '4':  playAudio("/M16b24kS.wav"); break; //Play  16bit, 44khz, Stereo
      //Note: Audio playback should be stopped before engaging the ADC->DAC output
      case '5':  doADC = !doADC; if (doADC) {
          firstADC = true;
          aaAudio.setSampleRate(88200);
          aaAudio.dacBitsPerSample = 8;
        } else {
          aaAudio.disableDAC();
        }  break;
      case '6':  volume(1); break;
      case '7':  volume(0); break;
      case '8':  volumeSet(1); break;
      case '9':  volumeSet(5); break;
      case '0':  playAudio("/calibrat.wav"); break;
      //Note: Switching the use of tasks while audio is playing will cause problems
      case 't':  useTasks = !useTasks; Serial.print("Using Tasks: "); Serial.println(useTasks ? "True" : "False"); break;
      case 'e':  adcReadings = 32; break;
      case 'r':  adcReadings = MAX_BUFFER_SIZE; break;
      case 'a':  Serial.println("Manual Close"); aaAudio.rampOut(0); aaAudio.disableDAC(); myFile.close(); break;
      case 's':  Serial.println("Playing 8-bit sine wave"); aaAudio.setSampleRate(32000); aaAudio.dacBitsPerSample = 8; sineSetup8(); doSine = true; break;
      case 'd':  Serial.println("Playing 16-bit sine wave"); aaAudio.setSampleRate(32000); aaAudio.dacBitsPerSample = 16; sineSetup16(); doSine = true; break;
      case 'f':  Serial.println("Stop sine wave"); doSine = false; aaAudio.disableDAC(); break;
    }
  }

  //If tasks are disabled, handle loading of data and feeding DAC from main loop
  if (!useTasks) {
    DACC_Handler();
  }

  //Simple heartbeat to show activity
  if (millis() - printTimer > 500) {
    printTimer = millis();
    Serial.print(".");
    count++;
    if (count >= 50) {
      count = 0;
      Serial.println("");
    }
  }

}

/*********************************************************/
