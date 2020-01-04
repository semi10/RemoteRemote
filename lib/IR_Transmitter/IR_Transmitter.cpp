#include "IR_Transmitter.h"

#define TEMP_OFFSET 15

IR_Transmitter::IR_Transmitter()
{
  // IR Setup
  for (int IR_channel = 0; IR_channel < IR_CHANNELS_AVAILABLE; IR_channel++)
  {
    if ((rmt_send[IR_channel] = rmtInit(IR_Pin[IR_channel], true, RMT_MEM_64)) == NULL)
    {
      Serial.println("init sender failed\n");
    }
    else
    {
      rmtSetCarrier(rmt_send[IR_channel], true, 1, 1050, 1050);
      float realTick = rmtSetTick(rmt_send[IR_channel], IR_TICK * 1000);
      printf("real tick set to: %fns\n", realTick);
    }
  }
}

void IR_Transmitter::IR_Send(const uint8_t checkedAC, const bool toggleOnOff, const char *checkedMode, const char *checkedStrength, const uint8_t tempVal)
{
  byte IR_Code[4] = {0};
  uint8_t modeMask = 0;
  uint8_t strengthMask = 0;
  uint8_t formatedTempVal = 0;
  

  IR_Code[0] = ~(uint8_t(0xFF >> toggleOnOff));

  if      (!strcmp(checkedMode, "Vent"))  {modeMask = VENT_MODE;}
  else if (!strcmp(checkedMode, "Heat"))  {modeMask = HOT_MODE;}
  else if (!strcmp(checkedMode, "Chill")) {modeMask = CHILL_MODE;}
  else {Serial.println("Wrong AC Mode!!!");}

  IR_Code[0] |= modeMask;

  if      (!strcmp(checkedStrength, "Low"))     {strengthMask = LOW_STRENGTH;}
  else if (!strcmp(checkedStrength, "Medium"))  {strengthMask = MEDIUM_STRENGTH;}
  else if (!strcmp(checkedStrength, "High"))    {strengthMask = HI_STRENGTH;}
  else if (!strcmp(checkedStrength, "Auto"))    {strengthMask = AUTO_STRENGTH;}
  else {Serial.println("Wrong AC Strength!!!");}

  IR_Code[0] |= strengthMask;

  formatedTempVal = (tempVal - 15) << 1;

  IR_Code[1] |= formatedTempVal;

  Serial.println(IR_Code[0], BIN);
  Serial.println(IR_Code[1], BIN);

  createMsg(IR_Code, data, sizeof(IR_Code));
  
  for (uint8_t IR_channel = 0;  IR_channel < 3;  IR_channel++)
  {
    if (checkedAC & (1 << IR_channel)) 
    {
      Serial.print("Toggle AC ");
      Serial.println(IR_channel);
      rmtWrite(rmt_send[IR_channel], data, 106);
    }
  }
  

}

void IR_Transmitter::createMsg(byte *cmd, rmt_data_t *data, byte size) 
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

  // Finish (Postamble)
  data[index].duration0 = airConditioner.preamble[0];
  data[index].level0 = 1;
  data[index].duration1 = airConditioner.logic[0][0];
  data[index].level1 = 1;
  index++;
}

uint8_t IR_Transmitter::formatTempVal(uint8_t tempVal)
{
  uint8_t formatedTempVal = (tempVal - TEMP_OFFSET) << 1; 
  return formatedTempVal;
}