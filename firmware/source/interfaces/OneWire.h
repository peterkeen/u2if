#ifndef _INTERFACE_ONEWIRE_H
#define _INTERFACE_ONEWIRE_H

#include "PicoInterfacesBoard.h"
#include "BaseInterface.h"

class OneWire : public BaseInterface {
public:
  OneWire();
  virtual ~OneWire();

  CmdStatus process(uint8_t const *cmd, uint8_t response[64]);
  CmdStatus task(uint8_t response[64]);

protected:
  enum INTERNAL_STATE {
    IDLE = 0x00,
    WAIT_SEARCH = 0x01
  };

  CmdStatus init(uint8_t const *cmd);
  CmdStatus deinit(uint8_t const *cmd);
  CmdStatus search(uint8_t const *cmd, uint8_t response[64]);
  CmdStatus send(uint8_t const *cmd);
  CmdStatus read(uint8_t const *cmd, uint8_t response[64]);
  CmdStatus reset(uint8_t const *cmd);

  uint _offsetProgram;
};

#endif
