// modules/tab-cam.js
import { store } from "./store.js";
import { api } from "./api.js";
import { logActionUI } from "./ui.js";

/**
 * @brief Tai danh sach Camera/Mic tu server
 * @param {boolean} forceRefresh Neu true, yeu cau server quet lai
 */
export async function loadDevices(forceRefresh = false) {
  const camSelect = document.getElementById("camName");
  const audioSelect = document.getElementById("audioName");
  camSelect.innerHTML = "<option>Dang tai...</option>";
  audioSelect.innerHTML = "<option>Dang tai...</option>";

  let apiUrl = "/api/devices?t=" + Date.now();
  if (forceRefresh) {
    apiUrl += "&refresh=1"; // Them tham so de server biet can quet lai
    logActionUI("Đang quét lại Camera/Mic...", true);
  }

  let res = await api(apiUrl);
  camSelect.innerHTML = "";
  audioSelect.innerHTML = "";

  // Do du lieu vao <select> camera
  if (res.video && res.video.length > 0) {
    res.video.forEach((cam) => {
      const opt = document.createElement("option");
      opt.value = cam;
      opt.textContent = cam;
      if (cam.toLowerCase().includes("usb")) opt.selected = true; // Tu dong chon webcam
      camSelect.appendChild(opt);
    });
  } else {
    camSelect.innerHTML = "<option value=''>Khong tim thay camera</option>";
  }

  // Do du lieu vao <select> mic
  if (res.audio && res.audio.length > 0) {
    res.audio.forEach((audio) => {
      const opt = document.createElement("option");
      opt.value = audio;
      opt.textContent = audio;
      if (audio.toLowerCase().includes("realtek")) opt.selected = true; // Tu dong chon mic
      audioSelect.appendChild(opt);
    });
  } else {
    audioSelect.innerHTML = "<option value=''>Khong tim thay micro</option>";
  }
}

/**
 * @brief Gui lenh quay video (ghi file)
 */
export async function recordVideo() {
  const btn = document.getElementById("btnVid");
  const stat = document.getElementById("vidStatus");
  let dur = parseInt(document.getElementById("vidDur").value) || 5;

  let camName = document.getElementById("camName").value;
  let audioName = document.getElementById("audioName").value;

  // Kiem tra dau vao
  if (!camName || !audioName || camName.startsWith("Khong tim")) {
    stat.textContent = "❌ Vui lòng chọn camera và micro.";
    return;
  }
  if (dur > 300) {
    stat.textContent = "❌ Tối đa 300 giây.";
    return;
  }

  btn.disabled = true; // Vo hieu hoa nut quay
  stat.textContent = `⏳ Đang quay ${dur}s...`;

  // 1. Goi API yeu cau server quay
  let res = await api("/api/video/capture", {
    duration: dur,
    cam: camName,
    audio: audioName,
  });

  if (res.ok && res.path) {
    // 2. Neu server quay thanh cong, tai file video (.mp4) ve
    stat.textContent = "⬇️ Đang tải video...";
    let blob = await api(res.path); // Tai file mp4 ve
    if (blob instanceof Blob && blob.size > 0 && store.db) {
      // 3. Luu file video vao IndexedDB
      store.db
        .transaction(["videos"], "readwrite")
        .objectStore("videos")
        .add({ blob, date: new Date() });
      loadVidGallery(); // Tai lai thu vien video
      logActionUI(`Đã quay & lưu video ${dur}s`, true);
      stat.textContent = "✅ Đã lưu vào thư viện!";
    } else {
      stat.textContent = "❌ Lỗi tải video về client";
      logActionUI("Lỗi tải video", false);
    }
  } else {
    stat.textContent = `❌ ${res.error || "Lỗi quay video."}`;
    logActionUI(`Lỗi quay: ${res.error || "Unknown"}`, false);
  }
  btn.disabled = false; // Kich hoat lai nut quay
}

/**
 * @brief Tai thu vien video tu IndexedDB len giao dien
 */
export function loadVidGallery() {
  if (!store.db) return;
  let h = "";
  store.db
    .transaction(["videos"], "readonly")
    .objectStore("videos")
    .openCursor(null, "prev").onsuccess = (e) => {
    let c = e.target.result;
    if (c) {
      let url = URL.createObjectURL(c.value.blob);
      h += `<div class="gallery-item video-item"><video src="${url}" controls style="width:100%;height:80px"></video></div>`;
      c.continue();
    } else
      document.getElementById("vidGallery").innerHTML =
        h || "<small>Trống</small>";
  };
}

/**
 * @brief Xoa toan bo video trong IndexedDB
 */
export function clearVideos() {
  if (confirm("Xóa hết video?")) {
    if (!store.db) return;
    store.db
      .transaction(["videos"], "readwrite")
      .objectStore("videos")
      .clear().onsuccess = () => {
      loadVidGallery();
      logActionUI("Đã xóa thư viện video", true);
    };
  }
}

/**
 * @brief Bat/Tat luong livestream camera
 */
export function toggleCamStream(btn) {
  const streamView = document.getElementById("camStreamView");
  const streamStatus = document.getElementById("camStreamStatus");
  store.isCamStreamOn = !store.isCamStreamOn; // Dao trang thai

  if (store.isCamStreamOn) {
    // --- KHI BAT STREAM ---
    const camName = document.getElementById("camName").value;
    const audioName = document.getElementById("audioName").value;

    // Kiem tra da chon thiet bi chua
    if (!camName || !audioName || camName.startsWith("Khong tim")) {
      streamStatus.textContent = "❌ Vui lòng chọn Camera và Mic ở trên!";
      store.isCamStreamOn = false; // Dat lai trang thai
      return;
    }

    // Ma hoa ten de gui qua URL
    const encodedCam = encodeURIComponent(camName);
    const encodedAudio = encodeURIComponent(audioName);

    streamView.src =
      "/api/stream/cam?cam=" +
      encodedCam +
      "&audio=" +
      encodedAudio +
      "&t=" +
      Date.now();
    streamView.alt = "Đang tải luồng...";
    btn.textContent = "⏹️ Tắt Stream Camera";
    btn.classList.add("btn-danger");
    btn.classList.remove("btn-primary");
    streamStatus.textContent = "⏳ Đang kết nối với ffmpeg...";
    logActionUI("Bật livestream camera", true);
  } else {
    // --- KHI TAT STREAM ---
    streamView.src = ""; // Huy ket noi
    streamView.alt = "Stream đã tắt.";
    btn.textContent = "▶️ Bật Stream Camera";
    btn.classList.remove("btn-danger");
    btn.classList.add("btn-primary");
    streamStatus.textContent = "";
    logActionUI("Tắt livestream camera", true);
  }
}
