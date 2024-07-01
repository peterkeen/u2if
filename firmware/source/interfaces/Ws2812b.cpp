#include "Ws2812b.h"
#include "PioPrograms.h"

#include <string.h>
#include <algorithm>

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "ws2812.pio.h"

static const PIO _pio = pio0;

Ws2812b::Ws2812b(uint slot, uint maxLeds)
  : StreamedInterface(maxLeds * 4 +1 /**/), _maxLeds(maxLeds), _internalState(INTERNAL_STATE::IDLE), _slot(slot) {
    initDma();
}

Ws2812b::~Ws2812b() {
}

CmdStatus Ws2812b::process(uint8_t const *cmd, uint8_t response[64]) {
    CmdStatus status = CmdStatus::NOT_CONCERNED;

    if(cmd[1] != _slot) {
      return status;
    }

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
    if(getInterfaceState() == InterfaceState::INTIALIZED) {
        return CmdStatus::NOK;
    }

    const uint pinId = cmd[2];
    const bool rgbw = cmd[3] == 1;

    ws2812_program_init(_pio, _slot, PIO0_PROGRAM_WS2812_OFFSET, pinId, 800000, rgbw);

    setInterfaceState(InterfaceState::INTIALIZED);
    return CmdStatus::OK;
}

CmdStatus Ws2812b::deinit(uint8_t const *cmd) {
    (void)cmd; 
    if(getInterfaceState() == InterfaceState::NOT_INITIALIZED) {
        return CmdStatus::OK; // do nothing
    }

    dma_channel_abort(_dmaChannel);
    dma_channel_wait_for_finish_blocking(_dmaChannel);
    pio_sm_set_enabled(_pio, _slot, false);

    setInterfaceState(InterfaceState::NOT_INITIALIZED);
    return CmdStatus::OK;
}

CmdStatus Ws2812b::write(uint8_t const *cmd, uint8_t response[64]) {
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

    // Tell the DMA to raise IRQ line 0 when the channel finishes a block
    // dma_channel_set_irq0_enabled(_dmaChannel, true);

    // Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
    // irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    //irq_add_shared_handler(DMA_IRQ_0, dma_handler, 0); => if DMA ira0 is shared
    // irq_set_enabled(DMA_IRQ_0, true);
}

bool Ws2812b::dmaInProgress() {
    return dma_channel_is_busy(_dmaChannel);
}

void Ws2812b::startTransfer(uint32_t *pixelBuf, uint32_t nbPixels) {
    // sync version
    /*for(uint32_t pxIt=0; pxIt < nbPixels; pxIt++) {
        pio_sm_put_blocking(_pio, 0, pixelBuf[pxIt]);
    }
    _internalState = INTERNAL_STATE::TRANSFER_FINISHED;*/

    // Async version
    // Give the channel a new wave table entry to read from, and re-trigger it
    dma_channel_transfer_from_buffer_now(_dmaChannel, pixelBuf, nbPixels);
}
