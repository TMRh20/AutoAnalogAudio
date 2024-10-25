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
#include <ArduinoBLE.h>
#include <AutoAnalogAudio.h>

AutoAnalog aaAudio;

/************** USER CONFIG ***********/
// File to play on startup
const char* audioFilename = "far8b16k.wav";  // 8-bit @ 24kHz audio is the max over SD card while BLE is running
uint8_t SD_CS_PIN = 2;                       // Set this to your CS pin for the SD card/module
#define USE_I2S 1                            // Set this to 0 for analog (PWM) audio output instead of I2S

/*********************************************************/
/* Tested with MAX98357A I2S breakout
/* BCLK connected to Arduino D1 (p0.03)
/* LRCK connected to Arduino D3 (p0.29)
/* DIN  connected to Arduino D5 (p0.05)
/* SD   connected to Arduino D6 (p1.11)
/*********************************************************/

#define FILENAME_BUFFER_LENGTH 64
char songName[FILENAME_BUFFER_LENGTH];
float volumeControl = 0.2;
#define AUDIO_BUFFER_SIZE 1600

BLEService audioService("1853");  // BLE LED Service

// BLE Audio Charactaristic
BLECharacteristic audioDataCharacteristic("2b79", BLERead | BLEWrite, FILENAME_BUFFER_LENGTH);
BLEByteCharacteristic audioVolumeCharactaristic("2b7b", BLERead | BLEWrite);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  aaAudio.begin(0, 1, USE_I2S);  //Setup aaAudio using DAC and I2S or PWM

  // BLE initialization
  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1) {};
  }

  BLE.setLocalName("BLE Audio Player");
  BLE.setAdvertisedService(audioService);

  audioService.addCharacteristic(audioDataCharacteristic);
  audioService.addCharacteristic(audioVolumeCharactaristic);
  BLE.addService(audioService);

  BLE.advertise();
  Serial.println("BLE Peripheral is now advertising");

  Serial.print("Init SD card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("init failed!");
    return;
  }
  Serial.println("SD init ok");
  pinMode(6, OUTPUT);  //Connected to SD pin of MAX98357A
  digitalWrite(6, HIGH);

  playAudio(audioFilename);
}

void loop() {

  BLEDevice central = BLE.central();

  if (central) {

    if (central.connected()) {
      if (audioDataCharacteristic.written()) {
        memset(songName, 0, sizeof(songName));
        audioDataCharacteristic.readValue((uint8_t*)songName, FILENAME_BUFFER_LENGTH);
        playAudio(songName);
        Serial.println(songName);
      }
      if (audioVolumeCharactaristic.written()) {
        uint8_t vol;
        audioVolumeCharactaristic.readValue(vol);
        volumeControl = vol / 10.0;
        Serial.print("BLE Set Volume: ");
        Serial.println(volumeControl);
      }
    }
  }

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

  if (myFile.available()) {

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
