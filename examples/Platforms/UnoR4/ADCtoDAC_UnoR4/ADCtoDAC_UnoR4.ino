
#include <AutoAnalogAudio.h>
AutoAnalog aaAudio;

#define bufferSize 256

void setup() {
  Serial.begin(115200);
  Serial.println("Analog Audio Begin");

  //Set pins low for MAX9814 microphone AR and GAIN pins
  pinMode(7,OUTPUT);
  digitalWrite(7,LOW);

  pinMode(8,OUTPUT);
  digitalWrite(8,LOW);


  aaAudio.begin(3, 3);  //Setup aaAudio using DAC and ADC
  aaAudio.dacBitsPerSample = 16;
  aaAudio.adcBitsPerSample = 16;
  aaAudio.setSampleRate(44100);

}

void loop() {
  
  aaAudio.getADC(bufferSize);

  // Copy our 14-bit ADC data to the DAC buffer which is only 12-bit
  // The dac input expects signed 12-bit samples
  for(int i=0; i<bufferSize; i++){
    aaAudio.dacBuffer16[i] = (aaAudio.adcBuffer16[i] ^ 0x8000) >> 2;  
  }
  aaAudio.feedDAC(0,bufferSize);
}
