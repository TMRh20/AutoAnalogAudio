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

**************************************************************************
  Dual Pin ADC Sampling (Requires Arduino Due or better)

  Sample the ADC in chunks of 32-bytes at a defined sample rate
  The first sample will be from pin A1, second from A0, 3rd from A1, etc...
  See AnalogAudio_config.h to change the MAX_BUFFER_SIZE allowing larger chunks

**************************************************************************
*/

#include <AutoAnalogAudio.h>
AutoAnalog aaAudio;

/*********************************************************/

void setup() {

  Serial.begin(115200);
  Serial.println("Analog Audio Begin");

  aaAudio.begin(1, 0);              //Setup aaAudio using ADC only
  aaAudio.autoAdjust = 0;           //Disable auto adjust of timers
  aaAudio.adcBitsPerSample = 12;    //Sample at 12-bits
  aaAudio.setSampleRate(32);        //Get 32 samples every second

  //AAAudio samples on analog pin A0 by default
  //Enable sampling on two pins at once
  aaAudio.enableAdcChannel(1);      //Channels correspond to pin numbers (A1 == channel 1)
  //aaAudio.enableAdcChannel(2);    //Optionally enable a third channel on pin A2
  //aaAudio.disableAdcChannel(0);   //Optionally disable pin A0 and only sample 1 pin

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
  float pinA0Samples = 0.0;
  float pinA1Samples = 0.0;
  for (int i = 0; i < 32; i += 2) {
    pinA1Samples += aaAudio.adcBuffer16[i];           //Samples from highest pin number (A1)
    pinA0Samples += aaAudio.adcBuffer16[i + 1];       //Samples from next lowest pin number (A0)
  }

  // Divide the total by the number of samples
  pinA1Samples /= 16.0;
  pinA0Samples /= 16.0;

  // This will print every second at a sample rate of 32 samples/second
  Serial.print("Pin A1 Samples Total / Number of Samples == ");
  Serial.println(pinA1Samples);
  Serial.print("Pin A0 Samples Total / Number of Samples == ");
  Serial.println(pinA0Samples);

}

/*********************************************************/

