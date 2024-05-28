#include "SpiMaster.h"
#include "string.h"


SPIMaster::SPIMaster(uint8_t spiIndex, uint streamBufferSize = 512)
    : StreamedInterface(streamBufferSize),
      #if SPI_ENABLED
      _spiInst(spiIndex == 0 ? spi0 : spi1),
      _clkGP(spiIndex == 0 ? U2IF_SPI0_CK : U2IF_SPI1_CK),
      _mosiGP(spiIndex == 0 ? U2IF_SPI0_MOSI : U2IF_SPI1_MOSI),
      _misoGP(spiIndex == 0 ? U2IF_SPI0_MISO : U2IF_SPI1_MISO)
      #else
      _spiInst(spi0),
      _clkGP(U2IF_SPI0_CK),
      _mosiGP(U2IF_SPI0_MOSI),
      _misoGP(U2IF_SPI0_MISO)
      #endif
{

}

SPIMaster::~SPIMaster() {

}

uint8_t SPIMaster::getInstIndex() {
    //return static_cast<uint8_t>(spi_hw_index(_spiInst));
    return _spiInst == spi1 ? 1 : 0;
}

CmdStatus SPIMaster::process(uint8_t const *cmd, uint8_t response[64]) {
    CmdStatus status = CmdStatus::NOT_CONCERNED;
    const uint spiIndex = getInstIndex();

    if(cmd[0] == (Report::ID::SPI0_INIT + spiIndex * Report::ID::SPI0_SPI1_OFFSET)) {
        status = init(cmd);
    } else if(cmd[0] == (Report::ID::SPI0_DEINIT + spiIndex * Report::ID::SPI0_SPI1_OFFSET) ) {
        status = deInit();
    } else if(cmd[0] == (Report::ID::SPI0_WRITE + spiIndex * Report::ID::SPI0_SPI1_OFFSET)) {
        status = write(cmd);
    } else if(cmd[0] == (Report::ID::SPI0_READ + spiIndex * Report::ID::SPI0_SPI1_OFFSET)) {
        status = read(cmd, response);
    } else if(cmd[0] == (Report::ID::SPI0_WRITE_FROM_UART + spiIndex * Report::ID::SPI0_SPI1_OFFSET)) {
        status = writeFromUart(cmd);
    }

    return status;
}

CmdStatus SPIMaster::task(uint8_t response[64]) {
    if(_totalRemainingBytesToSend == 0)
        return CmdStatus::NOT_CONCERNED;

    bool error = false;
    StreamBuffer &buf =  getBuffer();
    while(_totalRemainingBytesToSend > 0 && streamRxAvailableSize() && !error){
        streamRxRead();
        uint nbBytes = std::min(_totalRemainingBytesToSend, buf.size());
        int nbWritten = spi_write_blocking (_spiInst, buf.getDataPtr8(), nbBytes);
        if(nbWritten != static_cast<int>(nbBytes)) {
            error = true;
        } else {
            _totalRemainingBytesToSend -= static_cast<uint>(nbWritten);
        }
        buf.setSize(0);
        //printf("->Total = %d\n", _totalRemainingBytesToSend);

        break; // break ou pas ? est ce qu'on essaye de tout envoyer sans redonner la main a l'event loop ou on fait le contraire ?
    }

    if(error || _totalRemainingBytesToSend == 0) {
        _totalRemainingBytesToSend = 0;
        //printf("->END = %d\n", _totalRemainingBytesToSend);
        response[0] = Report::ID::SPI0_WRITE_FROM_UART + (getInstIndex() * 0x10);
        return error ? CmdStatus::NOK : CmdStatus::OK;
    }
    return CmdStatus::NOT_FINISHED;
}

CmdStatus SPIMaster::init(uint8_t const *cmd) {
    uint32_t mode = cmd[1];
    uint32_t baudrate = convertBytesToUInt32(&cmd[2]);

    spi_init(_spiInst, baudrate);
    gpio_set_function(_clkGP, GPIO_FUNC_SPI);
    gpio_set_function(_mosiGP, GPIO_FUNC_SPI);
    gpio_set_function(_misoGP, GPIO_FUNC_SPI);

    setInterfaceState(InterfaceState::INTIALIZED);
    return CmdStatus::OK;
}

CmdStatus SPIMaster::deInit() {
    spi_deinit(_spiInst);
    setInterfaceState(InterfaceState::NOT_INITIALIZED);
    return CmdStatus::OK;
}



CmdStatus SPIMaster::write(const uint8_t *cmd){
    const uint nbytes = cmd[1];
    int nbWritten = spi_write_blocking (_spiInst, cmd + 2, nbytes);
    if(nbWritten ==  PICO_ERROR_GENERIC || nbWritten != static_cast<int>(nbytes))
        return CmdStatus::NOK;
    return CmdStatus::OK;
}

CmdStatus SPIMaster::read(const uint8_t *cmd, uint8_t *ret){
    const uint8_t writeByte = cmd[1];
    const uint nbytes = cmd[2];
    int nbRead = spi_read_blocking (_spiInst, writeByte, ret + 2, nbytes);
    if(nbRead ==  PICO_ERROR_GENERIC || nbRead != nbytes)
        return CmdStatus::NOK;
    return CmdStatus::OK;
}

CmdStatus SPIMaster::writeFromUart(const uint8_t *cmd){
    flushStreamRx();
    _totalRemainingBytesToSend = convertBytesToUInt32(&cmd[1]);
    //printf("Total = %d", _totalRemainingBytesToSend);
    return CmdStatus::OK;
}

