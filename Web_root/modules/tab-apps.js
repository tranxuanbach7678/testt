// modules/tab-apps.js
import { api } from "./api.js";
import { logActionUI } from "./ui.js";

/**
 * @brief Tai danh sach cua so ung dung (app) tu server
 */
export async function loadApps() {
  let list = await api("/api/apps");
  if (Array.isArray(list)) {
    const tbody = document.querySelector("#appsTable tbody");
    if (!tbody) return;
    tbody.innerHTML = list
      .map((a) => {
        let name = a.path.split("\\").pop() || "Unknown"; // Lay ten file .exe
        let path = encodeURIComponent(a.path); // Ma hoa duong dan
        return `<tr><td><strong>${name}</strong><br><span class="app-title">${a.title}</span></td><td><button class="btn-danger" onclick="closeWin(${a.hwnd}, '${path}', '${name}')">Đóng</button></td></tr>`;
      })
      .join("");
  }
}

/**
 * @brief Gui lenh dong cua so bang HWND
 */
export async function closeWin(h, path, name) {
  if (confirm("Đóng cửa sổ này?")) {
    path = decodeURIComponent(path);
    if (path && path !== "Unknown" && path.length > 3) addRecent(path, name); // Them vao "gan day"
    let res = await api("/api/app/close", { hwnd: h });
    logActionUI(`Đóng: ${name}`, res.ok);
    loadApps(); // Tai lai danh sach
  }
}

/**
 * @brief Gui lenh mo mot ung dung/process
 */
export async function startCmd(inpId, statId, cmdOverride = null) {
  let val = cmdOverride || document.getElementById(inpId).value.trim();
  if (!val) return;

  const statusEl = document.getElementById(statId);
  if (statusEl) statusEl.textContent = "⏳ ...";

  let res = await api("/api/start", { name: val });
  logActionUI(`Mở: ${val.split("\\").pop()}`, res.ok);

  if (statusEl) statusEl.textContent = res.ok ? "✅ OK" : "❌ Lỗi";

  setTimeout(() => {
    // Tu dong tai lai danh sach app sau 1.5s
    if (
      res.ok &&
      document.getElementById("tab-apps").classList.contains("active")
    )
      loadApps();
  }, 1500);
}

// --- TIEN ICH "MO GAN DAY" ---
export function addRecent(path, name) {
  let r = JSON.parse(sessionStorage.getItem("recents") || "[]");
  r = r.filter((x) => x.path !== path); // Xoa cai cu (neu co)
  r.unshift({ path, name }); // Them vao dau
  if (r.length > 8) r.pop(); // Gioi han 8 muc
  sessionStorage.setItem("recents", JSON.stringify(r));
  renderRecents();
}

export function renderRecents() {
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
            "\\\\" // Escape dau backslash cho JS
          )}')">🔄 ${i.name}</span>`
      )
      .join("") || "<i>Chưa có</i>";
}
