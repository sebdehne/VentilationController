#include "config_service.h"
#include "flash.h"

ConfigServiceClass::ConfigServiceClass()
{
}

void ConfigServiceClass::init()
{
    char logbuf[2];

    if (FlashUtils.readUserdata(0) == 42) // yes, we have valid config in flash - use it
    {
        // config available
        for (int i = 0; i < CONFIG_LEN; i++)
        {
            for (int j = 0; j < CONFIG_WIDTH; j++)
            {
                char c = FlashUtils.readUserdata((i * CONFIG_WIDTH) + j + 1);
                sprintf(logbuf, "%i", c);
                Log.log(logbuf);
                config[i][j] = c;
            }
        }

#ifdef DEBUG
        char buf[1000];
        snprintf(buf, sizeof(buf), "Config loaded from flash");
        Log.log(buf);
#endif
    }
    else
    {
        // no config avaliable, use default values
        strcpy(config[CONFIG_POS_WIFI1_SSID], SECRET_SSID);
        strcpy(config[CONFIG_POS_WIFI1_PASSWD], SECRET_PASS);
        strcpy(config[CONFIG_POS_WIFI2_SSID], SECRET_SSID);
        strcpy(config[CONFIG_POS_WIFI2_PASSWD], SECRET_PASS);
        strcpy(config[CONFIG_POS_BASE], "20");
        strcpy(config[CONFIG_POS_BATH_LIMIT_FROM], "50");
        strcpy(config[CONFIG_POS_BATH_LIMIT_TO], "90");
        strcpy(config[CONFIG_POS_BATH_OFFSET], "-50");
        strcpy(config[CONFIG_POS_BATH_CURVEX1000], "1500");
        strcpy(config[CONFIG_POS_BATH_GIVE_UP], "60");
        strcpy(config[CONFIG_POS_KITCHEN_LIMIT_FROM], "1");
        strcpy(config[CONFIG_POS_KITCHEN_LIMIT_TO], "90");
        strcpy(config[CONFIG_POS_KITCHEN_OFFSET], "10");
        strcpy(config[CONFIG_POS_KITCHEN_CURVEX1000], "1500");
        strcpy(config[CONFIG_POS_KITCHEN_GIVE_UP], "-1");

#ifdef DEBUG
        char buf[1000];
        snprintf(buf, sizeof(buf), "Config loaded from defaults");
        Log.log(buf);
#endif
    }
}

void ConfigServiceClass::writeConfig()
{

    FlashUtils.prepareWritingUserdata(0, CONFIG_LEN * CONFIG_WIDTH + 1);
    FlashUtils.write(42);

    for (int i = 0; i < CONFIG_LEN; i++)
    {
        for (int j = 0; j < CONFIG_WIDTH; j++)
        {
            char c = config[i][j];
            FlashUtils.write(c);
        }
    }
    FlashUtils.finishWriting();
}

ConfigServiceClass ConfigService;
