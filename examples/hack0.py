from machine import I2C, u2if, Pin
from mcp23017 import MCP23017
import hid
from dataclasses import dataclass
from typing import Callable

import machine
import time
import _thread

import threading
from enum import Enum
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
MCP_BASE = 0x20
MCP_INDEX: dict = {
    'tout0p1': MCP_BASE,
    'tout4p9': MCP_BASE + 1,
    'bout0p1': MCP_BASE + 2,
    'bout4p9': MCP_BASE + 3,
    'pg0p1': MCP_BASE + 4,
    'pg4p0': MCP_BASE + 5,
    'failed_led': MCP_BASE + 6,
    'spare': MCP_BASE + 7,
}


def printhex(decimal: int, n: int = 4):
    print(f"0x{decimal:0{n}x}")


@dataclass
class UsrLED:
    class BlinkState(float, Enum):
        ON = 0.01  # ie. 10ms
        OFF = 0.0

    _on: list[Callable]
    _off: list[Callable]
    _pulsing: float = 0.0
    _blinking: tuple[BlinkState, int] = (BlinkState.OFF, 0)  # (state, controller_index)

    def __init__(self, on: list[Callable], off: list[Callable]):
        self._on = on
        self._off = off

    # def __post__(self):
    #     self._lock = threading.Lock()

    def pulse(self, duration: float = 0.01):
        # ic()
        # self._lock.acquire()
        self._pulsing += duration
        # self._lock.release()

    @property
    def disable(self):
        # self._lock.acquire()
        self._pulsing = 0.0
        self._blinking = (self.BlinkState.OFF, 0)
        # self._lock.release()

    @property
    def is_pulsing(self):
        return self._pulsing > 0.0

    @property
    def is_blinking(self):
        return self._blinking[0] > self.BlinkState.OFF

    def _pulse_all(self):
        sec: float = self._pulsing
        # self._lock.acquire()
        delta: float = self._pulsing - sec
        self._pulsing = delta if delta > 0.0 else 0.0
        # self._lock.release()
        _ = [x() for x in self._on]
        time.sleep(sec)
        _ = [x() for x in self._off]

    def _blink_until_stop(self):
        while self.is_blinking:
            _ = [x() for i, x in enumerate(self._on) if i == self._blinking[-1]]
            time.sleep(self._blinking[0])
            _ = [x() for x in self._off]
            time.sleep(self._blinking[0])


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
            self.mcp.append(mcp)

    @property
    def led(self) -> Pin:
        return self.ctrl_sig['led']

    @property
    def resetn(self) -> bool:
        return self.ctrl_sig['resetn'].value()

    @resetn.setter
    def resetn(self, val: bool) -> None:
        self.ctrl_sig['resetn'].value(val)

    @property
    def s1_on(self) -> bool:
        return self.ctrl_sig['s1_on'].value()

    @s1_on.setter
    def s1_on(self, val: bool) -> None:
        self.ctrl_sig['s1_on'].value(val)

    @property
    def s2_on(self) -> bool:
        return self.ctrl_sig['s2_on'].value()

    @s2_on.setter
    def s2_on(self, val: bool) -> None:
        self.ctrl_sig['s2_on'].value(val)

    @property
    def s3_on(self) -> bool:
        return self.ctrl_sig['s3_on'].value()

    @s3_on.setter
    def s3_on(self, val: bool) -> None:
        self.ctrl_sig['s3_on'].value(val)

    def _gpio2vect(self, key: str) -> list[bool]:
        val = self.mcp[MCP_INDEX[key] - MCP_BASE].gpio
        l = [bool((val >> i) & 1) for i in range(0, 16)]
        l.reverse()
        return l

    def _vect2int(self, vect: list[bool]) -> int:
        vect.reverse()
        return sum([b << i for i, b in enumerate(vect)])

    @property
    def tout_less_than_0p1v(self) -> list[bool]:
        return self._gpio2vect('tout0p1')

    @property
    def tout_greater_than_4p9v(self) -> list[bool]:
        return self._gpio2vect('tout4p9')

    @property
    def bout_less_than_0p1v(self) -> list[bool]:
        return self._gpio2vect('bout0p1')

    @property
    def bout_greater_than_4p9v(self) -> list[bool]:
        return self._gpio2vect('bout4p9')

    @property
    def pg_less_than_0p1v(self) -> list[bool]:
        return self._gpio2vect('pg0p1')

    @property
    def pg_greater_than_4p0v(self) -> list[bool]:
        return self._gpio2vect('pg4p0')

    @property
    def failed_led(self) -> list[bool]:
        return self._gpio2vect('failed_led')

    @failed_led.setter
    def failed_led(self, vect: list[bool]) -> None:
        val = self._vect2int(vect)
        self.mcp[MCP_INDEX['failed_led'] - MCP_BASE].gpio = val


@dataclass
class GAM02_Fxt_Controller_Group:
    controllers: list[GAM02_Fxt_Controller]

    def __post_init__(self):
        self.usrled_pins: list[Pin] = [x.led for x in self.controllers]
        self.mcps: list[MCP23017] = [x.mcp for x in self.controllers]
        self.usrled: UsrLED = UsrLED(
            on=[led.on for led in self.usrled_pins],
            off=[led.off for led in self.usrled_pins],
        )


hid_devs = hid.enumerate()
controllers = []
for hid_dev in hid_devs:
    if hid_dev['vendor_id'] == VID_U2IF_PICO and hid_dev['product_id'] == PID_U2IF_PICO:
        controllers.append(GAM02_Fxt_Controller(serial_number=hid_dev['serial_number']))


fxt_group = GAM02_Fxt_Controller_Group(controllers)


def core1_task(usrled: UsrLED):
    while True:
        if usrled.is_pulsing and not ('thread' in locals() and thread.is_alive()):
            thread = threading.Thread(target=usrled._pulse_all)
            thread.start()
        elif usrled.is_blinking and not ('thread' in locals() and thread.is_alive()):
            thread = threading.Thread(target=usrled._blink_until_stop)
            thread.start()

        # Other concurrent tasks ...


usrled = fxt_group.usrled
_thread.start_new_thread(core1_task, (usrled,))

# Turn on the user leds for 3s
usrled.pulse(3.0)

#
resetns = [x.ctrl_sig['resetn'] for x in controllers]
mcps = fxt_group.mcps
