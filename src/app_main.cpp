//app_main.cpp
#include "app_main.h"

void app_setup()
{
    bts7960_setup();
    relay4ch_setup();
}

void app_loop()
{
    bts7960_loop();
    relay4ch_loop();
}

void updateSystemStatus()
{
}