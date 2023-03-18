#include <Arduino.h>
#include "dac_dfr0971.h"
#include <Wire.h>
#include "logger.h"

DacDfr0971Class DacDfr0971;

DacDfr0971Class::DacDfr0971Class()
{
}

void DacDfr0971Class::setup()
{
    Wire.begin();
    Wire.setClock(400000);
    while (1)
    {
        Wire.beginTransmission(0x5f);
        Wire.write(0X01); // reg output range
        Wire.write(0X11); // 0-10V range
        int status = Wire.endTransmission();
        if (status == 0)
        {
            break;
        }
        Log.log("init error Dac Dfr0971");
        delay(1000);
    }

}

void DacDfr0971Class::setDacMillivoltage(uint16_t millivoltage)
{
    // 0..10000 -> 0..65535
    uint16_t data = millivoltage * 65535 / 10000;

    Wire.beginTransmission(0x5f);
    Wire.write(0X02); // voiltage reg
    Wire.write(data & 0xff);
    Wire.write((data >> 8) & 0xff);
    Wire.endTransmission();
}