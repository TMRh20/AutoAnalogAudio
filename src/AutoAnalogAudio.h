    /*
    AutoAnalogAudio streaming via DAC & ADC by TMRh20
    Copyright (C) 2016-2024  TMRh20 - tmrh20@gmail.com, github.com/TMRh20

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
#if defined (ESP32)
  #include "driver/i2s.h"
#endif


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

  /** Setup the timer(s)
   *
   *  Make sure to define your chosen pins before calling `begin();`  
   *  
   *  NRF52 Only: Call the below code prior to begin() to limit memory usage/buffer sizes:  
   *  @code
   *  aaAudio.maxBufferSize = YOUR_AUDIO_BUFFER_SIZE;
   *  @endcode
   *
   * Third option:
   * @param enADC 0 = Disabled, 1 = PDM input, 2 = I2S input, 3 = SAADC input
   * @param enDAC 0 = Disabled, 1 = PWM output, 2 = I2S output
   * @param _useI2S This is deprecated, use enADC and enDAC to control inputs/outputs
  */
  void begin(uint8_t enADC, uint8_t enDAC, uint8_t _useI2S = 0);

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
   * @param startInterrupts Only used for the ESP32, this will enable an RTOS task to handle DAC
   */
  void feedDAC(uint8_t dacChannel = 0, uint32_t samples = MAX_BUFFER_SIZE, bool startInterrupts = false);

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
  #if !defined (ARDUINO_ARCH_NRF52840) && !defined (ARDUINO_ARCH_NRF52) && !defined ARDUINO_NRF52840_FEATHER
  uint16_t adcBuffer16[MAX_BUFFER_SIZE];
  #elif defined __MBED__
  inline static uint16_t adcBuffer16[MAX_BUFFER_SIZE];  
  #else
  static uint16_t adcBuffer16[MAX_BUFFER_SIZE];
  #endif
  
  /** Set sample rate. 0 enables the default rate specified in AutoAnalog_config.h
   * @param sampRate This sets the defined sample rate ie: 32000 is 32Khz
   * @param stereo Only used for the ESP32 & NRF52840 this sets stereo or mono output and affects the sample rate
   */
  void setSampleRate(uint32_t sampRate = 0, bool stereo = false);

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
   * @param withinTask Only used for ESP32, set to true if calling from within a task itself, see included example
   */
  void disableDAC(bool withinTask = false);

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

  void dacInterrupts(bool enabled = true, bool withinTask = false);

  #if defined (ESP32) || defined (DOXYGEN_FORCED)
  /** Rampout and RampIn functions ramp the signal in/out to minimize 'pop' sound made when en/disabling the DAC
	 * @param sample For ESP32 only, provide the first or last sample to ramp the signal in/out
	 */
  void rampOut(uint8_t sample);
  void rampIn(uint8_t sample);
  TaskHandle_t dacTaskHandle;
  #endif


#if defined (ARDUINO_ARCH_NRF52840) || defined (ARDUINO_ARCH_NRF52) && !defined ARDUINO_NRF52840_FEATHER && defined __MBED__
  inline static uint8_t aCtr;
  inline static uint32_t aSize;
  inline static uint16_t *adcBuf0 = NULL;
  inline static uint16_t *adcBuf1 = NULL;
  inline static void (*_onReceive)(uint16_t *buf, uint32_t buf_len) = NULL;
  inline static void adcCallback(uint16_t *buf, uint32_t buf_len);
  inline static void set_callback(void(*function)(uint16_t *buf, uint32_t buf_len));
  inline static bool adcReady;
  inline static uint16_t *dacBuf0;
  inline static uint16_t *dacBuf1;
  inline static uint32_t sampleCounter;
  uint8_t enableADC;
  uint8_t enableDAC;
  bool whichBuf;
  bool adcWhichBuf;
  
#elif defined (ARDUINO_ARCH_NRF52840) || defined (ARDUINO_ARCH_NRF52) || defined (ARDUINO_NRF52840_FEATHER) && !defined __MBED__
  uint16_t *dacBuf0;
  uint16_t *dacBuf1;
  static uint8_t aCtr;
  static uint32_t aSize;
  static uint16_t *adcBuf0;
  static uint16_t *adcBuf1;
  static void (*_onReceive)(uint16_t *buf, uint32_t buf_len);
  static void adcCallback(uint16_t *buf, uint32_t buf_len);
  void set_callback(void(*function)(uint16_t *buf, uint32_t buf_len));
  static bool adcReady;
  uint32_t sampleCounter;
  uint8_t enableADC;
  uint8_t enableDAC;
  bool whichBuf;
  bool adcWhichBuf;
#endif

#if defined (ARDUINO_ARCH_NRF52840) || defined (ARDUINO_ARCH_NRF52) || defined (DOXYGEN_FORCED)
    /**@}*/
    /**
     * @name Section for nRF52 Devices Only
     *
     * This section is for nRF52 devices only  
     *  
     * 
     */
    /**@{*/
    
  /**
   * Set the Power Pin for PDM  
   * By default this is PIN_PDM_PWR  
   * If PIN_PDM_PWR is not defined, it is set to -1 by default  
   * Configure this before calling `begin()`  
   */
  int pwrPin;
  
  /**
  * Set the Input Pin for PDM or SAADC. With SAADC AIN0 is 1, AIN1 is 2 etc.
  * By default this is PIN_PDM_DIN  
  * If PIN_PDM_DIN is not defined, it is set to 35 by default  
   * Configure this before calling `begin()`  
  */
  int dinPin;
  
  /**
  * Set the Clock Pin for PDM  
  * By default this is PIN_PDM_CLK  
  * If PIN_PDM_CLK is not defined, it is set to 36 by default  
   * Configure this before calling `begin()`  
  */
  int clkPin;
  
  /**
  * Set the Gain for PDM  
  * By default this is set to 40  
   * Configure this before calling `begin()`  
  */
  int8_t gain;
  
   /**
  * Configure the pins and ports for nRF52 using GPIO numbers before calling `begin()`  
  * Defaults:  
  * I2S_PIN_MCK = 2;  
  * I2S_PORT_MCK = 0;  
  * I2S_PIN_SCK = 3;  
  * I2S_PORT_SCK = 0;  
  * I2S_PIN_LRCK = 29;  
  * I2S_PORT_LRCK = 0;  
  * I2S_PIN_SDOUT = 5;  
  * I2S_PORT_SDOUT = 0;  
  * I2S_PIN_SDIN = 4;  
  * I2S_PORT_SDIN = 0;  
  */
  uint16_t I2S_PIN_MCK;
  uint8_t  I2S_PORT_MCK;
  uint16_t I2S_PIN_SCK;
  uint8_t  I2S_PORT_SCK;
  uint16_t I2S_PIN_LRCK;
  uint8_t  I2S_PORT_LRCK;
  uint16_t I2S_PIN_SDOUT;
  uint8_t  I2S_PORT_SDOUT;  
  uint16_t I2S_PIN_SDIN;
  uint8_t  I2S_PORT_SDIN;
  
  /**
  * Set the maximum buffer size for nRF52  
  * The internal buffers are all allocated dynamically.  
  * By default the MAX_BUFFER_SIZE is defined in AutoAnalogAudio_config.h  
  * Override that value for internal buffers by setting this before calling `begin()`  
  */
  uint32_t maxBufferSize;
#endif

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

#if defined ESP32

  uint32_t sampleRate;
  i2s_config_t i2s_cfg;

  bool i2sStopped;
  adc1_channel_t adcChannel;

  bool taskCreated;

  bool dacEnabled;
  uint8_t lastDacSample;
  i2s_event_t myI2SQueue[5];
  //void dacTask(void *args);

  bool adcTaskCreated;
  bool adcDisabled;

  #define DELAY_250MS (250 / portTICK_PERIOD_MS)

#endif
  /**@}*/
};



#endif


/**
 * @example AudioRadioRelay.ino
 * <b>For Arduino Due</b><br>
 *
 * Audio Relay & Peripheral Test Example:
 *
 * This example demonstrates how to manage incoming and outgoing audio streams using
 * the AAAudio library and nrf24l01+ radio modules on Arduino Due.
 *
 * 1. This example uses the onboard DAC to play the incoming audio data via DAC0
 * 2. The ADC is used to sample the DAC0 pin, and the data is made available
 * 3. The data is re-broadcast over the radios
 * 4. Incoming radio data can be directly re-broadcast, but this example is a test of all peripherals
 *
 * @note This code depends on [radio.h](AudioRadioRelay_2myRadio_8h_source.html)
 * located in the same directory.
 */

/**
 * @example SimpleSine.ino
 * <b>For Arduino Due</b><br>
 *
 * Simple Sine Wave Generation Example:
 *
 * This example demonstrates simple generation of a sine wave & optionally broadcasting
 * the audio via radio
 *
 * Send a number 1 or 2 over Serial to change frequency, +/- to adjust volume
 *
 * @note This code depends on [radio.h](SimpleSine_2myRadio_8h_source.html)
 * located in the same directory.
 */

/**
 * @example SimpleSine12Bit.ino
 * <b>For Arduino Due</b><br>
 *
 * Simple Sine Wave Generation Example:
 *
 * This example demonstrates simple generation of a 12-bit sine wave
 *
 * Send a number 1 or 2 over Serial to change frequency, +/- to adjust volume
 */

/**
 * @example WirelessSpeaker.ino
 * <b>For Arduino Due</b><br>
 *
 * Simple Wireless Speaker:
 *
 * Demonstration of a single wireless speaker/wireless audio
 *
 * The incoming audio format is 16bit mono <br>
 * NRF24L01+ radios can support around 16-44khz sample rate w/16-bit samples, 88khz+ with 8-bit samples
 *
 * @note This code depends on [radio.h](WirelessSpeaker_2myRadio_8h_source.html)
 * located in the same directory.
 */

/**
 * @example WirelessMicrophone.ino
 * <b>For Arduino Due</b><br>
 *
 * Simple Wireless Microphone:
 *
 * Demonstration of a single wireless microphone/recording via ADC
 *
 * The outgoing audio format is 8bit, mono, 16khz <br>
 * NRF24L01+ radios can support around 16-44khz sample rate w/12-bit samples, 88khz+ with 8-bit samples
 *
 * @note This code depends on [radio.h](WirelessMicrophone_2myRadio_8h_source.html)
 * located in the same directory.
 */

/**
 * @example SimpleAdcStream.ino
 * <b>For Arduino Due</b><br>
 *
 * Simple ADC capturing Example:
 *
 * This example demonstrates how to capture a steady stream of ADC data
 *
 * See AnalogAudio_config.h to change the MAX_BUFFER_SIZE allowing larger chunks
 */

/**
 * @example MultiChannelAdcStream.ino
 * <b>For Arduino Due</b><br>
 *
 * Multi Channel ADC Sampling Example:
 *
 * This example demonstrates how to capture a steady stream of ADC data on
 * multiple channels. Currently pins A0-A6 are supported.
 *
 * See AnalogAudio_config.h to change the MAX_BUFFER_SIZE allowing larger chunks of data
 */

/**
 * @example SdAudioBasic.ino
 * <b>For Arduino Due</b><br>
 *
 * Basic SDAudio Example:
 *
 * This example demonstrates how to play *.wav files from SD Card.
 */

/**
 * @example SdAudioAuto.ino
 * <b>For Arduino Due</b><br>
 *
 * Auto SDAudio Example:
 *
 * This example demonstrates how to play *.wav files from SD Card using interrupts.
 */

/**
 * @example SdAudioWavPlayer.ino
 * <b>For Arduino Due</b><br>
 *
 * Wav Player SDAudio Example:
 *
 * This example demonstrates a simple *.wav player with a few features
 */

/**
 * @example SdAudioRecording.ino
 * <b>For Arduino Due</b><br>
 *
 * Wav Recording SDAudio Example:
 *
 * This example demonstrates recording standard format *.wav files
 * for playback on any PC or audio device.
 */

/**
 * @example NRF52_PDM_PWMTest.ino
 * <b>For Arduino XIAO 52840 Sense</b><br>
 *
 * Recording (pin A0) and playback (pin 5)
 *
 * This example demonstrates recording and playback on a single device
 * using PDM microphone and PWM output
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
 */
