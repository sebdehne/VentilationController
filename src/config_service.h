#ifndef _H_CONFIG_SERVICE
#define _H_CONFIG_SERVICE

#include <Arduino.h>
#include "logger.h"
#include "flash.h"
#include "secrets.h"

#define CONFIG_NAME_WIFI1_SSID "wifi1-ssid"
#define CONFIG_NAME_WIFI1_PASSWD "wifi1-pass"
#define CONFIG_NAME_WIFI2_SSID "wifi2-ssid"
#define CONFIG_NAME_WIFI2_PASSWD "wifi2-pass"
#define CONFIG_NAME_BASE "base"

#define CONFIG_POS_WIFI1_SSID 0
#define CONFIG_POS_WIFI1_PASSWD 1
#define CONFIG_POS_WIFI2_SSID 2
#define CONFIG_POS_WIFI2_PASSWD 3
#define CONFIG_POS_BASE 4
#define CONFIG_POS_BATH_LIMIT_FROM 5
#define CONFIG_POS_BATH_LIMIT_TO 6
#define CONFIG_POS_BATH_OFFSET 7
#define CONFIG_POS_BATH_CURVEX1000 8

// if the sensor has been giving a signal above the lower limit for N amount of time, then
// disable the output until is has gone below the lower-limit.
#define CONFIG_POS_BATH_GIVE_UP 9
#define CONFIG_POS_KITCHEN_LIMIT_FROM 10
#define CONFIG_POS_KITCHEN_LIMIT_TO 11
#define CONFIG_POS_KITCHEN_OFFSET 12
#define CONFIG_POS_KITCHEN_CURVEX1000 13
#define CONFIG_POS_KITCHEN_GIVE_UP 14
#define CONFIG_LEN 15

#define CONFIG_WIDTH 20

static const char *configNames[CONFIG_LEN] = {
    "wifi1-ssid",
    "wifi1-pass",
    "wifi2-ssid",
    "wifi2-pass",
    "base",
    "bath-limit-from",
    "bath-limit-to",
    "bath-offset",
    "bath-curve-x-1000",
    "bath-give-up-after-minutes",
    "kitchen-limit-from",
    "kitchen-limit-to",
    "kitchen-offset",
    "kitchen-curve-x-1000",
    "kitchen-give-up-after-minutes"
};

class ConfigServiceClass
{
private:
public:
    ConfigServiceClass();

    char config[CONFIG_LEN][CONFIG_WIDTH];

    void init();

    /* Reads 'config' and writes into to flash */
    void writeConfig();
};

extern ConfigServiceClass ConfigService;

#endif
