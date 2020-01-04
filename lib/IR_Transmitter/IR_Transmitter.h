#ifndef IR_TRANSMITTER_H
#define IR_TRANSMITTER_H

#include "esp32-hal.h"
#include "Arduino.h"

#define IR_TICK 2.5 //uSec

#define CHILL_MODE 0x10
#define HOT_MODE  0x20
#define VENT_MODE 0x50

#define LOW_STRENGTH    0x00
#define MEDIUM_STRENGTH 0x04
#define HI_STRENGTH     0x08
#define AUTO_STRENGTH   0x0C

#define IR_CHANNELS_AVAILABLE   3

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
    void IR_Send(const uint8_t checkedAC, const bool toggleOnOff, const char *checkedMode, const char *checkedStrength, const uint8_t tempVal);
    uint8_t formatTempVal(uint8_t tempVal);
  private:
    float realTick;
    rmt_obj_t* rmt_send[3] = {NULL};
    rmt_data_t data[256];
    void createMsg(byte *cmd, rmt_data_t *data, byte size);
    const uint8_t IR_Pin[IR_CHANNELS_AVAILABLE] = {18, 19, 23};
};

#endif