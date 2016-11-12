

#include <RF24.h>

RF24 radio(46,52);

const uint64_t pipes[14] = { 0xABCDABCD71LL, 0x544d52687CLL, 0x544d526832LL };

bool tx,fail,rx;
uint8_t pipeNo = 0;

void RX();

void setupRadio(){
  
  radio.begin();
  radio.setChannel(1);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.setAutoAck(0);
  radio.setCRCLength(RF24_CRC_8);
  radio.setAddressWidth(5);
  radio.openReadingPipe(1,pipes[1]);
  radio.openReadingPipe(2,pipes[2]);
  radio.setAutoAck(2,1);
  radio.openWritingPipe(pipes[0]);
  radio.txDelay = 0;
  radio.csDelay = 0;
  radio.maskIRQ(0,1,0);
  radio.printDetails();

  radio.startListening();

  attachInterrupt(digitalPinToInterrupt(4),RX,FALLING);
  
}




