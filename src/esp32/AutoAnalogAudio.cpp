/**
 * @file AutoAnalogAudio.cpp
 *
 * AutoAnalogAudio streaming via DAC & ADC by TMRh20
 *
 * Copyright (C) 2016-2020  TMRh20 - tmrh20@gmail.com, github.com/TMRh20

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/****************************************************************************/

#if defined (ESP32)

#include "../AutoAnalogAudio.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"


/****************************************************************************/
/* Public Functions */
/****************************************************************************/

  volatile bool dacTaskActive;
  volatile bool dacTaskCreated;
  uint32_t adcSamples;
  uint16_t tmpADCBuffer16[MAX_BUFFER_SIZE];
  volatile uint32_t lastAdcTime;
  uint8_t *adcPtr = NULL;

  uint8_t dacVar;

AutoAnalog::AutoAnalog(){

  adcBitsPerSample = 16;
  dacBitsPerSample = 16;

  dacTaskActive = false;
  dacTaskCreated = false;
  dacEnabled = false;
  dacTaskHandle = NULL;
  i2sStopped = true;
  dacVar = 1;
  adcPtr = &adcBuffer[0];

  sampleRate = 0;
  lastDacSample = 0;
  adcChannel = (adc1_channel_t)0;

  for(int i=0; i<MAX_BUFFER_SIZE; i++){
      dacBuffer[i] = 0;
      dacBuffer16[i] = 0;
  }
  adcSamples = 0;
  lastAdcTime = 0;

}

/****************************************************************************/

void DACC_Handler();

/****************************************************************************/

void adcTask(void *args){
  for( ;; ){
      if(*(uint32_t*)args > 0){
        size_t bytesRead = 0;
        i2s_read(I2S_NUM_0, &tmpADCBuffer16, *(uint32_t*)args * 2, &bytesRead, 500 / portTICK_PERIOD_MS);
        adcSamples = 0;
        lastAdcTime = millis();
      }else{
        if(millis() - lastAdcTime > 150){
          vTaskDelay( 5 / portTICK_PERIOD_MS);
        }
      }
  }
}

/****************************************************************************/

uint32_t dacTTimer = 0;

void dacTask(void *args){
  for( ;; ){

    if(*(uint8_t*)args == 1 && dacTaskActive == true){
      DACC_Handler();
    }else{
      *(uint8_t*)args = 3;
      vTaskDelay( 10 / portTICK_PERIOD_MS);

    }
  }
}

/****************************************************************************/

void AutoAnalog::begin(bool enADC, bool enDAC){

  i2s_mode_t myMode = (i2s_mode_t)I2S_MODE_MASTER;


  if(enADC){
    myMode = (i2s_mode_t)(myMode | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN);
  }
  if(enDAC){
    myMode = (i2s_mode_t)(myMode | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN);
  }

   i2s_cfg = {
    .mode = (i2s_mode_t)myMode,
    .sample_rate =  16000,              // The format of the signal using ADC_BUILT_IN
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_LSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = MAX_BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
   };
   i2s_driver_install(I2S_NUM_0, &i2s_cfg, 0, NULL);


   if(enADC){
     i2s_set_adc_mode(ADC_UNIT_1, adcChannel); //pin 32
     adc1_config_channel_atten(adcChannel, ADC_ATTEN_DB_11);
     i2s_adc_enable(I2S_NUM_0);
   }
   if(enDAC){
     i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
     dacEnabled = true;
   }

   sampleRate = 16000;

   setSampleRate(sampleRate,true);


}

/****************************************************************************/

void AutoAnalog::dacInterrupts(bool enabled, bool withinTask){

  if(enabled){
    if(dacTaskCreated == false){
      dacTaskCreated = true;
      //Serial.print("Cr Task ");
      dacVar = 1;
      dacTaskActive = true;
      xTaskCreate(dacTask,"DAC Task",3062,&dacVar,tskIDLE_PRIORITY + 1,&dacTaskHandle);
	}
  }else{
    if(dacTaskCreated == true){
      dacTaskCreated = false;
      dacTaskActive = false;
      //Serial.println("Dac Int false");

      if(dacTaskHandle != NULL){
        if(dacVar == 1 && !withinTask){
          //Serial.println("Set Dac Var 2");
          dacVar = 2;
          while(dacVar != 3){ delay(3); }
        }

        //Serial.println("Del Task");
        vTaskDelete(dacTaskHandle);
      }

    }
  }

}

/****************************************************************************/

void AutoAnalog::rampIn(uint8_t sample){


  uint8_t tmpBuf[255];
  uint8_t zeroBuff[MAX_BUFFER_SIZE];
  memset(zeroBuff,0,MAX_BUFFER_SIZE);

  uint16_t counter = 0;
  for(uint16_t i=0; i<sample; i++){
    tmpBuf[i] = i;
    counter++;
  }

  size_t bytesWritten = 0;
  size_t bytesWritten2 = 0;

  i2s_write_expand(I2S_NUM_0,&zeroBuff[0],MAX_BUFFER_SIZE,8,16,&bytesWritten, 500 / portTICK_PERIOD_MS);
  i2s_write_expand(I2S_NUM_0,&zeroBuff[0],MAX_BUFFER_SIZE,8,16,&bytesWritten, 500 / portTICK_PERIOD_MS);
  i2s_write_expand(I2S_NUM_0,&zeroBuff[0],MAX_BUFFER_SIZE,8,16,&bytesWritten, 500 / portTICK_PERIOD_MS);

  i2s_write_expand(I2S_NUM_0,&zeroBuff[0],MAX_BUFFER_SIZE-counter,8,16,&bytesWritten, 500 / portTICK_PERIOD_MS);
  i2s_write_expand(I2S_NUM_0,&tmpBuf[0],counter,8,16,&bytesWritten2, 500 / portTICK_PERIOD_MS);
}

/****************************************************************************/

void AutoAnalog::rampOut(uint8_t sample){


  uint8_t tmpBuf[255];
  uint8_t zeroBuff[MAX_BUFFER_SIZE];
  memset(zeroBuff,0,MAX_BUFFER_SIZE);

  uint16_t counter = 0;
  for(uint8_t i=lastDacSample; i > 0; i--){
    tmpBuf[counter++] = i;
  }
  size_t bytesWritten = 0;

  i2s_write_expand(I2S_NUM_0,&tmpBuf[0],counter,8,16,&bytesWritten, 500 / portTICK_PERIOD_MS);
  i2s_write_expand(I2S_NUM_0,&zeroBuff[0],MAX_BUFFER_SIZE-counter,8,16,&bytesWritten, 500 / portTICK_PERIOD_MS);
  i2s_write_expand(I2S_NUM_0,&zeroBuff[0],MAX_BUFFER_SIZE,8,16,&bytesWritten, 500 / portTICK_PERIOD_MS);
  i2s_write_expand(I2S_NUM_0,&zeroBuff[0],MAX_BUFFER_SIZE,8,16,&bytesWritten, 500 / portTICK_PERIOD_MS);
  i2s_write_expand(I2S_NUM_0,&zeroBuff[0],MAX_BUFFER_SIZE,8,16,&bytesWritten, 500 / portTICK_PERIOD_MS);

}

/****************************************************************************/

void AutoAnalog::setSampleRate(uint32_t sampRate, bool stereo){

   //if(sampRate == sampleRate){ return; }

   sampleRate = sampRate;
   //i2s_stop(I2S_NUM_0);
   //i2s_driver_uninstall(I2S_NUM_0);
   //delay(5);
   //i2s_driver_install(I2S_NUM_0, &i2s_cfg, 0, NULL);
   if(stereo == false){
     i2s_set_clk(I2S_NUM_0, sampRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
   }else{
     i2s_set_clk(I2S_NUM_0, sampRate/2, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
   }


}

/****************************************************************************/

void AutoAnalog::triggerADC(){



}

/****************************************************************************/

void AutoAnalog::enableAdcChannel(uint8_t pinAx){

    adcChannel = (adc1_channel_t)pinAx;

}

/****************************************************************************/

void AutoAnalog::disableAdcChannel(uint8_t pinAx){

    /*if(pinAx > 6){ return; }
    pinAx = 7 - pinAx;
    ADC->ADC_CHDR |= 1<< pinAx;*/

}

/****************************************************************************/

void AutoAnalog::getADC(uint32_t samples){

  if(!adcTaskCreated){
    adcTaskCreated = true;
    xTaskCreate(adcTask,"ADC Task",2048,&adcSamples,tskIDLE_PRIORITY + 1,NULL);
    //Serial.println("Cr ADC");
  }
  while(adcSamples > 0){ delayMicroseconds(100); };
  //Serial.println("Get ADC");
  uint16_t offset = (int)adcChannel * 0x1000 + 0xFFF; // 4high bits == channel. Data is inverted.
  for(uint32_t i=0; i<samples;i++){
    tmpADCBuffer16[i] = offset - tmpADCBuffer16[i];
    adcBuffer16[i] = tmpADCBuffer16[i];
  }
  //Serial.println("samps");
  adcSamples = samples;




}

/****************************************************************************/

void AutoAnalog::feedDAC(uint8_t dacChannel, uint32_t samples, bool startInterrupts){


  //uint8_t buf[MAX_BUFFER_SIZE * 2];
  size_t bytesWritten = 0;

  if(dacBitsPerSample == 16){
    for(int i=0; i<samples; i++){
      dacBuffer[i] = (uint8_t)dacBuffer16[i];
    }
  }

  /*if(startInterrupts == true){

    //uint8_t zeroSample[MAX_BUFFER_SIZE ];
    memset(buf,0,MAX_BUFFER_SIZE*2);
    i2s_write_expand(I2S_NUM_0,&buf[0],(MAX_BUFFER_SIZE*2) - dacBuffer[0],8,16,&bytesWritten, 100 / portTICK_PERIOD_MS);
    //i2s_write_expand(I2S_NUM_0,&zeroSample,(MAX_BUFFER_SIZE),8,16,&bytesWritten, 100 / portTICK_PERIOD_MS);

    for(uint i=0; i <= (dacBuffer[0] * 2); i++){
      uint test = i / 2;
      i2s_write_expand(I2S_NUM_0,&test,1,8,16,&bytesWritten, 100 / portTICK_PERIOD_MS);
    }
  }*/

  /*if(i2sStopped){
    i2sStopped = false;
    i2s_start(I2S_NUM_0);
  }*/

  if(dacEnabled == false){
    //dacEnabled = true;
    //i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
  }

  i2s_write_expand(I2S_NUM_0,&dacBuffer[0],samples,8,16,&bytesWritten, 500 / portTICK_PERIOD_MS);
  lastDacSample = dacBuffer[bytesWritten-1];


  if(startInterrupts == true){
    dacTaskActive = true;
    //i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
    dacInterrupts(true);

  }



}

/****************************************************************************/
/* Private Functions */
/****************************************************************************/

void AutoAnalog::dacBufferStereo(uint8_t dacChannel){



}

/****************************************************************************/

uint32_t AutoAnalog::frequencyToTimerCount(uint32_t frequency){
  //return VARIANT_MCK / 2UL / frequency;
  return 0;
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

  dacTaskActive = false;
  if(!withinTask){
    dacInterrupts(false,false);
  }


  //i2s_set_dac_mode(I2S_DAC_CHANNEL_DISABLE);
  i2s_zero_dma_buffer(I2S_NUM_0);
  //dacEnabled = false;
  if(withinTask){
    dacInterrupts(false,true);
  }

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



#endif //#if defined (ESP32)
