// Import required libraries
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>
#include "SPIFFS.h"
#include <DNSServer.h>
#include <ArduinoJson.h>

const byte DNS_PORT = 53;
IPAddress ap_local_IP(192,168,1,77);
IPAddress ap_gateway(192,168,1,254);
IPAddress ap_subnet(255,255,255,0);

IPAddress apIP(192, 168, 100, 1);

// Set LED GPIO
const int ledPin = 26;
// Stores LED state
String ledState;


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
DNSServer dnsServer;

// Replaces placeholder with LED state value
String processor(const String& var){
  //Serial.println(var);
  if(var == "STATE"){
    if(digitalRead(ledPin)){
      ledState = "ON";
    }
    else{
      ledState = "OFF";
    }
    //Serial.print(ledState);
  }
  return "?";
}
 
void setup(){
  // Serial port for debugging purposes
  //Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    //Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Creating Wi-Fi
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("CaptivePortal2");

  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  
  // Route for root / web page
  server.onNotFound( [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

   // Route to load jquery.js file
  server.on("/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("jquery requested");
    request->send(SPIFFS, "/jquery.min.js", "text/javascript");
  });

   // Route to load script.js file
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("script requested");
    request->send(SPIFFS, "/script.js", "text/javascript");
  });

  // Route to set GPIO to HIGH
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, HIGH);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to set GPIO to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, LOW);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/submit", HTTP_POST, [](AsyncWebServerRequest *request){
    String message;
    if (request->hasParam("green", true)) {
        message = request->getParam("green", true)->value();
    } else {
        message = "No message sent";
    }
    Serial.println(message);
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });


  // Start server
  server.begin();
}
 
void loop(){
    dnsServer.processNextRequest();
}
