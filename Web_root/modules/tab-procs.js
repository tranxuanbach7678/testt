// modules/tab-procs.js
import { api } from "./api.js";
import { logActionUI } from "./ui.js";

/**
 * @brief Tai danh sach tien trinh (process) tu server
 */
export async function loadProcs() {
  let list = await api("/api/processes");
  if (Array.isArray(list)) {
    list.sort((a, b) => a.exe.localeCompare(b.exe)); // Sap xep theo ten
    const tbody = document.querySelector("#procTable tbody");
    if (!tbody) return;
    tbody.innerHTML = list
      .map(
        (p) =>
          `<tr><td>${p.pid}</td><td><strong>${p.exe}</strong></td><td><button class="btn-danger" onclick="kill(${p.pid})">Kill</button></td></tr>`
      )
      .join("");
  }
}

/**
 * @brief Gui lenh "giet" process bang PID
 */
export async function kill(pid) {
  if (confirm("Kill PID " + pid + "?")) {
    let res = await api("/api/kill", { pid });
    logActionUI(`Kill PID ${pid}`, res.ok);
    loadProcs(); // Tai lai danh sach
  }
}
