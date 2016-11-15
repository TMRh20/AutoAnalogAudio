
uint8_t channelSelection = 2;

void loadBuffer();

/*********************************************************/

void DACC_Handler(void) {
  aaAudio.feedDAC(channelSelection);      //Feed the DAC with the data loaded into the dacBuffer
  aaAudio.dacHandler();                   //Link the DAC ISR/IRQ to the library. Called by the MCU when DAC is ready for data
  loadBuffer();
}

/*********************************************************/
/* Function to open the audio file, seek to starting position and enable the DAC */

void playAudio(char *audioFile) {

  uint32_t sampleRate = 16000;
  uint16_t numChannels = 1;
  uint16_t bitsPerSample = 8;
  uint32_t dataSize = 0;
  uint32_t startPosition = 44;
  
  if (myFile) {
    aaAudio.disableDAC();
    myFile.close();
    //delay(25);
  }
  
  
  //Open the designated file
  myFile = SD.open(audioFile);
  
  if (myFile) {
    myFile.seek(22);
    myFile.read((byte*)&numChannels, 2);
    myFile.read((byte*)&sampleRate, 4);
    myFile.seek(34);
    myFile.read((byte*)&bitsPerSample, 2);
    myFile.seek(40);
    myFile.read((byte*)&dataSize, 4);
    dataSize += 44; //Set this variable to the total size of header + data

    #if defined (AUDIO_DEBUG)
      Serial.print("\nNow Playing ");
      Serial.println(audioFile);
      Serial.print("Channels ");
      Serial.print(numChannels);
      Serial.print(", SampleRate ");
      Serial.print(sampleRate);
      Serial.print(", BitsPerSample ");
      Serial.println(bitsPerSample);      
    #endif

    if(myFile.size() > dataSize){
      startPosition = myFile.size() - dataSize;
      Serial.println("Skipping metadata");      
    }

    if(bitsPerSample > 10 ){
       bitsPerSample = 12;
    }else
    if(bitsPerSample > 8){
       bitsPerSample = 10;
    }else{
       bitsPerSample = 8;
    }

    sampleRate *= numChannels;
    aaAudio.dacBitsPerSample = bitsPerSample;
    aaAudio.setSampleRate(sampleRate);

   #if defined (AUDIO_DEBUG)
      Serial.print("Timer Rate ");
      Serial.print(sampleRate);
      Serial.print(", DAC Bits Per Sample ");
      Serial.println(bitsPerSample); 
    #endif
    
    //Skip past the WAV header
    myFile.seek(startPosition);
    //Load one buffer
    loadBuffer();
    //Feed the DAC to start playback
    aaAudio.feedDAC();
  }else{
    #if defined (AUDIO_DEBUG)
      Serial.print("Failed to open ");
      Serial.println(audioFile);
    #endif
  }
}

/*********************************************************/
/* Function called from DAC interrupt after dacHandler(). Loads data into the dacBuffer */

void loadBuffer() {
  
  if (myFile) {
    if (myFile.available()) {
      if (aaAudio.dacBitsPerSample == 8) {
        //Load 32 samples into the 8-bit dacBuffer
        myFile.read((byte*)aaAudio.dacBuffer, MAX_BUFFER_SIZE);
      }else{
        //Load 32 samples (64 bytes) into the 16-bit dacBuffer
        myFile.read((byte*)aaAudio.dacBuffer16, MAX_BUFFER_SIZE * 2);
        //Convert the 16-bit samples to 12-bit
        for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
          aaAudio.dacBuffer16[i] = (aaAudio.dacBuffer16[i] + 0x8000) >> 4;
        }
      }
    }else{
      #if defined (AUDIO_DEBUG)
        Serial.println("File close");
      #endif
      myFile.close();
      aaAudio.disableDAC();
    }
  }
}

/*********************************************************/
