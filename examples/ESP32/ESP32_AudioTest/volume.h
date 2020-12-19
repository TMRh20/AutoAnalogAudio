
float volumeVar = 1.00;



void volume(bool val) {

  if (val) {
    if (volumeVar < 15) {
      volumeVar = volumeVar - 0.1 >= 1.0 ? volumeVar - 0.1 : volumeVar;
    } else {
      volumeVar -= 1;
    }
  } else {
    if (volumeVar < 15) {
      volumeVar += .1;
    } else if (volumeVar < 127) {
      volumeVar += 1;
    }
  }
  Serial.println(volumeVar);
  //Note: 50% digital reduction may not equal 50% reduction in audible sound output
  Serial.print("Volume Setting: ");
  Serial.print((128 - volumeVar) / 127.0 * 100);
  Serial.println("%");
}

void volumeSet(float val) {

  if (val >= 1 && val <= 127) {
    volumeVar = val;
    Serial.print("Volume Setting: ");
    Serial.print((128 - volumeVar) / 127.0 * 100);
    Serial.println("%");
  }
}
