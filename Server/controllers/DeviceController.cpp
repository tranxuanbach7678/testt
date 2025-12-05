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

std::vector<SOCKET> DeviceController::viewingClients;
std::vector<RecordingSession *> DeviceController::recordingSessions;
std::mutex DeviceController::streamMutex;
std::atomic<bool> DeviceController::isStreaming(false);

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

void DeviceController::processFinishedSession(RecordingSession *session)
{
    session->fileStream.close();

    logConsole("REC", "Dang convert sang MP4: " + session->finalPath);

    // Convert Mjpeg (raw dump) sang MP4
    string cmd = "ffmpeg -f mjpeg -r 20 -i \"" + session->tempFilename + "\" -c:v libx264 -preset ultrafast -y \"" + session->finalPath + "\"";

    system(cmd.c_str());

    // Xoa file tam
    remove(session->tempFilename.c_str());

    // Gui ket qua ve Client
    string url_path = "/" + session->finalPath.substr(session->finalPath.find("../public/") + 10); // Cat bo public/
    sendCmdTcp(session->client, session->correlationId, "JSON {\"ok\":true,\"path\":\"" + url_path + "\"}", *(session->socketMutex));

    delete session;
}

// === HAM BROADCAST (MASTER) ===
void DeviceController::broadcastWorker(string cam, string audio)
{
    logConsole("BROADCAST", "Master Stream KHOI DONG (" + cam + ")");

    // 1. Khoi tao Pipe & FFmpeg (Giu nguyen nhu cu)
    HANDLE hPipeRead, hPipeWrite;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    CreatePipe(&hPipeRead, &hPipeWrite, &sa, 0);
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
        logConsole("BROADCAST", "Loi khoi tao FFmpeg");
        CloseHandle(hPipeRead);
        CloseHandle(hPipeWrite);
        isStreaming = false;
        return;
    }
    CloseHandle(hPipeWrite);

    char buffer[40960];
    DWORD bytesRead;

    while (isStreaming)
    {
        if (!ReadFile(hPipeRead, buffer, sizeof(buffer), &bytesRead, NULL) || bytesRead == 0)
            break;

        lock_guard<mutex> lock(streamMutex);

        // A. DIEU KIEN DUNG: Khong con ai xem VA khong con ai quay
        if (viewingClients.empty() && recordingSessions.empty())
        {
            logConsole("BROADCAST", "Idle (No viewers/recorders). Shutdown.");
            isStreaming = false;
            break;
        }

        // B. GUI CHO VIEWERS
        for (auto it = viewingClients.begin(); it != viewingClients.end();)
        {
            if (send(*it, buffer, bytesRead, 0) == SOCKET_ERROR)
            {
                closesocket(*it);
                it = viewingClients.erase(it);
            }
            else
                ++it;
        }

        // C. GHI CHO RECORDERS
        time_t now = time(0);
        for (auto it = recordingSessions.begin(); it != recordingSessions.end();)
        {
            RecordingSession *sess = *it;

            // Ghi du lieu vao file
            sess->fileStream.write(buffer, bytesRead);

            // Kiem tra het gio
            if (now >= sess->endTime)
            {
                // Xoa khoi danh sach truoc
                it = recordingSessions.erase(it);

                // Chay thread xu ly convert & gui json (tach biet de khong block stream)
                std::thread(processFinishedSession, sess).detach();
            }
            else
            {
                ++it;
            }
        }
    }

    TerminateProcess(pi.hProcess, 0);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hPipeRead);

    // Cleanup de an toan
    lock_guard<mutex> lock(streamMutex);
    isStreaming = false;
}

// === HAM QUAY VIDEO (CHI DANG KY SESSION) ===
void DeviceController::recordVideoAsync(SOCKET client, string correlationId,
                                        string dur_str, string cam, string audio,
                                        std::mutex &socketMutex)
{
    lock_guard<mutex> lock(streamMutex);

    int duration = 5;
    try
    {
        duration = stoi(dur_str);
    }
    catch (...)
    {
    }

    time_t now = time(0);
    char fname_buf[100];
    strftime(fname_buf, sizeof(fname_buf), "vid_%Y%m%d_%H%M%S", localtime(&now));

    // Tao session moi
    RecordingSession *sess = new RecordingSession();
    sess->client = client;
    sess->correlationId = correlationId;
    sess->socketMutex = &socketMutex;
    sess->endTime = now + duration + 1;

    // Ten file: session nay se co hau to rieng de khong trung nhau neu 2 client cung quay
    // Them ID ngau nhien vao ten file
    string randId = to_string(rand() % 1000);

    // File tam (chua raw mjpeg data)
    sess->tempFilename = "../public/" + string(fname_buf) + "_" + randId + ".mjpeg";
    // File cuoi (mp4) - Day la path ma Server.exe nhin thay (tu thu muc core)
    sess->finalPath = "../public/" + string(fname_buf) + "_" + randId + ".mp4";

    sess->fileStream.open(sess->tempFilename, ios::binary);

    if (!sess->fileStream.is_open())
    {
        sendCmdTcp(client, correlationId, "JSON {\"ok\":false,\"error\":\"Cannot create file\"}", socketMutex);
        delete sess;
        return;
    }

    recordingSessions.push_back(sess);
    logConsole("REC", "Da dang ky quay " + dur_str + "s. File: " + sess->tempFilename);

    // Neu stream chua chay thi bat no len
    if (!isStreaming)
    {
        isStreaming = true;
        std::thread(broadcastWorker, cam, audio).detach();
    }
}

// === HAM LIVE STREAM (GIU KET NOI) ===
void DeviceController::handleStreamCam(SOCKET client, const string &clientIP, const string &cam, const string &audio)
{
    {
        lock_guard<mutex> lock(streamMutex);
        viewingClients.push_back(client);
        if (!isStreaming)
        {
            isStreaming = true;
            std::thread(broadcastWorker, cam, audio).detach();
        }
    }

    char dummy[10];
    while (true)
    {
        if (recv(client, dummy, sizeof(dummy), 0) <= 0)
            break;
    }
}