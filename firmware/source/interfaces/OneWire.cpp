#if ONEWIRE_ENABLED

#include "OneWire.h"

#include "hardware/clocks.h"
#include "onewire.pio.h"

static const PIO _pio = pio0;
static const uint _sm = 7;

OneWire::OneWire() {
  _offsetProgram = -1;
}

OneWire::~OneWire() {
}

CmdStatus OneWire::process(uint8_t const *cmd, uint8_t response[64]) {
  CmdStatus status = CmdStatus::NOT_CONCERNED;

  if(cmd[0] == Report::ID::ONEWIRE_INIT) {
    status = init(cmd);
  } else if(cmd[0] == Report::ID::ONEWIRE_DEINIT) {
    status = deinit(cmd);
  } else if(cmd[0] == Report::ID::ONEWIRE_SEARCH) {
    status = search(cmd, response);
  } else if(cmd[0] == Report::ID::ONEWIRE_SEND) {
    status = send(cmd);
  } else if(cmd[0] == Report::ID::ONEWIRE_READ) {
    status = read(cmd, response);
  } else if(cmd[0] == Report::ID::ONEWIRE_RESET) {
    status = reset(cmd);
  }

  return status;
}

CmdStatus OneWire::task(uint8_t response[64]) {
  CmdStatus status = CmdStatus::NOT_CONCERNED;

  return status;
}

CmdStatus OneWire::init(uint8_t const *cmd) {
  if(getInterfaceState() == InterfaceState::INITIALIZED) {
    return CmdStatus::NOK;
  }

  const uint pinId = cmd[1];

  //onewire_program_init(_pio

  return CmdStatus::NOT_CONCERNED;
}

CmdStatus OneWire::deinit(uint8_t const *cmd) {
  return CmdStatus::NOT_CONCERNED;
}

CmdStatus OneWire::search(uint8_t const *cmd, uint8_t response[64]) {
  CmdStatus status = CmdStatus::NOT_CONCERNED;

  return status;
}

CmdStatus OneWire::send(uint8_t const *cmd) {
  CmdStatus status = CmdStatus::NOT_CONCERNED;

  return status;  
}

CmdStatus OneWire::read(uint8_t const *cmd, uint8_t response[64]) {
  CmdStatus status = CmdStatus::NOT_CONCERNED;

  return status;  
}

CmdStatus OneWire::reset(uint8_t const *cmd) {
  CmdStatus status = CmdStatus::NOT_CONCERNED;

  return status;  
}
#endif
