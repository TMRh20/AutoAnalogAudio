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

#if defined (ARDUINO_ARCH_SAM)

#include "../AutoAnalogAudio.h"

//#include "AutoDAC.h"
//#include "AudoADC.h"
//#include "AutoTimer.h"

/****************************************************************************/
/* Public Functions */
/****************************************************************************/

AutoAnalog::AutoAnalog(){
    
  dataReady = 0;  
  dataTimer = 0;
  sampleCount = 0;
  tcTicks  =    2625;
  tcTicks2  =    2625;
  adjustDivider = 2;
  adjustCtr = 0;
  adjustCtr2 = 0;
  whichDma = 0;  
  aCtr = 0;
  dacChan = 0;
  whichDac = 0;
  adcBitsPerSample = 8;
  dacBitsPerSample = 8;
  autoAdjust = true;
  adcLastAdjust = 0;
  for(int i=0; i<MAX_BUFFER_SIZE; i++){
      dacBuffer[i] = 0;
  }
  adcNumSamples = 0;
}

void AutoAnalog::begin(bool enADC, bool enDAC){
  
  if(enADC){
    adcSetup();
  }
  if(enDAC){
    dacSetup();
  }  
  tc2Setup(DEFAULT_FREQUENCY);
  tcSetup(DEFAULT_FREQUENCY);
  tcSetup();
}

/****************************************************************************/

void AutoAnalog::setSampleRate(uint32_t sampRate, bool stereo){
    
   tc2Setup(sampRate);
   tcSetup(sampRate);
   tcSetup();
}

/****************************************************************************/

void AutoAnalog::triggerADC(){
    

  
}

/****************************************************************************/

void AutoAnalog::enableAdcChannel(uint8_t pinAx){
    
    if(pinAx > 6){ return; }
    pinAx = 7 - pinAx;    
    ADC->ADC_CHER |= 1<< pinAx;

}

/****************************************************************************/

void AutoAnalog::disableAdcChannel(uint8_t pinAx){
    
    if(pinAx > 6){ return; }
    pinAx = 7 - pinAx;    
    ADC->ADC_CHDR |= 1<< pinAx;

}

/****************************************************************************/

void AutoAnalog::getADC(uint32_t samples){
  
  
  if(sampleCount < 100){ ++sampleCount; }
    if(sampleCount >= 100 && autoAdjust){
      
      TcChannel * t = &TC0->TC_CHANNEL[0];
      TcChannel * tt = &TC0->TC_CHANNEL[1];
      
      ++adjustCtr2;
      if(adjustCtr2 > 2){ adjustCtr2 = 0;}

      if( tt->TC_RC >= t->TC_RC - 25 && adjustCtr2 == 0){
         tcTicks2--;
         tc2Setup();
      }else
      if( (ADC->ADC_RNCR > 0 || ADC->ADC_RCR > 0 ) ){
        tcTicks2-=10;
        tc2Setup();
      }else
      if( (ADC->ADC_RNCR == 0 && ADC->ADC_RCR == 0 ) && millis() - adcLastAdjust > 500 && tt->TC_RC < t->TC_RC - 35){
        adcLastAdjust = millis();
        tcTicks2++;
        tc2Setup();
      }
    }
        
    if(!autoAdjust){
      while(ADC->ADC_RNCR > 0 || ADC->ADC_RCR > 0 ){;}
    }  
    
    for(int i=0; i<adcNumSamples; i++){
      if(adcBitsPerSample == 8){
        adcBuffer[i] = adcDma[i]>>4;
      }else
      if(adcBitsPerSample == 10){
        adcBuffer16[i] = adcDma[i]>>2;  
      }else{
        adcBuffer16[i] = adcDma[i];    
      }
    }    

    whichDma = !whichDma;
    ADC->ADC_RNPR=(uint32_t) adcDma;
    ADC->ADC_RNCR=samples;   
    
    adcNumSamples = samples;
}

/****************************************************************************/

void AutoAnalog::feedDAC(uint8_t dacChannel, uint32_t samples, bool startInterrupts){
    
    // Adjusts the timer by comparing the rate of incoming data
    // of data vs rate of the DACC   
  if(sampleCount < 100){ ++sampleCount; }
  if(sampleCount >= 100 && autoAdjust){
      
    TcChannel * t = &TC0->TC_CHANNEL[0];
    int tcr = DACC->DACC_TCR;
    int tncr = DACC->DACC_TNCR;

    ++adjustCtr;
    if(adjustCtr > 200){ adjustCtr = 0;}

    if( (DACC->DACC_TCR > 0 || DACC->DACC_TNCR > 0) && adjustCtr < 25){
          tcTicks-=5;
          tcSetup();
    }
  }
  
  if(!autoAdjust){
    while((dacc_get_interrupt_status(DACC) & DACC_ISR_ENDTX) != DACC_ISR_ENDTX ){;}
  }
  
  for(int i=0; i<samples; i++){
    if(dacBitsPerSample == 12){
      realBuf[i] = dacBuffer16[i];  
    }else
    if(dacBitsPerSample == 10){
      realBuf[i] = dacBuffer16[i] << 2;   
    }else{
      realBuf[i] = dacBuffer[i] << 4;  
    }
    if(dacChannel == 2){
      dacChan = !dacChan;      
    }else{
      dacChan = dacChannel;
    }

    realBuf[i] = (realBuf[i]& ~0x3000) | dacChan << 12;
  }
  
  dacNumSamples = samples;
  dataReady = 0;
  dacc_enable_interrupt(DACC, DACC_IER_ENDTX);
}

/****************************************************************************/
/* Private Functions */
/****************************************************************************/

void AutoAnalog::dacBufferStereo(uint8_t dacChannel){



}

/****************************************************************************/

uint32_t AutoAnalog::frequencyToTimerCount(uint32_t frequency){
  return VARIANT_MCK / 2UL / frequency;
}

/****************************************************************************/

void AutoAnalog::adcSetup(void){
    
    pmc_enable_periph_clk(ID_ADC);   //power management controller told to turn on adc
    
    ADC->ADC_IDR = 0xFFFFFFFF ;   // disable interrupts    
    ADC->ADC_IDR = 0xFFFFFFFF;
    ADC->ADC_CHDR = 0xFFFF ;      // disable all channels
    ADC->ADC_CHER = 0x80 ;        // enable just A0
    ADC->ADC_CGR = 0x15555555 ;   // All gains set to x1
    ADC->ADC_COR = 0x00000000 ;   // All offsets off
      
    ADC->ADC_MR = (ADC->ADC_MR & 0x0F00FF00) | 1 << 2 | ADC_MR_TRGEN;//& ~ADC_MR_SLEEP & ~ADC_MR_FWUP // 1 = trig source TIO from TC0
    //ADC->ADC_MR = (ADC->ADC_MR & 0xFF00FF00);
    //ADC->ADC_MR |= ADC_MR_LOWRES;
    //MR Prescalar = 255     ADCClock == 84mhz / ( (256) * 2) == ?? MIN is 1Mhz
    //ADC->ADC_MR |= 5 << 8; //Prescalar ? sets ADC Clock to 1,615,384.6 hz, 5 is 7mhz
    //ADC->ADC_MR |= 3 << 20; //Settling time, full is 17ADC Clocks, 411,764.7hz, 9 is 179487.2, 5
    //ADC->ADC_MR |= 1; //51470.6hz
    
    ADC->ADC_PTCR=ADC_PTCR_RXTEN;    
}

/****************************************************************************/

void AutoAnalog::adcInterrupts(bool enabled){
  NVIC_DisableIRQ(ADC_IRQn);
  if(enabled){    
    NVIC_ClearPendingIRQ(ADC_IRQn);
    NVIC_EnableIRQ(ADC_IRQn);
    adc_enable_interrupt(ADC,ADC_IER_ENDRX);
  }
}
  
/****************************************************************************/
  
void AutoAnalog::dacSetup(void){
    
  pmc_enable_periph_clk (DACC_INTERFACE_ID) ; // start clocking DAC
  dacc_reset(DACC);
  dacc_set_transfer_mode(DACC, 0);
  dacc_set_power_save(DACC, 0, 0);
  dacc_set_analog_control(DACC, DACC_ACR_IBCTLCH0(0x02) | DACC_ACR_IBCTLCH1(0x02) | DACC_ACR_IBCTLDACCORE(0x01));
  dacc_set_trigger(DACC, 1);
  
  //dacc_set_channel_selection(DACC, 0);
  dacc_enable_channel(DACC, 0);
  dacc_enable_channel(DACC, 1);

  DACC->DACC_MR |= 1<<8; // Refresh period of 24.3uS  (1024*REFRESH/DACC Clock)
  DACC->DACC_MR |= 1<<21;// Max speed bit
  DACC->DACC_MR |= 1<<20;// TAG mode
  //DACC->DACC_MR |= 2 << 24;
  
  
  //Serial.println(DACC->DACC_CDR,BIN);
  
  NVIC_DisableIRQ(DACC_IRQn);
  NVIC_ClearPendingIRQ(DACC_IRQn);
  NVIC_EnableIRQ(DACC_IRQn);
  dacc_enable_interrupt(DACC, DACC_IER_ENDTX);

  
  //dacc_enable_interrupt(DACC, DACC_IER_TXRDY);
  //dacc_enable_interrupt(DACC, DACC_IER_EOC);

  //NVIC_SetPriority(TC0_IRQn,5);
  //NVIC_SetPriority(DACC_IRQn,5);
  //NVIC_SetPriority(ADC_IRQn,5);
}

/****************************************************************************/

void AutoAnalog::disableDAC(bool withinTask){dacc_disable_interrupt(DACC, DACC_IER_ENDTX);} 

/****************************************************************************/

void AutoAnalog::dacHandler(void){


    uint32_t status = dacc_get_interrupt_status(DACC);
    
    if((status & DACC_ISR_ENDTX) == DACC_ISR_ENDTX) {

    if(dataReady < 1){
    
      if(whichDac){
       DACC->DACC_TPR = (uint32_t)realBuf;      
       DACC->DACC_TCR = dacNumSamples;
       DACC->DACC_PTCR = DACC_PTCR_TXTEN;
      }else{
       DACC->DACC_TNPR = (uint32_t)realBuf;      
       DACC->DACC_TNCR = dacNumSamples;
       DACC->DACC_PTCR = DACC_PTCR_TXTEN; 
      }
      
      whichDac = !whichDac;
      dataReady = 1;

    }else{
      dacc_disable_interrupt(DACC, DACC_IER_ENDTX);
      if(autoAdjust && adjustCtr == 0){
        TcChannel * t = &TC0->TC_CHANNEL[0];
        tcTicks+=1;
        tcSetup();
      }        
    }
   }
}

/****************************************************************************/

void AutoAnalog::tcSetup (uint32_t sampRate){
  
  if(sampRate > 0){
    tcTicks = frequencyToTimerCount(sampRate);
    sampleCount = 0;
  }
  
  pmc_enable_periph_clk(TC_INTERFACE_ID);

  Tc * tc = TC0;
  TcChannel * t = &tc->TC_CHANNEL[0];  
  t->TC_CCR = TC_CCR_CLKDIS;        
  t->TC_IDR = 0xFFFFFFFF;                           
  t->TC_SR;  
  t->TC_RC = tcTicks;    
  t->TC_RA = tcTicks /2;
  t->TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK1 |          
              TC_CMR_WAVE |                         
              TC_CMR_WAVSEL_UP_RC;
  t->TC_CMR = (t->TC_CMR & 0xFFF0FFFF) | TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_SET;
  t->TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
  
}

/****************************************************************************/

void AutoAnalog::tc2Setup (uint32_t sampRate)
{  

  if(sampRate > 0){
    tcTicks2 = frequencyToTimerCount(sampRate);
    sampleCount = 0;
  }

  pmc_enable_periph_clk(TC_INTERFACE_ID+1);
  
  Tc * tc = TC0;
  TcChannel * tt = &tc->TC_CHANNEL[1];
  tt->TC_CCR = TC_CCR_CLKDIS;
  tt->TC_IDR = 0xFFFFFFFF;
  tt->TC_SR;  
  tt->TC_RC = tcTicks2;
  tt->TC_RA = tcTicks2 / 2;
  tt->TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK1 | TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC;    
  tt->TC_CMR = (tt->TC_CMR & 0xFFF0FFFF) | TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_SET; 
  tt->TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG; 
  
}

/****************************************************************************/



#endif //#if defined (ARDUINO_ARCH_SAM)
