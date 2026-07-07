//websocket_handler.cpp
#include "websocket_handler.h"

//Ex Use currentState.relay1 = digitalRead(RELAY_PIN);

AsyncWebSocket ws("/ws");

// --- Configuration ---
#define WS_UPDATE_INTERVAL 50   // 20Hz (ส่งข้อมูลทุก 50ms)
#define WS_FORCE_SEND_MS 500    // ส่งข้อมูลครบชุดทุก 0.5 วินาที แม้ค่าไม่เปลี่ยน

// --- System State ---
unsigned long ws_last_update_time = 0;
unsigned long ws_last_force_send = 0;

dataControl currentState = {0};
dataControl lastState = {0};


// --- Helper Functions ---
// ตรวจสอบการเปลี่ยนแปลงของสถานะ (State Tracking)
template <typename T>
bool hasChanged(T &oldVal, T newVal) {
    if (oldVal != newVal) {
        oldVal = newVal;
        return true;
    }
    return false;
}

// --- Core Logic ---
void ws_broadcast_telemetry() {
    // ถ้าไม่มีคนเชื่อมต่อ หรือ Buffer เต็ม ให้ข้ามไป
    if (ws.count() == 0 || ws.availableForWriteAll() == 0) return;

    bool forceUpdate = (millis() - ws_last_force_send > WS_FORCE_SEND_MS);
    bool changed = false;
    
    String json = "{";
    bool firstEntry = true;

    // Lambda สำหรับช่วยต่อ JSON String
    auto appendJson = [&](const String &key, const String &value) {
        if (!firstEntry) json += ",";
        json += "\"" + key + "\":" + value;
        firstEntry = false;
        changed = true;
    };

    // ตรวจสอบค่า Channel (ch1, ch3, ch6, ch7, ch8)
    if (hasChanged(lastState.ch1, currentState.ch1) || forceUpdate) appendJson("ch1", String(currentState.ch1));
    if (hasChanged(lastState.ch3, currentState.ch3) || forceUpdate) appendJson("ch3", String(currentState.ch3));
    if (hasChanged(lastState.ch6, currentState.ch6) || forceUpdate) appendJson("ch6", String(currentState.ch6));
    if (hasChanged(lastState.ch7, currentState.ch7) || forceUpdate) appendJson("ch7", String(currentState.ch7));
    if (hasChanged(lastState.ch8, currentState.ch8) || forceUpdate) appendJson("ch8", String(currentState.ch8));

    // ตรวจสอบค่า Relay (relay1, relay2, relay3, relay4)
    if (hasChanged(lastState.relay1, currentState.relay1) || forceUpdate) appendJson("relay1", currentState.relay1 ? "true" : "false");
    if (hasChanged(lastState.relay2, currentState.relay2) || forceUpdate) appendJson("relay2", currentState.relay2 ? "true" : "false");
    if (hasChanged(lastState.relay3, currentState.relay3) || forceUpdate) appendJson("relay3", currentState.relay3 ? "true" : "false");
    if (hasChanged(lastState.relay4, currentState.relay4) || forceUpdate) appendJson("relay4", currentState.relay4 ? "true" : "false");

    json += "}";

    if (changed) { 
        ws.textAll(json);
        if (forceUpdate) ws_last_force_send = millis();
    }
}

// --- WebSocket Events ---
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[WS] Client %u connected\n", client->id());
        // เมื่อ Client ต่อเข้ามาใหม่ ให้ Force Send ทันทีเพื่อให้หน้าเว็บมีข้อมูล
        ws_last_force_send = 0; 
    } 
    else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("[WS] Client %u disconnected\n", client->id());
    }
}

// --- Public API ---
void ws_init() {
    ws.onEvent(onWsEvent);
    server.addHandler(&ws); // server ต้องประกาศเป็น extern หรือ global ใน main
    Serial.println(F("[WS] WebSocket Server Initialized"));
}

void ws_process() {
    // 1. จัดการส่งข้อมูล Serial หากมีข้อความใหม่เข้ามา
    // ตรวจสอบว่า lastSerialMsg ไม่ว่างเปล่า
    if (lastSerialMsg.length() > 0) {
        ws_broadcast_serial(lastSerialMsg.c_str());
        
        // เคลียร์ค่าทิ้งเพื่อป้องกันการส่งซ้ำใน Loop ถัดไป
        lastSerialMsg = ""; 
    }

    // 2. ส่งข้อมูล Telemetry (Channel & Relay) ตามรอบเวลา
    if (millis() - ws_last_update_time > WS_UPDATE_INTERVAL) {
        ws_broadcast_telemetry();
        ws_last_update_time = millis();
    }

    // 3. จัดการคืนค่า Memory ของ Client ที่หลุดไป
    ws.cleanupClients();
}

// --- Serial WS ---
void ws_broadcast_serial(const char *msg) {
    if (msg && msg[0] != '\0' && ws.count() > 0) {
        String json = "{\"Serial\":\"";
        json += msg;
        json += "\"}";
        ws.textAll(json);
    }
}

