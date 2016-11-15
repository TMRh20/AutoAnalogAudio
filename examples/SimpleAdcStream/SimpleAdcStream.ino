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

  Features:
  1. Very simple user interface to Arduino DUE DAC and ADC
  2. PCM/WAV Audio/Analog Data playback using Arduino Due DAC
  3. PCM/WAV Audio/Analog Data recording using Arduino Due ADC
  4. Onboard timers drive the DAC & ADC automatically
  5. Automatic sample rate/timer adjustment based on rate of user-driven data requests/input
  6. Uses DMA (Direct Memory Access) to buffer DAC & ADC data

  Auto Analog Audio Library Information:
  http://github.com/TMRh20
  http://tmrh20.blogspot.com

  Simple ADC Sampling

  Sample the ADC in chunks of 32-bytes at a defined sample rate
  See AnalogAudio_config.h to change the MAX_BUFFER_SIZE allowing larger chunks
*/


#include <AutoAnalogAudio.h>

AutoAnalog aaAudio;

/*********************************************************/

void setup() {

  pinMode(A3,OUTPUT);
  digitalWrite(A3,HIGH);

  Serial.begin(115200);
  Serial.println("Analog Audio Begin");

  aaAudio.begin(1,0);               //Setup aaAudio using ADC only
  aaAudio.autoAdjust = 0;           //Disable auto adjust of timers
  aaAudio.adcBitsPerSample = 12;    //Sample at 12-bits
  aaAudio.setSampleRate(32);        //Get 32 samples every second

  //Start loading ADC buffers
  aaAudio.getADC(32);  
}

/*********************************************************/

void loop() {

  // Get 32 samples from the ADC at the sample rate defined above
  // Note: This function only blocks if the ADC is currently sampling and autoAdjust is set to 0
  // As long as any additional code completes before the ADC is finished sampling, a continuous stream of ADC data
  // at the defined sample rate will be available 
  aaAudio.getADC(32);

  // Sum all the samples into a float
  float allSamples = 0.0;
  for(int i=0; i<32; i++){
    allSamples += aaAudio.adcBuffer16[i];    
  }

  // Divide the total by the number of samples
  allSamples /= 32.0;  

  // This will print every second at a sample rate of 32 samples/second
  Serial.print("Samples Total Value / Number of Samples == ");
  Serial.println(allSamples);  

}

/*********************************************************/

