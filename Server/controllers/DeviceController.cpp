// controllers/DeviceController.cpp
#include "DeviceController.h"
#include "../utils/helpers.h"
#include "../utils/logging.h"
#include <sstream>
#include <tchar.h>
#include <stdio.h>
#include <stdexcept>

using namespace std;

// Khoi tao bien static
string DeviceController::G_DEVICE_LIST_JSON = "{\"video\":[],\"audio\":[]}";

void DeviceController::buildDeviceListJson()
{
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
    vector<string> videoDevices, audioDevices;
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
            targetVector->push_back(line.substr(startQuote + 1, endQuote - startQuote - 1));
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
string DeviceController::getDevices(bool refresh)
{
    if (refresh)
    {
        logConsole("Gateway", "Yeu cau quet lai thiet bi...");
        buildDeviceListJson();
    }
    // --- SUA LOI: Khong them 'command' hay 'payload' ---
    return G_DEVICE_LIST_JSON;
}

string DeviceController::recordVideo(const string &dur_str, const string &cam, const string &audio)
{
    if (cam.empty() || audio.empty())
    {
        return "{\"ok\":false,\"error\":\"Chua chon Camera/Audio.\"}";
    }

    string fname = "public/vid_temp.mp4";

    // === FIX 2: Tra lai codec .mp4 va '2> NUL' (GIONG HET CODE CU) ===
    string cmd = "ffmpeg -f dshow -i video=\"" + cam + "\":audio=\"" + audio +
                 "\" -t " + dur_str +
                 " -c:v libx264 -preset ultrafast -c:a aac -b:a 128k -y \"" +
                 fname + "\" 2> NUL"; // <-- THEM 2> NUL

    logConsole("Gateway", "Dang quay " + dur_str + "s...");

    if (system(cmd.c_str()) == 0)
    {
        logConsole("Gateway", "Quay xong: " + fname);
        // === FIX 3: Tra ve duong dan ma web co a truy cap ===
        return "{\"ok\":true,\"path\":\"/vid_temp.mp4\"}"; // <-- Bỏ "public/"
    }
    else
    {
        logConsole("Gateway", "Loi quay video FFmpeg.");
        return "{\"ok\":false,\"error\":\"FFmpeg Error. Check device name.\"}";
    }
}
void DeviceController::handleStreamCam(SOCKET client, const string &clientIP, const string &cam, const string &audio)
{
    if (cam.empty() || audio.empty())
    {
        logConsole(clientIP, "Loi stream cam: Thieu ten cam/audio.");
        return;
    }

    logConsole(clientIP, "Bat dau stream camera (MJPEG Stream): " + cam);

    HANDLE hPipeRead, hPipeWrite;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

    if (!CreatePipe(&hPipeRead, &hPipeWrite, &sa, 0))
    {
        logConsole(clientIP, "Loi CreatePipe");
        return;
    }

    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hPipeWrite;
    si.hStdError = hPipeWrite;
    PROCESS_INFORMATION pi;

    string cmd_s = "ffmpeg -f dshow -i video=\"" + cam + "\":audio=\"" + audio + "\" -f mpjpeg -q:v 3 -r 20 -";
    char cmd[1024];
    strcpy_s(cmd, cmd_s.c_str());

    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    { /* ... */
        return;
    }
    CloseHandle(hPipeWrite);

    sendTcp(client, "STREAM_START");

    char relayBuffer[4096];
    DWORD bytesRead;

    while (ReadFile(hPipeRead, relayBuffer, sizeof(relayBuffer), &bytesRead, NULL) && bytesRead > 0)
    {
        // === THEM LAI: Kiem tra lenh moi ===
        u_long bytes_available = 0;
        ioctlsocket(client, FIONREAD, &bytes_available);
        if (bytes_available > 0)
        {
            logConsole(clientIP, "Nhan duoc lenh moi -> Dung stream cam.");
            break;
        }
        // === KET THUC THEM ===

        if (send(client, relayBuffer, bytesRead, 0) == SOCKET_ERROR)
        {
            logConsole(clientIP, "Client ngat ket noi (stream loi).");
            break;
        }
    }

    logConsole(clientIP, "Dung stream camera (MJPEG).");
    TerminateProcess(pi.hProcess, 1);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hPipeRead);
}