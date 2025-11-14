// controllers/DeviceController.cpp
#include "DeviceController.h"
#include "../utils/helpers.h"
#include "../utils/logging.h"
#include <sstream>
#include <tchar.h>
#include <stdio.h>
#include <stdexcept> // Can cho stoi

using namespace std;

// ... (ham buildDeviceListJson va G_DEVICE_LIST_JSON khong doi) ...
string DeviceController::G_DEVICE_LIST_JSON = "{\"video\":[],\"audio\":[]}";

void DeviceController::buildDeviceListJson()
{
    // ... (khong doi) ...
    logConsole("SYSTEM", "Dang tim FFmpeg, Camera va Mic...");
    string output;
    try
    {
        output = exec("ffmpeg -list_devices true -f dshow -i dummy 2>&1");
    }
    catch (...)
    {
        logConsole("SYSTEM", "LOI: Khong the chay FFmpeg.");
        return;
    }

    stringstream ss(output);
    string line;
    vector<string> videoDevices;
    vector<string> audioDevices;

    while (getline(ss, line))
    {
        size_t typePos = string::npos;
        vector<string> *targetVector = nullptr;

        if ((typePos = line.find("(video)")) != string::npos)
            targetVector = &videoDevices;
        else if ((typePos = line.find("(audio)")) != string::npos)
            targetVector = &audioDevices;

        if (targetVector)
        {
            size_t endQuote = line.rfind('"', typePos);
            if (endQuote == string::npos)
                continue;
            size_t startQuote = line.rfind('"', endQuote - 1);
            if (startQuote == string::npos)
                continue;
            string name = line.substr(startQuote + 1, endQuote - startQuote - 1);
            targetVector->push_back(name);
        }
    }
    stringstream json_ss;
    json_ss << "{\"video\":[";
    for (size_t i = 0; i < videoDevices.size(); ++i)
        json_ss << (i ? "," : "") << "\"" << jsonEscape(videoDevices[i]) << "\"";
    json_ss << "],\"audio\":[";
    for (size_t i = 0; i < audioDevices.size(); ++i)
        json_ss << (i ? "," : "") << "\"" << jsonEscape(audioDevices[i]) << "\"";
    json_ss << "]}";
    G_DEVICE_LIST_JSON = json_ss.str();
    logConsole("SYSTEM", "Da cap nhat danh sach thiet bi.");
}

// --- Public Handlers ---

void DeviceController::handleGetDevices(SOCKET client, const string &path)
{
    // ... (khong doi) ...
    if (path.find("refresh=1") != string::npos)
    {
        logConsole("CLIENT", "Yeu cau quet lai thiet bi...");
        buildDeviceListJson(); // Chay lai ham quet
    }
    sendFileResponse(client, G_DEVICE_LIST_JSON, "application/json");
}

void DeviceController::handleRecordVideo(SOCKET client, const string &body, const string &clientId)
{
    // *** START: SUA LOI STOI("") ***
    // Phan tich body JSON de lay thong so
    // getQueryParam(..., false) -> tim gia tri la so (khong co "")
    // getQueryParam(..., true) -> tim gia tri la chuoi (co "")

    string s_dur = getQueryParam(body, "duration", false);
    string cam_client = getQueryParam(body, "cam", true);
    string audio_client = getQueryParam(body, "audio", true);

    if (s_dur.empty())
        s_dur = "5";       // Mac dinh 5s neu khong tim thay
    int dur = stoi(s_dur); // stoi("5") se thanh cong
    // *** END: SUA LOI STOI("") ***

    if (cam_client.empty() || audio_client.empty())
    {
        sendFileResponse(client, "{\"ok\":false,\"error\":\"Chua chon Camera/Audio.\"}", "application/json");
        return;
    }

    // Tao ten file video ngau nhien
    string fname = "/vid_" + to_string(time(0)) + "_" + to_string(rand() % 1000) + ".mp4";
    // Tao lenh ffmpeg
    string cmd = "ffmpeg -f dshow -i video=\"" + cam_client + "\":audio=\"" + audio_client + "\" -t " + to_string(dur) + " -c:v libx264 -preset ultrafast -c:a aac -b:a 128k -y \"." + fname + "\" 2> NUL";

    logConsole(clientId, "Dang quay " + to_string(dur) + "s (Cam: " + cam_client + ", Mic: " + audio_client + ")...");
    if (system(cmd.c_str()) == 0) // Thuc thi lenh
    {
        logConsole(clientId, "Quay xong: " + fname);
        sendFileResponse(client, "{\"ok\":true,\"path\":\"" + fname + "\"}", "application/json"); // Tra ve duong dan file
    }
    else
    {
        logConsole(clientId, "Loi quay video FFmpeg");
        sendFileResponse(client, "{\"ok\":false,\"error\":\"FFmpeg Error. Check device name.\"}", "application/json");
    }
}

void DeviceController::handleStreamCam(SOCKET client, const string &path, const string &clientId)
{
    // ... (khong doi) ...
    string camName = urlDecode(getQueryParam(path, "cam", true));
    string audioName = urlDecode(getQueryParam(path, "audio", true));

    if (camName.empty() || audioName.empty())
    {
        logConsole(clientId, "Loi stream cam: Thieu ten cam/audio.");
        sendFileResponse(client, "400 Bad Request", "text/plain", 400);
        return;
    }

    logConsole(clientId, "Bat dau stream camera: " + camName);

    // 1. Tao Pipe
    HANDLE hPipeRead, hPipeWrite;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    if (!CreatePipe(&hPipeRead, &hPipeWrite, &sa, 0))
    {
        logConsole(clientId, "Loi CreatePipe");
        return;
    }

    // 2. Thiet lap de redirect stdout/stderr cua process con
    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // An cua so ffmpeg
    si.hStdOutput = hPipeWrite;
    si.hStdError = hPipeWrite;

    PROCESS_INFORMATION pi;
    string cmd_s = "ffmpeg -f dshow -i video=\"" + camName + "\":audio=\"" + audioName + "\" -f mpjpeg -q:v 4 -r 10 -";
    char cmd[1024];
    strcpy_s(cmd, cmd_s.c_str());

    // 3. Tao process ffmpeg
    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        logConsole(clientId, "Loi CreateProcess ffmpeg");
        CloseHandle(hPipeRead);
        CloseHandle(hPipeWrite);
        return;
    }

    CloseHandle(hPipeWrite); // Khong can ghi tu process nay nua

    // 4. Gui Header MJPEG (cho trinh duyet)
    string mjpeg_header = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: multipart/x-mixed-replace; boundary=--ffmpeg\r\n"
                          "Connection: keep-alive\r\n\r\n";

    if (sendAll(client, mjpeg_header) == SOCKET_ERROR)
    {
        logConsole(clientId, "Client ngat ket noi truoc khi stream.");
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hPipeRead);
        return;
    }

    // 5. Vong lap Relay: Doc tu Pipe -> Gui vao Socket
    char relayBuffer[4096];
    DWORD bytesRead;
    while (ReadFile(hPipeRead, relayBuffer, sizeof(relayBuffer), &bytesRead, NULL) && bytesRead > 0)
    {
        if (send(client, relayBuffer, bytesRead, 0) == SOCKET_ERROR)
        {
            logConsole(clientId, "Client ngat ket noi (stream loi).");
            break; // Client ngat ket noi
        }
    }

    // 6. Don dep
    logConsole(clientId, "Dung stream camera.");
    TerminateProcess(pi.hProcess, 1); // Giet ffmpeg.exe
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hPipeRead);
}