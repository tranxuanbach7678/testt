// admin-panel.js
import { exec } from "child_process";
import express from "express";
import http from "http";
import { WebSocketServer } from "ws";
import { spawn } from "child_process";
import path from "path";
import { fileURLToPath } from "url";

const ADMIN_PORT = 8000; // Cổng riêng cho Bảng Điều Khiển
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// --- Cấu trúc để quản lý các server con ---
const childProcesses = {
  cpp: null,
  gateway: null,
};

const app = express();
const server = http.createServer(app);

// --- 1. Thiết lập Admin WebSocket (để gửi log lên UI) ---
const wss = new WebSocketServer({ server });
const adminClients = new Set();

// Hàm helper để gửi log cho tất cả admin đang xem
function broadcastToAdmin(data) {
  const payload = JSON.stringify(data);
  adminClients.forEach((client) => {
    if (client.readyState === client.OPEN) {
      client.send(payload);
    }
  });
}

// Hàm helper để định dạng log
function log(source, message) {
  console.log(`[${source}]: ${message}`); // In ra console của admin-panel
  broadcastToAdmin({ source, message }); // Gửi lên UI
}

// --- 2. Hàm để Khởi động / Dừng các server con ---
function startServers() {
  if (childProcesses.cpp || childProcesses.gateway) {
    log("AdminPanel", "Lỗi: Server đang chạy!");
    return;
  }

  // Khởi động C++ Server
  log("AdminPanel", "Đang khởi động C++ Server (server.exe)...");
  try {
    childProcesses.cpp = spawn(path.join(__dirname, "server.exe"), [], {
      cwd: __dirname, // Đặt thư mục làm việc
    });

    // Lắng nghe log C++
    childProcesses.cpp.stdout.on("data", (data) => {
      log("C++ Server", data.toString().trim());
    });
    childProcesses.cpp.stderr.on("data", (data) => {
      log("C++ Server (ERR)", data.toString().trim());
    });
    childProcesses.cpp.on("close", (code) => {
      log("AdminPanel", `C++ Server đã dừng với mã ${code}.`);
      childProcesses.cpp = null;
    });
  } catch (e) {
    log("AdminPanel (ERR)", "Không thể khởi động server.exe: " + e.message);
    return;
  }

  // Khởi động Gateway
  log("AdminPanel", "Đang khởi động Gateway (gateway.js)...");
  try {
    childProcesses.gateway = spawn(
      "node",
      [path.join(__dirname, "gateway.js")],
      {
        cwd: __dirname,
      }
    );

    // Lắng nghe log Gateway
    childProcesses.gateway.stdout.on("data", (data) => {
      log("Gateway.js", data.toString().trim());
    });
    childProcesses.gateway.stderr.on("data", (data) => {
      log("Gateway.js (ERR)", data.toString().trim());
    });
    childProcesses.gateway.on("close", (code) => {
      log("AdminPanel", `Gateway.js đã dừng với mã ${code}.`);
      childProcesses.gateway = null;
    });
  } catch (e) {
    log("AdminPanel (ERR)", "Không thể khởi động gateway.js: " + e.message);
    return;
  }

  log("AdminPanel", "Đã khởi động cả hai server.");
}

function stopServers() {
  if (childProcesses.cpp) {
    const pid = childProcesses.cpp.pid;
    log(
      "AdminPanel",
      `Dang gui lenh dung (taskkill /T /F) cho C++ Server (PID: ${pid})...`
    );

    // Dung lenh 'taskkill' cua Windows
    // /T - (Tree) Giet process cha va tat ca process con (ffmpeg.exe)
    // /F - (Force) Ep buoc dung
    exec(`taskkill /PID ${pid} /T /F`, (err, stdout, stderr) => {
      if (err) {
        // Co the loi 'process not found' neu no da tu tat
        log(
          "AdminPanel (WARN)",
          `Taskkill C++ co loi (co the bo qua): ${stderr}`
        );
      } else {
        log("AdminPanel", "Taskkill C++ (va cay process) thanh cong.");
      }
    });

    // Dat lai ngay lap tuc de giai quyet loi tu buoc truoc
    childProcesses.cpp = null;
  }

  if (childProcesses.gateway) {
    childProcesses.gateway.kill("SIGTERM");
    log("AdminPanel", "Đã gửi lệnh dừng cho Gateway.js.");
    childProcesses.gateway = null;
  }

  log("AdminPanel", "Đã dọn dẹp handles. Sẵn sàng để khởi động lại.");
}

// --- 3. Xử lý kết nối từ Giao diện Admin ---
wss.on("connection", (ws) => {
  log("AdminPanel", "Giao diện Admin đã kết nối.");
  adminClients.add(ws);

  // Xử lý lệnh từ Giao diện Admin (Start/Stop)
  ws.on("message", (message) => {
    const msg = message.toString();
    if (msg === "START") {
      log("AdminPanel", "Nhận lệnh START từ UI...");
      startServers();
    } else if (msg === "STOP") {
      log("AdminPanel", "Nhận lệnh STOP từ UI...");
      stopServers();
    }
  });

  ws.on("close", () => {
    log("AdminPanel", "Giao diện Admin đã ngắt kết nối.");
    adminClients.delete(ws);
  });
});

// --- 4. Phục vụ file HTML cho Giao diện Admin ---
app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "admin-ui.html"));
});

server.listen(ADMIN_PORT, () => {
  console.log(`=================================================`);
  console.log(`  Bảng Điều Khiển Server đang chạy tại:`);
  console.log(`  http://localhost:${ADMIN_PORT}`);
  console.log(`=================================================`);
  console.log("Bấm 'Start Servers' trên giao diện web để bắt đầu.");
});
