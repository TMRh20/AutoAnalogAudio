
File myFile;

/*******************************************/
void stopPlayback() {
  playbackStopped = true;
  if (myFile) {
    myFile.close();
  }
  aaAudio.disableDAC();
}

/*******************************************/

void playAudio(const char* audioFile) {

  stopPlayback();
  playbackStopped = false;

  myFile = SD.open(audioFile);

  if (!myFile) {
    Serial.print("Failed to open: ");
    Serial.println(audioFile);
    stopPlayback();
    return;
  }
  Serial.print("Open file ok: ");
  Serial.println(audioFile);

  myFile.seek(22);
  uint16_t var;
  uint32_t var2;
  myFile.read(&var, 2);   // Get channels (Stereo or Mono)
  myFile.read(&var2, 4);  // Get Sample Rate
  aaAudio.setSampleRate(var2, var - 1);
  Serial.print("Sample Rate: ");
  Serial.println(var2);
  Serial.print("Channels: ");
  Serial.println(var - 1 ? "stereo" : "mono");

  myFile.seek(34);
  myFile.read(&var, 2);  // Get Bits Per Sample
  aaAudio.dacBitsPerSample = var;

  Serial.print("BitsPerSample: ");
  Serial.println(var);

  myFile.seek(44);  //Skip past the WAV header
}

/*******************************************/

void loadBuffer() {

  if (playbackStopped) {
    return;
  }
  if (myFile.available()) {

    if (aaAudio.dacBitsPerSample == 8) {             // Handle 8-bit samples different from 16-bit or higher
      myFile.read(aaAudio.dacBuffer, BUFFER_SIZE);   // Read buffer_size bytes into the 8-bit buffer
      for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
        aaAudio.dacBuffer[i] *= volumeControl;       // Simple volume control
      }
      aaAudio.feedDAC(0, BUFFER_SIZE);               // Feed the DAC on channel 0 with our buffer
    } else {
      myFile.read(aaAudio.dacBuffer16, BUFFER_SIZE * 2); // Handle 16-bit samples, 2-bytes per sample, so double the buffer size in bytes
      for (uint32_t i = 0; i < BUFFER_SIZE; i++) {       // Simple volume control
        int16_t sample = aaAudio.dacBuffer16[i];         // Copy data from unsigned buffer in to signed variable, so its accurate
        sample *= volumeControl;                         // Multiply by volume control variable to lower volume
        aaAudio.dacBuffer16[i] = (uint16_t)sample;       // Set our unsigned buffer back to the signed variable value
      }
      aaAudio.feedDAC(0, BUFFER_SIZE);                   // Feed the DAC on channel 0 with our 16-bit buffer
    }

  } else {
    stopPlayback();
  }
}

/*******************************************/
