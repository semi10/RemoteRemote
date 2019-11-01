#include "SPIFFS.h"
#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <set>
#include "esp32-hal.h"

#define PWM_FREQ  5000
#define PWM_RESOLUTION 8
#define PWM_CHANNEL_0 0
#define PWM_CHANNEL_1 1
#define PWM_CHANNEL_2 2

#define IR_TICK 2.5 //uSec

struct s_irTimes
{
    float preamble[2];
    float logic[2][2];
};

static s_irTimes samsung { 4500 / IR_TICK, 4500 / IR_TICK, 539 / IR_TICK, 585 / IR_TICK,  539 / IR_TICK, 1709 / IR_TICK };
static s_irTimes airConditioner { 2900 / IR_TICK, 2900 / IR_TICK, 900 / IR_TICK, 1000 / IR_TICK, 1000 / IR_TICK, 900 / IR_TICK };

rmt_data_t data[256];
rmt_obj_t* rmt_send = NULL;

void fillData(byte *cmd, rmt_data_t *data, byte size);

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
  Serial.begin(115200);

  // IR Setup
  if ((rmt_send = rmtInit(18, true, RMT_MEM_64)) == NULL)
  {
    Serial.println("init sender failed\n");
  }

  rmtSetCarrier(rmt_send, true, 1, 1050, 1050);
  float realTick = rmtSetTick(rmt_send, IR_TICK * 1000);
  printf("real tick set to: %fns\n", realTick);


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

    byte cmd[] = {0x9C, 0x1C, 0x00, 0x00};  // On/Off
    fillData(cmd, data, sizeof(cmd));
    rmtWrite(rmt_send, data, 106);

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
  else if (!strcmp(CMD, "Source"))
  {
    byte cmd[] = {0xE0, 0xE0, 0x80, 0x7F}; // Source
    fillData(cmd, data, sizeof(cmd));
    rmtWrite(rmt_send, data, 34);
  }
  else Serial.println("???");
}

void fillData(byte *cmd, rmt_data_t *data, byte size) 
{
  int index = 0;
  int bit = 0;

  for (int repeat = 0; repeat < 3; repeat++)
  {
    //Preamble Byte
    data[index].duration0 = airConditioner.preamble[0];
    data[index].level0 = 1;
    data[index].duration1 = airConditioner.preamble[1];
    data[index].level1 = 0;
    index++;
    
    for (int i = 0; i < size; i++)
    {
      uint8_t mask = 0x80;
      for (int j = 0; j < 8; j++)
      {
        bit = bool(cmd[i] & mask);
        data[index].duration0 = airConditioner.logic[bit][0];
        data[index].level0 = !bit;
        data[index].duration1 = airConditioner.logic[bit][1];
        data[index].level1 = bit;
        index++;
        
        mask >>=1;
      }
    }  

    // '1'
    bit = 1;
    data[index].duration0 = airConditioner.logic[bit][0];
    data[index].level0 = !bit;
    data[index].duration1 = airConditioner.logic[bit][1];
    data[index].level1 = bit;
    index++;

    // '0'
    bit = 0;
    data[index].duration0 = airConditioner.logic[bit][0];
    data[index].level0 = !bit;
    data[index].duration1 = airConditioner.logic[bit][1];
    data[index].level1 = bit;
    index++;
  }

  // Finish 
  data[index].duration0 = airConditioner.preamble[0];
  data[index].level0 = 1;
  data[index].duration1 = airConditioner.logic[0][0];
  data[index].level1 = 1;
  index++;
}