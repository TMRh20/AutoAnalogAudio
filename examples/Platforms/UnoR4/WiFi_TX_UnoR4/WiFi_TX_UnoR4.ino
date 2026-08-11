#include <AutoAnalogAudio.h>
#include "WiFiS3.h"
#include <WiFiUdp.h>
#include "secrets.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

/*********************** USER CONFIG ****************************/
const char* myName   = "Arduino-R4";
const char* targetIP = "10.10.1.174";
const uint16_t targetPort = 5004;

// Set the above targetIP to the IP of your VLC machine
// In VLC open a Network Stream at rtp://@:5004

/********************************************************/

// RTP Configuration - 8-bit PCM at 8kHz
const uint8_t RTP_PAYLOAD_TYPE = 0;   // PCMU (will work for raw PCM too)
const uint16_t SAMPLE_RATE = 8000;
const uint16_t SAMPLES_PER_PACKET = 512;

uint16_t rtp_sequence = 0;
uint32_t rtp_timestamp = 0;
uint32_t rtp_ssrc = 0x12345678;

AutoAnalog aaAudio;
WiFiUDP udp;

#define bufferSize 512

void setup() {
  Serial.begin(115200);

  modem.begin(); 
  WiFi.setHostname(myName);

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Analog Audio Begin");
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);

  aaAudio.begin(3, 0);
  aaAudio.dacBitsPerSample = 16;
  aaAudio.adcBitsPerSample = 16;
  aaAudio.setSampleRate(8000);

  udp.begin(4444);
}

uint32_t timer = millis();
uint32_t packetCount = 0;


void loop() {
  aaAudio.getADC(bufferSize);
  
  
  uint8_t finalBuf[bufferSize];
  for(int i=0; i<bufferSize; i++){
    finalBuf[i] = encodePcm16ToPcmu(aaAudio.adcBuffer16[i]);
  }
  
  // Send 8-bit PCMU samples directly
  sendRTPPacket(finalBuf, bufferSize);
  packetCount++;

  if(millis() - timer > 5000){
    timer = millis();
    Serial.print("Packets/sec: ");
    Serial.print(packetCount / 5.0, 2);
    Serial.print(" | Seq: ");
    Serial.print(rtp_sequence);
    Serial.print(" | TS: ");
    Serial.println(rtp_timestamp);
    packetCount = 0;
  }
}

void sendRTPPacket(uint8_t* payload, int payload_len) {
  uint8_t rtpPacket[12 + payload_len];
  
  rtpPacket[0] = 0x80;
  rtpPacket[1] = (0 << 7) | RTP_PAYLOAD_TYPE;
  
  rtpPacket[2] = (rtp_sequence >> 8) & 0xFF;
  rtpPacket[3] = rtp_sequence & 0xFF;
  rtp_sequence++;
  
  rtpPacket[4] = (rtp_timestamp >> 24) & 0xFF;
  rtpPacket[5] = (rtp_timestamp >> 16) & 0xFF;
  rtpPacket[6] = (rtp_timestamp >> 8) & 0xFF;
  rtpPacket[7] = rtp_timestamp & 0xFF;
  rtp_timestamp += SAMPLES_PER_PACKET;
  
  rtpPacket[8]  = (rtp_ssrc >> 24) & 0xFF;
  rtpPacket[9]  = (rtp_ssrc >> 16) & 0xFF;
  rtpPacket[10] = (rtp_ssrc >> 8) & 0xFF;
  rtpPacket[11] = rtp_ssrc & 0xFF;
  
  memcpy(&rtpPacket[12], payload, payload_len);
  
  udp.beginPacket(targetIP, targetPort);
  udp.write(rtpPacket, 12 + payload_len);
  udp.endPacket();
}

uint8_t encodePcm16ToPcmu(int16_t sample) {
    // 1. Extract sign bit (0x80 for negative, 0x00 for positive)
    uint8_t sign = (sample < 0) ? 0x80 : 0x00;
    if (sample < 0) {
        // Convert to absolute value safely handling INT16_MIN
        sample = (sample == INT16_MIN) ? INT16_MAX : -sample;
    }

    // 2 & 3. Add G.711 bias and clip to max 15-bit value
    sample += 33;
    if (sample > 32767) {
        sample = 32767;
    }

    // 4. Determine exponent (chord)
    // Find the position of the highest set bit
    int16_t exponent = 7;
    for (int16_t mask = 0x4000; (sample & mask) == 0 && exponent > 0; mask >>= 1) {
        exponent--;
    }

    // 5. Extract mantissa (step) - 4 bits following the leading 1
    uint8_t mantissa = (sample >> (exponent + 3)) & 0x0F;

    // 6. Combine and flip all bits (G.711 standard requirement)
    return ~(sign | (exponent << 4) | mantissa);
}
