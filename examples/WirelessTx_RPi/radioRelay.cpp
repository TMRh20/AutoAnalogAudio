/*
TMRh20 2016

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/** RPi audio broadcast to RF24Audio nodes
 *
 * Reads from WAV file and broadcasts audio
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <RF24/RF24.h>
#include <unistd.h>
#include <fstream>

using namespace std;

/****************** Raspberry Pi ***********************/

RF24 radio(25,0);

// File should be 16 khz sample rate, mono, wav format
char audioFile[] = "Guitar8b16kMono.wav";

// *** Set the radio to RF24_2MBPS below and in the example to use this:
//char audioFile[] = "Guitar8b24kMono.wav";

/**************************************************************/

// The addresses used in RF24Audio
const uint64_t pipes[14] = { 0xABCDABCD71LL, 0x544d52687CLL, 0x544d526832LL, 0x544d52683CLL,0x544d526846LL, 0x544d526850LL,0x544d52685ALL, 0x544d526820LL, 0x544d52686ELL, 0x544d52684BLL, 0x544d526841LL, 0x544d526855LL,0x544d52685FLL,0x544d526869LL};
unsigned int sz = 0;
void intHandler();
uint32_t reads = 0,writes = 0;
uint8_t bufR[32];

int main(int argc, char** argv){

  cout << "RF24Audio On Rpi\n";

  // Configure radio settings for RF24Audio
  radio.begin(); 
  radio.setChannel(1);
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_1MBPS);
  radio.setAutoAck(0); 
  radio.setCRCLength(RF24_CRC_8);
  radio.setAddressWidth(5);

  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  radio.startListening();
  radio.txDelay = 0;
  radio.printDetails();
  radio.stopListening();
  
  radio.maskIRQ(0,1,0);
  radio.stopListening();
  
  attachInterrupt(24, INT_EDGE_FALLING, intHandler);
  
  uint32_t sampleDelay = 2000;
  
  // Read the entire wav file into memory
  streampos size;
  char * memblock;
  
  ifstream file (audioFile, ios::in|ios::binary|ios::ate);
  if (file.is_open()){
      
    size = file.tellg();//file.tellg();
    sz = file.tellg();
    printf("%d\n",sz);
    memblock = new char [sz];
    file.seekg (0, ios::beg);
    file.read (memblock, sz);
    file.close();
    cout << "the entire file content is in memory\n";   
    
  }else{
      exit(1);
  }
  
    uint16_t numChannels = memblock[22];
    numChannels |= memblock[23];
    
  // Get the sample rate of the wav file
    uint32_t sampleRate = memblock[24];
    sampleRate |= memblock[25] << 8;
    sampleRate |= memblock[26] << 16;
    sampleRate |= memblock[27] << 24;    
    cout << "Sample Rate " << sampleRate << "\n";      
    
    // Calculate the required delay based on sample rate
    sampleDelay = (1000000.0 / sampleRate * 32) - 1;
    cout << "Sample Delay " << sampleDelay << "\n";    
    
    
    // Data starts at byte 44, after the .wav header as long as there is no metadata
    int32_t filePos = 44;    
    cout << "Output via radio...\n";    
    
    sampleDelay /= numChannels;
    sampleRate *= numChannels;
    sampleDelay-=80;//620 for 2MBPS, 790 for 1MBPS
   
    
    //radio.flush_tx();
    radio.setAutoAck(0,1);
    radio.openWritingPipe(pipes[2]);
    radio.writeFast(&sampleRate,4);
    radio.txStandBy();
    radio.setAutoAck(0,0);
    radio.openWritingPipe(pipes[1]);
    delay(50); //Give the recipient a ms or two to process and setup timers, DAC, ADC
    uint32_t dTimer = 0;
    
    while (1){        
     
      radio.stopListening();
      radio.writeFast(&memblock[filePos],32,1);
      ++writes;

      delayMicroseconds(sampleDelay);
      filePos+=32;
      if(filePos >= sz){
        break;          
      }
      if(millis()-dTimer>1000){
        dTimer=millis();
	    printf("%d %d\n",reads,writes);
        reads=writes=0;
        printf("%d %d %d %d\n",bufR[0],bufR[1],bufR[2],bufR[3]);
      }
} // loop

cout << "Complete, exit...\n";  
delete[] memblock;

} // main




void intHandler(){

    if(radio.available()){
      radio.read(&bufR,32);
      ++reads;      
    }else{
      radio.txStandBy();
      radio.startListening();
    }
}


