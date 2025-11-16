// CommandRouter.h
#ifndef COMMANDROUTER_H
#define COMMANDROUTER_H

#include <winsock2.h>
#include <string>

// Bao gom tat ca cac controller headers
// (Ngoai tru StaticFileController)
#include "controllers/AppController.h"
#include "controllers/ProcessController.h"
#include "controllers/ScreenController.h"
#include "controllers/DeviceController.h"
#include "controllers/KeylogController.h"
#include "controllers/SystemController.h"

/**
 * @brief Class "tong dai", tiep nhan ket noi TCP tho
 * va dieu phoi cac lenh (text) toi Controller.
 */
class CommandRouter
{
public:
    CommandRouter();

    /**
     * @brief Ham chinh xu ly ket noi tu Gateway (TCP).
     * Duoc goi boi server.cpp trong mot luong rieng.
     */
    void handleClient(SOCKET client, std::string clientIP);

private:
    // Router so huu cac controller
    AppController m_appController;
    ProcessController m_processController;
    ScreenController m_screenController;
    DeviceController m_deviceController;
    KeylogController m_keylogController;
    SystemController m_systemController;
};

#endif // COMMANDROUTER_H