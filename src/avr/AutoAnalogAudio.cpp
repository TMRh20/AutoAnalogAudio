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
#if !defined (ARDUINO_ARCH_SAM)

#include "../AutoAnalogAudio.h"

//#include "AutoDAC.h"
//#include "AudoADC.h"
//#include "AutoTimer.h"

#if !defined (ARDUINO_ARCH_SAM)
  volatile uint32_t dacNumSamples = 0;              /* Internal variable for number of samples sent to the DAC */
  volatile uint32_t adcNumSamples = 0;
  uint16_t adjustCtr = 0;                  /* Internal variables for adjusting timers on-the-fly */
  uint16_t adjustCtr2 = 0;                 /* Internal variables for adjusting timers on-the-fly */
  uint32_t adcLastAdjust = 0;
  volatile uint16_t adcSampleCounter = 0;
  volatile uint16_t dacSampleCounter = 0;
  uint8_t aCtr = 0;                    /* Internal counter for ADC data */
  uint16_t realBuf[MAX_BUFFER_SIZE];   /* Internal DAC buffer */
  uint16_t adcDma[MAX_BUFFER_SIZE]; /* Buffers for ADC DMA transfers */  
  uint16_t dataReady;                  /* Internal indicator for DAC data */ 
  uint16_t sampleCount = 0;                /* Internal counter for delaying analysis of timing */
  uint16_t tcTicks = 1000;                    /* Stores the current TC0 Ch0 counter value */
  uint16_t tcTicks2;                   /* Stores the current TC0 Ch1 counter value */  
  bool dacChan = 0;
  uint8_t defaultShift = 0;
#endif //ARDUINO_ARCH_SAM

/****************************************************************************/
/* Public Functions */
/****************************************************************************/

AutoAnalog::AutoAnalog(){
    
  for(int i=0; i<MAX_BUFFER_SIZE; i++){
    realBuf[i] = 0;
  }
  adcBitsPerSample = 8;
  dacBitsPerSample = 8;
  autoAdjust = true;
  adcSampleCounter = 0;
}

void AutoAnalog::begin(bool enADC, bool enDAC){
  
  if(enADC){
    analogRead(A0);
    adcSetup();
  }
  if(enDAC){
    dacSetup();
  }  
  tcSetup(DEFAULT_FREQUENCY);
}

/****************************************************************************/

void AutoAnalog::setSampleRate(uint32_t sampRate){
    

  if(sampRate > 0){
    tcTicks = max(5, frequencyToTimerCount(sampRate));
    //sampleCount = 0;
    ICR1 = tcTicks;
    OCR1A = tcTicks/2;
    OCR1B = tcTicks/2;
    
    //Based on the frequency chosen, figure out how many bits of precision are being used for the timer PWM
    //Allows to shift 8-bit samples when the timer is running at >=10-bit pwm
    uint16_t ticks = tcTicks;
    uint8_t ctr = 0;
    
    while(ticks){
        ticks >>= 1;
        ctr++;
    }
    if(ctr >= 10){
      defaultShift = ctr - 10;
    }else{
      defaultShift = 0;
    }
  }
  
}

/****************************************************************************/
void AutoAnalog::triggerADC(){}
/****************************************************************************/

void AutoAnalog::enableAdcChannel(uint8_t pinAx){
    
    analogRead(pinAx);
    adcSetup();
}

/****************************************************************************/

void AutoAnalog::disableAdcChannel(uint8_t pinAx){
    
    ADCSRA &= ~(_BV(ADIE));
}

/****************************************************************************/

void AutoAnalog::getADC(uint32_t samples){
  
  while(adcSampleCounter < adcNumSamples && !autoAdjust){}
  
  if(adcBitsPerSample == 8){    
    for(uint16_t i=0; i<adcNumSamples; i++){
      adcBuffer[i] = adcDma[i];
    }    
  }else{    
    memcpy(&adcBuffer16,&adcDma,adcNumSamples);  
  }
  
  noInterrupts();
  adcSampleCounter = 0;
  adcNumSamples = samples;
  interrupts();
  
}

/****************************************************************************/

void AutoAnalog::feedDAC(uint8_t dacChannel, uint32_t samples){
    
    while(dacSampleCounter < dacNumSamples && !autoAdjust){}
    
    if(dacBitsPerSample == 8){    
      for(uint16_t i=0; i<samples; i++){
        realBuf[i] = dacBuffer[i] << (defaultShift+2); 
      }    
    }else{
      for(uint16_t i=0; i<samples; i++){
        realBuf[i] = dacBuffer16[i] << defaultShift; 
      } 
    }
    
    noInterrupts();
    dacChan = dacChannel;
    dacSampleCounter = 0;
    dacNumSamples = samples;
    interrupts();
  

}

/****************************************************************************/
/* Private Functions */
/****************************************************************************/

uint32_t AutoAnalog::frequencyToTimerCount(uint32_t frequency){
    
    if(frequency < 5){
      TCCR1B &= ~(_BV(CS11)) | ~(_BV(CS10));
      TCCR1B |= _BV(CS12);
      return F_CPU / 256UL / frequency;
    }else
    if(frequency < 35){
      TCCR1B |= _BV(CS11) | _BV(CS10);      //Prescaler F_CPU/64
      return F_CPU / 64UL / frequency;
    }else
    if(frequency < 250){
      TCCR1B |= _BV(CS11);                  //Prescaler F_CPU/8
      TCCR1B &= ~(_BV(CS10));
      return F_CPU / 8UL / frequency;
    }else
    
    {
      TCCR1B &= ~(_BV(CS11));               //Prescaler F_CPU
      TCCR1B |= _BV(CS10);
      return F_CPU / frequency;
    }

}

/****************************************************************************/

ISR(ADC_vect){
    
    if(adcSampleCounter < adcNumSamples){
        adcDma[adcSampleCounter] = ADCH;
        ++adcSampleCounter;
    }    
}

/****************************************************************************/
extern void DACC_Handler();
/****************************************************************************/

ISR(TIMER1_OVF_vect){
    
    DACC_Handler();
    
    if(dacSampleCounter < dacNumSamples){
        if(dacChan == 2){
            OCR1A = OCR1B = realBuf[dacSampleCounter];
        }else
        if(dacChan){
            OCR1B = realBuf[dacSampleCounter];
        }else{
            OCR1A = realBuf[dacSampleCounter];
        }        
        ++dacSampleCounter;
    }
    
}

/****************************************************************************/

void AutoAnalog::adcSetup(void){
    
    ADCSRA = _BV(ADEN) | _BV(ADATE) | _BV(ADPS2) ; // En ADC, En Auto Trigger, Prescaler 8 (16Mhz/13/8 = ~150Khz)
    ADCSRB = _BV(ADTS2) | _BV(ADTS1);
    ADMUX |= _BV(ADLAR);                           //Left adjust result for 8-bit
    
    TIMSK1 |= _BV(TOIE1);
    ADCSRA |= _BV(ADIE);  
}

/****************************************************************************/

void AutoAnalog::adcInterrupts(bool enabled){

  if(enabled){
    ADCSRA |= _BV(ADIE);  
  }else{
    ADCSRA &= ~(_BV(ADIE));  
  }
}
  
/****************************************************************************/
  
void AutoAnalog::dacSetup(void){
    
    pinMode(DAC0_PIN,OUTPUT);
    pinMode(DAC1_PIN,OUTPUT);
    TIMSK1 |= _BV(TOIE1);    
}

/****************************************************************************/

void AutoAnalog::disableDAC(){ TIMSK1 |= _BV(TOIE1); } 

/****************************************************************************/

void AutoAnalog::dacHandler(void){}

/****************************************************************************/

void AutoAnalog::tcSetup (uint32_t sampRate){
  
  TCCR1A = _BV(WGM11) | _BV(COM1A1) | _BV(COM1B0) | _BV(COM1B1); // Set WGM mode, opposite action for output mode
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10); // Set WGM mode & no prescaling
          
  ICR1 = 1000;    // Default 16Khz with 16Mhz CPU
  OCR1A = 250;    // Default 25%/75% duty cycle
  
}

/****************************************************************************/

void AutoAnalog::tc2Setup (uint32_t sampRate){}

/****************************************************************************/



#endif //#if !defined (ARDUINO_ARCH_SAM)
