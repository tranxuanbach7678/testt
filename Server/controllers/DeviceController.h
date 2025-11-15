// controllers/DeviceController.h
#ifndef DEVICECONTROLLER_H
#define DEVICECONTROLLER_H

#include <winsock2.h>
#include <string>

class DeviceController
{
public:
    static void buildDeviceListJson();
    void handleGetDevices(SOCKET client, const std::string &path, const std::string &clientId);  // Doi lai
    void handleRecordVideo(SOCKET client, const std::string &body, const std::string &clientId); // Doi lai
    void handleStreamCam(SOCKET client, const std::string &path, const std::string &clientId);   // Doi lai

private:
    static std::string G_DEVICE_LIST_JSON;
};

#endif // DEVICECONTROLLER_H