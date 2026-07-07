var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getReadings() {
    websocket.send("getReadings");
}

function initWebSocket() {
    websocket = new WebSocket(gateway);

    websocket.onopen = () => {
        console.log("WebSocket Opened");
        getReadings();
    };

    websocket.onclose = () => {
        console.log("WebSocket Closed");
        setTimeout(initWebSocket, 2000);
    };

    websocket.onmessage = (event) => {
        try {
            const obj = JSON.parse(event.data);

            if (obj.Serial) {
                if (obj.Serial) appendToTerminal(`Serial : ${obj.Serial}`);
            }

        } catch (err) {
            console.warn("Received non-JSON data");
        }
    };

}

document.getElementById("sendBtn").addEventListener("click", () => {
    const ts = new Date().toLocaleTimeString();
    const msg = document.getElementById("messageInput").value.trim();
    if (msg) {
        fetchToserver(msg);
        appendToTerminal(`Sent : ${msg}`);
        document.getElementById("messageInput").value = "";
    }
});

document.getElementById("clearBtn").addEventListener("click", () => {
    terminal.innerHTML = "";
});

messageInput.addEventListener("keydown", (e) => {
    const ts = new Date().toLocaleTimeString();
    const msg = document.getElementById("messageInput").value.trim();
    if (e.key === 'Enter') {
        if (msg) {
            fetchToserver(msg);
            appendToTerminal(`Sent : ${msg}`);
            document.getElementById("messageInput").value = "";
        }
    }
});

function appendToTerminal(message) {
    const div = document.createElement("div");
    div.textContent = message;
    terminal.appendChild(div);
    terminal.scrollTop = terminal.scrollHeight;
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
