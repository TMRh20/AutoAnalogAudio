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
    #if defined ARDUINO_UNOR4_WIFI
        #include "WiFiS3.h"
    #endif
FspTimer high_speed_timer;
FspTimer adc_timer;
uint8_t high_speed_timer_index;
uint8_t adc_timer_index;
uint32_t sampleRateADC;
uint32_t sampleRateDAC;
bool stereoVar;

extern "C" void my_adc_isr(timer_callback_args_t* p_args);
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
void dac_callback(timer_callback_args_t* p_args);

/****************************************************************************/
/* Public Functions */
/****************************************************************************/

AutoAnalog::AutoAnalog()
{

    adcReady = false;
    adcBitsPerSample = 8;
    dacBitsPerSample = 8;
    autoAdjust = true;
    for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
        dacBuffer[i] = 0;
    }

    aSize = MAX_BUFFER_SIZE;
    aCtr = 0;
    sampleCounter = 0;
    sampleRateADC = 16000;
    sampleRateDAC = 16000;
    stereoVar = 0;
    maxBufferSize = MAX_BUFFER_SIZE;
    dacBuffersAllocated = false;
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

    stereoVar = stereo;
    
    if (enableDAC == 3) {
        high_speed_timer.stop();
        high_speed_timer.end();
        high_speed_timer.close();
        
        uint8_t timer_type = GPT_TIMER;

        high_speed_timer.begin(
            TIMER_MODE_PERIODIC,     // Fire continuously at the specified interval
            timer_type,              // Use the General PWM Timer peripheral block
            high_speed_timer_index,  // Selected hardware channel channel
            sampRate * (stereo + 1), // Target Frequency in Hz (16 kHz)
            0.0,                     // Duty cycle (unused for generic interrupts, keep 0.0)
            dac_callback             // Name of your ISR function to execute
        );

        high_speed_timer.setup_overflow_irq();
        high_speed_timer.open();
        high_speed_timer.start();
    }
    if (enableADC == 3) {
        adc_timer.stop();
        adc_timer.end();
        adc_timer.close();
        
        uint8_t timerType = GPT_TIMER;

        adc_timer.begin(TIMER_MODE_PERIODIC, timerType, adc_timer_index, sampRate * (stereo + 1), 0.0f, my_adc_isr);
        adc_timer.setup_overflow_irq();
        adc_timer.open();
        adc_timer.start();
    }
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
    while (R_ADC0->ADCSR_b.ADST) {
    }

    R_ADC0->ADANSA[0] = 1 << pinAx;

    analogChannel = pinAx;

    adc_timer.start();
}

/****************************************************************************/

void AutoAnalog::disableAdcChannel(uint8_t pinAx)
{

    adc_timer.stop();

    R_ADC0->ADCSR_b.ADST = 0;
    while (R_ADC0->ADCSR_b.ADST) {
    }

    R_ADC0->ADANSA[0] = 0;
}

/****************************************************************************/

void AutoAnalog::getADC(uint32_t samples)
{
    uint32_t timeout = millis();
    while (!adcReady) { 
      if(millis() - 1000 > timeout){
        break;
      }
    }

    adcReady = false;
    aSize = samples;
}

/****************************************************************************/

void AutoAnalog::feedDAC(uint8_t dacChannel, uint32_t samples, bool startInterrupts)
{

    if (enableDAC == 3) {

        if (dacDisabled) {
            R_DAC->DACR = 0x5F;
            setSampleRate(sampleRateDAC, stereoVar);
            dacDisabled = false;
        }

        if (aCtr == 1) {
            if (dacBitsPerSample == 8) {
                for (int i = 0; i < samples; i++) {
                    dacBuf0[i] = dacBuffer[i] << 4;
                }
            }
            else if (dacBitsPerSample == 16) {
                for (int i = 0; i < samples; i++) {
                    dacBuf0[i] = (dacBuffer16[i] >> 4) + 2048;
                }
            }
        }
        else {
            if (dacBitsPerSample == 8) {
                for (int i = 0; i < samples; i++) {
                    dacBuf1[i] = dacBuffer[i] << 4;
                }
            }
            else if (dacBitsPerSample == 16) {
                for (int i = 0; i < samples; i++) {
                    dacBuf1[i] = (dacBuffer16[i] >> 4) + 2048;
                }
            }
        }

        uint32_t timeout = millis();
        while (sCounter < samples) {
            if(millis() - 1000 > timeout){
                break;
            }
        }
        aCtr = (aCtr + 1) % 2;
        aSize = samples;
        sCounter = 0;
    }
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
}

    /****************************************************************************/

void AutoAnalog::adcSetup(void)
{

    if (enableADC == 3) {

        analogReadResolution(14);
        pinMode(15 + analogChannel, INPUT);
        analogRead(15 + analogChannel);
        //analogReference(AR_INTERNAL);

        R_MSTP->MSTPCRD &= ~(1UL << 16); // MSTPD16 = 0 -> ADC140 enabled
        __asm volatile("nop");
        __asm volatile("nop");

        R_ADC0->ADCSR_b.ADST = 0;
        while (R_ADC0->ADCSR_b.ADST) {
        }

        R_ADC0->ADCSR = 0x8000;
        R_ADC0->ADANSA[0] = 1 << analogChannel;

        R_ADC0->ADSSTR[analogChannel] = 12;

        uint8_t timerType = GPT_TIMER;
        adc_timer_index = FspTimer::get_available_timer(timerType);
        if (adc_timer_index < 0) {
            FspTimer::force_use_of_pwm_reserved_timer();
            timerType = GPT_TIMER;
            adc_timer_index = FspTimer::get_available_timer(timerType);
        }
        adc_timer.begin(TIMER_MODE_PERIODIC, timerType, adc_timer_index, sampleRateADC, 0.0f, my_adc_isr);
        adc_timer.setup_overflow_irq();
        adc_timer.open();
        adc_timer.start();
    }
}

/****************************************************************************/

void AutoAnalog::adcInterrupts(bool enabled)
{
}

/****************************************************************************/

void AutoAnalog::dacSetup(void)
{

    if (enableDAC == 3) {

        analogWriteResolution(12);
        analogWrite(A0, 0);

        R_DAC->DACR = 0x5F;      // enable
        R_DAC->DADPR = 0;        // Right justified
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
            TIMER_MODE_PERIODIC,    // Fire continuously at the specified interval
            timer_type,             // Use the General PWM Timer peripheral block
            high_speed_timer_index, // Selected hardware channel channel
            sampleRateDAC,                // Target Frequency in Hz (16 kHz)
            0.0,                    // Duty cycle (unused for generic interrupts, keep 0.0)
            dac_callback            // Name of your ISR function to execute
        );

        high_speed_timer.setup_overflow_irq();
        high_speed_timer.open();
        high_speed_timer.start();
    }
}

/****************************************************************************/

void AutoAnalog::disableDAC(bool withinTask)
{
    if (enableDAC == 3) {
        high_speed_timer.stop();
        high_speed_timer.end();
        high_speed_timer.close();
        R_DAC->DACR = 0x4F;
        dacDisabled = true;
    }
    else if (enableDAC == 2) {
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
}
}

/****************************************************************************/

void dac_callback(timer_callback_args_t* p_args)
{
    if (AutoAnalog::sCounter >= AutoAnalog::aSize) {
        AutoAnalog::sCounter++;
        return;
    }else{
        if (AutoAnalog::aCtr == 0) {
            R_DAC->DADR[0] = AutoAnalog::dacBuf0[AutoAnalog::sCounter];
        }
        else {
            R_DAC->DADR[0] = AutoAnalog::dacBuf1[AutoAnalog::sCounter];
        }
    }
    AutoAnalog::sCounter++;    
}

extern "C" void my_adc_isr(timer_callback_args_t* p_args)
{

    (void)p_args;

    if (AutoAnalog::adcWhichBuf == 0) {
        AutoAnalog::adcBuf0[AutoAnalog::sampleCounter] = R_ADC0->ADDR[AutoAnalog::analogChannel];
    }
    else {
        AutoAnalog::adcBuf1[AutoAnalog::sampleCounter] = R_ADC0->ADDR[AutoAnalog::analogChannel];
    }

    AutoAnalog::sampleCounter++;

    if (AutoAnalog::sampleCounter >= AutoAnalog::aSize && AutoAnalog::adcReady == false) {
        if (AutoAnalog::adcWhichBuf == 0) {
            if (AutoAnalog::adcBitsPerSample == 8) {
                for (uint32_t i = 0; i < AutoAnalog::aSize; i++) {
                    AutoAnalog::adcBuffer[i] = AutoAnalog::adcBuf0[i] >> 6;
                }
            }
            else {
                for (uint32_t i = 0; i < AutoAnalog::aSize; i++) {
                    int16_t sample = AutoAnalog::adcBuf0[i] - 8192;
                    AutoAnalog::adcBuffer16[i] = sample << 2;
                }
            }
        }
        else {
            if (AutoAnalog::adcBitsPerSample == 8) {
                for (uint32_t i = 0; i < AutoAnalog::aSize; i++) {
                    AutoAnalog::adcBuffer[i] = AutoAnalog::adcBuf1[i] >> 6;
                }
            }
            else {
                for (uint32_t i = 0; i < AutoAnalog::aSize; i++) {
                    int16_t sample = AutoAnalog::adcBuf1[i] - 8192;
                    AutoAnalog::adcBuffer16[i] = sample << 2;
                }
            }
        }
        AutoAnalog::adcReady = true;
        AutoAnalog::sampleCounter = 0;
        AutoAnalog::adcWhichBuf = !AutoAnalog::adcWhichBuf;
    }else
    if (AutoAnalog::sampleCounter >= AutoAnalog::aSize){
        AutoAnalog::sampleCounter = 0;
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