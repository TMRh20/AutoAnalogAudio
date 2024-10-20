/* Example of receiving audio over radio and playing it back. 
 * Note: This example uses I2S output. Modify the USE_I2S #define
 * below to enable PWM output.
 */ 

#include <AutoAnalogAudio.h>
#include <nrf_to_nrf.h>

AutoAnalog aaAudio;
nrf_to_nrf radio;

uint8_t address[][6] = { "1Node", "2Node" };
#define BUFFER_SIZE 125
#define USE_I2S 0 // Change to 1 to enable I2S output instead of PWM

void setup() {

  Serial.begin(115200);

  radio.begin();
  radio.setPayloadSize(BUFFER_SIZE);
  radio.setAutoAck(0);
  radio.setDataRate(NRF_2MBPS);
  radio.openReadingPipe(1,address[1]);
  radio.startListening();

  Serial.println("Analog Audio Begin");
  aaAudio.begin(0, 1, USE_I2S);  //Setup aaAudio using DAC and PWM. Change the third value to a 1 to enable I2S

  //Setup for audio: Use 8-bit, mono WAV files @ 16kHz
  aaAudio.dacBitsPerSample = 8;    // 8-bit
  aaAudio.setSampleRate(16000, 0); // 16khz, mono

  pinMode(6,OUTPUT); // For I2S output: Connected to SD pin of MAX98357A
  digitalWrite(6,HIGH);

}

void loop() {
  
  if(radio.available()){
    radio.read(&aaAudio.dacBuffer[0],BUFFER_SIZE);
    aaAudio.feedDAC(0,BUFFER_SIZE);
  }

}
