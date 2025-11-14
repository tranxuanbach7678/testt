// RequestRouter.h
#ifndef REQUESTROUTER_H
#define REQUESTROUTER_H

#include <winsock2.h>
#include <string>

// Bao gom tat ca cac controller headers
#include "controllers/StaticFileController.h"
#include "controllers/AppController.h"
#include "controllers/ProcessController.h"
#include "controllers/ScreenController.h"
#include "controllers/DeviceController.h"
#include "controllers/KeylogController.h"
#include "controllers/SystemController.h"

/**
 * @brief Class "tong dai", tiep nhan client va dieu phoi
 * toi cac Controller con de xu ly.
 */
class RequestRouter
{
public:
    // Ham khoi tao
    RequestRouter();

    /**
     * @brief Ham chinh xu ly ket noi tu mot client.
     * Moi client se duoc xu ly trong mot luong (thread) rieng biet.
     */
    void handleClient(SOCKET client, std::string clientIP);

private:
    // Router se "so huu" cac instance (doi tuong) cua tat ca controller
    StaticFileController m_staticFileController;
    AppController m_appController;
    ProcessController m_processController;
    ScreenController m_screenController;
    DeviceController m_deviceController;
    KeylogController m_keylogController;
    SystemController m_systemController;
};

#endif // REQUESTROUTER_H