from colored import Back, Style
from colored.controls import Controls

class SimulatedWS2812B:
    def __init__(self, pin_id, direction=None, pull=None, value=None, serial_number_str=None, rgbw=False, color_order="GRB"):
        self.pin_id = pin_id
        self.rgbw = rgbw

    def write(self, buffer):
        cursor = Controls()

        chars = []
        for pixel in buffer:
            chars.append(f'{Back.rgb(pixel[0], pixel[1], pixel[2])} ')
        print(f'{cursor.nav("up", 1)}{cursor.nav("erase_line", 0)}' + ''.join(chars) + f'{Style.reset}')
