#include "SPIFFS.h"
#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <set>

#define PWM_FREQ  5000
#define PWM_RESOLUTION 8
#define PWM_CHANNEL_0 0
#define PWM_CHANNEL_1 1
#define PWM_CHANNEL_2 2



struct AirConditioner {
  uint8_t IR_Pin;
  bool State;
  uint8_t Temperature;
  uint8_t PWM_Channel;
};

DNSServer dnsServer;
AsyncWebServer server(80);

AirConditioner AC200 = {25, LOW, 255, PWM_CHANNEL_0};
AirConditioner AC201 = {26, LOW, 255, PWM_CHANNEL_1};
AirConditioner AC202 = {27, LOW, 255, PWM_CHANNEL_2};


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
  // Serial port for debugging purposes
  pinMode(AC200.IR_Pin, OUTPUT);    
  pinMode(AC201.IR_Pin, OUTPUT); 
  pinMode(AC202.IR_Pin, OUTPUT); 

  digitalWrite(AC200.IR_Pin, AC200.State); 
  digitalWrite(AC201.IR_Pin, AC201.State); 
  digitalWrite(AC202.IR_Pin, AC202.State); 

  ledcSetup(PWM_CHANNEL_0, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_1, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_2, PWM_FREQ, PWM_RESOLUTION);

  ledcAttachPin(AC200.IR_Pin, AC200.PWM_Channel);
  ledcAttachPin(AC201.IR_Pin, AC201.PWM_Channel);
  ledcAttachPin(AC202.IR_Pin, AC202.PWM_Channel);



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

  AirConditioner *currentAC;
  switch (ID)
  {
    case 200: 
      currentAC = &AC200;
      break;
    case 201:
      currentAC = &AC201;
      break;
    case 202:
      currentAC = &AC202;
      break;
    default:
      Serial.println("Unexpected AC ID");
      return;
  }

  if (!strcmp(CMD, "Toggle"))
  {
    currentAC->State ^= 1;

    if (currentAC->State == HIGH) ledcWrite(currentAC->PWM_Channel, currentAC->Temperature);
    else ledcWrite(currentAC->PWM_Channel, LOW);
  } 
  else if (!strcmp(CMD, "ChangeTemp"))
  {
    if (currentAC->State == LOW) return;

    const uint8_t tempVal = jsonObj["AirConditioner"]["TempVal"];
    currentAC->Temperature = tempVal;
    ledcWrite(currentAC->PWM_Channel, currentAC->Temperature);
  }
  else Serial.println("???");
}
