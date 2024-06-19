#ifndef _PIO_PROGRAMS_H
#define _PIO_PROGRAMS_H

#include "pico/types.h"

const uint PIO0_PROGRAM_ONEWIRE_OFFSET = 0;
const uint PIO0_PROGRAM_WS2812_OFFSET = 17;

const uint PIO1_PROGRAM_AUDIO_I2S_OFFSET = 0;
const uint PIO1_PROGRAM_HUB_75_ROW_OFFSET = 8;
const uint PIO1_PROGRAM_HUB_75_RGB888_OFFSET = 11;

class PioPrograms {
public:
  PioPrograms();
  ~PioPrograms();

  void init();
};

#endif
