//control_bts7960.cpp
#include "control_bts7960.h"

// ===== CONFIGURATION =====
const int DEADZONE = 40;        // ระยะฟรีของจอยสติ๊ก (ป้องกันมอเตอร์ครางตอนอยู่ตรงกลาง)
const int PWM_FREQ = 15000;     // 15kHz (ลดเสียงวี๊ดของมอเตอร์)
const int PWM_RES = 8;          // 8-bit (0-255)

// ตัวคูณความนุ่มนวล (Ramping)
// ค่าเข้าใกล้ 0 = นุ่มนวลมาก/ช้า | ค่ามากขึ้น = ตอบสนองไว/กระชาก
const float RAMP_STEP = 10.0;    

// ===== INPUT PWM (Receiver HotRC F-10A) =====
#define CH1_PIN 16  // ต่อเข้ากับ Channel 1 (คุมมอเตอร์ซ้าย)
#define CH3_PIN 17 // ต่อเข้ากับ Channel 2 (คุมมอเตอร์ขวา)

// ===== BTS7960 PINS =====
// ฝั่งซ้าย (Left Motor)
#define L_RPWM 18
#define L_LPWM 19
// ฝั่งขวา (Right Motor)
#define R_RPWM 21
#define R_LPWM 22

// ตัวแปรเก็บสถานะปัจจุบัน (ใช้สำหรับคำนวณความนุ่มนวล)
float currentL = 0;
float currentR = 0;

// ฟังก์ชันคำนวณการไล่ระดับความเร็ว
float applyRamping(float current, int target) {
  if (current < target) {
    current += RAMP_STEP;
    if (current > target) current = target;
  } 
  else if (current > target) {
    current -= RAMP_STEP;
    if (current < target) current = target;
  }
  return current;
}

// ฟังก์ชันสั่งงานมอเตอร์ผ่าน BTS7960
void controlBTS7960(float speed, int chRPWM, int chLPWM) {
  if (speed > 0) { // ทิศทางเดินหน้า (หรือตามที่ Mix ไว้)
    ledcWrite(chRPWM, (int)speed);
    ledcWrite(chLPWM, 0);
  } 
  else if (speed < 0) { // ทิศทางถอยหลัง
    ledcWrite(chRPWM, 0);
    ledcWrite(chLPWM, (int)abs(speed));
  } 
  else { // หยุดนิ่ง
    ledcWrite(chRPWM, 0);
    ledcWrite(chLPWM, 0);
  }
}

void bts7960_setup() {
  
  Serial.println("System Initialized...");

  // ตั้งค่า PWM Channels สำหรับ ESP32 (0-3)
  ledcSetup(0, PWM_FREQ, PWM_RES);
  ledcSetup(1, PWM_FREQ, PWM_RES);
  ledcSetup(2, PWM_FREQ, PWM_RES);
  ledcSetup(3, PWM_FREQ, PWM_RES);

  ledcAttachPin(L_RPWM, 0);
  ledcAttachPin(L_LPWM, 1);
  ledcAttachPin(R_RPWM, 2);
  ledcAttachPin(R_LPWM, 3);

  // กำหนดโหมด Pin สำหรับรับสัญญาณจาก Receiver
  pinMode(CH1_PIN, INPUT);
  pinMode(CH3_PIN, INPUT);
}

void bts7960_loop() {
  // 1. อ่านค่า Pulse จาก Receiver 
  // Timeout 30000us (30ms) เพื่อให้โปรแกรมไม่ค้างนานเกินไปหากสัญญาณหาย
  long rawCH1 = pulseIn(CH1_PIN, HIGH, 30000); 
  long rawCH3 = pulseIn(CH3_PIN, HIGH, 30000);

  currentState.ch1 = rawCH1;
  currentState.ch3 = rawCH3;

  int targetL = 0;
  int targetR = 0;

  // 2. ตรวจสอบระบบความปลอดภัย (Failsafe)
  // หากค่าเป็น 0 แสดงว่าไม่มีสัญญาณจาก Receiver
  if (rawCH1 == 0 || rawCH3 == 0) {
    targetL = 0; 
    targetR = 0;
    // (Optional) พิมพ์แจ้งเตือนใน Serial Monitor เพื่อเช็คอาการ
    // Serial.println("Signal Lost! Failsafe Active."); 
  } else {
    // แปลงค่าพัลส์ (1000-2000) เป็นช่วงความเร็ว (-255 ถึง 255)
    targetL = map(rawCH1, 1000, 2000, -255, 255);
    targetR = map(rawCH3, 1000, 2000, -255, 255);

    // เช็ค Deadzone เพื่อให้หยุดสนิทจริงๆ
    if (abs(targetL) < DEADZONE) targetL = 0;
    if (abs(targetR) < DEADZONE) targetR = 0;
  }

  // 3. ระบบ RAMPING (หัวใจของความสมูท)
  // ค่อยๆ ขยับค่า current เข้าหา target เพื่อลดแรงกระชากและ Back EMF
  currentL = applyRamping(currentL, targetL);
  currentR = applyRamping(currentR, targetR);

  // 4. สั่งงาน Driver BTS7960
  controlBTS7960(currentL, 0, 1); // คุมมอเตอร์ซ้าย
  controlBTS7960(currentR, 2, 3); // คุมมอเตอร์ขวา

  vTaskDelay(pdMS_TO_TICKS(10)); // หน่วงเวลาเล็กน้อยเพื่อให้ลูป Ramping ทำงานสม่ำเสมอ
}

