from machine import I2C, u2if, Pin
from mcp23017 import MCP23017
import hid
from dataclasses import dataclass
from typing import Callable

import machine
import time
import _thread

import threading
from icecream import ic

VID_U2IF_PICO = 0xCAFE
PID_U2IF_PICO = 0x4005
CTRL_SIGS: dict = {
    's1_on': u2if.GP10,
    's2_on': u2if.GP11,
    's3_on': u2if.GP12,
    'tout_open': u2if.GP13,
    'resetn': u2if.GP15,
    'vbias_5_on': u2if.GP16,
    'vbin_2p9_on': u2if.GP17,
    'vtin_2p9_on': u2if.GP18,
    'vbout_on': u2if.GP19,
    'vtout_on': u2if.GP20,
    'led': u2if.LED,
}


def printhex(decimal: int, n: int = 4):
    print(f"0x{decimal:0{n}x}")


@dataclass
class CtrlPin:
    name: str
    gpio_num: int
    pin: Pin


@dataclass
class GAM02_Fxt_Controller:
    """
    The GAM02 Test Fixture Controller class
    """

    vendor_id: int = VID_U2IF_PICO
    product_id: int = PID_U2IF_PICO
    serial_number: str | None = None

    def __post_init__(self):
        # __import__('pdb').set_trace()

        # The Pico's GPIOs used as output signals
        # Init the ctrl_pin via dict comprehension based on the CTRL_SIGS
        self.ctrl_pin: dict = {
            k: CtrlPin(k, v, Pin(v, Pin.OUT, serial_number_str=self.serial_number))
            for (k, v) in CTRL_SIGS.items()
        }

        # The control signals
        self.ctrl_sig: dict = {k: v.pin for (k, v) in self.ctrl_pin.items()}

        self.i2c: I2C = I2C(
            i2c_index=0,
            frequency=400000,
            pullup=False,
            serial_number_str=self.serial_number,
        )

        # The list of MCP23017s that are connected to the i2c bus
        self.mcp: list[MCP23017] = []

        # Scan the i2c bus and init the mcp list with the discovered MCP23017s
        i2c_addresses = self.i2c.scan()
        for i2c_addr in i2c_addresses:
            mcp = MCP23017(self.i2c, i2c_addr)
            # mcp.porta.mode = 0xFF  # All IOs as inputs
            # mcp.portb.mode = 0xFF
            # mcp.porta.input_polarity = 0x00  # normal
            # mcp.portb.input_polarity = 0x00
            # # mcp.porta.pullup = 0xFF
            # # mcp.portb.pullup = 0xFF
            self.mcp.append(mcp)


# spLock = _thread.allocate_lock()
# spLock.acquire()

hid_devs = hid.enumerate()
controllers = []
for hid_dev in hid_devs:
    if hid_dev['vendor_id'] == VID_U2IF_PICO and hid_dev['product_id'] == PID_U2IF_PICO:
        controllers.append(GAM02_Fxt_Controller(serial_number=hid_dev['serial_number']))


leds = [x.ctrl_sig['led'] for x in controllers]


@dataclass
class UsrLED:
    _on: list[Callable]
    _off: list[Callable]
    _duration: float = 0.0

    def __init__(self, on: list[Callable], off: list[Callable]):
        self._on = on
        self._off = off

    # def __post__(self):
    #     self._lock = threading.Lock()

    def blink(self, duration: float = 0.01):
        # ic()
        # self._lock.acquire()
        self._duration += duration
        # self._lock.release()

    @property
    def disable(self):
        # self._lock.acquire()
        self._duration = 0.0
        # self._lock.release()

    @property
    def is_enable(self):
        return self._duration > 0.0

    def _blink(self):
        sec: float = self._duration
        # self._lock.acquire()
        delta: float = self._duration - sec
        self._duration = delta if delta > 0.0 else 0.0
        # self._lock.release()
        # _ = [led.on() for led in leds]
        _ = [x() for x in self._on]
        time.sleep(sec)
        _ = [x() for x in self._off]


def core1_task(usrled: UsrLED):
    while True:
        if usrled.is_enable and not ('thread' in locals() and thread.is_alive()):
            thread = threading.Thread(target=usrled._blink)
            thread.start()
            # print(f"core1_task ...", flush=True)

        # Other concurrent tasks ...


usrled = UsrLED(on=[led.on for led in leds], off=[led.off for led in leds])
_thread.start_new_thread(core1_task, (usrled,))

# Turn on the user leds for 3s
usrled.blink(3.0)

#
resetns = [x.ctrl_sig['resetn'] for x in controllers]
mcps = [x.mcp for x in controllers]
