// controllers/SystemController.h
#ifndef SYSTEMCONTROLLER_H
#define SYSTEMCONTROLLER_H

#include <string>

/**
 * @brief Xu ly logic cho tab "He thong" (Shutdown, Restart)
 */
class SystemController
{
public:
    std::string powerCommand(const std::string &action);
};

#endif // SYSTEMCONTROLLER_H