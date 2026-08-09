
#include <AutoAnalogAudio.h>
#include <RF24.h>

AutoAnalog aaAudio;
RF24 radio(7,8);

uint8_t address[6] = { "1Node" };
#define bufferSize 16

void setup() {
  Serial.begin(115200);
  while(!Serial){}
  Serial.println("Analog Audio Begin");

  radio.begin();
  radio.setChannel(23);
  radio.setPayloadSize(32);
  radio.setAutoAck(0);
  radio.openReadingPipe(1, address);
  radio.startListening();

  aaAudio.begin(0, 3);  //Setup aaAudio using DAC and ADC
  aaAudio.dacBitsPerSample = 16;
  aaAudio.adcBitsPerSample = 16;
  aaAudio.setSampleRate(44100);

}

uint32_t timer = millis();

void loop() {
  
  if(radio.available()){ 
    if(millis() - timer > 1000){
      Serial.println("Data received");
      timer = millis();
    }
    radio.read(aaAudio.dacBuffer16, bufferSize * 2);  // Each 16-bit sample takes up 2 bytes
    aaAudio.feedDAC(0,bufferSize);
  }
}
