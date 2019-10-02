#include "SPIFFS.h"
#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <set>

DNSServer dnsServer;
AsyncWebServer server(80);


class CaptiveRequestHandler : public AsyncWebHandler {
public:
  std::set<String> availableUrl;
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}


  bool canHandle(AsyncWebServerRequest *request){
      Serial.println(request->url());
    if (request->url() == "/generate_204") return true;
    if (request->url() == "/index.html") return true;
    if (request->url() == "/jquery.min.js")  return true;
    if (request->url() == "/style.css")  return true;
    //request->addInterestingHeader("ANY");
    return false;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    //Send index.htm as text
    String requestedURL = request->url(); 
    Serial.println(requestedURL);

    AsyncWebServerResponse *response;

    int params = request->params();

    for(int i=0; i<params; i++)
    {
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost())
      {
        Serial.println("POST!!!");
        return;
      }
    }

    if(request->method() == HTTP_POST){
        Serial.println("POST!?!!!");
    } 

    //Serial.println(request->contentType());

    if (request->url() == "/index.html") response = request->beginResponse(SPIFFS, "/index.html");
    else if (request->url() == "/jquery.min.js") response = request->beginResponse(SPIFFS, "/jquery.min.js");
    else if (request->url() == "/style.css") response = request->beginResponse(SPIFFS, "/style.css");
    else response = request->beginResponse(SPIFFS, "/index.html");
    
    request->send(response);
  }
};


void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Initialize SPIFFS
  if(!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //your other setup stuff...
  WiFi.softAP("esp-captive");
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP

  
  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/submit", [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject& jsonObj = json.as<JsonObject>();
    //jsonObj.prettyPrintTo(Serial);

    int ID = jsonObj["AirConditioner"]["ID"];
    const char * CMD = jsonObj["AirConditioner"]["CMD"];

    Serial.println(ID);
    Serial.println(CMD);
    request->send(200, "application/json", "{test: \"ok\"}");
  });

  //more handlers...
  server.addHandler(handler);
  server.begin();
}

void loop(){
  dnsServer.processNextRequest();
}