/*
 * Use example code of TinyUsb (See License file)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "tusb.h"
#include "ModeActivity.h"
extern "C" {
#include "pico/util/queue.h"
}
#include "board_config.h"
#include "ModeActivity.h"
#include "PioPrograms.h"
#include "interfaces/I2cMaster.h"
#include "interfaces/SpiMaster.h"
#include "interfaces/Gpio.h"
#include "interfaces/System.h"
#include "interfaces/Pwm.h"
#include "interfaces/Adc.h"
#include "interfaces/Uart.h"
#if WS2812_ENABLED
#include "interfaces/Ws2812b.h"
#endif
#if I2S_ENABLED
#include "interfaces/I2s.h"
#endif
#if HUB75_ENABLED
#include "interfaces/Hub75.h"
#endif
#if ONEWIRE_ENABLED
#include "interfaces/OneWire.h"
#endif
#include "interfaces/GroupGpio.h"


void sendOrSaveResponse(uint8_t response[64]);
void sendSavedResponses();


// ModeActivity
static ModeActivity modeActivity;
// Interfaces
static System sys;
static PioPrograms pioPrograms;

#if GPIO_ENABLED
static Gpio gpio;
static GroupGpio group_gpio;
#endif

#if I2C0_ENABLED
static I2CMaster ic2_0(0, 19*64);
#endif

#if I2C1_ENABLED
static I2CMaster ic2_1(1, 19*64);
#endif

#if SPI0_ENABLED
static SPIMaster spi_0(0, 19*64);
#endif

#if SPI1_ENABLED
static SPIMaster spi_1(1, 19*64);
#endif

#if PWM_ENABLED
static Pwm pwm;
#endif

#if ADC_ENABLED
static Adc adc;
#endif

#if UART0_ENABLED
static Uart uart_0(0);
#endif

#if UART1_ENABLED
static Uart uart_1(1);
#endif

#if WS2812_ENABLED
static Ws2812b ws2812b_0(0, WS2812_SIZE);
static Ws2812b ws2812b_1(1, WS2812_SIZE);
#endif

#if I2S_ENABLED
static I2s i2s(4000, 5);
#endif

#if HUB75_ENABLED
static Hub75 hub75(HUB75_MAX_LEDS*4);
#endif

#if ONEWIRE_ENABLED
static OneWire onewire;
#endif

static std::vector<BaseInterface*> interfaces = { 
&gpio
, &group_gpio
#if I2C0_ENABLED
, &ic2_0
#endif
#if I2C1_ENABLED
, &ic2_1
#endif
#if SPI0_ENABLED
, &spi_0
#endif
#if SPI1_ENABLED
, &spi_1
#endif
#if PWM_ENABLED
, &pwm
#endif
#if ADC_ENABLED
, &adc
#endif
#if UART0_ENABLED
, &uart_0
#endif
#if UART1_ENABLED
, &uart_1
#endif
#if WS2812_ENABLED
, &ws2812b_0
, &ws2812b_1
#endif
#if I2S_ENABLED
, &i2s
#endif
#if HUB75_ENABLED
, &hub75
#endif
#if ONEWIRE_ENABLED
, &onewire
#endif
, &sys
};

static queue_t tx_report_queue;
static const uint TX_REPORT_QUEUE_SIZE = 20;

//--------------------------------------------------------------------+
// Main loop function
//--------------------------------------------------------------------+
int main(void) {
    //stdio_init_all(); // to debug with printf (set pico_enable_stdio_uart(u2if 1) in CMakeLists) Caution, it is UART0.

    modeActivity.init();
    pioPrograms.init();

    queue_init(&tx_report_queue, HID_RESPONSE_SIZE, TX_REPORT_QUEUE_SIZE);
    tusb_init();

    while (1) {
        tud_task(); // tinyusb device task
        static uint8_t response[HID_RESPONSE_SIZE];
        for (uint8_t index = 0; index < static_cast<uint8_t>(interfaces.size()); index++) {
          response[0] = 0x00;
          CmdStatus ret = interfaces[index]->task(response);

          if(ret != CmdStatus::NOT_CONCERNED && ret != CmdStatus::NOT_FINISHED) {
            modeActivity.setBlinking();
            response[1] = ret;
            sendOrSaveResponse(response);
          } else if(ret == CmdStatus::NOT_FINISHED) {
            //modeActivity.setBlinkingInfinite();
          }
        }

        sendSavedResponses();
    }
}

bool processCmd(uint8_t const *buffer, uint16_t bufsize){
    if(bufsize != 64)
        return false;

    modeActivity.setBlinkingInfinite();

    CmdStatus ret = CmdStatus::NOT_CONCERNED;
    static uint8_t response[HID_RESPONSE_SIZE];
      for (uint8_t index = 0; index < static_cast<uint8_t>(interfaces.size()); index++) {
        response[0] = 0x00;
        ret = interfaces[index]->process(buffer, response);
        if(ret != CmdStatus::NOT_CONCERNED)
          break;
      }

    if(ret != CmdStatus::NOT_CONCERNED) {
    response[0] = buffer[0];
    response[1] = ret;
    } else {
    response[0] = buffer[0];
    response[1] = CmdStatus::NOT_CONCERNED;
    }

    modeActivity.setBlinking();
    sendOrSaveResponse(response);
    return true;
}

void sendOrSaveResponse(uint8_t response[64]) {
    if(!tud_hid_n_ready(0)) {
        queue_try_add(&tx_report_queue, response); // lost if queue is full
    } else {
        tud_hid_report(0, response, HID_RESPONSE_SIZE);
    }
}

void sendSavedResponses() {
    if(queue_is_empty(&tx_report_queue) || !tud_hid_n_ready(0)) {
        return;
    }
    static uint8_t response[HID_RESPONSE_SIZE];
    if(queue_try_remove(&tx_report_queue, response)) {
        tud_hid_report(0, response, HID_RESPONSE_SIZE);
    }
}

//--------------------------------------------------------------------+
// USB Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
    ModeActivity::setDeviceState(DeviceState::MOUNTED);
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
    ModeActivity::setDeviceState(DeviceState::NOT_MOUNTED);
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
    ModeActivity::setDeviceState(DeviceState::SUSPENDED);
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
    ModeActivity::setDeviceState(DeviceState::MOUNTED);
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
    // TODO not Implemented
    (void)itf;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
    // This example doesn't use multiple report and report ID
    (void) instance;;
    (void) report_id;
    (void) report_type;
    processCmd(buffer, bufsize);
}

//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void) itf;
    (void) rts;
    (void) dtr;
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
    (void) itf;
}
