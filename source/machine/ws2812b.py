from .u2if import Device
from . import u2if_const as report_const


class WS2812B:
    def __init__(
            self, pin_id, slot=0, direction=None, pull=None, value=None, serial_number_str=None, rgbw=False, color_order="GRB"
    ):
        self._initialized = False
        self._device = Device(serial_number_str=serial_number_str)
        self.slot = slot        
        self.pin_id = pin_id
        self.rgbw = rgbw
        self._initialized = self._init()
        self._color_indexes = []

        colors = "RGBW"
        for i in range(len(color_order)):
            self._color_indexes.append(colors.index(color_order[i]))

    def __del__(self):
        self.deinit()

    def _init(self):
        if self._initialized:
            return
        chip_type = 0
        if self.rgbw:
            chip_type = 1
        res = self._device.send_report(bytes([report_const.WS2812B_INIT, self.slot, self.pin_id, chip_type]))
        if res[1] != report_const.OK:
            raise RuntimeError("WS2812B init error.")
        return True

    def deinit(self):
        if not self._initialized:
            return
        res = self._device.send_report(bytes([report_const.WS2812B_DEINIT, self.slot]))
        if res[1] != report_const.OK:
            raise RuntimeError("WS2812B deinit error.")
        self._initialized = False

    def write(self, buffer):
        return self._write_stream(buffer)

    def _write_stream(self, pixel_list):
        self._device.reset_output_serial()
        buffer = []
        # Not optimized version because buffer it is organized for PIO fifo and not for the transmission speed
        for pixel in pixel_list:
            # in uint32_t LSB order
            if not self.rgbw:
                buffer.append(0)

            for index in reversed(self._color_indexes):
                buffer.append(pixel[index] & 0xFF)

        remain_bytes = len(buffer)
        res = self._device.send_report(
            bytes([report_const.WS2812B_WRITE, self.slot])
            + remain_bytes.to_bytes(4, byteorder='little')
        )
        if res[1] != report_const.OK and res[2] == 0x01:
            raise RuntimeError("WS2812B write error : too many pixel for the firmware.")
        elif res[1] != report_const.OK and res[2] == 0x02:
            raise RuntimeError("WS2812B write error: transfer already in progress.")
        elif res[1] != report_const.OK:
            raise RuntimeError("WS2812B write error")

        self._device.write_serial(buffer)
        res = self._device.read_hid(report_const.WS2812B_WRITE)
        if res[1] != report_const.OK:
            raise RuntimeError("WS2812B write error.")
