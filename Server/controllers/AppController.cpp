// controllers/AppController.cpp
#include "AppController.h"
#include "../utils/helpers.h" // Can sendFileResponse, jsonEscape, getProcessPath
#include <sstream>
#include <vector>
#include <shellapi.h> // cho ShellExecuteA
#include <cstdint>    //(cho uint64_t)

using namespace std;

// Struct de luu thong tin cua so ung dung
struct AppController::AppInfo
{
    HWND hwnd;
    DWORD pid;
    string title;
    string path;
};

/**
 * @brief Liet ke tat ca cac CUA SO ung dung dang mo (co giao dien).
 */
vector<AppController::AppInfo> AppController::listApplications()
{
    vector<AppInfo> out;
    // Duyet qua tat ca cac cua so tren he thong
    EnumWindows([](HWND h, LPARAM l) -> BOOL
                {
        // Chi lay cac cua so co hien thi (visible) va co tieu de
        if (IsWindowVisible(h) && GetWindowTextLengthA(h) > 0) {
            char buf[256]; GetWindowTextA(h, buf, 256); // Lay tieu de
            DWORD pid; GetWindowThreadProcessId(h, &pid); // Lay PID
            string path = getProcessPath(pid); // Lay duong dan .exe
            if (path.empty()) path = "Unknown";
            ((vector<AppInfo>*)l)->push_back({h, pid, string(buf), path});
        } return TRUE; }, (LPARAM)&out);
    return out;
}

/**
 * @brief Dong mot cua so ung dung bang "handle" (HWND) cua no.
 */
bool AppController::closeWindowByHwnd(HWND hwnd)
{
    if (!IsWindow(hwnd))
        return false;
    PostMessage(hwnd, WM_CLOSE, 0, 0); // Gui tin hieu yeu cau dong (an toan)
    return true;
}

/**
 * @brief Mo mot ung dung/file bang ten lenh (vi du: "chrome", "notepad").
 */
bool AppController::startProcessFromCommand(const string &cmd)
{
    return ((intptr_t)ShellExecuteA(NULL, "open", cmd.c_str(), NULL, NULL, SW_SHOWNORMAL) > 32);
}

// --- Public Handlers ---

void AppController::handleGetApps(SOCKET client)
{
    auto apps = listApplications();
    stringstream ss;
    ss << "[";
    for (size_t i = 0; i < apps.size(); ++i)
    {
        ss << (i ? "," : "") << "{\"hwnd\":" << (uint64_t)(uintptr_t)apps[i].hwnd
           << ",\"pid\":" << apps[i].pid
           << ",\"title\":\"" << jsonEscape(apps[i].title)
           << "\",\"path\":\"" << jsonEscape(apps[i].path) << "\"}";
    }
    ss << "]";
    sendFileResponse(client, ss.str(), "application/json");
}

void AppController::handleCloseApp(SOCKET client, const string &body)
{
    uint64_t id = 0;
    for (char c : body)
        if (isdigit(c))
            id = id * 10 + (c - '0');

    bool ok = closeWindowByHwnd((HWND)(uintptr_t)id);
    sendFileResponse(client, ok ? "{\"ok\":true}" : "{\"ok\":false}", "application/json");
}

void AppController::handleStartApp(SOCKET client, const string &body)
{
    string v = getQueryParam(body, "name");
    bool ok = !v.empty() && startProcessFromCommand(v);
    sendFileResponse(client, ok ? "{\"ok\":true}" : "{\"ok\":false}", "application/json");
}