// controllers/DeviceController.h
#ifndef DEVICECONTROLLER_H
#define DEVICECONTROLLER_H

#include <winsock2.h>
#include <string>

/**
 * @brief Xu ly cac API cho tab "Camera" (Quet, Quay, Stream)
 */
class DeviceController
{
public:
    // Ham static de khoi tao va luu tru danh sach
    static void buildDeviceListJson();

    void handleGetDevices(SOCKET client, const std::string &path);
    void handleRecordVideo(SOCKET client, const std::string &body, const std::string &clientId);
    void handleStreamCam(SOCKET client, const std::string &path, const std::string &clientId);

private:
    // Bien static de luu tru danh sach
    static std::string G_DEVICE_LIST_JSON;
};

#endif // DEVICECONTROLLER_H