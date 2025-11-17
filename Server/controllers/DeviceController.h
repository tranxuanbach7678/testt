// controllers/DeviceController.h
#ifndef DEVICECONTROLLER_H
#define DEVICECONTROLLER_H

#include <winsock2.h>
#include <string>
#include <mutex>  // Them vao
#include <atomic> // Them vao

/**
 * @brief Xu ly logic cho tab "Camera" (Quet, Quay, Stream)
 */
class DeviceController
{
public:
    // Ham static de khoi tao va luu tru danh sach
    static void buildDeviceListJson();
    static std::string getDevices(bool refresh = false);

    // Ham tra ve JSON
    std::string recordVideo(const std::string &dur_str, const std::string &cam, const std::string &audio);

    // Ham stream (tu quan ly vong lap)
    void handleStreamCam(SOCKET client, const std::string &clientIP, const std::string &cam, const std::string &audio);

private:
    static std::string G_DEVICE_LIST_JSON;
    static std::mutex G_DEVICE_LIST_MUTEX; // Bảo vệ việc ghi biến static
    static std::atomic<bool> G_IS_REFRESHING;
};

#endif // DEVICECONTROLLER_H