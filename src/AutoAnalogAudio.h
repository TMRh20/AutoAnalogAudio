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
    */
/**
 * @file AutoAnalogAudio.h
 *
 * Class declaration for AutoAnalogAudio
 */

#ifndef __AUTO_ANALOG_H__
#define __AUTO_ANALOG_H__

#include "AutoAnalog_config.h"

class AutoAnalog
{
    
public:

  /**
   * @name User Interface
   *
   *  Main methods to drive the library
   */
  /**@{*/

  AutoAnalog();

  /** Setup the timers */ 
  void begin(bool enADC, bool enDAC);          
  
  /** Start the ADC running */
  void triggerADC();                           
  
  /** Load the current ADC data into the ADC Data Buffer */
  void getADC();                               
  
  /** Feed the current PCM/WAV data into the DAC for playback */
  void feedDAC();                              

  /** DAC data buffer */
  uint8_t dacBuffer[MAX_BUFFER_SIZE];          
   
  /** ADC Data buffer */
  uint8_t adcBuffer[MAX_BUFFER_SIZE];          

  /** Set sample rate. 0 enables the default rate specified in AutoAnalog_config.h */
  void setSampleRate(uint32_t sampRate = 0);   
  
  /** Function called by DAC IRQ */ 
  void dacHandler(void);                                  
  

  /**@}*/
  
private:

  /**
   * @name Internal interface.
   *
   *  Protected methods. Regular users cannot call these.
   *  They are documented for completeness and for developers who
   *  may want to extend this class.
   */
  /**@{*/
  
  bool whichDma = 0;  
  
  uint8_t aCtr = 0;                    /* Internal counter for ADC data */
  
  uint16_t realBuf[MAX_BUFFER_SIZE];   /* Internal DAC buffer */
  uint16_t adcDma[2][MAX_BUFFER_SIZE]; /* Buffers for ADC DMA transfers */  
  uint16_t dataReady = 0;              /* Internal indicator for DAC data */ 
  
  uint32_t dataTimer = 0;              /* Internal timer tracks timing of incoming data */
  uint32_t sampleCount = 0;            /* Internal counter for delaying analysis of timing */
  uint32_t tcTicks  =    0;            /* Stores the current TC0 Ch0 counter value */
  uint32_t tcTicks2  =    0;           /* Stores the current TC0 Ch1 counter value */
  uint32_t adjustDivider = 5;          /* Internal variables for adjusting timers on-the-fly */
  uint16_t adjustCtr = 0;              /* Internal variables for adjusting timers on-the-fly */

  void adcSetup(void);                 /* Enable the ADC */
  void dacSetup(void);                 /* Enable the DAC */
  
  void pioSetup(void);                 /* Enable the TIOA timer output for DAC and ADC */

  void tcSetup(uint32_t sampRate = 0);      /* Sets up Timer TC0 Channel 0 */  
  void tc2Setup(uint32_t sampRate = 0);     /* Sets up Timer TC0 Channel 1 */

  int frequencyToTimerCount(int Frequency); /* Function to calculate timer counters */

  /**@}*/
};

#endif


/**
 * @example AudioRadioRelay.ino
 * <b>For Arduino Due</b><br>
 *
 * * Audio Relay & Peripheral Test Example:
* This example demonstrates how to manage incoming and outgoing audio streams using 
* the AAAudio library and nrf24l01+ radio modules on Arduino Due.
* 
* 1. This example uses the onboard DAC to play the incoming audio data via DAC0
* 2. The ADC is used to sample the DAC0 pin, and the data is made available
* 3. The data is re-broadcast over the radios
* 4. Incoming radio data can be directly re-broadcast, but this example is a test of all peripherals
*
*/
 
 
 /**
 * @mainpage Automatic Analog Audio Library for Arduino Due (ARM SAM3X)
 *
 * @section LibInfo Auto Analog Audio (Automatic DAC, ADC & Timer) library
 * 
 * **Features:**
 * - Very simple user interface to Arduino DUE DAC and ADC
 * - PCM/WAV Audio/Analog Data playback using Arduino Due DAC
 * - PCM/WAV Audio/Analog Data recording using Arduino Due DAC
 * - Onboard timers drive the DAC & ADC automatically
 * - Automatic sample rate/timer adjustment based on rate of user-driven data requests/input
 * - Uses DMA (Direct Memory Access) to buffer DAC & ADC data
 * 
 * **Library Source Code & Information:**<br>
 * http://github.com/TMRh20 <br>
 * http://tmrh20.blogspot.com <br>
 *
 */
 
