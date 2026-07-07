//ha_integration.h
#ifndef HA_INTEGRATION_H
#define HA_INTEGRATION_H
#include "config.h"

// ตัวแปรที่อนุญาตให้ไฟล์อื่นเรียกใช้
extern HASwitch sw;
extern HASensorNumber temp;

// ฟังก์ชันหลัก
void iotHAsetup();
void HA_Diagnostic();
void iotHArun();

#endif