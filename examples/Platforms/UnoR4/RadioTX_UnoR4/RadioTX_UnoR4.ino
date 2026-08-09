
#include <AutoAnalogAudio.h>
#include <RF24.h>

AutoAnalog aaAudio;
RF24 radio(7,8);

uint8_t address[6] = { "1Node" };

#define bufferSize 16

void setup() {
  Serial.begin(115200);
  Serial.println("Analog Audio Begin");

  //Set pins low for MAX9814 microphone AR and GAIN pins
  pinMode(5,OUTPUT);
  digitalWrite(5,LOW);

  pinMode(6,OUTPUT);
  digitalWrite(6,LOW);

  radio.begin();
  radio.setChannel(23);
  radio.setPayloadSize(32);
  radio.setAutoAck(0);
  radio.openWritingPipe(address);
  radio.stopListening();

  aaAudio.begin(3, 0);  //Setup aaAudio using DAC and ADC
  aaAudio.dacBitsPerSample = 16;
  aaAudio.adcBitsPerSample = 16;
  aaAudio.setSampleRate(44100);
  radio.printDetails();
}

void loop() {
  
  aaAudio.getADC(bufferSize); // Get buffersize of 16-bit samples
  radio.writeFast(aaAudio.adcBuffer16, bufferSize * 2);  // Each 16-bit sample takes up 2 bytes

}
