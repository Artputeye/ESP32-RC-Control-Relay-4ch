//http_server.cpp
#include "http_server.h"

void initWebRoutes() {
    setupOTAManagement(); // เรียกใช้จาก ota.cpp
    handleStaticFiles();
    handleJsonAPI();
    handleCommandAPI();
    handleNotFound();
}

// --- Helper: ส่งไฟล์ JSON จาก LittleFS ไปยัง Client ---
void sendJsonResponse(AsyncWebServerRequest *request, const char* filename) {
    if (!LittleFS.exists(filename)) {
        request->send(404, "application/json", "{\"error\":\"File not found\"}");
        return;
    }
    request->send(LittleFS, filename, "application/json");
}

// --- Helper: รับ JSON จาก Client แล้วบันทึกลง LittleFS ---
void saveJsonFromRequest(AsyncWebServerRequest *request, uint8_t *data, size_t len, const char* filename) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        request->send(400, "application/json", "{\"status\":\"error\",\"msg\":\"Invalid JSON\"}");
        return;
    }

    if (saveJsonFile(filename, doc)) { // ใช้ฟังก์ชันจาก fsFile.cpp
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        request->send(500, "application/json", "{\"status\":\"error\",\"msg\":\"Save failed\"}");
    }
}

void handleStaticFiles() {
    // 1. เสิร์ฟไฟล์พื้นฐานทั้งหมด
    server.serveStatic("/", LittleFS, "/")
          .setDefaultFile("index.html")
          .setCacheControl("max-age=86400");

    // 2. Map path สั้นๆ ไปยังไฟล์ .html (เช่น /set -> /set.html)
    const char *pages[] = {"/set", "/ota", "/batt", "/device", "/filelist", "/info", "/monitor", "/network"};
    for (const char* p : pages) {
        server.on(p, HTTP_GET, [p](AsyncWebServerRequest *request) {
            String path = String(p) + ".html";
            if (LittleFS.exists(path)) {
                request->send(LittleFS, path, "text/html");
            } else {
                request->send(404, "text/plain", "Page Not Found");
            }
        });
    }
}

void handleJsonAPI() {
    // API: ดึงค่า (GET)
    server.on("/getsetting", HTTP_GET, [](AsyncWebServerRequest *request) {
        sendJsonResponse(request, "/setting.json");
    });

    server.on("/getnetworkconfig", HTTP_GET, [](AsyncWebServerRequest *request) {
        sendJsonResponse(request, "/networkconfig.json");
    });

    // API: บันทึกค่า (POST)
    server.on("/savesetting", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            saveJsonFromRequest(request, data, len, "/setting.json");
    });

    server.on("/networkconfig", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            saveJsonFromRequest(request, data, len, "/networkconfig.json");
            // หากต้องการให้ Restart หลังจากแก้ Network ให้ใส่ flag หรือเรียก ESP.restart() ที่นี่
    });
}

void handleCommandAPI() {
    server.on("/cmd", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("plain", true)) {
            String msg = request->getParam("plain", true)->value();
            Serial.printf("[CMD] Received: %s\n", msg.c_str());
            request->send(200, "text/plain", "OK: " + msg);
        } else {
            request->send(400, "text/plain", "No message");
        }
    });
}

void handleNotFound() {
    server.onNotFound([](AsyncWebServerRequest *request) {
        String path = request->url();
        if (LittleFS.exists(path)) {
            request->send(LittleFS, path, getContentType(path));
        } else {
            request->send(404, "text/plain", "404: Not Found");
        }
    });
}

String getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    if (filename.endsWith(".css"))  return "text/css";
    if (filename.endsWith(".js"))   return "application/javascript";
    if (filename.endsWith(".png"))  return "image/png";
    if (filename.endsWith(".jpg"))  return "image/jpeg";
    if (filename.endsWith(".ico"))  return "image/x-icon";
    if (filename.endsWith(".json")) return "application/json";
    return "text/plain";
}