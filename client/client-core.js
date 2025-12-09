/**
 * CLIENT CORE (STANDALONE VERSION - FIXED LAYOUT)
 * Tách biệt logic hiển thị Stream và Screenshot
 */

// === 1. STORE & CONSTANTS ===
let DEVICE_ID = localStorage.getItem("rc_device_id");
if (!DEVICE_ID) {
  DEVICE_ID = "CL_" + Math.random().toString(16).substring(2, 6);
  localStorage.setItem("rc_device_id", DEVICE_ID);
}

const store = {
  db: null,
  DEVICE_ID: DEVICE_ID,
  clientIP: null,
  socketReady: false,
  socket: null,
  isScreenStreamOn: false,
  isCamStreamOn: false,
  autoShotInt: null,
  keylogInt: null,
  isSavingScreenshot: false,
};

const EventBus = new EventTarget();

// === 2. UI MODULE ===
function toggleTheme() {
  document.body.classList.toggle("dark-mode");
  localStorage.setItem(
    "theme",
    document.body.classList.contains("dark-mode") ? "dark" : "light"
  );
}
function initTheme() {
  if (localStorage.getItem("theme") === "dark")
    document.body.classList.add("dark-mode");
}
initTheme();

function moveSlider(targetButton) {
  const slider = document.getElementById("tab-slider");
  if (!slider || !targetButton) return;
  slider.style.left = `${targetButton.offsetLeft}px`;
  slider.style.width = `${targetButton.offsetWidth}px`;
}
function handleTabHover(targetButton) {
  moveSlider(targetButton);
}
function handleTabLeave() {
  const activeButton = document.querySelector(".tab-btn.active");
  if (activeButton) moveSlider(activeButton);
}

function logActionUI(msg, success) {
  const list = document.getElementById("actionLogList");
  if (list) {
    const i = document.createElement("div");
    i.className = "log-item " + (success ? "success" : "error");
    i.innerHTML = `<span class="log-time">[${new Date().toLocaleTimeString()}]</span> ${msg}`;
    list.insertBefore(i, list.firstChild);
  }
}
function toggleActionLog() {
  document.getElementById("actionLogList").classList.toggle("minimized");
}

function showTab(id) {
  // Logic dừng stream khi chuyển tab để tiết kiệm băng thông
  if (store.isScreenStreamOn || store.isCamStreamOn) {
    logActionUI("Chuyển tab -> Dừng tất cả Stream.", true);
    if (store.isScreenStreamOn) toggleScreenStream(null);
    if (store.isCamStreamOn) toggleCamStream(null);
    sendCommand("STOP_STREAM");
  }

  document
    .querySelectorAll(".tab-content")
    .forEach((el) => el.classList.remove("active"));
  document
    .querySelectorAll(".tab-btn")
    .forEach((el) => el.classList.remove("active"));

  const btn = document.querySelector(`button[onclick="showTab('${id}')"]`);
  if (btn) {
    btn.classList.add("active");
    moveSlider(btn);
  }
  const tabContent = document.getElementById("tab-" + id);
  if (tabContent) tabContent.classList.add("active");

  if (store.socketReady) {
    if (id === "apps") {
      loadApps();
      renderRecents();
    }
    if (id === "procs") loadProcs();
    if (id === "keylog") loadKeylog();
  }
}

function filterTable(tid, col, txt) {
  document
    .querySelectorAll(`#${tid} tbody tr`)
    .forEach(
      (tr) =>
        (tr.style.display = tr.innerText
          .toLowerCase()
          .includes(txt.toLowerCase())
          ? ""
          : "none")
    );
}

// === 3. SOCKET MODULE ===
const responseHandlers = {};
function onCommand(command, handler) {
  responseHandlers[command] = handler;
}

function sendCommand(command, payload = null) {
  if (store.socket && store.socket.readyState === WebSocket.OPEN) {
    store.socket.send(JSON.stringify({ command: command, payload: payload }));
  } else {
    console.error(`Loi: WebSocket chua san sang (dinh goi lenh: ${command})!`);
    EventBus.dispatchEvent(
      new CustomEvent("socket:error", { detail: "WebSocket not ready" })
    );
  }
}

// === 4. FEATURE MODULES ===

// --- Apps Tab ---
function handleAppsData(list) {
  const tbody = document.querySelector("#appsTable tbody");
  if (!tbody || !Array.isArray(list)) return;
  tbody.innerHTML = list
    .map((a) => {
      let name = a.path.split("\\").pop() || "Unknown";
      let path = encodeURIComponent(a.path);
      return `<tr><td><strong>${name}</strong><br><span class="app-title">${a.title}</span></td>
      <td><button class="btn-danger" onclick="closeWin('${a.hwnd}', '${path}', '${name}')">Đóng</button></td></tr>`;
    })
    .join("");
}
function loadApps() {
  sendCommand("GET_APPS");
}
function closeWin(h, path, name) {
  if (confirm("Đóng cửa sổ này?")) {
    if (path && path !== "Unknown" && path.length > 3) addRecent(path, name);
    sendCommand("CLOSE_HWND", h);
    logActionUI(`Đóng: ${name}`, true);
    setTimeout(loadApps, 1000);
  }
}
function startCmd(inpId, statId, cmdOverride = null) {
  let val = cmdOverride || document.getElementById(inpId).value.trim();
  if (!val) return;
  const statusEl = document.getElementById(statId);
  if (statusEl) statusEl.textContent = "⏳ ...";
  sendCommand("START_CMD", val);
  let name = val.split("\\").pop();
  addRecent(val, name);
  logActionUI(`Mở: ${name}`, true);
  if (statusEl) statusEl.textContent = "✅ Đã gửi lệnh";
  setTimeout(() => {
    if (document.getElementById("tab-apps").classList.contains("active"))
      loadApps();
  }, 2000);
}
function addRecent(path, name) {
  let r = JSON.parse(sessionStorage.getItem("recents") || "[]");
  r = r.filter((x) => x.path !== path);
  r.unshift({ path, name });
  if (r.length > 8) r.pop();
  sessionStorage.setItem("recents", JSON.stringify(r));
  renderRecents();
}
function renderRecents() {
  const listEl = document.getElementById("recentListTags");
  if (!listEl) return;
  listEl.innerHTML =
    JSON.parse(sessionStorage.getItem("recents") || "[]")
      .map(
        (i) =>
          `<span class="tag" title="${
            i.path
          }" onclick="startCmd(null,'statusApp','${i.path.replace(
            /\\/g,
            "\\\\"
          )}')">🔄 ${i.name}</span>`
      )
      .join("") || "<i>Chưa có</i>";
}

// --- Procs Tab ---
function handleProcsData(list) {
  const tbody = document.querySelector("#procTable tbody");
  if (!tbody || !Array.isArray(list)) return;
  list.sort((a, b) => a.exe.localeCompare(b.exe));
  tbody.innerHTML = list
    .map(
      (p) =>
        `<tr><td>${p.pid}</td><td><strong>${p.exe}</strong></td><td><button class="btn-danger" onclick="kill(${p.pid})">Kill</button></td></tr>`
    )
    .join("");
}
function loadProcs() {
  sendCommand("GET_PROCS");
}
function kill(pid) {
  if (confirm("Kill PID " + pid + "?")) {
    sendCommand("KILL_PID", pid);
    logActionUI(`Kill PID ${pid}`, true);
    setTimeout(loadProcs, 500);
  }
}

// --- Keylog Tab ---
function handleKeylogData(payload) {
  const chk = document.getElementById("chkKeylog");
  if (chk) chk.checked = payload.enabled;
  if (payload.log) {
    let area = document.getElementById("logArea");
    area.value += payload.log;
    area.scrollTop = area.scrollHeight;
    sessionStorage.setItem("keylogs", area.value);
  }
}
function loadKeylog() {
  sendCommand("GET_KEYLOG");
}
function toggleKeylog(cb) {
  sendCommand("KEYLOG_SET", cb.checked);
  logActionUI(`Keylog: ${cb.checked ? "BẬT" : "TẮT"}`, true);
  if (cb.checked) {
    if (!store.keylogInt)
      store.keylogInt = setInterval(() => sendCommand("GET_KEYLOG"), 200);
  } else {
    if (store.keylogInt) {
      clearInterval(store.keylogInt);
      store.keylogInt = null;
    }
  }
}
function clearLogs() {
  if (confirm("Xóa log?")) {
    document.getElementById("logArea").value = "";
    sessionStorage.removeItem("keylogs");
    logActionUI("Đã xóa log phím", true);
  }
}

// --- Screen Tab (FIXED: SEPARATE LOGIC) ---
function handleScreenshotData(payload) {
  const imgData = "data:image/jpeg;base64," + payload;

  // 1. Cập nhật ảnh vào khung Chụp ảnh (#screenImg)
  const imgEl = document.getElementById("screenImg");
  if (imgEl) imgEl.src = imgData;

  // 2. Logic Lưu ảnh
  if (store.isSavingScreenshot && store.db) {
    fetch(imgData)
      .then((res) => res.blob())
      .then((blob) => {
        store.db
          .transaction(["images"], "readwrite")
          .objectStore("images")
          .add({ blob, date: new Date() });
        logActionUI("Đã chụp & lưu", true);
        loadGallery();
      });
    store.isSavingScreenshot = false;
  }
}

function updateScreen(save = false) {
  store.isSavingScreenshot = save;
  sendCommand("GET_SCREENSHOT");
}

function toggleAutoShot(cb) {
  if (cb.checked) {
    store.isSavingScreenshot = false;
    updateScreen(false);
    store.autoShotInt = setInterval(() => updateScreen(false), 2000);
  } else clearInterval(store.autoShotInt);
}

function loadGallery() {
  if (!store.db) return;
  let h = "";
  store.db
    .transaction(["images"], "readonly")
    .objectStore("images")
    .openCursor(null, "prev").onsuccess = (e) => {
    let c = e.target.result;
    if (c) {
      h += `<div class="gallery-item" onclick="window.open('${URL.createObjectURL(
        c.value.blob
      )}')"><img src="${URL.createObjectURL(
        c.value.blob
      )}" title="${c.value.date.toLocaleString()}"></div>`;
      c.continue();
    } else
      document.getElementById("gallery").innerHTML =
        h || "<small>Trống</small>";
  };
}

function clearGallery() {
  if (confirm("Xóa hết ảnh?") && store.db) {
    store.db
      .transaction(["images"], "readwrite")
      .objectStore("images")
      .clear().onsuccess = () => {
      loadGallery();
      logActionUI("Đã xóa thư viện ảnh", true);
    };
  }
}

function toggleScreenStream(btn) {
  // Chỉ tác động vào khung Stream (#screenStreamView)
  const streamView = document.getElementById("screenStreamView");
  const streamStatus = document.getElementById("screenStreamStatus");

  if (btn === null) {
    // Tắt Stream
    store.isScreenStreamOn = false;
    // Reset khung stream về trạng thái rỗng
    streamView.removeAttribute("src");
    streamView.src = "";
    streamView.alt = "Stream đã tắt.";

    // Reset nút bấm
    const b = document.getElementById("btnToggleScreenStream");
    if (b) {
      b.textContent = "▶️ Bật Stream Màn Hình";
      b.classList.remove("btn-danger");
      b.classList.add("btn-primary");
    }
    if (streamStatus) streamStatus.textContent = "";
    return;
  }

  store.isScreenStreamOn = !store.isScreenStreamOn;

  if (store.isScreenStreamOn) {
    // Nếu bật Stream Màn hình, tắt Stream Camera (nếu đang chạy)
    if (store.isCamStreamOn) toggleCamStream(null);

    streamView.alt = "Đang tải luồng...";
    btn.textContent = "⏹️ Tắt Stream Màn Hình";
    btn.classList.add("btn-danger");
    btn.classList.remove("btn-primary");
    if (streamStatus) streamStatus.textContent = "⏳ Đang kết nối...";

    logActionUI("Bật livestream màn hình", true);
    sendCommand("START_STREAM_SCREEN");
  } else {
    // Tắt thủ công
    toggleScreenStream(null);
    sendCommand("STOP_STREAM");
    logActionUI("Tắt livestream màn hình", true);
  }
}

// --- Cam Tab ---
let camRecorder = null,
  camChunks = [],
  camInterval = null,
  isCamRec = null,
  camRecTimeout = null;

function toggleRecMode() {
  const mode = document.querySelector('input[name="recMode"]:checked').value;
  document.getElementById("timerInputRow").style.display =
    mode === "timer" ? "flex" : "none";
}

function handleDevicesData(data) {
  const camSelect = document.getElementById("camName");
  if (data.status === "refresh_pending" || data.status === "refresh_busy") {
    if (camSelect && camSelect.options.length === 0)
      camSelect.innerHTML = "<option>⏳ Đang quét...</option>";
    setTimeout(() => sendCommand("GET_DEVICES"), 2000);
    return;
  }
  camSelect.innerHTML = "";
  if (data.video && data.video.length > 0) {
    data.video.forEach((cam) => {
      const opt = document.createElement("option");
      opt.value = cam;
      opt.textContent = cam;
      if (cam.toLowerCase().includes("usb")) opt.selected = true;
      camSelect.appendChild(opt);
    });
  } else
    camSelect.innerHTML = "<option value=''>Không tìm thấy camera</option>";
  if (data.status === "not_ready") loadDevices(true);
}
function loadDevices(force = false) {
  force ? sendCommand("REFRESH_DEVICES") : sendCommand("GET_DEVICES");
}

function recordVideo() {
  const btnVid = document.getElementById("btnVid");
  const btnStream = document.getElementById("btnToggleCamStream");
  const imgView = document.getElementById("camStreamView");
  const canvas = document.getElementById("camRecorderCanvas");
  const stat = document.getElementById("vidStatus");

  if (!store.isCamStreamOn || !imgView.src)
    return alert("Vui lòng BẬT STREAM trước khi quay!");
  if (isCamRec) {
    stopCamRecording();
    return;
  }

  try {
    canvas.width = 640;
    canvas.height = 480;
    const ctx = canvas.getContext("2d");
    const stream = canvas.captureStream(25);
    try {
      camRecorder = new MediaRecorder(stream, {
        mimeType: "video/webm;codecs=vp8",
      });
    } catch (e) {
      camRecorder = new MediaRecorder(stream);
    }
    camChunks = [];
    camRecorder.ondataavailable = (e) => {
      if (e.data.size > 0) camChunks.push(e.data);
    };
    camRecorder.onstop = () => {
      const blob = new Blob(camChunks, { type: "video/webm" });
      if (store.db) {
        store.db
          .transaction(["videos"], "readwrite")
          .objectStore("videos")
          .add({ blob, date: new Date() });
        loadVidGallery();
        logActionUI("Đã lưu video mới vào thư viện.", true);
      }
    };
    camInterval = setInterval(() => {
      if (imgView.complete && imgView.naturalHeight !== 0)
        ctx.drawImage(imgView, 0, 0, canvas.width, canvas.height);
    }, 40);
    camRecorder.start();
    isCamRec = true;
    if (btnStream) btnStream.disabled = true;

    btnVid.textContent = "⏹️ DỪNG QUAY NGAY";
    btnVid.classList.add("btn-danger");
    btnVid.classList.remove("btn-primary");
    const mode = document.querySelector('input[name="recMode"]:checked').value;
    if (mode === "timer") {
      const sec = parseInt(document.getElementById("vidDur").value) || 10;
      stat.innerText = `⏳ Đang quay ${sec} giây...`;
      camRecTimeout = setTimeout(stopCamRecording, sec * 1000);
    } else stat.innerText = "🔴 Đang quay thủ công...";
  } catch (e) {
    alert("Lỗi: " + e.message);
    isCamRec = false;
    if (btnStream) btnStream.disabled = false;
  }
}
function stopCamRecording() {
  if (camRecorder && camRecorder.state !== "inactive") camRecorder.stop();
  if (camInterval) clearInterval(camInterval);
  if (camRecTimeout) clearTimeout(camRecTimeout);
  isCamRec = false;
  camRecTimeout = null;
  const btnStream = document.getElementById("btnToggleCamStream");
  if (btnStream) btnStream.disabled = false;
  const btnVid = document.getElementById("btnVid");
  btnVid.textContent = "🔴 BẮT ĐẦU QUAY";
  btnVid.classList.remove("btn-danger");
  btnVid.classList.add("btn-primary");
  document.getElementById("vidStatus").innerText = "✅ Đã lưu vào thư viện.";
}
function loadVidGallery() {
  if (!store.db) return;
  let h = "";
  store.db
    .transaction(["videos"], "readonly")
    .objectStore("videos")
    .openCursor(null, "prev").onsuccess = (e) => {
    let c = e.target.result;
    if (c) {
      let u = URL.createObjectURL(c.value.blob);
      h += `<div class="gallery-item video-item"><video src="${u}" controls style="width:100%;height:80px"></video></div>`;
      c.continue();
    } else
      document.getElementById("vidGallery").innerHTML =
        h || "<small>Trống</small>";
  };
}
function clearVideos() {
  if (confirm("Xóa hết video?") && store.db) {
    store.db
      .transaction(["videos"], "readwrite")
      .objectStore("videos")
      .clear().onsuccess = () => {
      loadVidGallery();
      logActionUI("Đã xóa thư viện video", true);
    };
  }
}
function toggleCamStream(btn) {
  const streamView = document.getElementById("camStreamView");
  const streamStatus = document.getElementById("camStreamStatus");
  if (btn === null) {
    store.isCamStreamOn = false;
    streamView.removeAttribute("src");
    streamView.src = "";
    streamView.style.display = "none";
    const b = document.getElementById("btnToggleCamStream");
    if (b) {
      b.textContent = "▶️ Bật Stream";
      b.classList.remove("btn-danger");
      b.classList.add("btn-primary");
      b.disabled = false;
    }
    streamStatus.textContent = "";
    if (isCamRec) stopCamRecording();
    return;
  }
  store.isCamStreamOn = !store.isCamStreamOn;
  if (store.isCamStreamOn) {
    if (store.isScreenStreamOn) toggleScreenStream(null);
    const camName = document.getElementById("camName").value;
    if (!camName) {
      alert("Chưa chọn Camera");
      store.isCamStreamOn = false;
      return;
    }
    streamView.src = "";
    streamView.style.display = "block";
    btn.textContent = "⏹️ Tắt Stream";
    btn.classList.add("btn-danger");
    btn.classList.remove("btn-primary");
    streamStatus.textContent = "⏳ Đang kết nối...";
    sendCommand("START_STREAM_CAM", { cam: camName, audio: "" });
  } else {
    toggleCamStream(null);
    sendCommand("STOP_STREAM");
  }
}

// --- Sys Tab ---
function sendPower(act) {
  if (confirm("CHẮC CHẮN " + act.toUpperCase() + " MÁY TÍNH?")) {
    sendCommand("POWER_CMD", act);
    logActionUI("Lệnh nguồn: " + act, true);
  }
}

// === 5. STARTUP LOGIC ===

// Event Registry
onCommand("GET_APPS", handleAppsData);
onCommand("GET_PROCS", handleProcsData);
onCommand("GET_DEVICES", handleDevicesData);
onCommand("GET_KEYLOG", handleKeylogData);
onCommand("GET_SCREENSHOT", handleScreenshotData);

// Auth Screen Logic
function showAuthScreen(emoji, message, color) {
  document.body.innerHTML = `
        <div style="padding: 40px; text-align: center; font-size: 1.2em; white-space: pre-wrap; color: ${color}; background: #222; height: 100vh; display: flex; flex-direction: column; align-items: center; justify-content: center; margin: 0; line-height: 1.6;">
            <div style="font-size: 3em; margin-bottom: 20px;">${emoji}</div>
            <pre style="font-family: inherit; margin: 0;">${message}</pre>
            <div style="font-size: 0.8em; color: #888; margin-top: 20px; font-family: monospace;">Device ID: ${
              store.DEVICE_ID || "N/A"
            }</div>
        </div>`;
}

// Start Connection Logic
function startConnection() {
  let ip = document.getElementById("ipInput").value.trim();
  if (!ip) return alert("Vui lòng nhập IP!");
  ip = ip.replace(/^(ws|http)s?:\/\//, "");
  if (ip.endsWith("/")) ip = ip.slice(0, -1);
  if (!ip.includes(":")) ip += ":8080";
  store.clientIP = ip;
  addToHistory(ip);

  document.getElementById("client-info").innerText = "CONNECTING...";

  // Init WebSocket with DeviceID
  store.socket = new WebSocket(`ws://${ip}?id=${store.DEVICE_ID}`);

  store.socket.onopen = () =>
    console.log("WebSocket connected waiting auth...");

  store.socket.onmessage = (event) => {
    // Binary Stream (Hình ảnh)
    if (event.data instanceof Blob || event.data instanceof ArrayBuffer) {
      const url = URL.createObjectURL(
        new Blob([event.data], { type: "image/jpeg" })
      );

      // Kiểm tra xem đang bật stream nào thì cập nhật khung hình đó
      const screenView = document.getElementById("screenStreamView");
      const camView = document.getElementById("camStreamView");

      if (store.isScreenStreamOn && screenView) {
        screenView.src = url;
        screenView.onload = () => URL.revokeObjectURL(url);
      } else if (store.isCamStreamOn && camView) {
        camView.src = url;
        camView.onload = () => URL.revokeObjectURL(url);
      }
      return;
    }
    // JSON Messages
    try {
      const msg = JSON.parse(event.data);
      if (msg.type === "auth")
        EventBus.dispatchEvent(new CustomEvent("socket:auth", { detail: msg }));
      else if (msg.type === "stream_start") {
        if (store.isScreenStreamOn)
          document.getElementById("screenStreamStatus").textContent =
            "✅ Đã kết nối luồng.";
        if (store.isCamStreamOn)
          document.getElementById("camStreamStatus").textContent =
            "✅ Đã kết nối luồng.";
      } else if (msg.type === "stream_stop") {
        if (store.isScreenStreamOn) toggleScreenStream(null);
        if (store.isCamStreamOn) toggleCamStream(null);
      } else if (msg.type === "error")
        logActionUI(`Lỗi Gateway: ${msg.payload}`, false);
      else if (msg.type === "json") {
        if (responseHandlers[msg.command])
          responseHandlers[msg.command](msg.payload);
      }
    } catch (e) {
      console.error(e);
    }
  };

  store.socket.onclose = (e) => {
    logActionUI("Mất kết nối: " + e.reason, false);
    store.socketReady = false;
    alert("Mất kết nối với Server!");
    location.reload();
  };

  store.socket.onerror = () => logActionUI("Lỗi WebSocket connection", false);
}

// Listen for Auth Events
EventBus.addEventListener("socket:auth", (event) => {
  const { status, message, clientInfo } = event.detail;
  if (status === "approved") {
    // Hide Login, Show Main App
    document.getElementById("login-screen").style.display = "none";
    document.getElementById("main-app").style.display = "block";
    logActionUI("Đã kết nối và xác thực!", true);
    store.socketReady = true;
    showTab("apps");
    document.getElementById(
      "client-info"
    ).textContent = `ID: ${store.DEVICE_ID}`;

    // Init DB after auth
    const req = indexedDB.open("RemoteDB_V2", 2);
    req.onupgradeneeded = (e) => {
      let db = e.target.result;
      if (!db.objectStoreNames.contains("images"))
        db.createObjectStore("images", { keyPath: "id", autoIncrement: true });
      if (!db.objectStoreNames.contains("videos"))
        db.createObjectStore("videos", { keyPath: "id", autoIncrement: true });
    };
    req.onsuccess = (e) => {
      store.db = e.target.result;
      loadGallery();
      loadVidGallery();
    };
  } else if (status === "pending") {
    store.socketReady = false;
    showAuthScreen("⌛", message, "#ffcc80");
  } else if (status === "rejected") {
    store.socketReady = false;
    showAuthScreen("⛔", message, "#ef9a9a");
  }
});

// History Helpers
function loadHistory() {
  const h = JSON.parse(localStorage.getItem("remote_ip_history") || "[]");
  document.getElementById("historyItems").innerHTML =
    h
      .map(
        (ip) =>
          `<div class="history-item" onclick="document.getElementById('ipInput').value='${ip}'"><span>${ip}</span> <span style="color:var(--danger)" onclick="event.stopPropagation();delHistory('${ip}')">×</span></div>`
      )
      .join("") || "<small>Trống</small>";
  if (h.length && !document.getElementById("ipInput").value)
    document.getElementById("ipInput").value = h[0];
}
function addToHistory(ip) {
  let h = JSON.parse(localStorage.getItem("remote_ip_history") || "[]");
  h = h.filter((x) => x !== ip);
  h.unshift(ip);
  if (h.length > 5) h.pop();
  localStorage.setItem("remote_ip_history", JSON.stringify(h));
}
window.delHistory = function (ip) {
  let h = JSON.parse(localStorage.getItem("remote_ip_history") || "[]");
  localStorage.setItem(
    "remote_ip_history",
    JSON.stringify(h.filter((x) => x !== ip))
  );
  loadHistory();
};

// === 6. INITIALIZATION ===
if (document.getElementById("lblDeviceId"))
  document.getElementById("lblDeviceId").textContent = store.DEVICE_ID;
const logArea = document.getElementById("logArea");
if (logArea) logArea.value = sessionStorage.getItem("keylogs") || "";
loadHistory();

document.getElementById("ipInput").addEventListener("keypress", (e) => {
  if (e.key === "Enter") startConnection();
});

// Expose global functions for HTML onclick
window.toggleTheme = toggleTheme;
window.toggleActionLog = toggleActionLog;
window.showTab = showTab;
window.handleTabHover = handleTabHover;
window.handleTabLeave = handleTabLeave;
window.filterTable = filterTable;
window.startCmd = startCmd;
window.loadApps = loadApps;
window.closeWin = closeWin;
window.startConnection = startConnection;
window.loadProcs = loadProcs;
window.kill = kill;
window.updateScreen = updateScreen;
window.toggleAutoShot = toggleAutoShot;
window.clearGallery = clearGallery;
window.toggleScreenStream = toggleScreenStream;
window.toggleKeylog = toggleKeylog;
window.clearLogs = clearLogs;
window.toggleRecMode = toggleRecMode;
window.loadDevices = loadDevices;
window.recordVideo = recordVideo;
window.clearVideos = clearVideos;
window.toggleCamStream = toggleCamStream;
window.sendPower = sendPower;
