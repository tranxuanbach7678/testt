// controllers/ScreenController.h
#ifndef SCREENCONTROLLER_H
#define SCREENCONTROLLER_H

#include <winsock2.h>
#include <string>

/**
 * @brief Xu ly logic cho "Man hinh"
 */
class ScreenController
{
public:
    void handleScreenStream(SOCKET client, const std::string &clientIP);
    std::string getScreenshotBase64();

private:
    std::string captureScreenToRam();
};

#endif // SCREENCONTROLLER_H