// modules/socket.js
import { store } from "./store.js";
// Khong import 'ui' de tranh loi phu thuoc vong

// 1. Tao ket noi WebSocket
export const socket = new WebSocket(`ws://${window.location.host}`);

const responseHandlers = {};

export function onCommand(command, handler) {
  responseHandlers[command] = handler;
}

// 2. Ham xu ly trung tam KHI NHAN DU LIEU tu Gateway
socket.onmessage = (event) => {
  // Kiem tra stream binary (anh)
  if (event.data instanceof Blob || event.data instanceof ArrayBuffer) {
    const url = URL.createObjectURL(
      new Blob([event.data], { type: "image/jpeg" })
    );

    const screenView = document.getElementById("screenStreamView");
    const camView = document.getElementById("camStreamView");

    if (store.isScreenStreamOn && screenView) {
      screenView.src = url;
      screenView.onload = () => URL.revokeObjectURL(url);
    } else if (store.isCamStreamOn && camView) {
      camView.src = url;
      camView.onload = () => URL.revokeObjectURL(url);
    }
    return;
  }

  // Xu ly JSON
  const msg = JSON.parse(event.data);

  if (msg.type === "stream_start") {
    console.log("Nhan tin hieu STREAM_START tu server");
    if (store.isScreenStreamOn)
      document.getElementById("screenStreamStatus").textContent =
        "✅ Đã kết nối luồng.";
    if (store.isCamStreamOn)
      document.getElementById("camStreamStatus").textContent =
        "✅ Đã kết nối luồng.";
    return;
  }

  // MOI: Xu ly khi stream dung (do C++ dong TCP)
  if (msg.type === "stream_stop") {
    console.log("Nhan tin hieu STREAM_STOP (tu Gateway)");
    // Goi ham toggle de reset UI
    if (store.isScreenStreamOn) {
      window.toggleScreenStream(null); // Gui 'null' de biet day la tu dong tat
    }
    if (store.isCamStreamOn) {
      window.toggleCamStream(null); // Gui 'null'
    }
    return;
  }

  // MOI: Xu ly loi tu Gateway
  if (msg.type === "error") {
    window.dispatchEvent(
      new CustomEvent("socket:error", {
        detail: `Lỗi Gateway/TCP: ${msg.payload}`,
      })
    );
    return;
  }

  if (msg.type === "json") {
    const command = msg.command;
    const payload = msg.payload;

    if (responseHandlers[command]) {
      responseHandlers[command](payload);
    } else {
      console.log("Nhan duoc phan hoi khong xu ly:", command, payload);
    }
  }
};

socket.onopen = () => {
  console.log("WebSocket da ket noi!");
  // Phat ra su kien de app.js biet va bat dau tai du lieu
  window.dispatchEvent(new CustomEvent("socket:open"));
};

socket.onclose = (event) => {
  console.error("WebSocket da dong!", event.reason);
  window.dispatchEvent(
    new CustomEvent("socket:close", { detail: event.reason })
  );
};

socket.onerror = (err) => {
  console.error("Loi WebSocket!", err);
  window.dispatchEvent(
    new CustomEvent("socket:error", { detail: "Lỗi WebSocket Chung" })
  );
};

/**
 * @brief Ham "van nang" de GUI LENH den server
 */
export function sendCommand(command, payload = null) {
  if (socket.readyState === WebSocket.OPEN) {
    socket.send(
      JSON.stringify({
        command: command,
        payload: payload,
      })
    );
  } else {
    console.error(`Loi: WebSocket chua san sang (dinh goi lenh: ${command})!`);
    window.dispatchEvent(
      new CustomEvent("socket:error", { detail: "WebSocket not ready" })
    );
  }
}
