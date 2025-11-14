// controllers/ProcessController.h
#ifndef PROCESSCONTROLLER_H
#define PROCESSCONTROLLER_H

#include <winsock2.h>
#include <string>

/**
 * @brief Xu ly cac API cho tab "Tien trinh" (PID, Kill)
 */
class ProcessController
{
public:
    void handleGetProcesses(SOCKET client);
    void handleKillProcess(SOCKET client, const std::string &body);

private:
    std::string listProcessesJson();
    bool killProcessByPid(DWORD pid);
};

#endif // PROCESSCONTROLLER_H