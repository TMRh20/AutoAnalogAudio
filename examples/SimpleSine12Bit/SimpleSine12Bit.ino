
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
* SineWave12Bit Example (Requires Arduino Due or better):
* Simple generation of a 12-bit sine wave
* Send a number 1 or 2 over Serial to change frequency, +/- to adjust volume
* 
*/

 
#include <AutoAnalogAudio.h>

AutoAnalog aaAudio;

void DACC_Handler(void){ 
  aaAudio.dacHandler();   //Link the DAC ISR/IRQ to the library. Called by the MCU when DAC is ready for data
}


void setup() {
  
  Serial.begin(115200);
  Serial.println("Analog Audio Begin");
  
  aaAudio.begin(0,1);           //Setup aaAudio using DAC
  aaAudio.autoAdjust = 0;       //Disable automatic timer adjustment
  aaAudio.setSampleRate(16000); //Set the sample rate to 16khz
  aaAudio.dacBitsPerSample=12;  //Use 12-bit samples for the DAC
  arraysetup();                 //Load the DAC buffer using a 32-step sine wave

}

char shiftVal = 0;

uint32_t dispTimer = 0;

void loop() {

  //AutoAdjust is disabled above, so this function will block until the DAC is ready for more data
  //All other processing needs to be completed before the DAC is out of data
  //In this example, the DAC is being fed data in chunks of 32 bytes or 32 8-bit samples
  aaAudio.feedDAC(0,32);

  //Choose between two different frequencies via Serial command
  //Adjust the volume by sending a '+' or '-' over Serial
  if(Serial.available()){
    char d = Serial.read();
    switch(d){
      case '1': arraysetup(); break;
      case '2': arraysetup2(); break;
      case '+': shiftVal > 0 ? --shiftVal : NULL; break;
      case '-': shiftVal < 11 ? ++shiftVal : NULL; break;
      default: arraysetup(); break;
    }
    Serial.print("Volume: ");
    Serial.print(11-shiftVal,DEC);
    Serial.println("/11");
  }  
}

//Load a 32-step sine wave into the dacBuffer
//Shift the values according to volume
void arraysetup(void){ 
  aaAudio.dacBuffer16[0]=0x800 >> shiftVal; 
  aaAudio.dacBuffer16[1]=0x990 >> shiftVal; 
  aaAudio.dacBuffer16[2]=0xB10 >> shiftVal; 
  aaAudio.dacBuffer16[3]=0xC72  >> shiftVal; 
  aaAudio.dacBuffer16[4]=0xDA8 >> shiftVal; 
  aaAudio.dacBuffer16[5]=0xEA7 >> shiftVal; 
  aaAudio.dacBuffer16[6]=0xF64 >> shiftVal; 
  aaAudio.dacBuffer16[7]=0xFD9 >> shiftVal; 
  aaAudio.dacBuffer16[8]=0xFFF >> shiftVal; 
  aaAudio.dacBuffer16[9]=0xFD9 >> shiftVal; 
  aaAudio.dacBuffer16[10]=0xF64 >> shiftVal; 
  aaAudio.dacBuffer16[11]=0xEA7 >> shiftVal; 
  aaAudio.dacBuffer16[12]=0xDA8 >> shiftVal; 
  aaAudio.dacBuffer16[13]=0xC72 >> shiftVal; 
  aaAudio.dacBuffer16[14]=0xB10 >> shiftVal; 
  aaAudio.dacBuffer16[15]=0x990 >> shiftVal; 
  aaAudio.dacBuffer16[16]=0x800 >> shiftVal; 
  aaAudio.dacBuffer16[17]=0x670 >> shiftVal; 
  aaAudio.dacBuffer16[18]=0x4F0 >> shiftVal; 
  aaAudio.dacBuffer16[19]=0x38E >> shiftVal; 
  aaAudio.dacBuffer16[20]=0x258 >> shiftVal; 
  aaAudio.dacBuffer16[21]=0x159 >> shiftVal; 
  aaAudio.dacBuffer16[22]=0x9C >> shiftVal; 
  aaAudio.dacBuffer16[23]=0x27 >> shiftVal; 
  aaAudio.dacBuffer16[24]=0x0 >> shiftVal; 
  aaAudio.dacBuffer16[25]=0x27 >> shiftVal; 
  aaAudio.dacBuffer16[26]=0x9c >> shiftVal; 
  aaAudio.dacBuffer16[27]=0x159 >> shiftVal; 
  aaAudio.dacBuffer16[28]=0x258 >> shiftVal; 
  aaAudio.dacBuffer16[29]=0x38E >> shiftVal; 
  aaAudio.dacBuffer16[30]=0x4F0 >> shiftVal; 
  aaAudio.dacBuffer16[31]=0x670 >> shiftVal; 
}


//Load a 16-step sine wave into the dacBuffer
//Shift the values according to volume
void arraysetup2(void){ 
  aaAudio.dacBuffer16[0]=0x800 >> shiftVal;
  aaAudio.dacBuffer16[1]=0xB10 >> shiftVal; 
  aaAudio.dacBuffer16[2]=0xDA8 >> shiftVal; 
  aaAudio.dacBuffer16[3]=0xF64 >> shiftVal; 
  aaAudio.dacBuffer16[4]=0xFFF >> shiftVal; 
  aaAudio.dacBuffer16[5]=0xF64 >> shiftVal; 
  aaAudio.dacBuffer16[6]=0xDA8 >> shiftVal; 
  aaAudio.dacBuffer16[7]=0xB10 >> shiftVal; 
  aaAudio.dacBuffer16[8]=0x800 >> shiftVal; 
  aaAudio.dacBuffer16[9]=0x4F0 >> shiftVal; 
  aaAudio.dacBuffer16[10]=0x258 >> shiftVal; 
  aaAudio.dacBuffer16[11]=0x9C >> shiftVal; 
  aaAudio.dacBuffer16[12]=0x0 >> shiftVal; 
  aaAudio.dacBuffer16[13]=0x9C >> shiftVal; 
  aaAudio.dacBuffer16[14]=0x258 >> shiftVal; 
  aaAudio.dacBuffer16[15]=0x4F0 >> shiftVal;
  aaAudio.dacBuffer16[16]=0x800 >> shiftVal;
  aaAudio.dacBuffer16[17]=0xB10 >> shiftVal; 
  aaAudio.dacBuffer16[18]=0xDA8 >> shiftVal; 
  aaAudio.dacBuffer16[19]=0xF64 >> shiftVal; 
  aaAudio.dacBuffer16[20]=0x1000 >> shiftVal; 
  aaAudio.dacBuffer16[21]=0xF64 >> shiftVal; 
  aaAudio.dacBuffer16[22]=0xDA8 >> shiftVal; 
  aaAudio.dacBuffer16[23]=0xB10 >> shiftVal; 
  aaAudio.dacBuffer16[24]=0x800 >> shiftVal; 
  aaAudio.dacBuffer16[25]=0x4F0 >> shiftVal; 
  aaAudio.dacBuffer16[26]=0x258 >> shiftVal; 
  aaAudio.dacBuffer16[27]=0x9C >> shiftVal; 
  aaAudio.dacBuffer16[28]=0x0 >> shiftVal; 
  aaAudio.dacBuffer16[29]=0x9C >> shiftVal; 
  aaAudio.dacBuffer16[30]=0x258 >> shiftVal; 
  aaAudio.dacBuffer16[31]=0x4F0 >> shiftVal;
}
