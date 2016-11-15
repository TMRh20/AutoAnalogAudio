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

  SDAudioAuto Example:
  This example demonstrates a more complex method of playing *.wav files using AAAudio using interrupts to load the data
  while other code is able to run.
  Wav files in this example are 8-bit, 32khz, Stereo
*/

#include <SPI.h>
#include <SD.h>
#include <AutoAnalogAudio.h>

AutoAnalog aaAudio;

File myFile;

uint8_t channelSelection = 2;             //Play audio on both DAC0 and DAC1 by default

/*********************************************************/

void DACC_Handler(void) {
  aaAudio.feedDAC(channelSelection);      //Feed the DAC with the data loaded into the dacBuffer
  aaAudio.dacHandler();                   //Link the DAC ISR/IRQ to the library. Called by the MCU when DAC is ready for data
  loadBuffer();                           //Call the loadBuffer() function defined below to buffer from SD Card
}

/*********************************************************/

void setup() {
  Serial.begin(115200);

  if (!SD.begin(10)) {
    Serial.println("SD init failed!");
    return;
  }
  Serial.println("SD ok");

  Serial.println("Analog Audio Begin");
  aaAudio.begin(0, 1);  //Setup aaAudio using DAC
  aaAudio.autoAdjust = 0;
}

/*********************************************************/

void loop() {

  if (Serial.available()) {
    char input = Serial.read();
    switch (input) {
                 //Play an 8-bit, 22Khz, Stereo WAV file
      case '1':  aaAudio.dacBitsPerSample = 8;  aaAudio.setSampleRate(24000); playAudio("M8b24kM.wav");  break;
                 //Play an 8-bit, 32Khz, Stereo WAV file
      case '2':  aaAudio.dacBitsPerSample = 8;  aaAudio.setSampleRate(48000); playAudio("M8b24kS.wav");  break;
      case '3':  channelSelection = 0;  break;
      case '4':  channelSelection = 1;  break;
      case '5':  channelSelection = 2;  break;
      case '6': Serial.println("OK"); break;
    }
  }
}

/*********************************************************/
/* Function to open the audio file, seek to starting position and enable the DAC interrupt */

void playAudio(char *audioFile) {

  if (myFile) {
    aaAudio.disableDAC();
    myFile.close();
  }

  //Open the designated file
  myFile = SD.open(audioFile);

  if (myFile) {
    //Skip past the WAV header
    myFile.seek(44);
    //Load one buffer
    loadBuffer();
    //Feed the DAC to start playback
    aaAudio.feedDAC();
  }
}

/*********************************************************/
/* Function called from DAC interrupt after dacHandler(). Loads data into the dacBuffer */

void loadBuffer() {
  
  if (myFile) {
    if (myFile.available()) {
      if (aaAudio.dacBitsPerSample == 8) {
        //Load 32 samples into the 8-bit dacBuffer
        myFile.read((byte*)aaAudio.dacBuffer, MAX_BUFFER_SIZE);
      }else{
        //Load 32 samples (64 bytes) into the 16-bit dacBuffer
        myFile.read((byte*)aaAudio.dacBuffer16, MAX_BUFFER_SIZE * 2);
        //Convert the 16-bit samples to 12-bit
        for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
          aaAudio.dacBuffer16[i] = (aaAudio.dacBuffer16[i] + 0x8000) >> 4;
        }
      }
    }else{
      myFile.close();
      dacc_disable_interrupt(DACC, DACC_IER_ENDTX);
    }
  }
}

/*********************************************************/
