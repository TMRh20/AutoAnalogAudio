
uint8_t channelSelection = 2;
uint32_t loadBuffer();

/*********************************************************/


/*********************************************************/

void DACC_Handler(void) {

  aaAudio.dacHandler();                   //Link the DAC ISR/IRQ to the library. Called by the MCU when DAC is ready for data
  uint32_t samples = loadBuffer();
  if (samples) {
    aaAudio.feedDAC(channelSelection, samples);
  }

}

/*********************************************************/
/* Function to open the audio file, seek to starting position and enable the DAC */
uint32_t endPosition = 0;

void playAudio(const char *audioFile) {

  uint32_t sampleRate = 16000;
  uint16_t numChannels = 1;
  uint16_t bitsPerSample = 8;
  uint32_t dataSize = 0;
  uint32_t startPosition = 44;

  Serial.println("Play");

  if (myFile) {
    Serial.println("Close Current");
    //Ramp in and ramp out functions prevent popping and clicking when starting/stopping playback
    aaAudio.rampOut(0);
    aaAudio.disableDAC();
    myFile.close();

    /*if(aaAudio.dacBitsPerSample == 8){
      aaAudio.rampOut(aaAudio.dacBuffer[MAX_BUFFER_SIZE-1]);
    }else{
      aaAudio.rampOut(aaAudio.dacBuffer16[MAX_BUFFER_SIZE-1]);
    }*/
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
    endPosition = dataSize + 44;

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

    if (myFile.size() > endPosition) {
      //startPosition = myFile.size() - dataSize;
      endPosition = dataSize + 44;
      myFile.seek(endPosition);
      uint8_t buf[myFile.size() - (endPosition)];
      myFile.read(buf, myFile.size() - (endPosition));
      Serial.println("Metadata:");
      Serial.println(myFile.size() - (endPosition));
      for (int i = 0; i < myFile.size() - (endPosition); i++) {
        Serial.print((char)buf[i]);
      }
      Serial.println();
    }

    if (bitsPerSample > 12) {
      bitsPerSample = 16;
    } else if (bitsPerSample > 10 ) {
      bitsPerSample = 12;
    } else if (bitsPerSample > 8) {
      bitsPerSample = 10;
    } else {
      bitsPerSample = 8;
    }

    sampleRate *= numChannels;
    Serial.print("SampleRate ");
    Serial.println(sampleRate);
    Serial.println("Set smp rate");
    bool stereo = numChannels > 1 ? true : false;
    aaAudio.setSampleRate(sampleRate, stereo);
    aaAudio.dacBitsPerSample = bitsPerSample;

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

    if (aaAudio.dacBitsPerSample == 8) {
      aaAudio.rampIn(aaAudio.dacBuffer[0]);
    } else {
      aaAudio.rampIn((uint8_t)aaAudio.dacBuffer16[0]);
    }

    aaAudio.feedDAC(channelSelection, MAX_BUFFER_SIZE, useTasks);

  } else {
#if defined (AUDIO_DEBUG)
    Serial.print("Failed to open ");
    Serial.println(audioFile);
#endif
  }
}

/*********************************************************/
/* Function called from DAC interrupt after dacHandler(). Loads data into the dacBuffer */

uint32_t loadBuffer() {

  uint32_t samplesToRead = 0;
  uint32_t metaDataSize = myFile.size() - endPosition;


  if (myFile) {
    if (myFile.available() > metaDataSize + 1 ) {
      samplesToRead = MAX_BUFFER_SIZE;
      size_t availableBytes = 0;

      if (aaAudio.dacBitsPerSample == 8) {
        //Load 32 samples into the 8-bit dacBuffer
        if ( (availableBytes = (myFile.available() - metaDataSize)) <= samplesToRead) {
          samplesToRead = availableBytes;
          Serial.print("File Size ");
          Serial.print(myFile.size());
          Serial.print(" Bytes Read ");
          Serial.println(myFile.position() + samplesToRead);
        }
        myFile.read(aaAudio.dacBuffer, samplesToRead);
        for (int i = 0; i < samplesToRead; i++) {
          int16_t tmpVar = (uint16_t)aaAudio.dacBuffer[i] - 0x80;
          tmpVar = (tmpVar / volumeVar) + 0x80;
          //int16_t tmpInt = ( ((int16_t)aaAudio.dacBuffer[i]) - 127) / volumeVar;
          aaAudio.dacBuffer[i] = tmpVar;
        }

      } else {
        if ( (availableBytes = (myFile.available() - metaDataSize)) <= (samplesToRead * 2) ) {
          samplesToRead = availableBytes / 2;
          Serial.print("File Size16 ");
          Serial.print(myFile.size());
          Serial.print(" Bytes Read ");
          Serial.println(myFile.position() + availableBytes);
        }
        //Load 32 samples (64 bytes) into the 16-bit dacBuffer
        int16_t tmpBuffer[samplesToRead];
        myFile.read((byte*)tmpBuffer, samplesToRead * 2);
        //Convert the 16-bit samples to 12-bit
        for (int i = 0; i < samplesToRead; i++) {
          /*if(i&0x01){
            uint16_t tmp = aaAudio.dacBuffer16[i] >> volumeVar;
            aaAudio.dacBuffer16[i] = tmp + (tmp>>1);
          }else{
            aaAudio.dacBuffer16[i] >>= volumeVar;
          }*/
          tmpBuffer[i] /= volumeVar;
          aaAudio.dacBuffer16[i] = (tmpBuffer[i] + 0x8000) >> 8;
          //aaAudio.dacBuffer16[i] = aaAudio.dacBuffer16[i] >> volumeVar;
        }
      }
    } else {
#if defined (AUDIO_DEBUG)
      Serial.print("Close: ");
#endif
      //aaAudio.disableDAC();
      //aaAudio.dacInterrupts(false);

      Serial.println("File close");
      myFile.close();
      Serial.print("Dis DAC, ");
      aaAudio.rampOut(0);
      //If using tasks, disabling the active task and DAC will be done from within the task itself
      //Need to let aaAudio know by setting the parameter to 'true'. Using useTasks variable
      aaAudio.disableDAC(useTasks);

      /*if(useTasks){
        Serial.println(" Task Del");
        vTaskDelete(aaAudio.dacTaskHandle);
      }*/

      /*if(aaAudio.dacBitsPerSample == 8){
        aaAudio.rampOut(aaAudio.dacBuffer[MAX_BUFFER_SIZE-1]);
      }else{
        aaAudio.rampOut(aaAudio.dacBuffer16[MAX_BUFFER_SIZE-1]);
      }*/



    }
  }

  return samplesToRead;
}

/*********************************************************/
