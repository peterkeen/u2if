#include "PioPrograms.h"

#include "hardware/clocks.h"

#include "interfaces/audio_i2s.pio.h"
#include "interfaces/hub75.pio.h"
#include "interfaces/onewire.pio.h"
#include "interfaces/ws2812.pio.h"

PioPrograms::PioPrograms() {

}

PioPrograms::~PioPrograms() {

}

void PioPrograms::init() {
    // pio0 total: 21 instructions
  // pio0, 17 instructions
  pio_add_program_at_offset(pio0, &onewire_program, PIO0_PROGRAM_ONEWIRE_OFFSET);
  // pio0, 4 instructions
  pio_add_program_at_offset(pio0, &ws2812_program, PIO0_PROGRAM_WS2812_OFFSET);
  
  // pio1 total: 27 instructions
  // pio1, 8 instructions
  pio_add_program_at_offset(pio1, &audio_i2s_program, PIO1_PROGRAM_AUDIO_I2S_OFFSET);
  // pio1, 3 instructions
  pio_add_program_at_offset(pio1, &hub75_row_program, PIO1_PROGRAM_HUB_75_ROW_OFFSET);
  // pio1, 16 instructions
  pio_add_program_at_offset(pio1, &hub75_data_rgb888_program, PIO1_PROGRAM_HUB_75_RGB888_OFFSET);
}
