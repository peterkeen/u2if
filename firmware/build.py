import json
import os
import sys


FLAVORS = {
    'pico_i2s': ('PICO', 1, 1, 1000, 0),
    'pico_hub75': ('PICO', 0, 1, 1000, 1),
    'feather': ('FEATHER', 0, 0, 1000, 0),
    'itsybitsy': ('ITSYBITSY', 0, 0, 1000, 0),
    'qtpy': ('QTPY', 0, 0, 1000, 0),
    'qt2040_trinkey': ('QT2040_TRINKEY', 0, 0, 1000, 0),
    'feather_epd': ('FEATHER_EPD', 0, 1, 1000, 0),
    'feather_rfm': ('FEATHER_RFM', 0, 1, 1000, 0),
    'feather_can': ('FEATHER_CAN', 0, 1, 1000, 0),
    'kb2040': ('KB2040', 0, 1, 1000, 0),
    'sparkfun_pro_micro_rp2040': ('SPARKFUN_PRO_MICRO_RP2040', 0, 1, 1000, 0)
}

def build(flavor):
    args = " ".join(map(str, FLAVORS[flavor]))
    build_str = "./build.sh " + flavor + " " + args
    print(build_str)
    os.system(build_str)

if len(sys.argv) == 1:
    for flavor in FLAVORS:
        build(flavor)
else:
    if sys.argv[1] == "-flavors":
        print(json.dumps({"target": list(FLAVORS.keys())}))
    else:
        build(sys.argv[1])
