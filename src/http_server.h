//http_server.h
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H
#include "config.h"

// ฟังก์ชันหลักในการรัน Web Route ทั้งหมด
void initWebRoutes();

// Helpers
String getContentType(String filename);
void handleStaticFiles();
void handleJsonAPI();
void handleCommandAPI();
void handleNotFound();

#endif