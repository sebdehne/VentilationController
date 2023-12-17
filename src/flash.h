#ifndef _H_FLASH
#define _H_FLASH

#include <Arduino.h>
#include "logger.h"
#include <CRC32.h>

#define FIRMWARE_START_ADDR 0x2000

class FlashUtilsClass
{
private:
    const unsigned long pageSize = 64; // read from NVMCTRL->PARAM.bit.PSZ
    const unsigned long rowSize = pageSize * 4;

    unsigned long addrFirmware;
    unsigned long addrTempFirmware;
    unsigned long addrUserdata;

    uint8_t writeBuffer[4];
    uint8_t writeBufferIndex;
    uint32_t * writeBufferAddr;

    uint32_t *writeAddr;

    void prepareWriting(unsigned long addr, int len);
    uint8_t read(unsigned addr);


public:
    FlashUtilsClass();

    void init();
    unsigned long sectionSize();

    void prepareWritingUserdata(int offset, int len);
    void prepareWritingFirmware();
    void write(uint8_t b);
    void finishWriting();

    unsigned long applyFirmwareAndReset(unsigned long firmwareLength, unsigned long crc32);

    uint8_t readUserdata(unsigned long offset);
};

extern FlashUtilsClass FlashUtils;

#endif