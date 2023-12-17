# Smart ventilation controller
Arduino code which controls a ventilation fan which uses a 0-10V interface, 
like the [VEF-2 EC](https://shop.systemair.com/no-NO/vef--2--ec/p399812) by utilizing the [Df-Robot SKU DFR0971 0-10V DAC](https://wiki.dfrobot.com/SKU_DFR0971_2_Channel_I2C_0_10V_DAC_Module). It 
takes three parameters into account:

1. Configured base ventilation level; it will never go below that
2. Selected fan speed at the kitchen cooking hood
3. Relative humidity in the bath room (using the [Adafruit BME280 I2C Humidity sensor](https://www.adafruit.com/product/2652))


The control loop is simple: whichever parameter requests the highest fan speed wins, either the 
selected speed in the kitchen or the bath room sensor which reports too high humidity value.

## Wifi support
It has wifi support and runs a simple Webserver, which lets the user access all
parameters and change them. Two pairs of Wifi SSID/Pass settings can be configured, 
such that if one pair does not work, the alternative pair will be tried.

## How to extract the selected fan speed from the AC kitchen cooking hoods

When using a central fan, the kitchen hood should be without a built in fan. Unfortunately, there 
eems to be a rather limited variety of hoods, which support 0-10V fans, to choose from; at least here in Norway.

But it is possible to read out the selected fan speed from any (AC) kitchen hood with a little bit of 
electronics. In my case, I bought an standard AC kitchen hood, which turns out to produce a PWM 
signal (20V DC) internally. This signal is quite easy to extract by feeding it through a voltage 
divider and into a digital input pin in the arduino.

