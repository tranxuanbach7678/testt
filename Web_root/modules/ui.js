// modules/ui.js
import { api } from "./api.js";
// Import cac ham tu module khac de xu ly viec load tab
import { loadApps, renderRecents } from "./tab-apps.js";
import { loadProcs } from "./tab-procs.js";
import { loadDevices } from "./tab-cam.js";

/**
 * @brief Hien thi mot dong log trong o "Nhat ky thao tac"
 * @param {string} msg Noi dung log
 * @param {boolean} success Thanh cong (true) hay That bai (false)
 */
export function logActionUI(msg, success) {
  const list = document.getElementById("actionLogList");
  if (list) {
    const i = document.createElement("div");
    i.className = "log-item " + (success ? "success" : "error");
    i.innerHTML = `<span class="log-time">[${new Date().toLocaleTimeString()}]</span> ${msg}`;
    list.insertBefore(i, list.firstChild); // Them vao dau danh sach
  }
  // Dong thoi gui log nay len server de server biet client da lam gi
  api("/api/clientlog", { msg: (success ? "[OK] " : "[FAIL] ") + msg });
}

/**
 * @brief An/Hien o "Nhat ky thao tac"
 */
export function toggleActionLog() {
  const l = document.getElementById("actionLogList");
  if (l) l.style.display = l.style.display === "none" ? "block" : "none";
}

/**
 * @brief Ham chinh de chuyen doi giua cac tab (Apps, Procs, Screen...)
 * @param {string} id ID cua tab can hien thi
 */
export function showTab(id) {
  // An tat ca cac tab-content
  document
    .querySelectorAll(".tab-content")
    .forEach((el) => el.classList.remove("active"));
  // Xoa class 'active' khoi tat ca cac nut tab
  document
    .querySelectorAll(".tab-btn")
    .forEach((el) => el.classList.remove("active"));

  // Hien thi tab duoc chon
  const btn = document.querySelector(`button[onclick="showTab('${id}')"]`);
  if (btn) btn.classList.add("active");

  const tabContent = document.getElementById("tab-" + id);
  if (tabContent) {
    tabContent.classList.add("active");
  }

  // Goi ham tai du lieu tuong ung voi tab
  if (id === "apps") {
    loadApps();
    renderRecents();
  }
  if (id === "procs") {
    loadProcs();
  }
  if (id === "cam") {
    loadDevices(); // Tai danh sach thiet bi khi mo tab
  }
}

/**
 * @brief Ham loc (tim kiem) noi dung trong bang
 * @param {string} tid ID cua bang (vi du: 'procTable')
 * @param {number} col Chi so cot can tim (khong dung trong ban nay)
 * @param {string} txt Tu khoa tim kiem
 */
export function filterTable(tid, col, txt) {
  document.querySelectorAll(`#${tid} tbody tr`).forEach(
    (tr) =>
      (tr.style.display = tr.innerText.toLowerCase().includes(txt.toLowerCase())
        ? ""
        : "none") // An/hien dong tuy theo ket qua tim kiem
  );
}
