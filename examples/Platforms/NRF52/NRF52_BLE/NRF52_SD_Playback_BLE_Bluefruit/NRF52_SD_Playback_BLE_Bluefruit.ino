/* Arduino BLE controlled Audio Player for nRF52
 * 
 * This is an example of me playing around with BLE control and different
 * services/characteristics to test the AutoAnalogAudio library.
 *
 * Requirements:
 * 1. nRF52 Device (Tested on nRF52840)
*  2. SD Card with WAV files: 8-bit, 16-24kHz, Mono
 * 3. I2S or Analog Amplifier + Speaker connected
 * 4. Mobile device or 'other' with nRF Connect installed
 *
 * Connect via nRF Connect App:
 * 1. Device should come up as BLE Audio Player
 * 2. You should see:
 *   a: Common Audio
*    b: Audio Input Type:
      Send a UTF-8 String to play a file: myfileDirectory/myfilename.wav
 *   c: Audio Input Control Point:
      Send an Unsigned value between 0-10 to set the volume low-high
 */


#include <SPI.h>
#include <SD.h>
#include <bluefruit.h>
#include <AutoAnalogAudio.h>

AutoAnalog aaAudio;

/************** USER CONFIG ***********/
// File to play on startup
const char* audioFilename = "calibrat.wav";  // 8-bit @ 24kHz audio is the max over SD card while BLE is running
uint8_t SD_CS_PIN = 2;                       // Set this to your CS pin for the SD card/module
#define USE_I2S 1

/*********************************************************/
/* Tested with MAX98357A I2S breakout
/* BCLK connected to Arduino D1 (p0.03)
/* LRCK connected to Arduino D3 (p0.29)
/* DIN  connected to Arduino D5 (p0.05)
/* SD   connected to Arduino D6 (p1.11)
/*********************************************************/

char songName[64];
float volumeControl = 0.2;
#define AUDIO_BUFFER_SIZE 1600

BLEService audioService = BLEService(0x1853);

BLECharacteristic audioDataCharacteristic = BLECharacteristic(0x2b79);
BLECharacteristic audioVolumeCharacteristic = BLECharacteristic(0x2b7b);

uint8_t beaconUuid[16] = {
  0x01, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78,
  0x89, 0x9a, 0xab, 0xbc, 0xcd, 0xde, 0xef, 0xf0
};
BLEBeacon beacon(beaconUuid, 1, 2, -54);


void setup() {

  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.print("Init SD card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("init failed!");
    return;
  }
  Serial.println("SD init ok");

  Bluefruit.begin();
  Bluefruit.setName("BLE Audio Player");
  audioService.begin();

  audioDataCharacteristic.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE);
  audioDataCharacteristic.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  audioDataCharacteristic.setMaxLen(64);
  audioDataCharacteristic.setWriteCallback(playBack);
  audioDataCharacteristic.begin();

  audioVolumeCharacteristic.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE);
  audioVolumeCharacteristic.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  audioVolumeCharacteristic.setMaxLen(1);
  audioVolumeCharacteristic.setWriteCallback(volume);
  audioVolumeCharacteristic.begin();

  Bluefruit.Advertising.setBeacon(beacon);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(160, 160);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  

  pinMode(6, OUTPUT);  //Connected to SD pin of MAX98357A
  digitalWrite(6, HIGH);

  aaAudio.begin(0, 1, USE_I2S);
  playAudio(audioFilename);
}

void loop() {

  loadBuffer();

  // Control via Serial for testing
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '=') {
      volumeControl += 0.1;
    } else if (c == '-') {
      volumeControl -= 0.1;
      volumeControl = max(0.0, volumeControl);
    } else if (c == 'p') {
      playAudio("brick/brick24.wav");
    }
    Serial.println(volumeControl);
  }

}

void playBack(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  memset(songName, 0, sizeof(songName));
  memcpy(songName,data,len);
  playAudio(songName);
  Serial.println(songName);
}

void volume(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  volumeControl = data[0] / 10.0;
  Serial.print("BLE Set Volume: ");
  Serial.println(volumeControl);
}


/*********************************************************/
/* A simple function to handle playing audio files
/*********************************************************/

File myFile;

void playAudio(const char* audioFile) {

  if (myFile) {
    myFile.close();
  }
  //Open the designated file
  myFile = SD.open(audioFile);

  myFile.seek(22);
  uint16_t var;
  uint32_t var2;
  myFile.read(&var, 2);   // Get channels (Stereo or Mono)
  myFile.read(&var2, 4);  // Get Sample Rate
  aaAudio.setSampleRate(var2, var - 1);

  myFile.seek(34);
  myFile.read(&var, 2);  // Get Bits Per Sample
  aaAudio.dacBitsPerSample = var;

  myFile.seek(44);  //Skip past the WAV header
}

void loadBuffer() {

  if (myFile.available() > AUDIO_BUFFER_SIZE) {

    if (aaAudio.dacBitsPerSample == 8) {
      myFile.read(aaAudio.dacBuffer, AUDIO_BUFFER_SIZE);
      for (uint32_t i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        aaAudio.dacBuffer[i] *= volumeControl;
      }
      aaAudio.feedDAC(0, AUDIO_BUFFER_SIZE);
    } else {
      myFile.read(aaAudio.dacBuffer16, AUDIO_BUFFER_SIZE);
      for (uint32_t i = 0; i < AUDIO_BUFFER_SIZE / 2; i++) {
        int16_t sample = aaAudio.dacBuffer16[i];
        sample *= volumeControl;
        aaAudio.dacBuffer16[i] = (uint16_t)sample;
      }
      aaAudio.feedDAC(0, AUDIO_BUFFER_SIZE / 2);
    }

  } else {
    myFile.seek(44);
  }
}

