import serial.tools.list_ports


# class Singleton(type):
#     _instances = {}

#     def __call__(cls, *args, **kwargs):
#         # __import__('pdb').set_trace()
#         if cls not in cls._instances:
#             cls._instances[cls] = super(Singleton, cls).__call__(*args, **kwargs)
#         # else:
#         #    cls._instances[cls].__init__(*args, **kwargs)
#         return cls._instances[cls]


class Singleton(type):
    _instances = {}
    _idicts = []

    def __call__(cls, *args, **kwargs):
        # __import__('pdb').set_trace()

        # Same as the original Singleton ie. when the attribute 'serial_number_str' is not in the argument list or
        # it equals None
        if (
            'serial_number_str' not in kwargs.keys()
            or kwargs['serial_number_str'] == None
        ):
            if cls not in cls._instances:
                cls._instances[cls] = super(Singleton, cls).__call__(*args, **kwargs)
            return cls._instances[cls]

        # When the attribute 'serial_number_str' is in the argument list
        for idict in cls._idicts:
            if (
                cls in idict
                and idict[cls].get_serial_number() == kwargs['serial_number_str']
            ):
                # A device with matching serial number exists;  return its value
                return idict[cls]

        # No device with the matching serial number exists.
        # Need to initiate a new device, store, then return its value
        xdict = {}
        xdict[cls] = super(Singleton, cls).__call__(*args, **kwargs)
        cls._idicts.append(xdict)
        return xdict[cls]


def find_serial_port(vid, pid, serial_number_str=None):
    ports = serial.tools.list_ports.comports()

    for check_port in ports:
        if hasattr(serial.tools, 'list_ports_common'):
            if (
                check_port.vid == vid
                and check_port.pid == pid
                and (
                    serial_number_str is None
                    or check_port.serial_number == serial_number_str
                )
            ):
                return check_port.device
    return None


def get_event_bytes_from_gpio_event(gpio, event_id):
    return bytes(((event_id & 0b11) >> 6) | (gpio & 0b111111))
