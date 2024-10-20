/* Example of reading audio from SD card and sending over radio
 * Note: This example uses I2S output. Modify the USE_I2S #define
 * below to enable PWM output.
 */ 

#include <SPI.h>
#include <SD.h>
#include <AutoAnalogAudio.h>
#include <nrf_to_nrf.h>

AutoAnalog aaAudio;
nrf_to_nrf radio;

uint8_t address[][6] = { "1Node", "2Node" };
#define BUFFER_SIZE 125
#define USE_I2S 1 // Change to 0 to enable PWM output instead of I2S

/*********************************************************/
/* Tested with MAX98357A I2S breakout on XIAO 52840 Sense
/* BCLK connected to Arduino D1 (p0.03)
/* LRCK connected to Arduino D3 (p0.29)
/* DIN  connected to Arduino D5 (p0.05)
/* SD   connected to Arduino D6 (p1.11)
/*********************************************************/

void setup() {

  Serial.begin(115200);

  radio.begin();
  radio.setPayloadSize(BUFFER_SIZE);
  radio.setAutoAck(0);
  radio.setDataRate(NRF_2MBPS);
  radio.openWritingPipe(address[1]);
  radio.stopListening();

  Serial.print("Init SD card...");
  if (!SD.begin(10000000, 2)) {
    Serial.println("init failed!");
    return;
  }
  Serial.println("init ok");
  Serial.println("Analog Audio Begin");
  aaAudio.begin(0, 1, USE_I2S);  //Setup aaAudio using DAC and I2S

  //Setup for audio: Use 8-bit, mono WAV files @ 16kHz
  aaAudio.dacBitsPerSample = 8;     // 8-bit
  aaAudio.setSampleRate(16000, 0);  // 16khz, mono

  pinMode(6, OUTPUT);  //Connected to SD pin of MAX98357A
  digitalWrite(6, HIGH);

  playAudio("guitar/g8b16km.wav");  // 16kHz @ 8-bits is about the maximum when reading from SD card & streaming over radio
}

void loop() {

  loadBuffer();
}

/*********************************************************/
/* A simple function to handle playing audio files
/*********************************************************/

File myFile;

void playAudio(const char *audioFile) {

  if (myFile) {
    myFile.close();
  }
  //Open the designated file
  myFile = SD.open(audioFile);
  //Skip past the WAV header
  myFile.seek(44);
}

void loadBuffer() {

  if (myFile.available()) {
    myFile.read(aaAudio.dacBuffer, BUFFER_SIZE);  // Change this to dacBuffer16 for 16-bit samples
    aaAudio.feedDAC(0, BUFFER_SIZE);
    radio.startWrite(&aaAudio.dacBuffer[0], BUFFER_SIZE, 1);
  } else {
    myFile.seek(44);
  }
}
