//main.cpp
#include "config.h"

// --- Global Objects Definition ---
// ประกาศที่นี่ที่เดียวเพื่อให้ Linker หาเจอได้ง่าย
WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device); 

// --- Function Prototypes ---
void TaskMain(void *pvParameters);
void TaskSub(void *pvParameters);
void TaskLED(void *pvParameters);
void TaskSerialReader(void *pvParameters);

void setup() {
    // 1. Initial HW & Debug
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, HIGH);
    pinMode(AP_PIN, INPUT_PULLUP);

    Serial.begin(115200);
    // ไม่ควรใช้ while(!Serial) นานเกินไปในโปรดักชัน เพราะถ้าไม่ต่อคอมเครื่องจะค้าง
    unsigned long startSerial = millis();
    while (!Serial && millis() - startSerial < 3000); 
    
    Serial.println(F("\n[System] Booting..."));

    // 2. Storage Setup
    if (!LittleFS.begin(true)) {
        Serial.println(F("❌ LittleFS Mount Failed"));
    } else {
        Serial.println(F("✅ LittleFS Mounted"));
    }

    // 3. Network & Config Setup
    
    WiFi.onEvent(WiFiEvent);
    wifi_Setup();
    
    // 4. Services Setup
    iotHAsetup();
    
    if (MDNS.begin(HOSTNAME)) {
        Serial.printf("[System] mDNS Started: %s.local\n", HOSTNAME);
    }

    initWebRoutes();
    ws_init();
    server.begin();

    NTPbegin();
  
    app_setup();

    // 5. Watchdog Configuration (ESP32-IDF Style)
    // หมายเหตุ: หากใช้ ESP32 Core 3.x ขึ้นไป อาจต้องปรับ syntax ตามที่แจ้งในรอบก่อน
    esp_task_wdt_init(WDT_TIMEOUT, true);
    esp_task_wdt_delete(NULL); // ไม่ใช้ Loop() หลักคุม WDT

    // 6. Task Creation (แบ่งโหลดความสำคัญ)
    // Core 0: งานหลัก (Processing/Operation)
    // Core 1: งานสื่อสาร (WiFi/MQTT/Web) และ UI (LED)
    xTaskCreatePinnedToCore(TaskMain,           "Main",         4096, NULL, 3, NULL, 0); 
    xTaskCreatePinnedToCore(TaskSub,            "Sub",          4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(TaskLED,            "LED",          2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(TaskSerialReader,   "SerialReader", 4096, NULL, 1, NULL, 1);

    Serial.println(F("[System] Setup Complete"));
}

void loop() {
    // ปล่อยว่างไว้ เพราะเราใช้ TaskManagement (FreeRTOS)
    vTaskDelete(NULL); 
}

// ---------------------------------------------------------
// TaskMain: งานประมวลผลหลัก (Core 0)
// ---------------------------------------------------------
void TaskMain(void *pvParameters) {
    esp_task_wdt_add(NULL); // เพิ่มตัวมันเองลงใน Watchdog
    
    unsigned long lastDiag = 0;
    
    for (;;) {
        esp_task_wdt_reset();
        
        // ฟังก์ชันคำนวณหลัก
        app_loop();

        // Home Assistant Diagnostic ทุก 60 วินาที
        if (millis() - lastDiag > 60000) {
            lastDiag = millis();
            HA_Diagnostic();
            
            // ตรวจสอบ Stack เพื่อป้องกันโปรแกรมแฮงค์ (Debug)
            // Serial.printf("[Debug] Main Stack: %u\n", uxTaskGetStackHighWaterMark(NULL));
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

// ---------------------------------------------------------
// TaskSub: งานสื่อสาร Network (Core 1)
// ---------------------------------------------------------
void TaskSub(void *pvParameters) {
    esp_task_wdt_add(NULL);
    
    unsigned long lastStatus = 0;

    for (;;) {
        esp_task_wdt_reset();
        
        mqtt.loop();
        ws_process();
        APmode_Check();

        // ตรวจสอบสถานะทุก 10 วินาที
        if (millis() - lastStatus > 10000) {
            lastStatus = millis();
            showAPClients();
        }

        vTaskDelay(pdMS_TO_TICKS(5)); // ให้เวลาความสำคัญกับ Network สูงหน่อย
    }
}

// ---------------------------------------------------------
//TaskLED: การแสดงผลไฟสถานะ (Core 1)
// ---------------------------------------------------------
void TaskLED(void *pvParameters) {
    // งาน LED มักไม่จำเป็นต้องลงใน WDT เพราะถ้ามันค้างระบบยังทำงานต่อได้
    for (;;) {
        ledPatternSelect();
        vTaskDelay(pdMS_TO_TICKS(20)); 
    }
}


// ---------------------------------------------------------
// Task สำหรับดักฟัง Serial (Core 0)
// ---------------------------------------------------------
void TaskSerialReader(void *pvParameters) {
    for (;;) {
        if (Serial.available() > 0) {
            // ลองใช้ Serial.readString() แบบไม่มี Until เพื่อดูว่ามีอะไรเข้ามาบ้าง
            String receivedData = Serial.readString(); 
            receivedData.trim(); // ตัด \r \n ทิ้งเอง
            
            Serial.print("Received: "); // Debug ดูว่ามันเห็นอะไร
            Serial.println(receivedData);
            
            processSerialCommand(receivedData);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

