const fields = {
  statusBadge: document.querySelector("#statusBadge"),
  reactorOnline: document.querySelector("#reactorOnline"),
  rodPosition: document.querySelector("#rodPosition"),
  alarmAck: document.querySelector("#alarmAck"),
  lightRaw: document.querySelector("#lightRaw"),
  coreTemp: document.querySelector("#coreTemp"),
  roomTemp: document.querySelector("#roomTemp"),
  humidity: document.querySelector("#humidity"),
  wifi: document.querySelector("#wifi"),
  mqtt: document.querySelector("#mqtt"),
  offlineQueue: document.querySelector("#offlineQueue"),
  heap: document.querySelector("#heap"),
  rodInput: document.querySelector("#rodInput"),
};

function valueOrDash(value) {
  return value === null || value === undefined ? "—" : value;
}

function setBool(element, value) {
  element.textContent = value ? "OK" : "Non";
}

async function refreshState() {
  try {
    const response = await fetch("/api/state", { cache: "no-store" });
    const payload = await response.json();
    const data = payload.data || {};
    const status = data.status || "UNKNOWN";

    fields.statusBadge.textContent = status;
    fields.statusBadge.className = `badge ${status.toLowerCase()}`;
    setBool(fields.reactorOnline, data.online);
    fields.rodPosition.textContent = valueOrDash(data.rodPositionPercent);
    fields.alarmAck.textContent = data.alarmAcknowledged ? "Oui" : "Non";
    fields.lightRaw.textContent = valueOrDash(data.lightRaw);
    fields.coreTemp.textContent = valueOrDash(data.coreRadiationC);
    fields.roomTemp.textContent = valueOrDash(data.temp);
    fields.humidity.textContent = valueOrDash(data.humidity);
    setBool(fields.wifi, data.wifiConnected);
    setBool(fields.mqtt, data.mqttConnected);
    fields.offlineQueue.textContent = valueOrDash(data.offlineQueueSize);
    fields.heap.textContent = valueOrDash(data.freeHeap);
    fields.rodInput.value = data.rodPositionPercent || 0;
  } catch (error) {
    fields.statusBadge.textContent = "LOCAL API DOWN";
    fields.statusBadge.className = "badge critical";
  }
}

async function sendCommand(command, value) {
  const payload = value === undefined ? { command } : { command, value };
  await fetch("/api/command", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  await refreshState();
}

document.querySelectorAll("button[data-command]").forEach((button) => {
  button.addEventListener("click", () => sendCommand(button.dataset.command));
});

document.querySelector("#sendRod").addEventListener("click", () => {
  sendCommand("SET_ROD", Number(fields.rodInput.value));
});

document.querySelector("#mqttForm").addEventListener("submit", async (event) => {
  event.preventDefault();
  const form = new FormData(event.currentTarget);
  await fetch("/api/config/mqtt", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      host: form.get("host"),
      port: Number(form.get("port")),
      username: form.get("username"),
      password: form.get("password"),
      baseTopic: form.get("baseTopic"),
    }),
  });
});

refreshState();
setInterval(refreshState, 1000);
