// modules/ui.js
import { api } from "./api.js";
// Import cac ham tu module khac de xu ly viec load tab
import { loadApps, renderRecents } from "./tab-apps.js";
import { loadProcs } from "./tab-procs.js";
import { loadDevices } from "./tab-cam.js";

// --- MOI: Ham di chuyen thanh slider ---
/**
 * @brief Di chuyen thanh slider den vi tri cua mot nut tab
 * @param {HTMLElement} targetButton Nut tab (dang active hoac dang duoc hover)
 */
function moveSlider(targetButton) {
  const slider = document.getElementById("tab-slider");
  if (!slider || !targetButton) return;

  // Lay vi tri va kich thuoc cua nut
  const left = targetButton.offsetLeft; // Vi tri 'left' so voi cha (.tabs)
  const width = targetButton.offsetWidth; // Chieu rong cua nut

  // Dat vi tri va chieu rong cho slider
  slider.style.left = `${left}px`;
  slider.style.width = `${width}px`;
}

// --- MOI: Xu ly khi rê chuột VÀO mot tab ---
export function handleTabHover(targetButton) {
  moveSlider(targetButton);
}

// --- MOI: Xu ly khi rê chuột RA KHOI vung tab ---
export function handleTabLeave() {
  // Tim nut dang 'active'
  const activeButton = document.querySelector(".tab-btn.active");
  if (activeButton) {
    moveSlider(activeButton); // Tra slider ve vi tri cua nut active
  }
}

/**
 * @brief Hien thi mot dong log trong o "Nhat ky thao tac"
 * @param {string} msg Noi dung log
 * @param {boolean} success Thanh cong (true) hay That bai (false)
 */
export function logActionUI(msg, success) {
  // ... (code cu, khong doi) ...
  const list = document.getElementById("actionLogList");
  if (list) {
    const i = document.createElement("div");
    i.className = "log-item " + (success ? "success" : "error");
    i.innerHTML = `<span class="log-time">[${new Date().toLocaleTimeString()}]</span> ${msg}`;
    list.insertBefore(i, list.firstChild);
  }
  api("/api/clientlog", { msg: msg, success: success });
}

/**
 * @brief An/Hien o "Nhat ky thao tac" voi hieu ung dong mo
 */
export function toggleActionLog() {
  // ... (code cu, khong doi) ...
  const l = document.getElementById("actionLogList");
  if (l) {
    l.classList.toggle("minimized");
  }
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
  if (btn) {
    btn.classList.add("active");
    // --- MOI: Di chuyen thanh slider den nut vua click ---
    moveSlider(btn);
  }

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
    loadDevices();
  }
}

/**
 * @brief Ham loc (tim kiem) noi dung trong bang
 * @param {string} tid ID cua bang (vi du: 'procTable')
 * @param {number} col Chi so cot can tim (khong dung trong ban nay)
 * @param {string} txt Tu khoa tim kiem
 */
export function filterTable(tid, col, txt) {
  // ... (code cu, khong doi) ...
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
