// modules/tab-screen.js
import { store } from "./store.js";
import { api } from "./api.js";
import { logActionUI } from "./ui.js";

/**
 * @brief Tai anh chup man hinh tu server
 * @param {boolean} save Neu true, luu vao IndexedDB
 */
export async function updateScreen(save = false) {
  // Neu 'save' la true, goi /screenshot. Neu la auto, goi /screenshot?auto=1
  let blob = await api(
    save ? "/screenshot?t=" + Date.now() : "/screenshot?auto=1&t=" + Date.now()
  );
  if (blob instanceof Blob) {
    document.getElementById("screenImg").src = URL.createObjectURL(blob); // Hien thi anh
    if (save && store.db) {
      // Luu anh vao CSDL trinh duyet
      store.db
        .transaction(["images"], "readwrite")
        .objectStore("images")
        .add({ blob, date: new Date() });
      logActionUI("Đã chụp & lưu", true);
      loadGallery(); // Tai lai thu vien
    }
  }
}

/**
 * @brief Bat/Tat che do tu dong chup man hinh
 */
export function toggleAutoShot(cb) {
  if (cb.checked) {
    updateScreen(); // Chup ngay lap tuc
    store.autoShotInt = setInterval(() => updateScreen(false), 2000); // Lap lai sau 2s
  } else clearInterval(store.autoShotInt); // Dung lai
}

/**
 * @brief Tai thu vien anh tu IndexedDB len giao dien
 */
export function loadGallery() {
  if (!store.db) return;
  let h = "";
  // Mo "bang" images va doc nguoc (de lay anh moi nhat truoc)
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
      c.continue(); // Di den muc tiep theo
    } else
      document.getElementById("gallery").innerHTML =
        h || "<small>Trống</small>";
  };
}

/**
 * @brief Xoa toan bo anh trong IndexedDB
 */
export function clearGallery() {
  if (confirm("Xóa hết ảnh?")) {
    if (!store.db) return;
    store.db
      .transaction(["images"], "readwrite")
      .objectStore("images")
      .clear().onsuccess = () => {
      loadGallery();
      logActionUI("Đã xóa thư viện ảnh", true);
    };
  }
}

/**
 * @brief Bat/Tat luong livestream man hinh
 */
export function toggleScreenStream(btn) {
  const streamView = document.getElementById("screenStreamView");
  const streamStatus = document.getElementById("screenStreamStatus");
  store.isScreenStreamOn = !store.isScreenStreamOn; // Dao trang thai

  if (store.isScreenStreamOn) {
    // --- KHI BAT STREAM ---
    streamView.src = "/api/stream/screen?t=" + Date.now();
    streamView.alt = "Đang tải luồng...";
    btn.textContent = "⏹️ Tắt Stream Màn Hình";
    btn.classList.add("btn-danger");
    btn.classList.remove("btn-primary");
    streamStatus.textContent = "⏳ Đang kết nối...";
    logActionUI("Bật livestream màn hình", true);
  } else {
    // --- KHI TAT STREAM ---
    streamView.src = ""; // Huy ket noi
    streamView.alt = "Stream đã tắt.";
    btn.textContent = "▶️ Bật Stream Màn Hình";
    btn.classList.remove("btn-danger");
    btn.classList.add("btn-primary");
    streamStatus.textContent = "";
    logActionUI("Tắt livestream màn hình", true);
  }
}
