function sendConfig() {
  const inputs = document.querySelectorAll('.setting-input input');
  const config = {};

  // เก็บค่าจาก input text
  inputs.forEach(input => {
    if (input.id && input.value !== "") {
      config[input.id] = input.value;
    }
  });

  // เก็บค่าจาก toggle switch
  config.wifi_mode = document.getElementById("wifiModeToggle").checked ? 1 : 0;
  config.ip_config = document.getElementById("ipConfigToggle").checked ? 1 : 0;

  fetch('/networkconfig', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(config)
  })
    .then(response => {
      if (response.ok) {
        alert("Configuration saved successfully.");
        fetchToserver("espreset");
      } else {
        alert("Error saving configuration.");
      }
    })
    .catch(error => {
      console.error('Fetch error:', error);
      alert("Fetch failed.");
    });
  console.log(config);
}

function wifiMode(checkbox) {
  const mode = document.getElementById("wifi-mode");
  const status = checkbox.checked ? "STATION" : "ACCESS POINT";
  mode.textContent = status;
}

document.querySelectorAll('.icon-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    const targetId = btn.getAttribute('data-target');
    const input = document.getElementById(targetId);

    // --- แก้ไขตรงนี้: เช็คว่ามี input อยู่จริงไหมก่อนทำอย่างอื่น ---
    if (!input) {
      console.error(`Element with id="${targetId}" not found`);
      return;
    }
    // ------------------------------------------------------

    // เมื่อมั่นใจว่ามี input แน่นอนแล้ว ถึงจะเข้าถึง properties ได้
    const isShown = input.type === 'text';
    input.type = isShown ? 'password' : 'text';

    btn.setAttribute('aria-pressed', String(!isShown));
    btn.setAttribute('aria-label', isShown ? 'แสดงรหัสผ่าน' : 'ซ่อนรหัสผ่าน');
  });
});

/////////////////////////////////////////////////////////////////////////////////////////////////
// โหลด config ตอนเปิดหน้า
window.addEventListener("load", () => {
  loadConfig();
});

function setElementValue(id, value) {
    const el = document.getElementById(id);
    if (el) {
        el.value = value || "";
    } else {
        console.warn(`Element with id "${id}" not found in HTML.`);
    }
}

function loadConfig() {
  fetch("/getnetworkconfig")
    .then(res => res.json())
    .then(config => {
      console.log("Received config:", config);

      // ใช้ฟังก์ชันช่วยเช็คเพื่อไม่ให้เกิด Error properties of null
      setElementValue("wifi_ssid", config.wifi_ssid);
      setElementValue("wifi_pass", config.wifi_pass);
      setElementValue("mac_receive", config.mac_receive);

      setElementValue("ip_address", config.ip_address);
      setElementValue("subnet_mask", config.subnet_mask);
      setElementValue("gateway", config.gateway);
      
      setElementValue("mqtt_user", config.mqtt_user);
      setElementValue("mqtt_pass", config.mqtt_password);
      setElementValue("mqtt_server", config.mqtt_server);
      setElementValue("mqtt_port", config.mqtt_port);

      // ส่วนของ Toggle ก็ควรเช็คเหมือนกัน
      const wifiToggle = document.getElementById("wifiModeToggle");
      const wifiModeLabel = document.getElementById("wifi-mode");
      if (wifiToggle) {
          wifiToggle.checked = config.wifi_mode === "1";
          if (wifiModeLabel) {
              wifiModeLabel.textContent = config.wifi_mode === "1" ? "STATION" : "ACCESS POINT";
          }
      }

      const ipConfigToggle = document.getElementById("ipConfigToggle");
      const ipHideSection = document.getElementById("ip-hide");
      if (ipConfigToggle) {
          ipConfigToggle.checked = config.ip_config === "1";
          if (ipHideSection) {
              ipHideSection.style.display = config.ip_config === "1" ? "block" : "none";
          }
      }
    })
    .catch(err => console.error("Config load error:", err));
}

// เวลา toggle IP Config เปลี่ยน
function IPconfig(el) {
  if (el.checked) {
    document.getElementById("ip-hide").style.display = "block";
  } else {
    document.getElementById("ip-hide").style.display = "none";
  }
}

function toggleSetting(checkbox, settingName) {
  const status = checkbox.checked ? "1" : "0";
  const cmd = settingName + " " + status;
  console.log(cmd);
  //fetchToserver(cmd);
}

function fetchToserver(message) {
    console.log(`${message} to Server`);
    const formdata = new FormData();
    formdata.append("plain", message);
    const requestOptions = {
        method: "POST",
        body: formdata,
        redirect: "follow"
    };
    fetch("/cmd", requestOptions)
        .then((response) => response.text())
        .then((result) => console.log("Respond:", result))
        .catch((error) => console.error("Error:", error));
}