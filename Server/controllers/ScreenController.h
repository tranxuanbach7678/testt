// controllers/ScreenController.h
#ifndef SCREENCONTROLLER_H
#define SCREENCONTROLLER_H

#include <winsock2.h>
#include <string>

class ScreenController
{
public:
    void handleScreenshot(SOCKET client, const std::string &path, const std::string &clientId);
    void handleScreenStream(SOCKET client, const std::string &clientId);

private:
    std::string captureScreenToRam();
};

#endif // SCREENCONTROLLER_H