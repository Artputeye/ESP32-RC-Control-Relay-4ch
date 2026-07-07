// control_relay4ch.cpp
#include "control_relay4ch.h"

#define CH6_PIN 35
#define CH7_PIN 34
#define CH8_PIN 27

#define RELAY1 32
#define RELAY2 33
#define RELAY3 25
#define RELAY4 26

#define RMT_CLK_DIV 80 // 1 tick = 1us

rmt_channel_t channels[3] = {
    RMT_CHANNEL_0,
    RMT_CHANNEL_1,
    RMT_CHANNEL_2};

RingbufHandle_t rb[3];

uint32_t pulseWidth[3] = {0, 0, 0};

bool relayState[4] = {0};
bool signalActive = false;

uint32_t lastSwitchTime = 0;
uint8_t lastMode = 1;
uint32_t lastSignalTime = 0;

// ===== RMT INIT =====
void setupRMT(gpio_num_t pin, rmt_channel_t ch, int idx)
{
  rmt_config_t config = {};
  config.rmt_mode = RMT_MODE_RX;
  config.channel = ch;
  config.gpio_num = pin;
  config.clk_div = RMT_CLK_DIV;

  // 🔥 ใส่ตรงนี้
  config.mem_block_num = 2; // จากเดิม 1 → เพิ่ม buffer ใน HW

  config.rx_config.filter_en = true;
  config.rx_config.filter_ticks_thresh = 50;
  config.rx_config.idle_threshold = 10000;

  rmt_config(&config);

  // 🔥 ใส่ตรงนี้ (สำคัญ)
  rmt_driver_install(ch, 2048, 0); // จากเดิม 1024 → เพิ่ม buffer ใน RAM

  rmt_get_ringbuf_handle(ch, &rb[idx]);
  rmt_rx_start(ch, true);
}

// ===== อ่าน PWM =====
uint32_t readPWM(int idx)
{
  size_t rx_size = 0;
  uint32_t result = 0;

  while (true)
  {
    rmt_item32_t *item = (rmt_item32_t *)
        xRingbufferReceive(rb[idx], &rx_size, 0); // 🔥 non-block

    if (!item)
      break;

    int num = rx_size / sizeof(rmt_item32_t);

    for (int i = 0; i < num; i++)
    {
      uint32_t d0 = item[i].duration0;
      uint32_t d1 = item[i].duration1;

      if (d0 > 900 && d0 < 2100)
        result = d0;
      if (d1 > 900 && d1 < 2100)
        result = d1;
      Serial.printf("[CH%d] items=%d\n", idx + 6, num);
      Serial.printf("[CH%d] d0=%d d1=%d\n", idx + 6, d0, d1);
    }

    vRingbufferReturnItem(rb[idx], (void *)item);
  }

  return result;
}

void relay4ch_setup()
{
  Serial.begin(115200);

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);

  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  digitalWrite(RELAY3, LOW);
  digitalWrite(RELAY4, LOW);

  setupRMT((gpio_num_t)CH6_PIN, channels[0], 0);
  setupRMT((gpio_num_t)CH7_PIN, channels[1], 1);
  setupRMT((gpio_num_t)CH8_PIN, channels[2], 2);
}

void relay4ch_loop()
{
  // ===== อ่าน PWM =====
  pulseWidth[0] = readPWM(0);
  pulseWidth[1] = readPWM(1);
  pulseWidth[2] = readPWM(2);

  uint32_t ch6 = pulseWidth[0];
  uint32_t ch7 = pulseWidth[1];
  uint32_t ch8 = pulseWidth[2];

  currentState.ch6 = ch6;
  currentState.ch7 = ch7;
  currentState.ch8 = ch8;

  // ===== 🔥 กันค่า 0 (สำคัญมาก) =====
  static uint32_t lastValid[3] = {1500, 1500, 1500};

  if (ch6 > 500)
    lastValid[0] = ch6;
  if (ch7 > 500)
    lastValid[1] = ch7;
  if (ch8 > 500)
    lastValid[2] = ch8;

  ch6 = lastValid[0];
  ch7 = lastValid[1];
  ch8 = lastValid[2];

  // ===== signal valid =====
  bool signalValid =
      (ch6 > 900 && ch6 < 2100) &&
      (ch7 > 900 && ch7 < 2100) &&
      (ch8 > 900 && ch8 < 2100);

  if (signalValid)
  {
    lastSignalTime = millis();
  }

  // ===== failsafe =====
  if (millis() - lastSignalTime > 500)
  {
    signalActive = false;

    // 🔥 reset กันค้าง
    lastValid[0] = 1500;
    lastValid[1] = 1500;
    lastValid[2] = 1500;
  }

  // ===== ต้องขยับก่อน (กันตอนเปิดเครื่อง) =====
  if (!signalActive && signalValid)
  {
    if (abs((int)ch6 - 1500) > 100 ||
        abs((int)ch7 - 1500) > 100 ||
        abs((int)ch8 - 1500) > 100)
    {
      signalActive = true;
    }
  }

  // ===== ยังไม่พร้อม =====
  if (!signalValid || !signalActive)
  {
    relayState[0] = 0;
    relayState[1] = 0;
    relayState[2] = 0;
    relayState[3] = 0;
  }
  else
  {
    // ===== RELAY1 (CH6) =====
    if (ch6 > 1700)
      relayState[0] = 1;
    else if (ch6 < 1300)
      relayState[0] = 0;

    // ===== RELAY2 (CH7) =====
    if (ch7 > 1700)
      relayState[1] = 1;
    else if (ch7 < 1300)
      relayState[1] = 0;

    // ===== RELAY3 / RELAY4 (CH8 3-STATE) =====
    if (ch8 < 1300)
    {
      relayState[2] = 1; // relay3
      relayState[3] = 0;
    }
    else if (ch8 > 1700)
    {
      relayState[2] = 0;
      relayState[3] = 1; // relay4
    }
    else
    {
      relayState[2] = 0;
      relayState[3] = 0;
    }
  }

  // ===== เขียน output =====
  digitalWrite(RELAY1, relayState[0]);
  digitalWrite(RELAY2, relayState[1]);
  digitalWrite(RELAY3, relayState[2]);
  digitalWrite(RELAY4, relayState[3]);

  currentState.relay1 = relayState[0];
  currentState.relay2 = relayState[1];
  currentState.relay3 = relayState[2];
  currentState.relay4 = relayState[3];

  // ===== DEBUG =====
  static int last_ch6 = 0;
  static int last_ch7 = 0;
  static int last_ch8 = 0;
  static int last_relay[4] = {0, 0, 0, 0};
  static bool last_signalActive = false;

  bool changed = false;

  // เช็คแต่ละค่า
  if (ch6 != last_ch6)
    changed = true;
  if (ch7 != last_ch7)
    changed = true;
  if (ch8 != last_ch8)
    changed = true;

  for (int i = 0; i < 4; i++)
  {
    if (relayState[i] != last_relay[i])
    {
      changed = true;
      break;
    }
  }

  if (signalActive != last_signalActive)
    changed = true;

  // ===== print เฉพาะตอนเปลี่ยน =====
  if (changed)
  {
    Serial.printf("CH6:%d CH7:%d CH8:%d | R:%d,%d,%d,%d | Active:%d\n",
                  ch6, ch7, ch8,
                  relayState[0],
                  relayState[1],
                  relayState[2],
                  relayState[3],
                  signalActive);

    // update ค่าเก่า
    last_ch6 = ch6;
    last_ch7 = ch7;
    last_ch8 = ch8;

    for (int i = 0; i < 4; i++)
      last_relay[i] = relayState[i];

    last_signalActive = signalActive;
  }
}

void serialRelay(String key, String val)
{
  if (key == "RELAY1")
  {
    if (val == "ON")
    {
      digitalWrite(RELAY1, true);
      currentState.relay1 = true; // อัปเดตสถานะไปหน้าเว็บด้วย
    }
    else if (val == "OFF")
    {
      digitalWrite(RELAY1, false);
      currentState.relay1 = false; // อัปเดตสถานะไปหน้าเว็บด้วย
    }
  }
  
}
