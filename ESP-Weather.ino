// includes
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ElegantOTA.h>
#include <FS.h>
#include <LittleFS.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

FS* filesystem = &LittleFS;

#define ALTITUDE 220
#define STASSID "ASM-modul"
#define STAPSK  "dreebit001"
#define HOSTNAME "ESP8266-"

const char* ap_default_ssid = STASSID; ///< Default SSID.
const char* ap_default_psk = STAPSK; ///< Default PSK.
const float cToKOffset = 273.15;
float temperature, dewpoint, humidity, humidity_r, pressure, pressure_r;
unsigned long startTime;
int delayTime;


ESP8266WebServer server(80);
//holds the current upload
File fsUploadFile;
Adafruit_BME280 bme;

#include "filesystem.h"
#include "webserver.h"
#include "sensor.h"



void setup(void) {
  String station_ssid = "esp001";
  String station_psk = "12345678";
  
  Serial.begin(9600);
  
 

  // Set Hostname.
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
    
  WiFi.hostname(hostname);

  filesystem->begin();
    {
        Dir dir = filesystem->openDir("/");
        while (dir.next()) {
            String fileName = dir.fileName();
            size_t fileSize = dir.fileSize();
        }
    }
    //Load Config -------------------------------------------
    if (!loadConfig()) {
        saveConfig();
    }
    
    

  // Check WiFi connection
  // ... check mode
  if (WiFi.getMode() != WIFI_STA) {
    WiFi.mode(WIFI_STA);
    delay(10);
  }

// ... Compare file config with sdk config.
  if (WiFi.SSID() != station_ssid || WiFi.psk() != station_psk) {
    
    // ... Try to connect to WiFi station.
    WiFi.begin(station_ssid.c_str(), station_psk.c_str());

    // ... Pritn new SSID
    
    // ... Uncomment this for debugging output.
    //WiFi.printDiag(Serial1);
  } else {
    // ... Begin with sdk config.
    WiFi.begin();
  }


  // ... Give ESP 10 seconds to connect to station.
  startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
  }

  // Check connection
  if (WiFi.status() != WL_CONNECTED) {
    // Go into software AP mode.
    WiFi.mode(WIFI_AP);

    delay(10);

    WiFi.softAP(ap_default_ssid, ap_default_psk);

    }

  

  bme.begin(0x76);   
  delay(100);
  setsensor();
  
  
//  server.on("/", []() {server.send(200, "text/plain", "Hi! I am ESP8266.");  });
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, []() {
      if (!handleFileRead("/edit.htm")) {
          server.send(404, "text/plain", "FileNotFound");
      }
     });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() {
      server.send(200, "text/plain", "");
      }, handleFileUpload);
  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([]() {
      if (!handleFileRead(server.uri())) {
          server.send(404, "text/plain", "FileNotFound");
      }
    });

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, []() {
    String json = "{";
    json += "\"tmp\":" + String(temperature);
    json += ", \"hum\":" + String(humidity_r);
    json += ", \"press\":" + String(pressure);
    json += ", \"dew\":" + String(dewpoint);
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });
  

  ElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  
  
  if (millis() - startTime > 5000) {
    getvalues();
      Serial.print(F("T: "));
  Serial.print((String)temperature);
  Serial.print(F(" *C\nDP: "));
  Serial.print((String)dewpoint);
  Serial.print(F(" *C\nH: "));
  Serial.print((String)humidity_r);
  Serial.print(F(" %\nAH: "));
  Serial.print((String)humidity);
  Serial.print(F(" g/m3\nRP: "));
  Serial.print((String)pressure_r);
  Serial.print(F(" hPa\nP: "));
  Serial.print((String)pressure);
  Serial.println(F(" hPa"));
  Serial.flush();
    startTime = millis();
    }
  
}
