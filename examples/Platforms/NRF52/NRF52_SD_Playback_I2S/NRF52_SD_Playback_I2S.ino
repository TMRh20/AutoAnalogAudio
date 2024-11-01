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
/* Tested with MAX98357A I2S breakout on XIAO 52840 Sense
/* BCLK connected to Arduino D1 (p0.03)
/* LRCK connected to Arduino D3 (p0.29)
/* DIN  connected to Arduino D5 (p0.05)
/* SD   connected to Arduino D6 (p1.11)
/*********************************************************/

void setup() {

  Serial.begin(115200);

  Serial.print("Init SD card...");
  if (!SD.begin(2)) {
    Serial.println("init failed!");
    return;
  }
  Serial.println("init ok");
  Serial.println("Analog Audio Begin");
  aaAudio.begin(0, 1, 1);  //Setup aaAudio using DAC and I2S

  //Setup for audio: Use 8 or 16-bit, mono or stereo. Valid sample rates are 16000, 24000, 32000, 44000
  aaAudio.dacBitsPerSample = 8;    // 8-bit
  aaAudio.setSampleRate(24000, 0); // 24khz, mono

  pinMode(6,OUTPUT); //Connected to SD pin of MAX98357A
  digitalWrite(6,HIGH);

  playAudio("brick/brick24.wav"); // 24kHz @ 16-bits is about the maximum when reading from SD card
}

void loop() {

  loadBuffer();
  
}

/*********************************************************/
/* A simple function to handle playing audio files
/*********************************************************/

File myFile;

void playAudio(char *audioFile) {

  if (myFile) {
    myFile.close();
  }
  //Open the designated file
  myFile = SD.open(audioFile);
  //Skip past the WAV header
  myFile.seek(44);
}

void loadBuffer() {

  if( myFile.available() ){
    myFile.read(aaAudio.dacBuffer, 6400); // Change this to dacBuffer16 for 16-bit samples
    aaAudio.feedDAC(0, 6400);             // change this to 3200 for 16-bit samples
  }else{
    myFile.seek(44);
  }
}
