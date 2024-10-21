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

  This example automatically determines the format of WAV files & 
  attempts to play them.
*/

#include <SPI.h>
#include <SdFat.h>
#include <AutoAnalogAudio.h>

AutoAnalog aaAudio;
SdFat SD;

/*********************************************************/
/* Tested with MAX98357A I2S breakout
/* BCLK connected to Arduino D1 (p0.03)
/* LRCK connected to Arduino D3 (p0.29)
/* DIN  connected to Arduino D5 (p0.05)
/* SD   connected to Arduino D6 (p1.11)
/*********************************************************/
float volumeControl = 0.2;
#define BUFFER_SIZE 6400

void setup() {

  Serial.begin(115200);

  Serial.print("Init SD card...");
  if (!SD.begin(2, 32000000)) {
    Serial.println("init failed!");
    return;
  }
  Serial.println("init ok");
  Serial.println("Analog Audio Begin");
  aaAudio.begin(0, 1, 1);  //Setup aaAudio using DAC and I2S

  pinMode(6, OUTPUT);  //Connected to SD pin of MAX98357A
  digitalWrite(6, HIGH);

  playAudio("brick/brick24.wav");  // 24kHz @ 16-bits is about the maximum when reading from SD card
}

void loop() {

  loadBuffer();


  if (Serial.available()) {
    char c = Serial.read();
    if (c == '=') {
      volumeControl += 0.1;
    } else if (c == '-') {
      volumeControl -= 0.1;
      volumeControl = max(0.0, volumeControl);
    } else if (c == 'p') {
      playAudio("brick/brick24.wav");
    }
    Serial.println(volumeControl);
  }
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

  myFile.seek(22);
  uint16_t var;
  uint32_t var2;
  myFile.read(&var, 2);   // Get channels (Stereo or Mono)
  myFile.read(&var2, 4);  // Get Sample Rate
  aaAudio.setSampleRate(var2, var - 1);

  myFile.seek(34);
  myFile.read(&var, 2);  // Get Bits Per Sample
  aaAudio.dacBitsPerSample = var;

  myFile.seek(44);  //Skip past the WAV header
}

void loadBuffer() {

  if (myFile.available()) {

    if (aaAudio.dacBitsPerSample == 8) {
      myFile.read(aaAudio.dacBuffer, BUFFER_SIZE);  // Change this to dacBuffer16 for 16-bit samples
      for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
        aaAudio.dacBuffer[i] *= volumeControl;
      }
      aaAudio.feedDAC(0, BUFFER_SIZE);  // change this to 3200 for 16-bit samples
    } else {
      myFile.read(aaAudio.dacBuffer16, BUFFER_SIZE);  // Change this to dacBuffer16 for 16-bit samples
      for (uint32_t i = 0; i < BUFFER_SIZE / 2; i++) {
        int16_t sample = aaAudio.dacBuffer16[i];
        sample *= volumeControl;
        aaAudio.dacBuffer16[i] = (uint16_t)sample;
      }
      aaAudio.feedDAC(0, BUFFER_SIZE / 2);  // change this to 3200 for 16-bit samples
    }

  } else {
    myFile.seek(44);
  }
}
