// เก็บสถานะล่าสุดเพื่อตรวจสอบการเปลี่ยนแปลง (Optimization)
let lastData = {
    ch1: 0, ch3: 0, ch6: 0, ch7: 0, ch8: 0,
    relay1: false, relay2: false, relay3: false, relay4: false
};

var websocket;

function initWebSocket() {
    // เชื่อมต่อ WebSocket
    websocket = new WebSocket(`ws://${window.location.hostname}/ws`);

    websocket.onmessage = (event) => {
        try {
            let json = JSON.parse(event.data);
            console.log("Received data:", json);

            // 1. อัปเดตค่า Channel (ตัวเลข)
            updateChannelValue("chanel1", json.ch1);
            updateChannelValue("chanel3", json.ch3);
            updateChannelValue("chanel6", json.ch6);
            updateChannelValue("chanel7", json.ch7);
            updateChannelValue("chanel8", json.ch8);

            // 2. อัปเดตสถานะ Relay (ON/OFF + ไฟกระพริบ)
            // สมมติ JSON ส่งมาเป็น { "r1": true, "r2": false ... }
            updateRelayStatus("relay1", "lamp1", json.relay1);
            updateRelayStatus("relay2", "lamp2", json.relay2);
            updateRelayStatus("relay3", "lamp3", json.relay3);
            updateRelayStatus("relay4", "lamp4", json.relay4);

        } catch (e) {
            console.error("JSON parsing error:", e);
        }
    };

    websocket.onclose = () => {
        console.log("WebSocket disconnected. Retrying...");
        setTimeout(initWebSocket, 2000); // พยายามเชื่อมต่อใหม่ทุก 2 วินาที
    };
}

// ฟังก์ชันอัปเดตตัวเลข Channel
function updateChannelValue(id, value) {
    const el = document.getElementById(id);
    if (el && value !== undefined) {
        el.innerText = value;
    }
}

/**
 * ฟังก์ชันอัปเดต Relay และ Lamp
 * @param {string} textId - ID ของ <span> ที่แสดง ON/OFF
 * @param {string} lampId - ID ของ <div> ที่แสดงดวงไฟ
 * @param {boolean} state - สถานะ true/false
 */
function updateRelayStatus(textId, lampId, state) {
    const textEl = document.getElementById(textId);
    const lampEl = document.getElementById(lampId);

    if (textEl) {
        textEl.innerText = state ? "ON" : "OFF";
        textEl.style.color = state ? "#00ff00" : "#ff4d4d"; // แถม: เปลี่ยนสีตัวอักษร
    }

    if (lampEl) {
        if (state) {
            lampEl.classList.add("lamp-on");
            lampEl.classList.remove("lamp-off");
        } else {
            lampEl.classList.add("lamp-off");
            lampEl.classList.remove("lamp-on");
        }
    }
}

window.addEventListener("load", initWebSocket);