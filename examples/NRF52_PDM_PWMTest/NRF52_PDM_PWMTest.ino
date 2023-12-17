#include <AutoAnalogAudio.h>
AutoAnalog aaAudio;

// REQUIRES MBed Enabled Core for NRF52 (XIAO 52840 Sense)

void setup() {
  
  //Startup the PDM Microphone on pin A0 and PWM(pseudo DAC) on pin 5
  aaAudio.begin(1, 1);
  aaAudio.autoAdjust = 0;
  aaAudio.adcBitsPerSample = 16; // 16-bit audio at 16khz is the default on NRF52 and cannot be modified currently (in progress)
  aaAudio.setSampleRate(16000);

}

void loop() {

  aaAudio.getADC(320); // Get 320 Samples from the ADC
  for (int i = 0; i < 320; i++) {  // Copy them into the DAC Buffer and change from signed to unsigned ( + 0x8000)
    aaAudio.dacBuffer16[i] = (uint16_t)(aaAudio.adcBuffer16[i] + 0x8000);
  }
  aaAudio.feedDAC(320); // Feed the DAC with the ADC data
}
