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
* Auto Analog Audio (Automatic DAC, ADC & Timer) library
* 
* Features:
* 1. Very simple user interface to Arduino DUE DAC and ADC
* 2. PCM/WAV Audio/Analog Data playback using Arduino Due DAC
* 3. PCM/WAV Audio/Analog Data recording using Arduino Due ADC
* 4. Onboard timers drive the DAC & ADC automatically
* 5. Automatic sample rate/timer adjustment based on rate of user-driven data requests/input
* 6. Uses DMA (Direct Memory Access) to buffer DAC & ADC data
* 
* Auto Analog Audio Library Information:
* http://github.com/TMRh20
* http://tmrh20.blogspot.com
* 
* Audio Relay & Peripheral Test Example:
* This example demonstrates how to manage incoming and outgoing audio streams using 
* the AAAudio library and nrf24l01+ radio modules on Arduino Due.
* 
* 1. This example uses the onboard DAC to play the incoming audio data via DAC0
* 2. The ADC is used to sample the DAC (connected to pin A0) and the data is made available
* 3. The data is re-broadcast over the radios
* 4. Incoming radio data can be directly re-broadcast, but this example is a test of all peripherals
* 
*/

/************************USER CONFIG**********************/
#define radioCEPin 7
#define radioCSPin 8
#define radioInterruptPin 2
/*********************************************************/

#include <RF24.h>
#include <AutoAnalogAudio.h>
#include "myRadio.h"

AutoAnalog aaAudio;

/*********************************************************/

void DACC_Handler(void){ 
  aaAudio.dacHandler();   //Link the DAC ISR/IRQ library. Called by the MCU when DAC is ready for data
}

/*********************************************************/

void setup() {

  Serial.begin(115200);
  Serial.println("Analog Audio Begin");

  aaAudio.begin(1,1);   //Setup aaAudio using both DAC and ADC
  #if defined (ARDUINO_AVR)
    aaAudio.autoAdjust = 0;
  #endif
  setupRadio();  
}


/*********************************************************/

uint32_t dispTimer = 0;

void loop() {

  //Display the timer period variable for each channel every 3 seconds
  if(millis() - dispTimer > 3000){
    dispTimer = millis();
    
    #if !defined (ARDUINO_ARCH_AVR)
      TcChannel * t = &(TC0->TC_CHANNEL)[0];
      TcChannel * tt= &(TC0->TC_CHANNEL)[1];
    
      Serial.print("Ch0:"); 
      Serial.println(t->TC_RC);      
      Serial.print("Ch1:"); 
      Serial.println(tt->TC_RC);
    #else
      Serial.print("Ch0/1:");
      Serial.println(ICR1);
    #endif
  }
}

/*********************************************************/

uint32_t dynSampleRate = 0;

// See myRadio.h: Function is attached to an interrupt triggered by radio RX/TX
void RX(){
    
  if(radio.available(&pipeNo)){             // Check for data and get the pipe number

    if(pipeNo == 2){      
      radio.read(&dynSampleRate,4);         // Receive commands using pipe #2
      aaAudio.setSampleRate(dynSampleRate); // Pipe 2 is being used for command data, pipe 1 & others for audio data    
    }else{

      #if !defined (ARDUINO_ARCH_AVR)         //AVR (Uno, Nano can't handle extra processing)
        radio.stopListening();                // Prepare to send data out via radio
      #endif  
      radio.read(&aaAudio.dacBuffer,32);      // Read the available radio data   
      
      aaAudio.feedDAC(0,32);                  // Feed the DAC with the received data      

      #if !defined (ARDUINO_ARCH_AVR)
      aaAudio.getADC(32);                     // Grab the available data from the ADC

      //Send the received ADC data via radio      
      radio.startFastWrite(&aaAudio.adcBuffer,32,1);
      #endif

    /*Note: The data initially recieved can directly be sent via radio, but
               this example is a test of all library peripherals               */

    }
  }else{  //If not RX IRQ, must be TX complete    
    radio.txStandBy();                      // Set the radio to standby mode from TX
    radio.startListening();                 // Put the radio into listening (RX) mode
  }
}

/*********************************************************/

