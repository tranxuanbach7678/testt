// controllers/StaticFileController.h
#ifndef STATICFILECONTROLLER_H
#define STATICFILECONTROLLER_H

#include <winsock2.h>
#include <string>

/**
 * @brief Xu ly viec doc va tra ve cac file tinh (HTML, CSS, JS, MP4)
 */
class StaticFileController
{
public:
    void handleFileRequest(SOCKET client, const std::string &path, const std::string &clientId);

private:
    std::string readFile(const std::string &path);
    std::string getContentType(const std::string &path);
};

#endif // STATICFILECONTROLLER_H