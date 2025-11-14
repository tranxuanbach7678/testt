// controllers/SystemController.h
#ifndef SYSTEMCONTROLLER_H
#define SYSTEMCONTROLLER_H

#include <winsock2.h>
#include <string>

/**
 * @brief Xu ly cac API cho tab "He thong" (Shutdown, Restart)
 */
class SystemController
{
public:
    void handlePower(SOCKET client, const std::string &body);
};

#endif // SYSTEMCONTROLLER_H