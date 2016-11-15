
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

* 
* SineWave Example:
* Simple generation of a sine wave & optionally broadcasting the audio via radio
* Send a number 1 or 2 over Serial to change frequency, +/- to adjust volume
* 
*/

 
#include <AutoAnalogAudio.h>
//#include <RF24.h>
//#include "myRadio.h"

AutoAnalog aaAudio;

void DACC_Handler(void){ 
  aaAudio.dacHandler();   //Link the DAC ISR/IRQ to the library. Called by the MCU when DAC is ready for data
}


void setup() {
  
  Serial.begin(115200);
  Serial.println("Analog Audio Begin");
  
  //Optional: Setup the radio to broadcast the generated audio
  //setupRadio();
  
  aaAudio.begin(0,1);           //Setup aaAudio using DAC
  aaAudio.autoAdjust = 0;       //Disable automatic timer adjustment
  aaAudio.setSampleRate(16050); //Set the sample rate to 16khz
  arraysetup();                 //Load the DAC buffer using a 32-step sine wave

}

char shiftVal = 0;

uint32_t dispTimer = 0;

void loop() {


  //AutoAdjust is disabled above, so this function will block until the DAC is ready for more data
  //All other processing needs to be completed before the DAC is out of data
  //In this example, the DAC is being fed data in chunks of 32 bytes or 32 8-bit samples
  aaAudio.feedDAC(0,32);

  //Optional: Broadcast the audio over radio
  //radio.startFastWrite(&aaAudio.dacBuffer,32,1);
  
  //Choose between two different frequencies via Serial command
  //Adjust the volume by sending a '+' or '-' over Serial
  if(Serial.available()){
    char d = Serial.read();
    switch(d){
      case '1': arraysetup(); break;
      case '2': arraysetup2(); break;
      case '+': shiftVal > 0 ? --shiftVal : NULL; break;
      case '-': shiftVal < 7 ? ++shiftVal : NULL; break;
      default: arraysetup(); break;
    }
    Serial.print("Volume: ");
    Serial.print(7-shiftVal,DEC);
    Serial.println("/7");
  }  
}


//Load a 32-step sine wave into the dacBuffer
//Shift the values according to volume
void arraysetup(void){ 
  aaAudio.dacBuffer[0]=127 >> shiftVal; 
  aaAudio.dacBuffer[1]=152 >> shiftVal; 
  aaAudio.dacBuffer[2]=176 >> shiftVal; 
  aaAudio.dacBuffer[3]=198 >> shiftVal; 
  aaAudio.dacBuffer[4]=217 >> shiftVal; 
  aaAudio.dacBuffer[5]=233 >> shiftVal; 
  aaAudio.dacBuffer[6]=245 >> shiftVal; 
  aaAudio.dacBuffer[7]=252 >> shiftVal; 
  aaAudio.dacBuffer[8]=254 >> shiftVal; 
  aaAudio.dacBuffer[9]=252 >> shiftVal; 
  aaAudio.dacBuffer[10]=245 >> shiftVal; 
  aaAudio.dacBuffer[11]=233 >> shiftVal; 
  aaAudio.dacBuffer[12]=217 >> shiftVal; 
  aaAudio.dacBuffer[13]=198 >> shiftVal; 
  aaAudio.dacBuffer[14]=176 >> shiftVal; 
  aaAudio.dacBuffer[15]=152 >> shiftVal; 
  aaAudio.dacBuffer[16]=128 >> shiftVal; 
  aaAudio.dacBuffer[17]=103 >> shiftVal; 
  aaAudio.dacBuffer[18]=79 >> shiftVal; 
  aaAudio.dacBuffer[19]=57 >> shiftVal; 
  aaAudio.dacBuffer[20]=38 >> shiftVal; 
  aaAudio.dacBuffer[21]=22 >> shiftVal; 
  aaAudio.dacBuffer[22]=10 >> shiftVal; 
  aaAudio.dacBuffer[23]=3 >> shiftVal; 
  aaAudio.dacBuffer[24]=0 >> shiftVal; 
  aaAudio.dacBuffer[25]=3 >> shiftVal; 
  aaAudio.dacBuffer[26]=10 >> shiftVal; 
  aaAudio.dacBuffer[27]=22 >> shiftVal; 
  aaAudio.dacBuffer[28]=38 >> shiftVal; 
  aaAudio.dacBuffer[29]=57 >> shiftVal; 
  aaAudio.dacBuffer[30]=79 >> shiftVal; 
  aaAudio.dacBuffer[31]=103 >> shiftVal; 
}

//Load a 16-step sine wave into the dacBuffer
//Shift the values according to volume
void arraysetup2(void){ 
  aaAudio.dacBuffer[0]=127 >> shiftVal;
  aaAudio.dacBuffer[1]=176 >> shiftVal; 
  aaAudio.dacBuffer[2]=217 >> shiftVal; 
  aaAudio.dacBuffer[3]=245 >> shiftVal; 
  aaAudio.dacBuffer[4]=254 >> shiftVal; 
  aaAudio.dacBuffer[5]=245 >> shiftVal; 
  aaAudio.dacBuffer[6]=217 >> shiftVal; 
  aaAudio.dacBuffer[7]=176 >> shiftVal; 
  aaAudio.dacBuffer[8]=128 >> shiftVal; 
  aaAudio.dacBuffer[9]=79 >> shiftVal; 
  aaAudio.dacBuffer[10]=38 >> shiftVal; 
  aaAudio.dacBuffer[11]=10 >> shiftVal; 
  aaAudio.dacBuffer[12]=0 >> shiftVal; 
  aaAudio.dacBuffer[13]=10 >> shiftVal; 
  aaAudio.dacBuffer[14]=38 >> shiftVal; 
  aaAudio.dacBuffer[15]=79 >> shiftVal;
  aaAudio.dacBuffer[16]=127 >> shiftVal;
  aaAudio.dacBuffer[17]=176 >> shiftVal; 
  aaAudio.dacBuffer[18]=217 >> shiftVal; 
  aaAudio.dacBuffer[19]=245 >> shiftVal; 
  aaAudio.dacBuffer[20]=254 >> shiftVal; 
  aaAudio.dacBuffer[21]=245 >> shiftVal; 
  aaAudio.dacBuffer[22]=217 >> shiftVal; 
  aaAudio.dacBuffer[23]=176 >> shiftVal; 
  aaAudio.dacBuffer[24]=128 >> shiftVal; 
  aaAudio.dacBuffer[25]=79 >> shiftVal; 
  aaAudio.dacBuffer[26]=38 >> shiftVal; 
  aaAudio.dacBuffer[27]=10 >> shiftVal; 
  aaAudio.dacBuffer[28]=0 >> shiftVal; 
  aaAudio.dacBuffer[29]=10 >> shiftVal; 
  aaAudio.dacBuffer[30]=38 >> shiftVal; 
  aaAudio.dacBuffer[31]=79 >> shiftVal; 
}
