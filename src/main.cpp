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
        Serial.println(request->url());
      if (request->url() == "/generate_204") return true;
      if (request->url() == "/index.html" || request->url() == "/") return true;
      if (request->url() == "/jquery.min.js")  return true;
      if (request->url() == "/style.css")  return true;
      //request->addInterestingHeader("ANY");
      return false;
    }

    void handleRequest(AsyncWebServerRequest *request) {
      String requestedURL = request->url(); 
      AsyncWebServerResponse *response;

      if (requestedURL == "/index.html") response = request->beginResponse(SPIFFS, "/index.html");
      else if (requestedURL == "/jquery.min.js") response = request->beginResponse(SPIFFS, "/jquery.min.js");
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
  const char * CMD = jsonObj["AirConditioner"]["CMD"];
  const uint8_t ID = jsonObj["AirConditioner"]["ID"];
  Serial.println(ID);

  //AirConditioner *currentAC;
  switch (ID)
  {
    case 200: 
      //currentAC = &AC200;
      break;
    case 201:
      //currentAC = &AC201;
      break;
    case 202:
      //currentAC = &AC202;
      break;
    default:
      Serial.println("Unexpected AC ID");
      return;
  }

  if (!strcmp(CMD, "Toggle"))
  {
    transmitter.IR_Send(IR_Cmd_TOGGLE);
  } 
  else if (!strcmp(CMD, "ChangeTemp"))
  {
    const uint8_t tempVal = jsonObj["AirConditioner"]["TempVal"];
    transmitter.IR_Send(IR_Cmd_ChangeTemp, tempVal);
  }
  else if (!strcmp(CMD, "Vent"))
  {
    transmitter.IR_Send(IR_Cmd_VENT);
  }
  else if (!strcmp(CMD, "Heat"))
  {
    transmitter.IR_Send(IR_Cmd_HEAT);
  }
  else if (!strcmp(CMD, "Chill"))
  {
    transmitter.IR_Send(IR_Cmd_CHILL);
  }
  else
  {
    Serial.println("Wrong IR Cmd!");
  }
}