//ha_integration.cpp
#include "ha_integration.h"

// --- Diagnostic Entities ---
HASensor ipAddrSensor("ip_address");
HASensor macAddrSensor("mac_address");
HASensor uptimeSensor("uptime");
HASensor rssiSensor("rssi");

// --- Control & Sensor Entities ---
HASwitch sw("sw1");
HASensorNumber temp("temp_sensor", HASensorNumber::PrecisionP1); // ทศนิยม 1 ตำแหน่ง

void iotHAsetup() {
    Serial.println(F("[HA] Initializing Device Details..."));

    // 1. ตั้งค่าข้อมูลพื้นฐานของอุปกรณ์ (ต้องทำก่อนเริ่ม mqtt.begin)
    device.setName(DEVICE_NAME);
    device.setSoftwareVersion(D_SoftwareVersion);
    device.setManufacturer(D_Mfac);
    device.setModel(D_Model);
    
    // 2. ตั้งค่า Entities (สวิตช์/เซนเซอร์)
    sw.setName("Main Switch");
    sw.setIcon("mdi:power");

    temp.setName("Temperature");
    temp.setUnitOfMeasurement("°C");
    temp.setDeviceClass("temperature");

    // 3. ตั้งค่า Diagnostic Entities
    ipAddrSensor.setName("IP Address");
    ipAddrSensor.setIcon("mdi:ip-network");

    macAddrSensor.setName("MAC Address");
    macAddrSensor.setIcon("mdi:lan-connect");

    uptimeSensor.setName("Uptime");
    uptimeSensor.setUnitOfMeasurement("s");
    uptimeSensor.setIcon("mdi:timer-outline");

    rssiSensor.setName("WiFi Signal");
    rssiSensor.setUnitOfMeasurement("dBm");
    rssiSensor.setDeviceClass("signal_strength");

    // 4. เตรียมการเชื่อมต่อ MQTT
    uint16_t port = (uint16_t)atoi(MQTT_PORT);
    IPAddress mqttIP;
    
    if (mqttIP.fromString(MQTT_ADDR)) {
        Serial.printf("[HA] Connecting to MQTT: %s:%d\n", MQTT_ADDR, port);
        mqtt.begin(mqttIP, port, MQTT_USER, MQTT_PASS);
    } else {
        Serial.println(F("❌ [HA] Invalid MQTT IP Address"));
    }
}

void HA_Diagnostic() {
    // 1. IP และ MAC
    ipAddrSensor.setValue(WiFi.localIP().toString().c_str());
    
    // แก้ไขจุดนี้: เติม .c_str() เพื่อแปลง String เป็น const char*
    macAddrSensor.setValue(MacAddr.c_str()); 
    
    // 2. Uptime (แปลง uint32_t เป็น String แล้วต่อด้วย .c_str())
    uint32_t currentUptime = millis() / 1000;
    uptimeSensor.setValue(String(currentUptime).c_str());
    
    // 3. RSSI (แปลง int8_t เป็น String แล้วต่อด้วย .c_str())
    int8_t rssiVal = WiFi.RSSI();
    rssiSensor.setValue(String(rssiVal).c_str());
    
    Serial.printf("[HA] Diag Update - Uptime: %u s, RSSI: %d dBm\n", currentUptime, rssiVal);
}

void iotHArun() {
    // ฟังก์ชันนี้ต้องถูกเรียกใน loop() หลักของโปรแกรม
    mqtt.loop();

    // อัปเดต Diagnostic ทุกๆ 30 วินาที (ตัวอย่าง)
    static unsigned long lastDiag = 0;
    if (millis() - lastDiag > 30000) {
        lastDiag = millis();
        HA_Diagnostic();
    }
}