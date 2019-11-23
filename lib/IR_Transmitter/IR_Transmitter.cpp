#include "IR_Transmitter.h"

#define TEMP_OFFSET 15

IR_Transmitter::IR_Transmitter()
{
  // IR Setup
  if ((rmt_send = rmtInit(18, true, RMT_MEM_64)) == NULL)
  {
    Serial.println("init sender failed\n");
  }
  else
  {
    rmtSetCarrier(rmt_send, true, 1, 1050, 1050);
    float realTick = rmtSetTick(rmt_send, IR_TICK * 1000);
    printf("real tick set to: %fns\n", realTick);
  }
}

void IR_Transmitter::IR_Send(IR_Cmd cmd, uint8_t value)
{
  byte IR_Code[4] = {0};
  uint8_t formatedTempVal;
  
  switch (cmd)
  {
    case IR_Cmd_TOGGLE:
      IR_Code[0] = 0x9C;
      IR_Code[1] = 0x1C;
      break;
    case IR_Cmd_VENT:
      IR_Code[0] = 0x58;
      IR_Code[1] = 0x12;
      break;
    case IR_Cmd_HEAT:
      IR_Code[0] = 0x20;
      IR_Code[1] = 0x12;
      break;
    case IR_Cmd_CHILL:
      IR_Code[0] = 0x18;
      IR_Code[1] = 0x08;
      break;
    case IR_Cmd_ChangeTemp:
      formatedTempVal = formatTempVal(value);
      IR_Code[0] = 0x1C;
      IR_Code[1] = formatedTempVal;
      break;
    default:
      Serial.println("Wrong IR Cmd!");
      return;
      break;
  }

  createMsg(IR_Code, data, sizeof(IR_Code));
  rmtWrite(rmt_send, data, 106);
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