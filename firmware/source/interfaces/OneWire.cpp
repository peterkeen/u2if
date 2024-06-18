#include "OneWire.h"

#include "hardware/clocks.h"
#include "onewire.pio.h"

static const PIO _pio = pio0;

CmdStatus OneWire::process(uint8_t const *cmd, uint8_t response[64]) {
  CmdStatus status = CmdStatus::NOT_CONCERNED;

  return status;
}

CmdStatus OneWire::task(uint8_t response[64]) {
  CmdStatus status = CmdStatus::NOT_CONCERNED;

  return status;
}
