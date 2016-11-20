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

#define AAA_CHANNEL0 0
#define AAA_CHANNEL1 1
#define AAA_MODE_STEREO 2

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

  /** Setup the timer(s) */ 
  void begin(bool enADC, bool enDAC);          
  
  /**
   * @note This function is no longer required and does nothing
   */
  void triggerADC();                           
  
  /** Load the current ADC data into the ADC Data Buffer
    *
    * @param samples The number of samples to retrieve from the ADC
    *
    * @note Changes to the number of samples will not take place until one
    * buffer has been returned with the previous number of samples
    *
    */
  void getADC(uint32_t samples = MAX_BUFFER_SIZE);                               
  
  /** Feed the current PCM/WAV data into the DAC for playback 
   * 
   * @param dacChannel 0 for DAC0, 1 for DAC1, 2 for alternating DAC0/DAC1
   * @param samples The number of samples to send to the DAC
   */
  void feedDAC(uint8_t dacChannel = 0, uint32_t samples = MAX_BUFFER_SIZE);                              

  /** DAC data buffer for 8-bit samples 
   *
   *  8-bit user samples are loaded directly into this buffer before calling feedDAC() <br>
   *  @see dacBitsPerSample
   */
  uint8_t dacBuffer[MAX_BUFFER_SIZE];          
   
  /** ADC Data buffer for 8-bit samples 
   *
   *  8-bit samples are read directly from this buffer after calling getADC() <br>
   * @see adcBitsPerSample
   */
  uint8_t adcBuffer[MAX_BUFFER_SIZE];          

  /** DAC data buffer for 10 or 12-bit samples 
   *
   *  10 or 12-bit user samples are loaded directly into this buffer before calling feedDAC() <br>
   * @see dacBitsPerSample
   */
  uint16_t dacBuffer16[MAX_BUFFER_SIZE];          
   
  /** ADC Data buffer for 10 or 12-bit samples 
   *
   *  10 or 12-bit samples are read directly from this buffer after calling getADC() <br>
   * @see adcBitsPerSample
   */
  uint16_t adcBuffer16[MAX_BUFFER_SIZE];    

  /** Set sample rate. 0 enables the default rate specified in AutoAnalog_config.h */
  void setSampleRate(uint32_t sampRate = 0);   
  
  /** Function called by DAC IRQ */ 
  void dacHandler(void);                                  
   
  /** Auto & Manual sample rates. En/Disables automatic adjustment of timers
   *
   *  Default: true
   **/
  bool autoAdjust;
  
  
  /** ADC (Analog to Digital Converter) <br>
   * Select the bits-per-sample for incoming data <br>
   * Default: 8:8-bit, 10:10-bit, 12:12-bit <br>
   * <br>
   * There are two separate data buffers for the ADC,selected by setting this variable. <br>
   * adcBuffer[] is an 8-bit buffer used solely for your 8-bit samples <br>
   * adcBuffer16[] is a 16-bit buffer used solely for placing your 10 and 12-bit samples <br>
   **/
  uint8_t adcBitsPerSample;
  
  /** DAC (Digital to Analog Converter) <br>
   * Select the bits-per-sample for incoming data <br>
   * Default: 8:8-bit, 10:10-bit, 12:12-bit <br>
   * <br>
   * There are two separate data buffers for the DAC, selected by setting this variable. <br>
   * dacBuffer[] is an 8-bit buffer used solely for your 8-bit samples <br>
   * dacBuffer16[] is a 16-bit buffer used solely for placing your 10 and 12-bit samples <br>
   *
   **/
  uint8_t dacBitsPerSample;
  
  /**
   * Enable reads from the specified channel (pins A0-A6)
   *
   * Active ADC Channels will be read in numeric order, high to low
   *
   * @note Specify pins numerically: 0=A0, 1=A1, etc...
   */
  void enableAdcChannel(uint8_t pinAx);

  /**
   * Disable reads from the specified channel (pins A0-A6)
   * @note Specify pins numerically: 0=A0, 1=A1, etc...
   */
  void disableAdcChannel(uint8_t pinAx);
  
  /**
   * Disable the DAC
   */
  void disableDAC();
  
  /**
   * En/Disable the interrupt for the ADC
   * 
   * If enabled, the following function needs to be added:
   * @code
   * void ADC_Handler(void){
   *  -code here-    
   * }
   * @endcode
   */
  void adcInterrupts(bool enabled = true);
  
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
#if defined (ARDUINO_ARCH_SAM)  
  bool whichDma = 0;
  bool whichDac;
  bool dacChan;
  
  uint8_t aCtr = 0;                    /* Internal counter for ADC data */
  uint16_t realBuf[MAX_BUFFER_SIZE];   /* Internal DAC buffer */
  uint16_t adcDma[MAX_BUFFER_SIZE]; /* Buffers for ADC DMA transfers */  
  uint16_t dataReady;                  /* Internal indicator for DAC data */ 
  
  uint32_t dataTimer;                  /* Internal timer tracks timing of incoming data */
  uint32_t sampleCount;                /* Internal counter for delaying analysis of timing */
  uint32_t tcTicks;                    /* Stores the current TC0 Ch0 counter value */
  uint32_t tcTicks2;                   /* Stores the current TC0 Ch1 counter value */
  uint32_t adjustDivider;              /* Internal variables for adjusting timers on-the-fly */

  uint32_t dacNumSamples;              /* Internal variable for number of samples sent to the DAC */
  uint32_t adcNumSamples;
  uint16_t adjustCtr;                  /* Internal variables for adjusting timers on-the-fly */
  uint16_t adjustCtr2;                 /* Internal variables for adjusting timers on-the-fly */
  uint32_t adcLastAdjust;
#endif
  void adcSetup(void);                 /* Enable the ADC */
  void dacSetup(void);                 /* Enable the DAC */
  
  void dacBufferStereo(uint8_t dacChannel);
  
  void tcSetup(uint32_t sampRate = 0);      /* Sets up Timer TC0 Channel 0 */  
  void tc2Setup(uint32_t sampRate = 0);     /* Sets up Timer TC0 Channel 1 */

  uint32_t frequencyToTimerCount(uint32_t Frequency); /* Function to calculate timer counters */

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
 * @example myRadio.h
 * <b>For Arduino Due</b><br>
 *
 * * Include file for nrf24l01+ radios:
 * 
 * Contains the settings and config used for the radio examples
 */

/**
 * @example SimpleSine.ino
 * <b>For Arduino Due</b><br>
 *
 * * Simple Sine Wave Generation Example:
 *
 *  This example demonstrates simple generation of a sine wave & optionally broadcasting
 *  the audio via radio
 *
 *  Send a number 1 or 2 over Serial to change frequency, +/- to adjust volume
 */

/**
 * @example SimpleSine12Bit.ino
 * <b>For Arduino Due</b><br>
 *
 * * Simple Sine Wave Generation Example:
 *
 *  This example demonstrates simple generation of a 12-bit sine wave
 *
 *  Send a number 1 or 2 over Serial to change frequency, +/- to adjust volume
 */ 
 
 /**
 * @example WirelessSpeaker.ino
 * <b>For Arduino Due</b><br>
 *
 * * Simple Wireless Speaker:
 *
 *  Demonstration of a single wireless speaker/wireless audio
 *
 *  The incoming audio format is 16bit mono <br>
 *  NRF24L01+ radios can support around 16-44khz sample rate w/16-bit samples, 88khz+ with 8-bit samples
 */ 
 
  /**
 * @example WirelessMicrophone.ino
 * <b>For Arduino Due</b><br>
 *
 * * Simple Wireless Microphone:
 *
 *  Demonstration of a single wireless microphone/recording via ADC
 *
 *  The outgoing audio format is 8bit, mono, 16khz <br>
 *  NRF24L01+ radios can support around 16-44khz sample rate w/12-bit samples, 88khz+ with 8-bit samples
 */ 
 
 /**
 * @example SimpleAdcStream.ino
 * <b>For Arduino Due</b><br>
 *
 * * Simple ADC capturing Example:
 *
 *  This example demonstrates how to capture a steady stream of ADC data
 *
 *  See AnalogAudio_config.h to change the MAX_BUFFER_SIZE allowing larger chunks
 */ 
 
/**
 * @example MultiChannelAdcStream.ino
 * <b>For Arduino Due</b><br>
 *
 * * Multi Channel ADC Sampling Example:
 *
 *  This example demonstrates how to capture a steady stream of ADC data on
 *  multiple channels. Currently pins A0-A6 are supported.
 *
 *  See AnalogAudio_config.h to change the MAX_BUFFER_SIZE allowing larger chunks of data
 */
 
/**
 * @example SDAudioBasic.ino
 * <b>For Arduino Due</b><br>
 *
 * * Basic SDAudio Example:
 *
 *  This example demonstrates how to play *.wav files from SD Card.
 *
 */
 
/**
 * @example SDAudioAuto.ino
 * <b>For Arduino Due</b><br>
 *
 * * Auto SDAudio Example:
 *
 *  This example demonstrates how to play *.wav files from SD Card using interrupts.
 *
 */

/**
 * @example SDAudioWavPlayer.ino
 * <b>For Arduino Due</b><br>
 *
 * * Wav Player SDAudio Example:
 *
 *  This example demonstrates a simple *.wav player with a few features
 *
 */
 
/**
 * @example SDAudioRecording.ino
 * <b>For Arduino Due</b><br>
 *
 * * Wav Recording SDAudio Example:
 *
 *  This example demonstrates recording standard format *.wav files
 *  for playback on any PC or audio device.
 */
 
 /**
 * @mainpage Automatic Analog Audio Library for Arduino
 *
 * @section LibInfo Auto Analog Audio (Automatic DAC, ADC & Timer) library
 * 
 * **Goals:**
 *
 * **Extremely low-latency digital audio recording, playback, communication and relaying at high speeds**
 *
 *
 * **Features:**
 * - Now supports AVR devices (Arduino Uno,Nano,Mega,etc)
 * - Designed with low-latency radio/wireless communication in mind
 * - Very simple user interface to Arduino DUE DAC and ADC
 * - PCM/WAV Audio/Analog Data playback using Arduino Due DAC
 * - PCM/WAV Audio/Analog Data recording using Arduino Due ADC
 * - Onboard timers drive the DAC & ADC automatically
 * - Automatic sample rate/timer adjustment based on rate of user-driven data requests/input
 * - Uses DMA (Direct Memory Access) to buffer DAC & ADC data
 * - ADC & DAC: 8, 10 or 12-bit sampling
 * - Single channel or stereo output
 * - Multi-channel ADC sampling
 * 
 *
 *
 * The library internally configures timing based on user driven data requests or delivery, making data available or processing 
 * it at the appropriate speed without delays or while() loops.
 *
 * The library can also be configured to operate at a set sample rate, with the getADC() and feedDAC() functions blocking until data
 * is available or ready to be processed.
 *
 * **Library Source Code & Information:**<br>
 * http://github.com/TMRh20 <br>
 * http://tmrh20.blogspot.com <br>
 *
 */
 