let deviceUrl = "http://172.16.3.201";
let lastSeen = null;
let pollTimer = null;
let connectionVersion = 0;
let requestVersion = 0;

function updateDispenserInfo(online) {
    document.getElementById("status-dot").classList.toggle("offline", !online);
    document.getElementById("connection-status").textContent = online ? "Online" : "Offline";
    document.getElementById("last-sync").textContent = lastSeen
        ? `Last response ${Math.floor((Date.now() - lastSeen) / 60000)} min ago`
        : "Never connected";
}

function setConnectionStatus(online, requestId) {
    if (requestId !== requestVersion) return;
    if (online) lastSeen = Date.now();
    updateDispenserInfo(online);
}

async function requestDevice(path, options = {}) {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 3000);

    try {
        const response = await fetch(`${deviceUrl}${path}`, {
            ...options,
            cache: "no-store",
            signal: controller.signal
        });
        const result = await response.json();
        if (!response.ok) throw new Error(result.error || `HTTP ${response.status}`);
        return result;
    } finally {
        clearTimeout(timeout);
    }
}

async function checkDispenser() {
    const connectionId = connectionVersion;
    const requestId = ++requestVersion;

    try {
        const status = await requestDevice("/status");
        if (status.online !== true) throw new Error("Device reported offline");
        setConnectionStatus(true, requestId);
    } catch (error) {
        setConnectionStatus(false, requestId);
        console.error(error);
    } finally {
        if (connectionId === connectionVersion) {
            pollTimer = setTimeout(checkDispenser, 5000);
        }
    }
}

document.getElementById("toggleLED").addEventListener("click", async () => {
    const requestId = ++requestVersion;

    try {
        const result = await requestDevice("/command", {
            method: "POST",
            headers: {"Content-Type": "application/json"},
            body: JSON.stringify({command: "LED", action: "TOGGLE"})
        });
        setConnectionStatus(true, requestId);
        console.log(result);
    } catch (error) {
        setConnectionStatus(false, requestId);
        console.error(error);
    }
});

document.getElementById("rotateMotor").addEventListener("click", async () => {
    const requestId = ++requestVersion;

    try {
        const result = await requestDevice("/command", {
            method: "POST",
            headers: {"Content-Type": "application/json"},
            body: JSON.stringify({command: "MOTOR", action: "ROTATE"})
        });
        setConnectionStatus(true, requestId);
        console.log(result);
    } catch (error) {
        setConnectionStatus(false, requestId);
        console.error(error);
    }
});

document.getElementById("device-form").addEventListener("submit", event => {
    event.preventDefault();
    const ip = document.getElementById("device-ip").value.trim();
    if (!ip) return;

    connectionVersion++;
    requestVersion++;
    clearTimeout(pollTimer);
    deviceUrl = `http://${ip.replace(/^https?:\/\//, "").replace(/\/$/, "")}`;
    lastSeen = null;
    updateDispenserInfo(false);
    checkDispenser();
});

checkDispenser();