// controllers/DeviceController.cpp (PHIEN BAN HYBRID)
#include "DeviceController.h"
#include "../utils/helpers.h"
#include "../utils/logging.h"
#include <sstream>
#include <tchar.h>
#include <stdio.h>
#include <stdexcept>
#include <thread>

using namespace std;

string DeviceController::G_DEVICE_LIST_JSON = "{\"video\":[],\"audio\":[]}";
std::mutex DeviceController::G_DEVICE_LIST_MUTEX;
std::atomic<bool> DeviceController::G_IS_REFRESHING(false);

void DeviceController::buildDeviceListJson()
{
    // Dam bao chi 1 luong duoc refresh
    if (G_IS_REFRESHING.exchange(true))
    {
        logConsole("SYSTEM", "Refresh bi trung lap, bo qua.");
        return;
    }

    logConsole("SYSTEM", "Dang tim FFmpeg, Camera va Mic (Async)...");
    string output;
    try
    {
        // === DAY LA TAC VU BLOCK ===
        output = exec("ffmpeg -list_devices true -f dshow -i dummy 2>&1");
    }
    catch (...)
    {
        logConsole("SYSTEM", "LOI: Khong the chay FFmpeg.");
        G_IS_REFRESHING.store(false); // Mo khoa refresh
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
    {
        std::lock_guard<std::mutex> lock(G_DEVICE_LIST_MUTEX);
        G_DEVICE_LIST_JSON = json_ss.str();
    }
    logConsole("SYSTEM", "Da cap nhat danh sach thiet bi.");
    G_IS_REFRESHING.store(false); // Mo khoa refresh
}

string DeviceController::getDevices(bool refresh)
{
    if (refresh)
    {
        if (G_IS_REFRESHING.load())
        {
            // Neu dang co 1 luong khac refresh, tra ve "busy"
            return "{\"video\":[],\"audio\":[], \"status\":\"refresh_busy\"}";
        }
        else
        {
            // Neu chua co, bat dau refresh trong luong MOI va tra ve "pending"
            logConsole("Gateway", "Yeu cau quet lai thiet bi (Async)...");
            std::thread(buildDeviceListJson).detach();
            return "{\"video\":[],\"audio\":[], \"status\":\"refresh_pending\"}";
        }
    }
    else
    {
        // === VA LOI 3: Dung Mutex de doc an toan ===
        std::lock_guard<std::mutex> lock(G_DEVICE_LIST_MUTEX);
        return G_DEVICE_LIST_JSON;
    }
}

void DeviceController::recordVideoAsync(SOCKET client, string correlationId,
                                        string dur_str, string cam, string audio,
                                        std::mutex &socketMutex)
{
    // Tao mot luong rieng biet (Detached Thread)
    // Luong nay se tu chay, tu ket thuc, khong anh huong luong chinh
    std::thread([=, &socketMutex]()
                {
        if (cam.empty() || audio.empty()) {
            sendCmdTcp(client, correlationId, "JSON {\"ok\":false,\"error\":\"No Device Selected\"}", socketMutex);
            return;
        }

        time_t now = time(0);
        char fname_buf[100];
        strftime(fname_buf, sizeof(fname_buf), "vid_%Y%m%d_%H%M%S.mp4", localtime(&now));

        string fname_rel_path = "../public/" + string(fname_buf);
        string url_path = "/" + string(fname_buf);

        // Lenh FFmpeg (da boc quote can than)
        string cmd = "ffmpeg -f dshow -i video=\"" + cam + "\":audio=\"" + audio +
                     "\" -t " + dur_str +
                     " -c:v libx264 -preset ultrafast -c:a aac -b:a 128k -y \"" +
                     fname_rel_path + "\" 2> NUL";

        logConsole("ASYNC_REC", "Dang quay (trong nen) " + dur_str + "s: " + fname_rel_path);

        // Hanh dong nay mat thoi gian (Block luong phu, nhung khong block Server)
        int ret = system(cmd.c_str());

        // Sau khi quay xong (hoac loi), moi gui ket qua ve
        if (ret == 0) {
            logConsole("ASYNC_REC", "Quay xong. Gui ket qua.");
            sendCmdTcp(client, correlationId, "JSON {\"ok\":true,\"path\":\"" + url_path + "\"}", socketMutex);
        } else {
            logConsole("ASYNC_REC", "Loi FFmpeg.");
            sendCmdTcp(client, correlationId, "JSON {\"ok\":false,\"error\":\"FFmpeg Failed\"}", socketMutex);
        } })
        .detach();
}
/**
 * @brief Xu ly vong lap stream camera (CHO CONG 9001)
 */
void DeviceController::handleStreamCam(SOCKET client, const string &clientIP, const string &cam, const string &audio)
{
    if (cam.empty() || audio.empty())
    {
        logConsole(clientIP, "Loi stream cam: Thieu ten cam/audio.");
        return;
    }
    logConsole(clientIP, "Bat dau stream camera (MJPEG Stream - Cong 9001): " + cam);

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
    {
        logConsole(clientIP, "Loi CreateProcessA");
        CloseHandle(hPipeRead);
        CloseHandle(hPipeWrite);
        return;
    }
    CloseHandle(hPipeWrite);

    // === XOA: sendTcp("STREAM_START") ===
    // (Cong 9001 chi gui video)

    char relayBuffer[4096];
    DWORD bytesRead;
    while (ReadFile(hPipeRead, relayBuffer, sizeof(relayBuffer), &bytesRead, NULL) && bytesRead > 0)
    {
        // === XOA: ioctlsocket ===
        // (Luong nay se tu dong ngat khi Gateway dong TCP)

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