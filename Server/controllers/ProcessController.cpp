// controllers/ProcessController.cpp
#include "ProcessController.h"
#include "../utils/helpers.h" // Can sendFileResponse, jsonEscape
#include <sstream>
#include <tlhelp32.h> // cho listProcessesJson
#include <cstdint>

using namespace std;

/**
 * @brief Liet ke tat ca tien trinh (process) dang chay thanh chuoi JSON.
 */
string ProcessController::listProcessesJson()
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    stringstream ss;
    ss << "[";
    if (Process32First(snap, &pe))
    {
        bool first = true;
        do
        {
            if (!first)
                ss << ",";
            first = false;
            ss << "{\"pid\":" << pe.th32ProcessID << ",\"exe\":\"" << jsonEscape(pe.szExeFile) << "\"}";
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    ss << "]";
    return ss.str();
}

/**
 * @brief "Giet" mot tien trinh bang ma PID cua no.
 */
bool ProcessController::killProcessByPid(DWORD pid)
{
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!h)
        return false;
    BOOL ok = TerminateProcess(h, 1);
    CloseHandle(h);
    return ok;
}

// --- Public Handlers ---

void ProcessController::handleGetProcesses(SOCKET client)
{
    sendFileResponse(client, listProcessesJson(), "application/json");
}

void ProcessController::handleKillProcess(SOCKET client, const string &body)
{
    uint64_t id = 0;
    for (char c : body)
        if (isdigit(c))
            id = id * 10 + (c - '0');

    bool ok = killProcessByPid((DWORD)id);
    sendFileResponse(client, ok ? "{\"ok\":true}" : "{\"ok\":false}", "application/json");
}