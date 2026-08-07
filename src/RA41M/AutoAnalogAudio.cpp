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

#if defined(ARDUINO_ARCH_RENESAS)

    #include "../AutoAnalogAudio.h"
    #include "FspTimer.h"
    #include "r_timer_api.h"
    #include "r_adc.h"
    FspTimer high_speed_timer;
    FspTimer adc_timer;
    uint8_t high_speed_timer_index;
    uint8_t adc_timer_index;
    volatile bool toggle_state = false;
    #define ADC_IRQ_NUMBER  ((IRQn_Type)15) 
    volatile uint16_t latestAdcValue = 0;
    volatile bool newDataAvailable = false;
    extern "C" void my_adc_isr(timer_callback_args_t *p_args);
    volatile uint8_t AutoAnalog::analogChannel = 0;
    
uint16_t AutoAnalog::adcBuffer16[MAX_BUFFER_SIZE];
uint8_t AutoAnalog::adcBuffer[MAX_BUFFER_SIZE];
volatile bool AutoAnalog::adcReady;
volatile uint32_t AutoAnalog::sampleCounter;
volatile bool AutoAnalog::adcWhichBuf;
uint8_t AutoAnalog::adcBitsPerSample;
volatile uint32_t AutoAnalog::aSize;
volatile uint8_t AutoAnalog::aCtr;
uint16_t* AutoAnalog::adcBuf0;
uint16_t* AutoAnalog::adcBuf1;
volatile uint16_t* AutoAnalog::dacBuf0;
volatile uint16_t* AutoAnalog::dacBuf1;
void (*AutoAnalog::_onReceive)(uint16_t* buf, uint32_t buf_len);
uint8_t AutoAnalog::dacBuffer[MAX_BUFFER_SIZE];
volatile uint32_t AutoAnalog::sCounter = 0;
void dac_callback(timer_callback_args_t *p_args); 

/******************* USER DEFINES - Configure pins etc here *****************/
    #define DEFAULT_PDM_GAIN 40

    /* I2s default config  MAX98357A Breakout also needs SD pin set HIGH
    I2S_PIN_MCK    2   // Not used with MAX98357A
    I2S_PORT_MCK   0
    I2S_PIN_SCK    3   // BCLK
    I2S_PORT_SCK   0
    I2S_PIN_LRCK   29  // LRCK
    I2S_PORT_LRCK  0
    I2S_PIN_SDOUT  5   // GPIO Pin numbers
    I2S_PORT_SDOUT 0
    */

    /* PWM Config */
    #define DEFAULT_PWM_PIN  11 // GPIO Pin number
    #define DEFAULT_PWM_PORT 1 // On XIAO port 0 is for pins 1-5 port 1 is for all higher pins
                               //#define DEFAULT_PWM_PIN2 4  //Enable a second output pin
                               //#define DEFAULT_PWM_PORT2 0

    #ifndef PIN_PDM_DIN // Arduino pin numbers
        #define PIN_PDM_DIN 35
        //#define PIN_PDM_CLK 36
        #define PIN_PDM_PWR -1
    #endif

/****************************************************************************/
/* Public Functions */
/****************************************************************************/

uint8_t analogCounter = 0;
uint16_t mycounter = 0;

extern "C" {
 void SAADC_IRQHandler(void);
}


AutoAnalog::AutoAnalog()
{

    adcReady = true;
    adcBitsPerSample = 8;
    //pwrPin = PIN_PDM_PWR;
    //dinPin = PIN_PDM_DIN;
    //clkPin = PIN_PDM_CLK;
    gain = -1;

    dacBitsPerSample = 8;
    autoAdjust = true;
    for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
        dacBuffer[i] = 0;
    }

    aSize = MAX_BUFFER_SIZE;
    aCtr = 0;
    sampleCounter = 0;
    maxBufferSize = 0;
    dacBuffersAllocated = false;
    I2S_PIN_MCK = 8;
    I2S_PORT_MCK = 2;
    I2S_PIN_SCK = 5;
    I2S_PORT_SCK = 1;
    I2S_PIN_LRCK = 7;
    I2S_PORT_LRCK = 1;
    I2S_PIN_SDOUT = 11;
    I2S_PORT_SDOUT = 1;
    I2S_PIN_SDIN = 10;
    I2S_PORT_SDIN = 1;
    manualI2S = false;
}



void AutoAnalog::begin(uint8_t enADC, uint8_t enDAC, uint8_t _useI2S)
{

    maxBufferSize = maxBufferSize > 0 ? maxBufferSize : MAX_BUFFER_SIZE;

    // Fix for backward compatiblity with badly thought out changes
    if (_useI2S == 1) {
        enDAC = 2;
    }
    else if (_useI2S == 2) {
        enADC = 2;
    }
    else if (_useI2S == 3) {
        enADC = 2;
        enDAC = 2;
    }
    else if (_useI2S == 4 || _useI2S == 5) {
        enADC = 3;
    }
    else if (_useI2S == 6) {
        enADC = 3;
        enDAC = 2;
    }

    enableADC = enADC;
    enableDAC = enDAC;

    if (enADC) {
        if (!adcBuffersAllocated) {
            adcBuffersAllocated = true;
            adcBuf0 = reinterpret_cast<uint16_t*>(malloc(maxBufferSize * 2));
            memset(adcBuf0, 0, maxBufferSize * 2);
            adcBuf1 = reinterpret_cast<uint16_t*>(malloc(maxBufferSize * 2));
            memset(adcBuf1, 0, maxBufferSize * 2);
        }
        adcSetup();
    }

    if (enDAC) {
        if (!dacBuffersAllocated) {
            dacBuffersAllocated = true;
            dacBuf0 = reinterpret_cast<uint16_t*>(malloc(maxBufferSize * 2));
            memset(const_cast<void*>(reinterpret_cast<volatile void*>(dacBuf0)), 0, maxBufferSize * 2);
            dacBuf1 = reinterpret_cast<uint16_t*>(malloc(maxBufferSize * 2));
            memset(const_cast<void*>(reinterpret_cast<volatile void*>(dacBuf1)), 0, maxBufferSize * 2);
        }
        dacSetup();
    }
}

/****************************************************************************/

void AutoAnalog::setSampleRate(uint32_t sampRate, bool stereo)
{

    if (enableDAC == 3) {
        high_speed_timer.stop();
        
        uint8_t timer_type = GPT_TIMER;
        
        high_speed_timer.begin(
        TIMER_MODE_PERIODIC,   // Fire continuously at the specified interval
        timer_type,            // Use the General PWM Timer peripheral block
        high_speed_timer_index,         // Selected hardware channel channel
        sampRate * (stereo + 1),               // Target Frequency in Hz (16 kHz)
        0.0,                   // Duty cycle (unused for generic interrupts, keep 0.0)
        dac_callback          // Name of your ISR function to execute
        );

        high_speed_timer.setup_overflow_irq();
        high_speed_timer.open();
        high_speed_timer.start();
    }
    if(enableADC == 3){
        adc_timer.stop();
        
        uint8_t timerType = GPT_TIMER; 

        adc_timer.begin(TIMER_MODE_PERIODIC, timerType, adc_timer_index, sampRate * (stereo + 1), 0.0f, my_adc_isr);
        adc_timer.setup_overflow_irq();
        adc_timer.open();
        adc_timer.start();
        
        
    }
    
    /*if (enableDAC == 1) {
        NRF_PWM20->TASKS_STOP = 1;
        uint32_t timer = millis();
        while (NRF_PWM20->EVENTS_STOPPED == 0) {
            if (millis() - timer > 1000) {
                break;
            }
        }

        NRF_PWM20->COUNTERTOP = (((uint16_t)((16000000 / sampRate))) << PWM_COUNTERTOP_COUNTERTOP_Pos);
        NRF_PWM20->TASKS_DMA.SEQ[0].START = 1;
    }

    if (enableDAC == 2 || enableADC == 2) {

        if (stereo) {
            NRF_I2S->CONFIG.CHANNELS = I2S_CONFIG_CHANNELS_CHANNELS_Stereo << I2S_CONFIG_CHANNELS_CHANNELS_Pos;
        }
        else {
            NRF_I2S->CONFIG.CHANNELS = I2S_CONFIG_CHANNELS_CHANNELS_Left << I2S_CONFIG_CHANNELS_CHANNELS_Pos;
        }

            if (sampRate <= 16000) {
                NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV16 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
                NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_128X << I2S_CONFIG_RATIO_RATIO_Pos;
            }
            else if (sampRate <= 22500) {
                NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV15 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
                NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_128X << I2S_CONFIG_RATIO_RATIO_Pos;
            }
            else if (sampRate <= 24000) {
                NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV11 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
                NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_128X << I2S_CONFIG_RATIO_RATIO_Pos;
            }
            else if (sampRate <= 32000) {
                NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV16 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
                NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_64X << I2S_CONFIG_RATIO_RATIO_Pos;
            }
            else if (sampRate <= 45000) {
                NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV23 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
                NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_32X << I2S_CONFIG_RATIO_RATIO_Pos;
            }
        
    }

    if (enableADC == 1) {
        struct PwmTiming* myTiming;
        computePwmTiming(sampRate, myTiming);
        myPDM->PRESCALER = myTiming->prescaler;
    }

    if (enableADC == 3) {
        NRF_SAADC->SAMPLERATE = 16000000 / sampRate / 2 | SAADC_SAMPLERATE_MODE_Timers << SAADC_SAMPLERATE_MODE_Pos;
    }*/
}

/****************************************************************************/

void AutoAnalog::triggerADC()
{
}

/****************************************************************************/

void AutoAnalog::enableAdcChannel(uint8_t pinAx)
{
    adc_timer.stop();
    
    pinMode(pinAx + 15, INPUT);
    analogRead(pinAx + 15);
  
    R_ADC0->ADCSR_b.ADST = 0;
    while (R_ADC0->ADCSR_b.ADST) {}

    R_ADC0->ADANSA[0] = 1 << pinAx;
        
    analogChannel = pinAx;
        
    adc_timer.start();
}

/****************************************************************************/

void AutoAnalog::disableAdcChannel(uint8_t pinAx)
{
    
    adc_timer.stop();
    
    R_ADC0->ADCSR_b.ADST = 0;
    while (R_ADC0->ADCSR_b.ADST) {}
    
    R_ADC0->ADANSA[0] = 0;
    
}

/****************************************************************************/

void AutoAnalog::getADC(uint32_t samples)
{
    
    //if(!enableDAC){
        while(!adcReady){}
    //}
    
    adcReady = false;
    aSize = samples;    
    /*if (enableADC == 2) {

        bool started = false;
        if (NRF_I2S->ENABLE == 0) {
            NRF_I2S->ENABLE = 1;
            started = true;
        }
        else {
            uint32_t timeout = millis() + 1000;
            while (NRF_I2S->EVENTS_RXPTRUPD == 0) {
                if (millis() > timeout) { 
                    return;
                }
            }
        }

        if (adcWhichBuf == 0) {
            NRF_I2S->RXD.PTR = (uint32_t)&adcBuf0[0];
        }
        else {
            NRF_I2S->RXD.PTR = (uint32_t)&adcBuf1[0];
        }
        uint8_t divider = 2;
        if (adcBitsPerSample == 24 || adcBitsPerSample == 32) {
            divider = 4;
        }
        else if (adcBitsPerSample == 8) {
            divider = 1;
        }

        if (enableDAC != 2) { // Only update MAXCNT if I2S output is disabled
            NRF_I2S->RXTXD.MAXCNT = samples * divider;
        }

        NRF_I2S->EVENTS_RXPTRUPD = 0;

        if (adcBitsPerSample == 24 || adcBitsPerSample == 32) {
            if (adcBitsPerSample == 24 && NRF_I2S->CONFIG.SWIDTH != I2S_CONFIG_SWIDTH_SWIDTH_24Bit << I2S_CONFIG_SWIDTH_SWIDTH_Pos) {
                NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_24Bit << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
            }else
            if (adcBitsPerSample == 32 && NRF_I2S->CONFIG.SWIDTH != I2S_CONFIG_SWIDTH_SWIDTH_32Bit << I2S_CONFIG_SWIDTH_SWIDTH_Pos) {
                NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_32Bit << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
            }
            
            if (adcWhichBuf == 0) {
                memcpy(adcBuffer16, adcBuf0, samples * 4);
            }
            else {
                memcpy(adcBuffer16, adcBuf1, samples * 4);
            }
        }
        else if (adcBitsPerSample == 16) {
            if (adcWhichBuf == 0) {
                memcpy(adcBuffer16, adcBuf0, samples * 2);
            }
            else {
                memcpy(adcBuffer16, adcBuf1, samples * 2);
            }
        }
        else if (adcBitsPerSample == 8) {
            if (adcWhichBuf == 0) {
                //memcpy(adcBuffer, adcBuf0, samples);
                for(int i=0; i<samples; i++){
                  adcBuffer[i] = (adcBuf0[i] >> 6) - 127;
                }
            }
            else {
                //memcpy(adcBuffer, adcBuf1, samples);
                for(int i=0; i<samples; i++){
                  adcBuffer[i] = (adcBuf1[i] >> 6) - 127;
                }
            }
        }

        adcWhichBuf = !adcWhichBuf;
        if (started) {
            NRF_I2S->TASKS_START = 1;
        }
    }
    else if (enableADC == 1) {
        while (!adcReady) {
            __WFE();
        };
        aSize = samples;
        adcReady = false;
    }
    else if (enableADC == 3) {
        while (!adcReady) {Serial.println("p");
        }
        aSize = samples;
    }*/
}

/****************************************************************************/

void AutoAnalog::feedDAC(uint8_t dacChannel, uint32_t samples, bool startInterrupts)
{

    if(enableDAC == 3){
        
        if(dacDisabled){
            R_DAC->DACR = 0x5F;
            high_speed_timer.start();
            dacDisabled = false;
        }
        
        //memcpy(dacBuf0,dacBuffer,samples);
        if(aCtr == 1){
            if(dacBitsPerSample == 8){
                for(int i=0; i<samples; i++){                
                    dacBuf0[i] = dacBuffer[i] << 4;
                }
            }else
            if(dacBitsPerSample == 16){
                for(int i=0; i<samples; i++){   
                    dacBuf0[i] = (dacBuffer16[i] >> 4) + 2048;
                }
            }
        }else{
            if(dacBitsPerSample == 8){
                for(int i=0; i<samples; i++){                
                    dacBuf1[i] = dacBuffer[i] << 4;
                }
            }else
            if(dacBitsPerSample == 16){
                for(int i=0; i<samples; i++){   
                    dacBuf1[i] = (dacBuffer16[i] >> 4) + 2048;
                }
            }
        }
        while(sCounter < samples){}
        aCtr = (aCtr + 1) % 2;
        aSize = samples;
        sCounter = 0;        
    }

    /*if (enableDAC == 2) {

        bool started = false;
        if (NRF_I2S->ENABLE == 0) {
            NRF_I2S->ENABLE = 1;
            started = true;
        }
        else {
            uint32_t timeout = millis() + 1000;
            while (NRF_I2S->EVENTS_TXPTRUPD == 0) {
                if (millis() > timeout) {
                    return;
                }
            }
        }

        if (dacBitsPerSample == 8) {
            if (whichBuf) {
                for (uint32_t i = 0; i < samples; i++) {
                    dacBuf0[i] = dacBuffer[i] << 7;
                }
                NRF_I2S->TXD.PTR = (uint32_t)&dacBuf0[0];
            }
            else {
                for (uint32_t i = 0; i < samples; i++) {
                    dacBuf1[i] = dacBuffer[i] << 7;
                }
                NRF_I2S->TXD.PTR = (uint32_t)&dacBuf1[0];
            }
        }
        else if (dacBitsPerSample == 16) {
            if (whichBuf) {
                memcpy(dacBuf0, dacBuffer16, samples * 2);
                NRF_I2S->TXD.PTR = (uint32_t)&dacBuf0[0];
            }
            else {
                memcpy(dacBuf1, dacBuffer16, samples * 2);
                NRF_I2S->TXD.PTR = (uint32_t)&dacBuf1[0];
            }
        }
        else if (dacBitsPerSample == 24 || dacBitsPerSample == 32) {
            if (dacBitsPerSample == 24 && NRF_I2S->CONFIG.SWIDTH != I2S_CONFIG_SWIDTH_SWIDTH_24Bit << I2S_CONFIG_SWIDTH_SWIDTH_Pos) {
                NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_24Bit << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
            }else
            if (dacBitsPerSample == 32 && NRF_I2S->CONFIG.SWIDTH != I2S_CONFIG_SWIDTH_SWIDTH_32Bit << I2S_CONFIG_SWIDTH_SWIDTH_Pos) {
                NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_32Bit << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
            }else
            if (dacBitsPerSample == 16 && NRF_I2S->CONFIG.SWIDTH != I2S_CONFIG_SWIDTH_SWIDTH_16Bit << I2S_CONFIG_SWIDTH_SWIDTH_Pos) {
                NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_16Bit << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
            }

            if (whichBuf) {
                memcpy(dacBuf0, dacBuffer16, samples * 4);
                NRF_I2S->TXD.PTR = (uint32_t)&dacBuf0[0];
            }
            else {
                memcpy(dacBuf1, dacBuffer16, samples * 4);
                NRF_I2S->TXD.PTR = (uint32_t)&dacBuf1[0];
            }
        }

        whichBuf = !whichBuf;

        uint8_t divider = 2;
        if (dacBitsPerSample == 16) {
            divider = 2;
        }
        else if (dacBitsPerSample == 24 || dacBitsPerSample == 32) {
            divider = 4;
        }

        NRF_I2S->RXTXD.MAXCNT = samples * divider;
        NRF_I2S->EVENTS_TXPTRUPD = 0;

        if (started) {
            NRF_I2S->TASKS_START = 1;
        }
    }
    else if (enableDAC == 1) {
        uint32_t timer = millis() + 1000;
        while (NRF_PWM20->EVENTS_DMA.SEQ[0].END == 0) {
            if (millis() > timer) {
                NRF_PWM20->TASKS_DMA.SEQ[0].START = 1;
                return;
            }
        }
        NRF_PWM20->EVENTS_DMA.SEQ[0].END = 0;

        if (dacBitsPerSample > 8) {
            if (whichBuf) {
                memcpy(dacBuf0, dacBuffer16, samples * 2);
            }
            else {
                memcpy(dacBuf1, dacBuffer16, samples * 2);
            }
        }
        else {
            if (whichBuf) {
                for (uint32_t i = 0; i < samples; i++) {
                    dacBuf0[i] = (uint16_t)(dacBuffer[i] << 1);
                }
            }
            else {
                for (uint32_t i = 0; i < samples; i++) {
                    dacBuf1[i] = (uint16_t)(dacBuffer[i] << 1);
                }
            }
        }
        if (whichBuf) {
            NRF_PWM20->DMA.SEQ[0].PTR = ((uint32_t)(&dacBuf0[0]) << PWM_DMA_SEQ_PTR_PTR_Pos);
        }
        else {
            NRF_PWM20->DMA.SEQ[0].PTR = ((uint32_t)(&dacBuf1[0]) << PWM_DMA_SEQ_PTR_PTR_Pos);
        }
        whichBuf = !whichBuf;

        NRF_PWM20->DMA.SEQ[0].MAXCNT = (samples << PWM_DMA_SEQ_MAXCNT_MAXCNT_Pos);
        NRF_PWM20->TASKS_DMA.SEQ[0].START = 1;
    }*/
}

/****************************************************************************/
/* Private Functions */
/****************************************************************************/

void AutoAnalog::dacBufferStereo(uint8_t dacChannel)
{
}

/****************************************************************************/

uint32_t AutoAnalog::frequencyToTimerCount(uint32_t frequency)
{

    return 1;
}

/****************************************************************************/

void AutoAnalog::startPwmI2sTimers()
{
 /*           // 1. Setup BCLK (WS * 64)
            NRF_PWM20->PSEL.OUT[0] = I2S_PIN_SCK | I2S_PORT_SCK << 5 | 0 << 31;
            NRF_PWM20->ENABLE = 1;
            NRF_PWM20->MODE = PWM_MODE_UPDOWN_Up;
            NRF_PWM20->PRESCALER = PWM_PRESCALER_PRESCALER_DIV_1; // 16MHz
            NRF_PWM20->COUNTERTOP = 8;                            // 16MHz / 8 = 2MHz
            static uint16_t bclk_seq[] = {4};                    // 50% duty cycle
            NRF_PWM20->DECODER = PWM_DECODER_LOAD_Common;
            NRF_PWM20->DMA.SEQ[0].PTR = (uint32_t)bclk_seq;
            NRF_PWM20->DMA.SEQ[0].MAXCNT = 1;
            NRF_PWM20->TASKS_DMA.SEQ[0].START = 1;

            // 2. Setup WS (31.25kHz)
            NRF_PWM21->PSEL.OUT[0] = I2S_PIN_LRCK | I2S_PORT_LRCK << 5 | 0 << 31;
            NRF_PWM21->ENABLE = 1;
            NRF_PWM21->MODE = PWM_MODE_UPDOWN_Up;
            NRF_PWM21->PRESCALER = PWM_PRESCALER_PRESCALER_DIV_1;
            NRF_PWM21->COUNTERTOP = 512;       // 16MHz / 512 = 31.25kHz
            static uint16_t ws_seq[] = {128}; // 50% duty cycle
            NRF_PWM21->DECODER = PWM_DECODER_LOAD_Common;
            NRF_PWM21->DMA.SEQ[0].PTR = (uint32_t)ws_seq;
            NRF_PWM21->DMA.SEQ[0].MAXCNT = 1;
            NRF_PWM21->TASKS_DMA.SEQ[0].START = 1;
            */
}

/****************************************************************************/
#define ARM_VTOR (*((volatile uint32_t *)0xE000ED08))

void AutoAnalog::adcSetup(void)
{
    
    
    if(enableADC == 3) {
        
        analogReadResolution(14); 
        pinMode(15 + analogChannel, INPUT);
        analogRead(15 + analogChannel);
        //analogReference(AR_INTERNAL);
        
        R_MSTP->MSTPCRD &= ~(1UL << 16);   // MSTPD16 = 0 -> ADC140 enabled
        __asm volatile ("nop");
        __asm volatile ("nop");
  
        R_ADC0->ADCSR_b.ADST = 0;
        while (R_ADC0->ADCSR_b.ADST) {}

        R_ADC0->ADCSR = 0x8000;
        R_ADC0->ADANSA[0] = 1 << analogChannel;
        //R_ADC0->ADANSA_b[1].ANSA0 = 2;    // AN001

  
        // Optional sample time
        R_ADC0->ADSSTR[0] = 1;
  
        
        uint8_t timerType = GPT_TIMER; 
        adc_timer_index = FspTimer::get_available_timer(timerType);
        if (adc_timer_index < 0) {
        FspTimer::force_use_of_pwm_reserved_timer();
        timerType = GPT_TIMER;
        adc_timer_index = FspTimer::get_available_timer(timerType);
        }
        adc_timer.begin(TIMER_MODE_PERIODIC, timerType, adc_timer_index, 16000, 0.0f, my_adc_isr);
        adc_timer.setup_overflow_irq();
        adc_timer.open();
        adc_timer.start();

        
    }
/*
    if (enableADC == 2) {
        NRF_I2S->TASKS_STOP = 1;
        NRF_I2S->ENABLE = 0;

        NRF_I2S->CONFIG.RXEN = (I2S_CONFIG_RXEN_RXEN_Enabled << I2S_CONFIG_RXEN_RXEN_Pos);

        if (enableDAC != 2) {
            NRF_I2S->CONFIG.TXEN = (I2S_CONFIG_TXEN_TXEN_Disabled << I2S_CONFIG_TXEN_TXEN_Pos);
        }

        if (manualI2S) {
            // Slave mode, 16Bit, left aligned
            NRF_I2S->CONFIG.MODE = I2S_CONFIG_MODE_MODE_Master << I2S_CONFIG_MODE_MODE_Pos;
            NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_32Bit << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
            NRF_I2S->CONFIG.FORMAT = I2S_CONFIG_FORMAT_FORMAT_I2S << I2S_CONFIG_FORMAT_FORMAT_Pos;
            NRF_I2S->CONFIG.ALIGN = I2S_CONFIG_ALIGN_ALIGN_Right << I2S_CONFIG_ALIGN_ALIGN_Pos;
            NRF_I2S->CONFIG.MCKEN = (I2S_CONFIG_MCKEN_MCKEN_Enabled << I2S_CONFIG_MCKEN_MCKEN_Pos);
            NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV16 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
            NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_128X << I2S_CONFIG_RATIO_RATIO_Pos;
        }
        else {
            NRF_I2S->CONFIG.MODE = I2S_CONFIG_MODE_MODE_Master << I2S_CONFIG_MODE_MODE_Pos;
            NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_16Bit << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
            NRF_I2S->CONFIG.FORMAT = I2S_CONFIG_FORMAT_FORMAT_I2S << I2S_CONFIG_FORMAT_FORMAT_Pos;
            NRF_I2S->CONFIG.ALIGN = I2S_CONFIG_ALIGN_ALIGN_Left << I2S_CONFIG_ALIGN_ALIGN_Pos;
            NRF_I2S->CONFIG.MCKEN = (I2S_CONFIG_MCKEN_MCKEN_Enabled << I2S_CONFIG_MCKEN_MCKEN_Pos);
            NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV16 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
            NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_128X << I2S_CONFIG_RATIO_RATIO_Pos;
        }

        NRF_I2S->CONFIG.CHANNELS = I2S_CONFIG_CHANNELS_CHANNELS_Left << I2S_CONFIG_CHANNELS_CHANNELS_Pos;

        // Configure pins
        NRF_I2S->PSEL.MCK = (I2S_PIN_MCK << I2S_PSEL_MCK_PIN_Pos) | (I2S_PSEL_MCK_CONNECT_Connected << I2S_PSEL_MCK_CONNECT_Pos) | (I2S_PORT_MCK << I2S_PSEL_MCK_PORT_Pos);
        NRF_I2S->PSEL.SCK = (I2S_PIN_SCK << I2S_PSEL_SCK_PIN_Pos) | (I2S_PSEL_SCK_CONNECT_Connected << I2S_PSEL_SCK_CONNECT_Pos) | (I2S_PORT_SCK << I2S_PSEL_SCK_PORT_Pos);
        NRF_I2S->PSEL.LRCK = (I2S_PIN_LRCK << I2S_PSEL_LRCK_PIN_Pos) | (I2S_PSEL_LRCK_CONNECT_Connected << I2S_PSEL_LRCK_CONNECT_Pos) | (I2S_PORT_LRCK << I2S_PSEL_LRCK_PORT_Pos);
        NRF_I2S->PSEL.SDIN = (I2S_PIN_SDIN << I2S_PSEL_SDIN_PIN_Pos) | (I2S_PSEL_SDIN_CONNECT_Connected << I2S_PSEL_SDIN_CONNECT_Pos) | (I2S_PORT_SDIN << I2S_PSEL_SDIN_PORT_Pos);

        // Configure data pointer
        NRF_I2S->RXD.PTR = (uint32_t)adcBuf0;
        NRF_I2S->RXTXD.MAXCNT = 16; // / sizeof(uint32_t);
    }
    /*else if (enableADC == 1) {

        set_callback(adcCallback);

        // Enable high frequency oscillator if not already enabled
        if (NRF_CLOCK->EVENTS_XOSTARTED == 0) {
            NRF_CLOCK->TASKS_XOSTART = 1;
            while (NRF_CLOCK->EVENTS_XOSTARTED == 0) {
            }
        }

        // Set default 16khz sample rate
        NRF_PDM->RATIO = ((PDM_RATIO_RATIO_Ratio80 << PDM_RATIO_RATIO_Pos) & PDM_RATIO_RATIO_Msk);

    #if defined __MBED__

        nrf_pdm_clock_set(NRF_PDM_FREQ_1280K);
        // Set default channel mono
        nrf_pdm_mode_set(NRF_PDM_MODE_MONO, NRF_PDM_EDGE_LEFTFALLING);
        if (gain == -1) {
            gain = DEFAULT_PDM_GAIN;
        }
        nrf_pdm_gain_set(gain, gain);

        pinMode(clkPin, OUTPUT);
        digitalWrite(clkPin, LOW);

        pinMode(dinPin, INPUT);

        nrf_pdm_psel_connect(digitalPinToPinName(clkPin), digitalPinToPinName(dinPin));

        // Enable PDM interrupts and clear events
        nrf_pdm_event_clear(NRF_PDM_EVENT_STARTED);
        nrf_pdm_event_clear(NRF_PDM_EVENT_END);
        nrf_pdm_event_clear(NRF_PDM_EVENT_STOPPED);
        nrf_pdm_int_enable(NRF_PDM_INT_STARTED | NRF_PDM_INT_STOPPED);

        // Turn on the mic
        if (pwrPin > -1) {
            pinMode(pwrPin, OUTPUT);
            digitalWrite(pwrPin, HIGH);
        }
        else {
        }

        // set the PDM IRQ priority and enable
        NVIC_SetPriority(PDM_IRQn, PDM_IRQ_PRIORITY);
        NVIC_ClearPendingIRQ(PDM_IRQn);
        NVIC_EnableIRQ(PDM_IRQn);

        // enable and trigger start task
        nrf_pdm_enable();
        nrf_pdm_event_clear(NRF_PDM_EVENT_STARTED);
        nrf_pdm_task_trigger(NRF_PDM_TASK_START);

    #else // MBED / Non-Mbed

        nrf_pdm_clock_set(myPDM, NRF_PDM_FREQ_1280K);

        // Set default channel mono
        nrf_pdm_mode_set(myPDM, NRF_PDM_MODE_MONO, NRF_PDM_EDGE_LEFTFALLING);
        if (gain == -1) {
            gain = DEFAULT_PDM_GAIN;
        }
        nrf_pdm_gain_set(myPDM, gain, gain);

        pinMode(clkPin, OUTPUT);
        digitalWrite(clkPin, LOW);

        pinMode(dinPin, INPUT);

        nrf_pdm_psel_connect(myPDM, digitalPinToPinName(clkPin), digitalPinToPinName(dinPin));

        // Enable PDM interrupts and clear events
        nrf_pdm_event_clear(myPDM, NRF_PDM_EVENT_STARTED);
        nrf_pdm_event_clear(myPDM, NRF_PDM_EVENT_END);
        nrf_pdm_event_clear(myPDM, NRF_PDM_EVENT_STOPPED);
        nrf_pdm_int_enable(myPDM, NRF_PDM_INT_STARTED | NRF_PDM_INT_STOPPED);

        // Turn on the mic
        if (pwrPin > -1) {
            pinMode(pwrPin, OUTPUT);
            digitalWrite(pwrPin, HIGH);
        }
        else {
        }

        // set the PDM IRQ priority and enable
        NVIC_SetPriority(PDM_IRQn, PDM_IRQ_PRIORITY);
        NVIC_ClearPendingIRQ(PDM_IRQn);
        NVIC_EnableIRQ(PDM_IRQn);

        // enable and trigger start task
        nrf_pdm_enable(myPDM);
        nrf_pdm_event_clear(myPDM, NRF_PDM_EVENT_STARTED);
        nrf_pdm_task_trigger(myPDM, NRF_PDM_TASK_START);

    #endif
    }*/
    /*else if (enableADC == 3) {

        NRF_SAADC->CH[0].PSELP = 4 << SAADC_CH_PSELP_PIN_Pos | 1 << SAADC_CH_PSELP_PORT_Pos | 1 << SAADC_CH_PSELP_CONNECT_Pos | 0 << SAADC_CH_PSELP_INTERNAL_Pos;
        NRF_SAADC->CH[0].PSELN = 4 << SAADC_CH_PSELN_PIN_Pos | 1 << SAADC_CH_PSELN_PORT_Pos | 1 << SAADC_CH_PSELN_CONNECT_Pos | 0 << SAADC_CH_PSELN_INTERNAL_Pos;
        NRF_SAADC->CH[0].CONFIG = SAADC_CH_CONFIG_GAIN_Gain2 << SAADC_CH_CONFIG_GAIN_Pos | SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos | 40 << SAADC_CH_CONFIG_TACQ_Pos | SAADC_CH_CONFIG_MODE_Diff << SAADC_CH_CONFIG_MODE_Pos | SAADC_CH_CONFIG_BURST_Disabled << SAADC_CH_CONFIG_BURST_Pos;
        NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_VAL_14bit << SAADC_RESOLUTION_VAL_Pos;
        NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Over2x << SAADC_OVERSAMPLE_OVERSAMPLE_Pos;
        NRF_SAADC->SAMPLERATE = 16000000 / 16000 / 2 | SAADC_SAMPLERATE_MODE_Timers << SAADC_SAMPLERATE_MODE_Pos;
        NRF_SAADC->RESULT.PTR = (uint32_t)adcBuf0;
        NRF_SAADC->RESULT.MAXCNT = 16;
        NRF_SAADC->ENABLE = true;

        NRF_SAADC->TASKS_CALIBRATEOFFSET = true;
        while (!NRF_SAADC->EVENTS_CALIBRATEDONE) {
        }

        NRF_SAADC->EVENTS_STARTED = 0;
        NRF_SAADC->TASKS_START = 1;
        while (!NRF_SAADC->EVENTS_STARTED) {
        };
        while (NRF_SAADC->STATUS == (SAADC_STATUS_STATUS_Busy << SAADC_STATUS_STATUS_Pos))
            ;

    uint32_t *vector_table = (uint32_t *)ARM_VTOR;
    vector_table[SAADC_IRQn + 16] = (uint32_t)SAADC_IRQHandler;
        NRF_SAADC->INTENSET = SAADC_INTENSET_END_Msk | SAADC_INTENSET_STARTED_Msk;
        NVIC_SetPriority(SAADC_IRQn, 7);
        NVIC_ClearPendingIRQ(SAADC_IRQn);
        NVIC_EnableIRQ(SAADC_IRQn);
        __enable_irq();
Serial.println("enable IRQ");
        NRF_SAADC->TASKS_SAMPLE = 1;
    } // USE_I2S
    */
}

/****************************************************************************/

void AutoAnalog::adcInterrupts(bool enabled)
{
}

/****************************************************************************/

void AutoAnalog::dacSetup(void)
{
    
    if(enableDAC == 3){

        analogWriteResolution(12); 
        analogWrite(A0, 0);
        
        R_DAC->DACR = 0x5F; // enable
        R_DAC->DADPR = 0; // Right justified
        R_DAC->DAADSCR = 1 << 7; // Sync ADC14 and DAC12
        //R_DAC->DAVREFCR = 1 << 3 | 1 << 2; // VREFH/VREFL
        
        uint8_t timer_type = GPT_TIMER;
        high_speed_timer_index = FspTimer::get_available_timer(timer_type);
        if (high_speed_timer_index < 0) {
        FspTimer::force_use_of_pwm_reserved_timer();
        timer_type = GPT_TIMER;
        high_speed_timer_index = FspTimer::get_available_timer(timer_type);
        }
        
        high_speed_timer.begin(
        TIMER_MODE_PERIODIC,   // Fire continuously at the specified interval
        timer_type,            // Use the General PWM Timer peripheral block
        high_speed_timer_index,// Selected hardware channel channel
        16000.0,               // Target Frequency in Hz (16 kHz)
        0.0,                   // Duty cycle (unused for generic interrupts, keep 0.0)
        dac_callback           // Name of your ISR function to execute
        );
        
        high_speed_timer.setup_overflow_irq();
        high_speed_timer.open();
        high_speed_timer.start();
        

    }

    /*if (enableDAC == 2) {
        NRF_I2S->TASKS_STOP = 1;
        NRF_I2S->ENABLE = 0;
        // Enable transmission

        if (enableADC != 2) {
            NRF_I2S->CONFIG.RXEN = (I2S_CONFIG_RXEN_RXEN_Disabled << I2S_CONFIG_RXEN_RXEN_Pos);
            //NRF_PWM0->TASKS_STOP = 1;
            //NRF_PWM1->TASKS_STOP = 1;
        }
        NRF_I2S->CONFIG.TXEN = (I2S_CONFIG_TXEN_TXEN_Enabled << I2S_CONFIG_TXEN_TXEN_Pos);

        
        if (manualI2S) {
            NRF_I2S->CONFIG.MODE = I2S_CONFIG_MODE_MODE_Master << I2S_CONFIG_MODE_MODE_Pos;
            NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_32Bit << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
            NRF_I2S->CONFIG.FORMAT = I2S_CONFIG_FORMAT_FORMAT_I2S << I2S_CONFIG_FORMAT_FORMAT_Pos;
            NRF_I2S->CONFIG.ALIGN = I2S_CONFIG_ALIGN_ALIGN_Right << I2S_CONFIG_ALIGN_ALIGN_Pos;
            NRF_I2S->CONFIG.MCKEN = (I2S_CONFIG_MCKEN_MCKEN_Enabled << I2S_CONFIG_MCKEN_MCKEN_Pos);
            NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV16 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
            NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_128X << I2S_CONFIG_RATIO_RATIO_Pos;

        }else{
            // Enable MCK generator
            NRF_I2S->CONFIG.MCKEN = (I2S_CONFIG_MCKEN_MCKEN_Enabled << I2S_CONFIG_MCKEN_MCKEN_Pos);

            // Master mode, 16Bit, left aligned
            NRF_I2S->CONFIG.MODE = I2S_CONFIG_MODE_MODE_Master << I2S_CONFIG_MODE_MODE_Pos;

            NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_16Bit << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
            NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV16 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
            NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_128X << I2S_CONFIG_RATIO_RATIO_Pos;
            NRF_I2S->CONFIG.ALIGN = I2S_CONFIG_ALIGN_ALIGN_Left << I2S_CONFIG_ALIGN_ALIGN_Pos;
            // Format = I2S
            NRF_I2S->CONFIG.FORMAT = I2S_CONFIG_FORMAT_FORMAT_I2S << I2S_CONFIG_FORMAT_FORMAT_Pos;
        }


            // Use left channel
            NRF_I2S->CONFIG.CHANNELS = I2S_CONFIG_CHANNELS_CHANNELS_Left << I2S_CONFIG_CHANNELS_CHANNELS_Pos;

        // Configure pins
        NRF_I2S->PSEL.MCK = (I2S_PIN_MCK << I2S_PSEL_MCK_PIN_Pos) | (I2S_PSEL_MCK_CONNECT_Connected << I2S_PSEL_MCK_CONNECT_Pos) | (I2S_PORT_MCK << I2S_PSEL_MCK_PORT_Pos);
        NRF_I2S->PSEL.SCK = (I2S_PIN_SCK << I2S_PSEL_SCK_PIN_Pos) | (I2S_PSEL_SCK_CONNECT_Connected << I2S_PSEL_SCK_CONNECT_Pos) | (I2S_PORT_SCK << I2S_PSEL_SCK_PORT_Pos);
        NRF_I2S->PSEL.LRCK = (I2S_PIN_LRCK << I2S_PSEL_LRCK_PIN_Pos) | (I2S_PSEL_LRCK_CONNECT_Connected << I2S_PSEL_LRCK_CONNECT_Pos) | (I2S_PORT_LRCK << I2S_PSEL_LRCK_PORT_Pos);
        NRF_I2S->PSEL.SDOUT = (I2S_PIN_SDOUT << I2S_PSEL_SDOUT_PIN_Pos) | (I2S_PSEL_SDOUT_CONNECT_Connected << I2S_PSEL_SDOUT_CONNECT_Pos) | (I2S_PORT_SDOUT << I2S_PSEL_SDOUT_PORT_Pos);

        // NRF_I2S->INTENSET = I2S_INTEN_TXPTRUPD_Enabled << I2S_INTEN_TXPTRUPD_Pos;
        // NVIC_EnableIRQ(I2S_IRQn);

        // Configure data pointer
        NRF_I2S->TXD.PTR = (uint32_t)&dacBuf0[0];
        //NRF_I2S->RXD.PTR = (uint32_t)dacBuf1;
        NRF_I2S->RXTXD.MAXCNT = 16; // / sizeof(uint32_t);
    }
    else if (enableDAC == 1) {

        NRF_PWM20->PSEL.OUT[0] = (DEFAULT_PWM_PIN << PWM_PSEL_OUT_PIN_Pos) | (PWM_PSEL_OUT_CONNECT_Connected << PWM_PSEL_OUT_CONNECT_Pos | DEFAULT_PWM_PORT << PWM_PSEL_OUT_PORT_Pos);
    #if defined DEFAULT_PWM_PIN2
        NRF_PWM20->PSEL.OUT[1] = (DEFAULT_PWM_PIN2 << PWM_PSEL_OUT_PIN_Pos) | (PWM_PSEL_OUT_CONNECT_Connected << PWM_PSEL_OUT_CONNECT_Pos | DEFAULT_PWM_PORT2 << PWM_PSEL_OUT_PORT_Pos);
    #endif
        NRF_PWM20->ENABLE = (PWM_ENABLE_ENABLE_Enabled << PWM_ENABLE_ENABLE_Pos);
        NRF_PWM20->MODE = (PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos);
        NRF_PWM20->PRESCALER = (PWM_PRESCALER_PRESCALER_DIV_1 << PWM_PRESCALER_PRESCALER_Pos);
        NRF_PWM20->COUNTERTOP = (((uint16_t)((16000000 / DEFAULT_FREQUENCY))) << PWM_COUNTERTOP_COUNTERTOP_Pos); // 1 msec
        NRF_PWM20->LOOP = (0 << PWM_LOOP_CNT_Pos);
        NRF_PWM20->DECODER = (PWM_DECODER_LOAD_Common << PWM_DECODER_LOAD_Pos) | (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);
        NRF_PWM20->DMA.SEQ[0].PTR = ((uint32_t)(&dacBuf0[0]) << PWM_DMA_SEQ_PTR_PTR_Pos);
        NRF_PWM20->DMA.SEQ[0].MAXCNT = 1 << PWM_DMA_SEQ_MAXCNT_MAXCNT_Pos; //((sizeof(dacBuf0) / sizeof(uint16_t)) << PWM_SEQ_CNT_CNT_Pos);
        NRF_PWM20->SEQ[0].REFRESH = 0;
        NRF_PWM20->SEQ[0].ENDDELAY = 0;

        // NRF_PWM0->INTENSET = PWM_INTENSET_SEQEND0_Enabled << PWM_INTENSET_SEQEND0_Pos;
        // NVIC_EnableIRQ(PWM0_IRQn);
        NRF_PWM20->TASKS_DMA.SEQ[0].START = 1;
    }*/
}

/****************************************************************************/

void AutoAnalog::disableDAC(bool withinTask)
{
    if (enableDAC == 3){
        high_speed_timer.stop();
        R_DAC->DACR = 0x4F;
        dacDisabled = true;
    }else
    if (enableDAC == 2) {
        /*NRF_I2S->TASKS_STOP = 1;
        NRF_I2S->ENABLE = 0;*/
    }
    else if (enableDAC == 1) {
        //NRF_PWM0->TASKS_STOP = 1;
    }
}

/****************************************************************************/

void AutoAnalog::dacHandler(void)
{
}

/****************************************************************************/

void AutoAnalog::tcSetup(uint32_t sampRate)
{
}

/****************************************************************************/

void AutoAnalog::tc2Setup(uint32_t sampRate)
{
}
extern "C" {
__attribute__((__used__)) void I2S_IRQHandler_v(void)
{
}
}

extern "C" {
__attribute__((__used__)) void SAADC_IRQHandler(void)
{
/*
    uint32_t samples = AutoAnalog::aSize;

    if (NRF_SAADC->EVENTS_END)
    {
        if (!AutoAnalog::adcWhichBuf) {
            NRF_SAADC->RESULT.PTR = (uint32_t)AutoAnalog::adcBuf1;
        }
        else {
            NRF_SAADC->RESULT.PTR = (uint32_t)AutoAnalog::adcBuf0;
        }
        NRF_SAADC->RESULT.MAXCNT = samples;
        NRF_SAADC->EVENTS_END = 0;
        NRF_SAADC->TASKS_START = 1;

        if (AutoAnalog::adcBitsPerSample == 16) {
            if (!AutoAnalog::adcWhichBuf) {
                for (uint32_t i = 0; i < samples; i++) {
                    AutoAnalog::adcBuffer16[i] = AutoAnalog::adcBuf0[i] << 2;
                }
            }
            else {
                for (uint32_t i = 0; i < samples; i++) {
                    AutoAnalog::adcBuffer16[i] = AutoAnalog::adcBuf1[i] << 2;
                }
            }
        }
        else if (AutoAnalog::adcBitsPerSample == 8) {
            if (!AutoAnalog::adcWhichBuf) {
                for (uint32_t i = 0; i < samples; i++) {
                    AutoAnalog::adcBuffer[i] = AutoAnalog::adcBuf0[i] >> 6;
                }
            }
            else {
                for (uint32_t i = 0; i < samples; i++) {
                    AutoAnalog::adcBuffer[i] = AutoAnalog::adcBuf1[i] >> 6;
                }
            }
        }
        AutoAnalog::adcWhichBuf = !AutoAnalog::adcWhichBuf;
        AutoAnalog::adcReady = true;
    }

    if (NRF_SAADC->EVENTS_STARTED)
    {
        NRF_SAADC->EVENTS_STARTED = 0;
    }
    __DSB();
*/
}
}

    /****************************************************************************/
/*    #if defined __MBED__
extern "C" {
__attribute__((__used__)) void PDM_IRQHandler_v(void)
{
    if (nrf_pdm_event_check(NRF_PDM_EVENT_STARTED)) {
        nrf_pdm_event_clear(NRF_PDM_EVENT_STARTED);

        // switch to fill

        if (AutoAnalog::aCtr) {
            nrf_pdm_buffer_set((uint32_t*)(AutoAnalog::adcBuf0), AutoAnalog::aSize);
            if (AutoAnalog::_onReceive) {
                NVIC_DisableIRQ(PDM_IRQn);
                AutoAnalog::_onReceive(AutoAnalog::adcBuf1, AutoAnalog::aSize);
                NVIC_EnableIRQ(PDM_IRQn);
            }
        }
        else {
            nrf_pdm_buffer_set((uint32_t*)(AutoAnalog::adcBuf1), AutoAnalog::aSize);
            if (AutoAnalog::_onReceive) {
                NVIC_DisableIRQ(PDM_IRQn);
                AutoAnalog::_onReceive(AutoAnalog::adcBuf0, AutoAnalog::aSize);
                NVIC_EnableIRQ(PDM_IRQn);
            }
        }

        // Flip to next buffer
        AutoAnalog::aCtr = (AutoAnalog::aCtr + 1) % 2;
    }
    else if (nrf_pdm_event_check(NRF_PDM_EVENT_STOPPED)) {
        nrf_pdm_event_clear(NRF_PDM_EVENT_STOPPED);
    }
    else if (nrf_pdm_event_check(NRF_PDM_EVENT_END)) {
        nrf_pdm_event_clear(NRF_PDM_EVENT_END);
    }
}
}

    #elif !defined MBED

extern "C" {
__attribute__((__used__)) void PDM_IRQHandler(void)
{
    if (nrf_pdm_event_check(myPDM, NRF_PDM_EVENT_STARTED)) {
        nrf_pdm_event_clear(myPDM, NRF_PDM_EVENT_STARTED);

        // switch to fill

        if (AutoAnalog::aCtr) {
            nrf_pdm_buffer_set(myPDM, (uint32_t*)(AutoAnalog::adcBuf0), AutoAnalog::aSize);
            if (AutoAnalog::_onReceive) {
                NVIC_DisableIRQ(PDM_IRQn);
                AutoAnalog::_onReceive(AutoAnalog::adcBuf1, AutoAnalog::aSize);
                NVIC_EnableIRQ(PDM_IRQn);
            }
        }
        else {
            nrf_pdm_buffer_set(myPDM, (uint32_t*)(AutoAnalog::adcBuf1), AutoAnalog::aSize);
            if (AutoAnalog::_onReceive) {
                NVIC_DisableIRQ(PDM_IRQn);
                AutoAnalog::_onReceive(AutoAnalog::adcBuf0, AutoAnalog::aSize);
                NVIC_EnableIRQ(PDM_IRQn);
            }
        }

        // Flip to next buffer
        AutoAnalog::aCtr = (AutoAnalog::aCtr + 1) % 2;
    }
    else if (nrf_pdm_event_check(myPDM, NRF_PDM_EVENT_STOPPED)) {
        nrf_pdm_event_clear(myPDM, NRF_PDM_EVENT_STOPPED);
    }
    else if (nrf_pdm_event_check(myPDM, NRF_PDM_EVENT_END)) {
        nrf_pdm_event_clear(myPDM, NRF_PDM_EVENT_END);
    }
}
}

extern "C" {
__attribute__((__used__)) void PWM0_IRQHandler(void)
{

    // AutoAnalog::sampleCounter++;
}
}
    #endif // defined __MBED__
*/


//volatile uint32_t intCtr = 0;

void dac_callback(timer_callback_args_t *p_args) {

   /*intCtr++;


   if(intCtr >= 16000){
       intCtr = 0;
   }*/
   
   if(AutoAnalog::sCounter < AutoAnalog::aSize){
       
       if(AutoAnalog::aCtr == 0){
        R_DAC->DADR[0] = AutoAnalog::dacBuf0[AutoAnalog::sCounter];
        
       }else{
        R_DAC->DADR[0] = AutoAnalog::dacBuf1[AutoAnalog::sCounter];
       }
   }
   AutoAnalog::sCounter++;
}

extern "C" void my_adc_isr(timer_callback_args_t *p_args) {
  
  (void)p_args;

  if(AutoAnalog::adcWhichBuf == 0){
    AutoAnalog::adcBuf0[AutoAnalog::sampleCounter] = R_ADC0->ADDR[AutoAnalog::analogChannel]; //analogRead(A1);
  }else{
    AutoAnalog::adcBuf1[AutoAnalog::sampleCounter] = R_ADC0->ADDR[AutoAnalog::analogChannel]; 
  }  
  
  AutoAnalog::sampleCounter++;
  
  if(AutoAnalog::sampleCounter >= AutoAnalog::aSize){
      if(AutoAnalog::adcWhichBuf == 0){
        if(AutoAnalog::adcBitsPerSample == 8){
            for(uint32_t i=0; i<AutoAnalog::aSize; i++){
                AutoAnalog::adcBuffer[i] = AutoAnalog::adcBuf0[i] >> 6;
            }                
        }else{
            memcpy(AutoAnalog::adcBuffer16,AutoAnalog::adcBuf0, AutoAnalog::aSize * 2);
        }
      }else{
        if(AutoAnalog::adcBitsPerSample == 8){
            for(uint32_t i=0; i<AutoAnalog::aSize; i++){
                AutoAnalog::adcBuffer[i] = AutoAnalog::adcBuf1[i] >> 6;
            }
        }else{
            memcpy(AutoAnalog::adcBuffer16,AutoAnalog::adcBuf1, AutoAnalog::aSize * 2 ); 
        }
      }
      AutoAnalog::sampleCounter = 0;
      AutoAnalog::adcReady = true;
      AutoAnalog::adcWhichBuf = !AutoAnalog::adcWhichBuf;
  }
  
  R_ADC0->ADCSR_b.ADST = 1;
}


void AutoAnalog::set_callback(void (*function)(uint16_t* buf, uint32_t buf_len))
{
    _onReceive = function;
}

void AutoAnalog::adcCallback(uint16_t* buf, uint32_t buf_len)
{

    memcpy(adcBuffer16, buf, buf_len * 2);
    adcReady = true;
}


#endif //#if defined (ARDUINO_ARCH_SAM)