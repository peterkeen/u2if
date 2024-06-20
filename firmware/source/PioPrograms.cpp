#include "PioPrograms.h"

#include "hardware/clocks.h"

#if I2S_ENABLED
#include "interfaces/audio_i2s.pio.h"
#endif

#if HUB75_ENABLED
#include "interfaces/hub75.pio.h"
#endif

#if ONEWIRE_ENABLED
#include "interfaces/onewire.pio.h"
#endif

#if WS2812_ENABLED
#include "interfaces/ws2812.pio.h"
#endif

PioPrograms::PioPrograms() {

}

PioPrograms::~PioPrograms() {

}

void PioPrograms::init() {
    // pio0 total: 21 instructions
#if ONEWIRE_ENABLED
  // pio0, 17 instructions
  pio_add_program_at_offset(pio0, &onewire_program, PIO0_PROGRAM_ONEWIRE_OFFSET);
#endif
#if WS2812_ENABLED
  // pio0, 4 instructions
  pio_add_program_at_offset(pio0, &ws2812_program, PIO0_PROGRAM_WS2812_OFFSET);
#endif  
  
  // pio1 total: 27 instructions
#if I2S_ENABLED  
  // pio1, 8 instructions
  pio_add_program_at_offset(pio1, &audio_i2s_program, PIO1_PROGRAM_AUDIO_I2S_OFFSET);
#endif
#if HUB75_ENABLED
  // pio1, 3 instructions
  pio_add_program_at_offset(pio1, &hub75_row_program, PIO1_PROGRAM_HUB_75_ROW_OFFSET);
  // pio1, 16 instructions
  pio_add_program_at_offset(pio1, &hub75_data_rgb888_program, PIO1_PROGRAM_HUB_75_RGB888_OFFSET);
#endif
}
