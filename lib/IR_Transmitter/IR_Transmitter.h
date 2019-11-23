#ifndef IR_TRANSMITTER_H
#define IR_TRANSMITTER_H

#include "esp32-hal.h"
#include "Arduino.h"

#define IR_TICK 2.5 //uSec

enum IR_Cmd { IR_Cmd_TOGGLE, IR_Cmd_VENT, IR_Cmd_HEAT, IR_Cmd_CHILL, IR_Cmd_ChangeTemp };

struct s_irTimes
{
  float preamble[2];
  float logic[2][2];
};

const s_irTimes airConditioner { 2900 / IR_TICK, 2900 / IR_TICK, 900 / IR_TICK, 1000 / IR_TICK, 1000 / IR_TICK, 900 / IR_TICK };

class IR_Transmitter
{
  public:
    IR_Transmitter();
    void IR_Send(IR_Cmd cmd, uint8_t value = 0);
    uint8_t formatTempVal(uint8_t tempVal);
  private:
    float realTick;
    rmt_obj_t* rmt_send = NULL;
    rmt_data_t data[256];
    void createMsg(byte *cmd, rmt_data_t *data, byte size);
};

#endif