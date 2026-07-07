////websocket_handler.h
#ifndef WEBSOCKET_HANDLER_H
#define WEBSOCKET_HANDLER_H

#include "config.h"

// โครงสร้างข้อมูลให้ตรงกับหน้าเว็บ
struct dataControl {
    int16_t ch1, ch3, ch6, ch7, ch8;
    bool relay1, relay2, relay3, relay4;
};

extern dataControl currentState;
extern dataControl lastState;

void ws_init();
void ws_process();
void ws_broadcast_serial(const char *msg);

#endif