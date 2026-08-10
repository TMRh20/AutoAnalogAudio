
/*
 * AutoAnalogAudio WiFi Server SD Playback example
 *
 * Lists all files on SD card via web browser and allows playback
 *
 * Connect SD card and set CS pin in the User Config section below
 * Analog amplifier should be connected to pin A0
 *
 * Then point your brower to the IP shown on startup, make sure you use HTTP:// NOT HTTPS://
 * Some browsers may revert it or try to block it
 */


#include <AutoAnalogAudio.h>
#include "WiFiS3.h"
#include <SD.h>
#include "secrets.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;
const char* myName = "Arduino-R4";

/********** USER CONFIG **************/
const int chipSelect = 2;    // SD Card Chip Select pin
float volumeControl = 0.7;   // Default volume
String homeDirectory = "/";  // Default directory to search for *.wav files, make sure to inclue a trailing slash /

/*************************************/

AutoAnalog aaAudio;
WiFiServer server(80);

#define BUFFER_SIZE MAX_BUFFER_SIZE
bool playbackStopped = true;

#include "handleAudio.h"

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
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present.");
    while (1)
      ;  // Stop execution
  }
  Serial.println("Card initialized successfully.\n Connect amplifer to pin A0 \n");

  Serial.print("Connect your browser to http://");
  Serial.println(WiFi.localIP());

  aaAudio.begin(0, 3);            // Setup the AutoAnalogAudio library here
  aaAudio.dacBitsPerSample = 16;  // These settings are mostly irrelevant, since they are detected from the *.wav files
  aaAudio.setSampleRate(44100);

  playAudio("calibrat.wav");      // Attempt to play a file on startup called "calibrat.wav"

  server.begin();
}

void loop() {

  WiFiClient client = server.available();

  if (client) {

    String wavFile = "";
  
    while (client.connected()) {
      if (client.available()) {
        Serial.println("Got request");
        if (client.find("GET /")) {


          // Max 25 character filename limit
          for(int i=0; i<25; i++){
            wavFile += (char)client.read();
            wavFile.toUpperCase();
            if (wavFile.endsWith(".WAV")) {
              stopPlayback();
              break;
            }
          }
          while(client.available()){
            client.read();
          }

          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.print("<html><head><style>body { background-color: #2e2e2e; color: #ffffff; font-family: sans-serif;}");
          client.print(" a:link { color: #30d5c8; text-decoration: underline; } a:visited { color: #a2add0; } a:hover { color: #ffffff; text-decoration: none; } </style> </head><body>");
          client.print("<b>AutoAnalogAudio Library WiFi Server Example</b><br> Select any file to begin playback<br><br>");

          File root;
          File entry;
          root = SD.open(homeDirectory);

          if (root) {
            while (1) {
              entry = root.openNextFile();

              if (!entry) {
                break;
              }
              if (!entry.isDirectory()) {
                String fName = entry.name();
                const char* fileName = fName.c_str();
                fName.toUpperCase();
                if (fName.endsWith(".WAV")) {
                  String fileEntry = "";
                  fileEntry += "<a href='http://";
                  fileEntry += WiFi.localIP().toString();
                  fileEntry += "/";
                  fileEntry += fileName;
                  fileEntry += "' target='_self'>";
                  fileEntry += fileName;
                  fileEntry += "</a></br>";
                  client.print(fileEntry);
                }
              }
              entry.close();
            }
          }
          root.close();
          client.println("</body></html>");
          break;
        } else {
          Serial.print("Invalid request");
          break;
        }
      }
    }

    delay(10);
    client.flush();
    client.stop();

    // If we have a valid wav file request, play it here
    if (wavFile.endsWith(".WAV")) {
      String fullPath = homeDirectory + wavFile;
      const char* cString = fullPath.c_str();
      Serial.print("Now Playing: ");
      Serial.println(cString);
      playAudio(cString);
    }
  }

  // This handles loading the audio data as it plays
  loadBuffer();
}
