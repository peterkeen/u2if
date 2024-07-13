#include "Ws2812b.h"
#include "PioPrograms.h"

#include <string.h>
#include <algorithm>

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "ws2812.pio.h"

static const PIO _pio = pio0;

Ws2812b::Ws2812b(uint maxLeds)
  : StreamedInterface(maxLeds * 4 +1 /**/), _maxLeds(maxLeds), _internalState(INTERNAL_STATE::IDLE), _sm(0), _pin(0) {
    initDma();
}

Ws2812b::~Ws2812b() {
}

CmdStatus Ws2812b::process(uint8_t const *cmd, uint8_t response[64]) {
    CmdStatus status = CmdStatus::NOT_CONCERNED;

    if(cmd[0] == Report::ID::WS2812B_INIT) {
        status = init(cmd);
    } else if(cmd[0] == Report::ID::WS2812B_DEINIT) {
        status = deinit(cmd);
    } else if(cmd[0] == Report::ID::WS2812B_WRITE) {
        status = write(cmd, response);
    }

    return status;
}

CmdStatus Ws2812b::task(uint8_t response[64]) {

    CmdStatus status = CmdStatus::NOT_CONCERNED;
    if(_internalState == INTERNAL_STATE::IDLE) {
        status = CmdStatus::NOT_CONCERNED;
    } else if(_internalState == INTERNAL_STATE::WAIT_PIXELS && getBuffer().size() < _totalRemainingBytesToSend) {
        streamRxRead();
        status = CmdStatus::NOT_FINISHED;
    } else if(_internalState == INTERNAL_STATE::WAIT_PIXELS) {// && getBuffer().size() >= _totalRemainingBytesToSend)
        _internalState = INTERNAL_STATE::TRANSFER_IN_PROGRESS;
        startTransfer(getBuffer().getDataPtr32(), _totalRemainingBytesToSend / 4);
        // send ACK
        response[0] = Report::ID::WS2812B_WRITE;
        status = CmdStatus::OK;
    } else if(_internalState == INTERNAL_STATE::TRANSFER_IN_PROGRESS && dmaInProgress()) {
        status = CmdStatus::NOT_FINISHED;
    } else if(_internalState == INTERNAL_STATE::TRANSFER_IN_PROGRESS) { // !dmaInProgress
        _internalState = INTERNAL_STATE::TRANSFER_FINISHED;
        status = CmdStatus::NOT_FINISHED;
    } else if(_internalState == INTERNAL_STATE::TRANSFER_FINISHED) {
        _totalRemainingBytesToSend = 0;
        getBuffer().setSize(0);
        _internalState = INTERNAL_STATE::IDLE;
        status = CmdStatus::NOT_CONCERNED; //ACK was already sent
    }
    return status;
}


CmdStatus Ws2812b::init(uint8_t const *cmd) {
    if(getInterfaceState() == InterfaceState::INITIALIZED) {
        return CmdStatus::OK;
    }

    const bool rgbw = cmd[1] == 1;

    _sm = pio_claim_unused_sm(_pio, true);

    ws2812_program_init(_pio, _sm, PIO0_PROGRAM_WS2812_OFFSET, 800000, rgbw);

    setInterfaceState(InterfaceState::INITIALIZED);
    return CmdStatus::OK;
}

CmdStatus Ws2812b::deinit(uint8_t const *cmd) {
    (void)cmd; 
    if(getInterfaceState() == InterfaceState::NOT_INITIALIZED) {
        return CmdStatus::OK; // do nothing
    }

    dma_channel_abort(_dmaChannel);
    dma_channel_wait_for_finish_blocking(_dmaChannel);
    pio_sm_set_enabled(_pio, _sm, false);

    setInterfaceState(InterfaceState::NOT_INITIALIZED);
    return CmdStatus::OK;
}

CmdStatus Ws2812b::write(uint8_t const *cmd, uint8_t response[64]) {
    ws2812_program_set_pin(_pio, _sm, cmd[1]);

    const uint32_t nbBytes = convertBytesToUInt32(&cmd[2]);
    response[2] = 0x00;
    if(_internalState != INTERNAL_STATE::IDLE){
        response[2] = 0x02;
        return CmdStatus::NOK;
    } else if(nbBytes > (_maxLeds * 4)) {
        _totalRemainingBytesToSend = 0;
        response[2] = 0x01;
        return CmdStatus::NOK;
    }

    flushStreamRx();
    _totalRemainingBytesToSend = nbBytes;
    _internalState = INTERNAL_STATE::WAIT_PIXELS;
    return CmdStatus::OK;
}

void Ws2812b::initDma() {
    // Init dma
    _dmaChannel = dma_claim_unused_channel(true);
    dma_channel_config dmaConfig = dma_channel_get_default_config(_dmaChannel);
    channel_config_set_transfer_data_size(&dmaConfig, DMA_SIZE_32);
    channel_config_set_read_increment(&dmaConfig, true);
    channel_config_set_dreq(&dmaConfig, DREQ_PIO0_TX0);
    dma_channel_configure(_dmaChannel,
            &dmaConfig,
            &pio0->txf[0],  // Write address (only need to set this once)
            NULL,           // Don't provide a read address yet
            0,              // Write the same value many times, then halt and interrupt
            false           // Don't start yet
        );
}

bool Ws2812b::dmaInProgress() {
    return dma_channel_is_busy(_dmaChannel);
}

void Ws2812b::startTransfer(uint32_t *pixelBuf, uint32_t nbPixels) {
    dma_channel_transfer_from_buffer_now(_dmaChannel, pixelBuf, nbPixels);
}
