//control_relay4ch.h
#ifndef CONTROL_RELAY4CH_H
#define CONTROL_RELAY4CH_H
#include "config.h"

void relay4ch_setup();
void relay4ch_loop();
void serialRelay(String key, String val);

#endif