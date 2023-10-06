from machine import I2C, u2if, Pin

DEV0_SN = '0xDE623134CF715537'
DEV1_SN = '0xDE623134CF856234'
dev0 = {
    'i2c': I2C(i2c_index=0, serial_number_str=DEV0_SN),
    'led': Pin(u2if.LED, Pin.OUT, serial_number_str=DEV0_SN),
}
dev1 = {
    'i2c': I2C(i2c_index=0, serial_number_str=DEV1_SN),
    'led': Pin(u2if.LED, Pin.OUT, serial_number_str=DEV1_SN),
}
