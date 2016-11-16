

/*********************************************************/

/* WAV HEADER STRUCTURE */
struct wavStruct {
 const char chunkID[4] = {'R','I','F','F'};
 uint32_t chunkSize = 36;                     //Size of (entire file in bytes - 8 bytes) or (data size + 36)
 const char format[4] = {'W','A','V','E'};
 const char subchunkID[4] = {'f','m','t',' '};
 const uint32_t subchunkSize = 16;
 const uint16_t audioFormat = 1;              //PCM == 1
 uint16_t numChannels = 1;                    //1=Mono, 2=Stereo
 uint32_t sampleRate = 11000;        
 uint32_t byteRate = 11000;                   //== SampleRate * NumChannels * BitsPerSample/8
 uint16_t blockAlign = 1;                     //== NumChannels * BitsPerSample/8
 uint16_t bitsPerSample = 8;                  //8,16,32...
 const char subChunk2ID[4]= {'d','a','t','a'};
 uint32_t subChunk2Size = 0;                  //== NumSamples * NumChannels * BitsPerSample/8
 //Data                                       //The audio data
};

/*********************************************************/
uint32_t counter = 0;
/*********************************************************/

void ADC_Handler(void){                                    //ADC Interrupt triggered by ADC sampling completion
  aaAudio.getADC();
  if(recFile){
    recFile.write(aaAudio.adcBuffer,MAX_BUFFER_SIZE);      //Write the data to SD as it is available
    counter++;
  }  
}

/*********************************************************/

void startRecording(const char *fileName, uint32_t sampleRate){

  #if defined (RECORD_DEBUG)
    Serial.print("Start Recording: ");
    Serial.println(fileName);
  #endif

  if(recFile){
    aaAudio.adcInterrupts(false);
    recFile.close();
  }
  if (myFile) {                                   //Close any open playback files & disable the DAC
    aaAudio.disableDAC();
    myFile.close();    
  }
  recFile = SD.open(fileName,FILE_WRITE);         //Open the file for writing

  if(!recFile){
    #if defined (RECORD_DEBUG)
      Serial.println("Failed to open file");
    #endif
    return;
  }
  recFile.seek(0);                                //Write a blank WAV header
  uint8_t bb = 0;
  for(int i=0; i<44; i++){
    recFile.write(bb);
  }

  aaAudio.adcBitsPerSample = 8;                   //Configure AAAudio
  aaAudio.setSampleRate(sampleRate);

  aaAudio.getADC();
  aaAudio.getADC();
  aaAudio.adcInterrupts(true);
  
  
}

/*********************************************************/

void createWavHeader(const char *fileName, uint32_t sampleRate ){

  if(!SD.exists(fileName)){
    #if defined (RECORD_DEBUG)
      Serial.println("File does not exist, please write WAV/PCM data starting at byte 44");
    #endif
    return;
  }
  recFile = SD.open(fileName,FILE_WRITE);
  
  if(recFile.size() <= 44){
    #if defined (RECORD_DEBUG)
      Serial.println("File contains no data, exiting");
    #endif
    recFile.close();
    return;
  }
  
  wavStruct wavHeader;
  wavHeader.chunkSize = recFile.size()-8;
  //wavHeader.numChannels = numChannels;
  wavHeader.sampleRate = sampleRate;
  wavHeader.byteRate = sampleRate * wavHeader.numChannels * wavHeader.bitsPerSample / 8;
  wavHeader.blockAlign = wavHeader.numChannels * wavHeader.bitsPerSample / 8;
  //wavHeader.bitsPerSample = bitsPerSample;
  wavHeader.subChunk2Size = recFile.size() - 44;

  #if defined (RECORD_DEBUG)
  Serial.print("WAV Header Write ");
  #endif
  
  recFile.seek(0);
  if( recFile.write((byte*)&wavHeader,44) > 0){
    #if defined (RECORD_DEBUG)    
      Serial.println("OK");
  }else{
      Serial.println("Failed");
    #endif
  }
  recFile.close();
  
}

/*********************************************************/

void stopRecording(const char *fileName, uint32_t sampleRate){

  aaAudio.adcInterrupts(false);                        //Disable the ADC interrupt
  recFile.close();                                         //Close the file
  createWavHeader(fileName,sampleRate);                    //Add appropriate header info, to make it a valid *.wav file
  #if defined (RECORD_DEBUG)
    Serial.println("Recording Stopped");
  #endif
}

/*********************************************************/


