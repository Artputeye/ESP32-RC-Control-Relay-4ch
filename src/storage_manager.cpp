//storage_manager.cpp
#include "storage_manager.h"

// แสดงรายการไฟล์ทั้งหมด (รองรับ Sub-folder)
void listAllFiles(const char *dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\n", dirname);

    File root = LittleFS.open(dirname);
    if (!root || !root.isDirectory()) {
        Serial.println(F("❌ Failed to open directory or not a directory"));
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.printf("  DIR : %s\n", file.name());
            if (levels > 0) {
                listAllFiles(file.path(), levels - 1);
            }
        } else {
            Serial.printf("  FILE: %-20s  SIZE: %u bytes\n", file.name(), file.size());
        }
        file = root.openNextFile();
    }
}

void formatFS() {
    Serial.println(F("⚠️ Formatting LittleFS..."));
    if (LittleFS.format()) {
        Serial.println(F("✅ Format successful"));
    } else {
        Serial.println(F("❌ Format failed"));
    }
}

// ฟังก์ชันกลางสำหรับโหลด JSON
bool loadJsonFile(const char *filename, JsonDocument &doc) {
    if (!LittleFS.exists(filename)) {
        Serial.printf("⚠️ File not found: %s\n", filename);
        return false;
    }

    File file = LittleFS.open(filename, "r");
    if (!file) return false;

    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("❌ JSON Parse Error (%s): %s\n", filename, error.c_str());
        return false;
    }
    return true;
}

// ฟังก์ชันกลางสำหรับบันทึก JSON
bool saveJsonFile(const char *filename, const JsonDocument &doc) {
    File file = LittleFS.open(filename, "w"); // "w" จะทับไฟล์เดิมโดยอัตโนมัติ
    if (!file) {
        Serial.printf("❌ Failed to open %s for writing\n", filename);
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.printf("❌ Failed to write to %s\n", filename);
        file.close();
        return false;
    }

    file.close();
    Serial.printf("✅ Saved: %s\n", filename);
    return true;
}

// โหลดการตั้งค่าทั้งหมดของระบบ
bool loadAllSettings() {
    JsonDocument doc;

    // 1. Load Network Config
    if (loadJsonFile("/networkconfig.json", doc)) {
        isWifiApMode = doc["wifi_mode"] | 0;
        Serial.println(F("📂 Network settings loaded"));
    } else {
        isWifiApMode = 0; // Default
    }

    // 2. Load General Settings
    doc.clear(); // ล้างข้อมูลเดิมก่อนโหลดไฟล์ใหม่
    if (loadJsonFile("/setting.json", doc)) {
        // ดึงค่าอื่นๆ เช่น fsFile = doc["file"] | 0;
        Serial.println(F("📂 General settings loaded"));
    }

    return true;
}

// บันทึกเฉพาะโหมด WiFi (เช่น เมื่อกดเปลี่ยนโหมดผ่านปุ่ม IO0)
bool saveWifiModeSetting() {
    JsonDocument doc;
    
    // โหลดไฟล์เดิมมาก่อนเพื่อไม่ให้ค่าอื่นๆ หาย (ถ้ามีข้อมูลอื่นในไฟล์นั้น)
    loadJsonFile("/networkconfig.json", doc);
    
    doc["wifi_mode"] = isWifiApMode;
    
    return saveJsonFile("/networkconfig.json", doc);
}