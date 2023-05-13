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
    
/****************************************************************************/

#if defined (ARDUINO_ARCH_NRF52840)

#include "../AutoAnalogAudio.h"
#include <hal/nrf_pdm.h>
#include <hal/nrf_i2s.h>

#define PDM_IRQ_PRIORITY     7
#define NRF_PDM_FREQ_1280K  (nrf_pdm_freq_t)(0x0A000000UL)               ///< PDM_CLK= 1.280 MHz (32 MHz / 25) => Fs= 20000 Hz
#define DEFAULT_PDM_GAIN     20
#define PIN_MCK    (13)
#define PIN_SCK    (14)
#define PIN_LRCK   (15)
#define PIN_SDOUT  (2)

/****************************************************************************/
/* Public Functions */
/****************************************************************************/

  uint16_t adcBuf0[MAX_BUFFER_SIZE];
  uint16_t adcBuf1[MAX_BUFFER_SIZE];
  uint8_t analogCounter = 0;
  uint32_t buf_size = MAX_BUFFER_SIZE;
  uint16_t mycounter = 0;
  //int16_t sine_table[] = { 0, 0, 23170, 23170, 32767, 32767, 23170, 23170, 0, 0, -23170, -23170, -32768, -32768, -23170, -23170};
  
AutoAnalog::AutoAnalog(){
   
  adcReady = false;   
  adcBitsPerSample = 8;
  dacBitsPerSample = 8;
  autoAdjust = true;
  for(int i=0; i<MAX_BUFFER_SIZE; i++){
      dacBuffer[i] = 0;
  }

  aSize = MAX_BUFFER_SIZE;//&buf_size;
  buf0 = &adcBuf0[0];
  buf1 = &adcBuf1[0];  
  aCtr = 0;
  micOn = 0;
  sampleCounter = 0;
  
}

void AutoAnalog::begin(bool enADC, bool enDAC){
  
  if(enADC){

    set_callback(adcCallback);
    dinPin = PIN_PDM_DIN;
    clkPin = PIN_PDM_CLK;
    pwrPin = PIN_PDM_PWR;
    gain = -1;

    // Enable high frequency oscillator if not already enabled
    if (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) {
      NRF_CLOCK->TASKS_HFCLKSTART    = 1;
      while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) { }
    }
  
  //Set default 16khz sample rate
  NRF_PDM->RATIO = ((PDM_RATIO_RATIO_Ratio80 << PDM_RATIO_RATIO_Pos) & PDM_RATIO_RATIO_Msk);
  nrf_pdm_clock_set(NRF_PDM_FREQ_1280K);
  //nrf_pdm_clock_set(NRF_PDM_FREQ_1067K);
 //   NRF_PDM_FREQ_1000K = PDM_PDMCLKCTRL_FREQ_1000K,  ///< PDM_CLK = 1.000 MHz.
 //   NRF_PDM_FREQ_1032K = PDM_PDMCLKCTRL_FREQ_Default,  ///< PDM_CLK = 1.032 MHz.
 //   NRF_PDM_FREQ_1067K = PDM_PDMCLKCTRL_FREQ_1067K   ///< PDM_CLK = 1.067 MHz.
  
  //Set default channel mono
  nrf_pdm_mode_set(NRF_PDM_MODE_MONO, NRF_PDM_EDGE_LEFTFALLING);
  if(gain == -1) {
    gain = DEFAULT_PDM_GAIN;
  }
  nrf_pdm_gain_set(gain, gain);
  
  pinMode(clkPin, OUTPUT);
  digitalWrite(clkPin, LOW);
  
  pinMode(dinPin, INPUT);
  
  nrf_pdm_psel_connect(digitalPinToPinName(clkPin), digitalPinToPinName(dinPin));
 
  //Enable PDM interrupts and clear events
  nrf_pdm_event_clear(NRF_PDM_EVENT_STARTED);
  nrf_pdm_event_clear(NRF_PDM_EVENT_END);
  nrf_pdm_event_clear(NRF_PDM_EVENT_STOPPED);
  nrf_pdm_int_enable(NRF_PDM_INT_STARTED | NRF_PDM_INT_STOPPED);
  
  //Turn on the mic
  if (pwrPin > -1) {
    pinMode(pwrPin, OUTPUT);
    digitalWrite(pwrPin, HIGH);
    micOn=1;
  }else{
    Serial.println(pwrPin);
  }
  
  // set the PDM IRQ priority and enable
  NVIC_SetPriority(PDM_IRQn, PDM_IRQ_PRIORITY);
  NVIC_ClearPendingIRQ(PDM_IRQn);
  NVIC_EnableIRQ(PDM_IRQn);
  
  // enable and trigger start task
  nrf_pdm_enable();
  nrf_pdm_event_clear(NRF_PDM_EVENT_STARTED);
  nrf_pdm_task_trigger(NRF_PDM_TASK_START);
  

  //Serial.println("ADC START");
  
  
  
  }
  
  
  
  if(enDAC){
      // Enable transmission
  NRF_I2S->CONFIG.TXEN = (I2S_CONFIG_TXEN_TXEN_ENABLE << I2S_CONFIG_TXEN_TXEN_Pos);
  
  // Enable MCK generator
  NRF_I2S->CONFIG.MCKEN = (I2S_CONFIG_MCKEN_MCKEN_ENABLE << I2S_CONFIG_MCKEN_MCKEN_Pos);
  
  // MCKFREQ = 4 MHz
  //NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV11  << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
  NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV63 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
 
  // Ratio = 64 
  NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_64X << I2S_CONFIG_RATIO_RATIO_Pos;
  //NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_256X << I2S_CONFIG_RATIO_RATIO_Pos;
 
  // Master mode, 16Bit, left aligned
  NRF_I2S->CONFIG.MODE = I2S_CONFIG_MODE_MODE_MASTER << I2S_CONFIG_MODE_MODE_Pos;
  NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_8BIT << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
  NRF_I2S->CONFIG.ALIGN = I2S_CONFIG_ALIGN_ALIGN_LEFT << I2S_CONFIG_ALIGN_ALIGN_Pos;
  
  // Format = I2S
  NRF_I2S->CONFIG.FORMAT = I2S_CONFIG_FORMAT_FORMAT_I2S << I2S_CONFIG_FORMAT_FORMAT_Pos;
  
  // Use stereo 
  NRF_I2S->CONFIG.CHANNELS = I2S_CONFIG_CHANNELS_CHANNELS_LEFT << I2S_CONFIG_CHANNELS_CHANNELS_Pos;
  
  // Configure pins
  NRF_I2S->PSEL.MCK = (PIN_MCK << I2S_PSEL_MCK_PIN_Pos);
  NRF_I2S->PSEL.SCK = (PIN_SCK << I2S_PSEL_SCK_PIN_Pos); 
  NRF_I2S->PSEL.LRCK = (PIN_LRCK << I2S_PSEL_LRCK_PIN_Pos); 
  NRF_I2S->PSEL.SDOUT = (PIN_SDOUT << I2S_PSEL_SDOUT_PIN_Pos);
  
  NRF_I2S->ENABLE = 1;
  
  // Configure data pointer
  NRF_I2S->TXD.PTR = (uint32_t)&dacBuf0[0];
  NRF_I2S->RXTXD.MAXCNT = MAX_BUFFER_SIZE / sizeof(uint32_t);
  //NRF_I2S->TXD.PTR = (uint32_t)&sine_table[0];
  //NRF_I2S->RXTXD.MAXCNT = sizeof(sine_table) / sizeof(uint32_t);
  
  // Start transmitting I2S data
  NRF_I2S->TASKS_START = 1;
  }
   
  
  //maybe need to call __WFE(); from the main loop for DAC
}

/****************************************************************************/

void AutoAnalog::setSampleRate(uint32_t sampRate, bool stereo){
    

}

/****************************************************************************/

void AutoAnalog::triggerADC(){
    

  
}

/****************************************************************************/

void AutoAnalog::enableAdcChannel(uint8_t pinAx){
    
  nrf_pdm_enable();

}

/****************************************************************************/

void AutoAnalog::disableAdcChannel(uint8_t pinAx){
    
  nrf_pdm_disable();

}

/****************************************************************************/

void AutoAnalog::getADC(uint32_t samples){
  
  while(!adcReady){__WFE();};
  aSize = samples;  
  adcReady = false;
  
}

/****************************************************************************/
bool dacBufCtr = 0;

void AutoAnalog::feedDAC(uint8_t dacChannel, uint32_t samples, bool startInterrupts){
    
   uint32_t ctr = millis();
   while(!nrf_i2s_event_check(NRF_I2S,NRF_I2S_EVENT_TXPTRUPD));//{ if(millis() - ctr > 5){break;}};
   
   
   //for(uint32_t i=0; i<samples; i++){
   //  dacBuf0[i] = dacBuffer16[i];
   //}
   memcpy(dacBuf0, dacBuffer, samples);
   //NRF_I2S->TXD.PTR = (uint32_t)&dacBuf0[0];
   //memcpy(&dacBuf[0], &dacBuffer16[0], samples);
   NRF_I2S->RXTXD.MAXCNT = samples /4;//sizeof(uint32_t);
   nrf_i2s_event_clear(NRF_I2S,NRF_I2S_EVENT_TXPTRUPD);
}

/****************************************************************************/
/* Private Functions */
/****************************************************************************/

void AutoAnalog::dacBufferStereo(uint8_t dacChannel){



}

/****************************************************************************/

uint32_t AutoAnalog::frequencyToTimerCount(uint32_t frequency){

}

/****************************************************************************/

void AutoAnalog::adcSetup(void){
    
 
}

/****************************************************************************/

void AutoAnalog::adcInterrupts(bool enabled){

}
  
/****************************************************************************/
  
void AutoAnalog::dacSetup(void){
    

}

/****************************************************************************/

void AutoAnalog::disableDAC(bool withinTask){
    NRF_I2S->ENABLE = 0;
} 

/****************************************************************************/

void AutoAnalog::dacHandler(void){


}

/****************************************************************************/

void AutoAnalog::tcSetup (uint32_t sampRate){
  

  
}

/****************************************************************************/

void AutoAnalog::tc2Setup (uint32_t sampRate)
{  

  
}

/****************************************************************************/

extern "C" {
  __attribute__((__used__)) void PDM_IRQHandler_v(void)
  {
if (nrf_pdm_event_check(NRF_PDM_EVENT_STARTED)) {
    nrf_pdm_event_clear(NRF_PDM_EVENT_STARTED);

    // switch to fill
	
    if (AutoAnalog::aCtr) {
        nrf_pdm_buffer_set((uint32_t*)(AutoAnalog::buf0), AutoAnalog::aSize);
        if(AutoAnalog::_onReceive){
            NVIC_DisableIRQ(PDM_IRQn);
            AutoAnalog::_onReceive(AutoAnalog::buf1, AutoAnalog::aSize);
            NVIC_EnableIRQ(PDM_IRQn);
        }
    } else {
        nrf_pdm_buffer_set((uint32_t*)(AutoAnalog::buf1), AutoAnalog::aSize);
        if(AutoAnalog::_onReceive){
            NVIC_DisableIRQ(PDM_IRQn);
            AutoAnalog::_onReceive(AutoAnalog::buf0, AutoAnalog::aSize);
            NVIC_EnableIRQ(PDM_IRQn);
        }
    }

    // Flip to next buffer
    AutoAnalog::aCtr = (AutoAnalog::aCtr + 1) % 2;


  } else if (nrf_pdm_event_check(NRF_PDM_EVENT_STOPPED)) {
    nrf_pdm_event_clear(NRF_PDM_EVENT_STOPPED);
  } else if (nrf_pdm_event_check(NRF_PDM_EVENT_END)) {
    nrf_pdm_event_clear(NRF_PDM_EVENT_END);
  }
  }
}

void AutoAnalog::set_callback(void(*function)(uint16_t *buf, uint32_t buf_len)){
  _onReceive = function;
}

void AutoAnalog::adcCallback(uint16_t *buf, uint32_t buf_len){

  for(int i=0; i < buf_len; i++){
    adcBuffer16[i] = buf[i];
  }
  
  adcReady = true;
}
#endif //#if defined (ARDUINO_ARCH_SAM)
