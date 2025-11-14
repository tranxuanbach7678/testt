// controllers/AppController.h
#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <winsock2.h>
#include <string>
#include <vector>

/**
 * @brief Xu ly cac API cho tab "Ung dung" (HWND, Close, Start)
 */
class AppController
{
public:
    void handleGetApps(SOCKET client);
    void handleCloseApp(SOCKET client, const std::string &body);
    void handleStartApp(SOCKET client, const std::string &body);

private:
    struct AppInfo; // Dinh nghia nested struct trong .cpp
    std::vector<AppInfo> listApplications();
    bool closeWindowByHwnd(HWND hwnd);
    bool startProcessFromCommand(const std::string &cmd);
};

#endif // APPCONTROLLER_H