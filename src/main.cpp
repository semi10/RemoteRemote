#include <SPIFFS.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <set>
//#include "esp32-hal.h"
#include <IR_Transmitter.h>

IR_Transmitter transmitter;


struct AirConditioner {
  uint8_t IR_Pin;
  bool State;
  uint8_t Temperature;
  uint8_t PWM_Channel;
};

DNSServer dnsServer;
AsyncWebServer server(80);

void handleJSON(JsonObject& jsonObj);

class CaptiveRequestHandler : public AsyncWebHandler {
  public:
    std::set<String> availableUrl;
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request){
      if (request->url() == "/generate_204") return true;
      else if (request->url() == "/index.html" || request->url() == "/") return true;
      else if (request->url() == "/jquery.js")  return true;
      else if (request->url() == "/jqm-demos.css")  return true;
      else if (request->url() == "/jquery.mobile.js")  return true;
      else if (request->url() == "/jquery.mobile.css")  return true;
      else if (request->url() == "/images/ajax-loader.gif")  return true;
      else if (request->url() == "/style.css") return true;
      else { Serial.println(request->url()); }
      return false;
    }

    void handleRequest(AsyncWebServerRequest *request) {
      String requestedURL = request->url(); 
      AsyncWebServerResponse *response;

      if (requestedURL == "/index.html") response = request->beginResponse(SPIFFS, "/index.html");
      else if (requestedURL == "/jquery.js") response = request->beginResponse(SPIFFS, "/jquery.js");
      else if (requestedURL == "/jqm-demos.css") response = request->beginResponse(SPIFFS, "/jqm-demos.css");
      else if (requestedURL == "/jquery.mobile.js") response = request->beginResponse(SPIFFS, "/jquery.mobile.js");
      else if (requestedURL == "/jquery.mobile.css") response = request->beginResponse(SPIFFS, "/jquery.mobile.css");
      else if (requestedURL == "/style.css") response = request->beginResponse(SPIFFS, "/style.css");
      else response = request->beginResponse(SPIFFS, "/index.html");
      
      request->send(response);
    }
};


void setup(){
  Serial.begin(115200);

  // Initialize SPIFFS
  if(!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //your other setup stuff...
  WiFi.softAP("IR3-captive");
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP
  
  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/submit", [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject& jsonObj = json.as<JsonObject>();
    //jsonObj.prettyPrintTo(Serial);
    handleJSON(jsonObj);
    request->send(200, "application/json", "{test: \"ok\"}");
  });

  //more handlers...
  server.addHandler(handler);
  server.begin();
}

void loop(){
  dnsServer.processNextRequest();
}

void handleJSON(JsonObject& jsonObj)
{
  const uint8_t checkedAC =     jsonObj["AirConditioner"]["CheckedAC"];
  const bool toggleOnOff =      jsonObj["AirConditioner"]["Toggle"];
  const uint8_t tempVal =       jsonObj["AirConditioner"]["TempVal"];
  const char *checkedMode =     jsonObj["AirConditioner"]["Mode"];
  const char *checkedStrength = jsonObj["AirConditioner"]["Strength"];

  transmitter.IR_Send(checkedAC, toggleOnOff, checkedMode, checkedStrength, tempVal);
}