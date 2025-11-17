// modules/tab-cam.js
import { store } from "./store.js";
import { sendCommand } from "./socket.js";
import { logActionUI } from "./ui.js";

// Dang ky ham xu ly
export function handleDevicesData(data) {
  const camSelect = document.getElementById("camName");
  const audioSelect = document.getElementById("audioName");
  camSelect.innerHTML = "";
  audioSelect.innerHTML = "";

  if (data.video && data.video.length > 0) {
    data.video.forEach((cam) => {
      const opt = document.createElement("option");
      opt.value = cam;
      opt.textContent = cam;
      if (cam.toLowerCase().includes("usb")) opt.selected = true;
      camSelect.appendChild(opt);
    });
  } else {
    camSelect.innerHTML = "<option value=''>Khong tim thay camera</option>";
  }

  if (data.audio && data.audio.length > 0) {
    data.audio.forEach((audio) => {
      const opt = document.createElement("option");
      opt.value = audio;
      opt.textContent = audio;
      if (audio.toLowerCase().includes("realtek")) opt.selected = true;
      audioSelect.appendChild(opt);
    });
  } else {
    audioSelect.innerHTML = "<option value=''>Khong tim thay micro</option>";
  }
}
async function downloadAndSaveVideo(path) {
  const stat = document.getElementById("vidStatus");
  stat.textContent = "⬇️ Đang tải video...";
  try {
    const res = await fetch(path + "?t=" + Date.now()); // Them cache-buster
    const blob = await res.blob();
    if (blob.size > 0 && store.db) {
      // Luu vao IndexedDB
      store.db
        .transaction(["videos"], "readwrite")
        .objectStore("videos")
        .add({ blob, date: new Date() });
      loadVidGallery(); // Ham nay da co san trong file
      stat.textContent = "✅ Đã lưu vào thư viện!";
      setTimeout(() => {
        logActionUI(`Dọn dẹp file server: ${path}`, true); // Đây là log "đã xóa"
        sendCommand("DELETE_VIDEO", path);
      }, 2000);
    } else {
      throw new Error("File rỗng hoặc DB lỗi");
    }
  } catch (e) {
    stat.textContent = "❌ Lỗi tải video về client";
    logActionUI(`Lỗi tải file ${path}: ${e.message}`, false);
  }
}
export function handleRecordVideoData(data) {
  const stat = document.getElementById("vidStatus");
  const btn = document.getElementById("btnVid");
  if (data.ok && data.path) {
    stat.textContent = "✅ Quay thành công!";
    logActionUI("Server đã quay xong: " + data.path, true);

    // === FIX: GOI HAM TAI FILE VE ===
    downloadAndSaveVideo(data.path);
    // === KET THUC FIX ===
  } else {
    stat.textContent = `❌ ${data.error || "Lỗi quay."}`;
  }
  if (btn) btn.disabled = false;
}

export function loadDevices(forceRefresh = false) {
  if (forceRefresh) {
    logActionUI("Yêu cầu quét lại thiết bị...", true);
    sendCommand("REFRESH_DEVICES");
  } else {
    sendCommand("GET_DEVICES");
  }
}

export function recordVideo() {
  const btn = document.getElementById("btnVid");
  const stat = document.getElementById("vidStatus");
  let dur = parseInt(document.getElementById("vidDur").value) || 5;
  let camName = document.getElementById("camName").value;
  let audioName = document.getElementById("audioName").value;

  if (!camName || !audioName || camName.startsWith("Khong tim")) {
    stat.textContent = "❌ Vui lòng chọn camera và micro.";
    return;
  }
  if (btn) btn.disabled = true;
  stat.textContent = `⏳ Đang ra lệnh quay ${dur}s...`;

  // Gui lenh voi payload la object
  sendCommand("RECORD_VIDEO", {
    duration: dur,
    cam: camName,
    audio: audioName,
  });
}

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

export function toggleCamStream(btn) {
  const streamView = document.getElementById("camStreamView");
  const streamStatus = document.getElementById("camStreamStatus");

  // (Giu logic 'btn === null' de app.js (don luong) cu van chay duoc)
  if (btn === null) {
    store.isCamStreamOn = false;
    streamView.src = "";
    streamView.alt = "Stream đã tắt.";
    const activeBtn = document.getElementById("btnToggleCamStream");
    if (activeBtn) {
      activeBtn.textContent = "▶️ Bật Stream Camera";
      activeBtn.classList.remove("btn-danger");
      activeBtn.classList.add("btn-primary");
    }
    streamStatus.textContent = "";
    return;
  }

  store.isCamStreamOn = !store.isCamStreamOn;

  if (store.isCamStreamOn) {
    // (Toan bo code Bat Stream... giu nguyen)
    const camName = document.getElementById("camName").value;
    const audioName = document.getElementById("audioName").value;
    if (!camName || !audioName || camName.startsWith("Khong tim")) {
      streamStatus.textContent = "❌ Vui lòng chọn Camera và Mic!";
      store.isCamStreamOn = false;
      return;
    }

    store.isScreenStreamOn = false;
    document.getElementById("screenStreamView").src = "";

    streamView.alt = "Đang tải luồng...";
    btn.textContent = "⏹️ Tắt Stream Camera";
    btn.classList.add("btn-danger");
    btn.classList.remove("btn-primary");
    streamStatus.textContent = "⏳ Đang kết nối với ffmpeg...";
    logActionUI("Bật livestream camera", true);

    sendCommand("START_STREAM_CAM", { cam: camName, audio: audioName });
  } else {
    // Logic khi tat stream bang nut
    streamView.src = ""; // Xoa nguon anh
    streamView.alt = "Stream đã tắt.";
    const activeBtn = document.getElementById("btnToggleCamStream");
    if (activeBtn) {
      activeBtn.textContent = "▶️ Bật Stream Camera";
      activeBtn.classList.remove("btn-danger");
      activeBtn.classList.add("btn-primary");
    }
    streamStatus.textContent = "";
    logActionUI("Tắt livestream camera", true);

    // === FIX: Gui lenh dung ===
    sendCommand("STOP_STREAM");
  }
}
