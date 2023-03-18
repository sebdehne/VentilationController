
#ifndef DAC_H
#define DAC_H

#include <Arduino.h>

class DacDfr0971Class
{

private:
public:
    DacDfr0971Class();
    void setup();

    void setDacMillivoltage(uint16_t millivoltage);
};

extern DacDfr0971Class DacDfr0971;

#endif