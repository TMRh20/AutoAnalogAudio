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

*/

#include <SPI.h>
#include <SD.h>
#include <AutoAnalogAudio.h>

AutoAnalog aaAudio;

/*********************************************************/

void DACC_Handler(void) {
  aaAudio.dacHandler();   //Link the DAC ISR/IRQ to the library. Called by the MCU when DAC is ready for data
}

/*********************************************************/

void setup() {
  Serial.begin(115200);

  Serial.print("Init SD card...");
  if (!SD.begin(10)) {
    Serial.println("init failed!");
    return;
  }
  Serial.println("init ok");
  
  Serial.println("Analog Audio Begin");
  aaAudio.begin(0, 1);  //Setup aaAudio using DAC
  aaAudio.autoAdjust = 0;
  
  //Setup for audio at 8-bit, 16khz, mono
  aaAudio.dacBitsPerSample = 8;
  aaAudio.setSampleRate(16000);
  
}

void loop() {

  if(Serial.available()){
    char input = Serial.read();
    switch(input){
      case '1': playAudio("M8b24kM.wav"); break;  //Play a *.wav file by name - 8bit, 24khz, Mono
      case '2': playAudio("M8b24kS.wav"); break;  //Play  8bit, 24khz, Stereo
      case '3': playAudio("audio.wav");   break;  //Play a random file called audio.wav
    }
  }
  
  loadBuffer();
}

/*********************************************************/
/* A simple function to handle playing audio files
/*********************************************************/

File myFile;

void playAudio(char *audioFile){

  if (myFile) {
    myFile.close();
  }
  //Open the designated file
  myFile = SD.open(audioFile);
  
  //Skip past the WAV header
  myFile.seek(44);
  loadBuffer();
  
}

void loadBuffer(){
  //Load 32 samples into the 8-bit dacBuffer
  if(myFile.available()){
    for(int i=0;i<32;i++){
      aaAudio.dacBuffer[i] = myFile.read();
    }
    //Feed the dac with the buffer of data
    aaAudio.feedDAC(0,32);
  }  
  
}

