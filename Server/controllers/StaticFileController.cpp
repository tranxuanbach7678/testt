// controllers/StaticFileController.cpp
#include "StaticFileController.h"
#include "../utils/helpers.h" // Can sendFileResponse
#include "../utils/logging.h" // Can logConsole
#include <fstream>
#include <sstream>

using namespace std;

/**
 * @brief Doc noi dung mot file (dang nhi phan).
 */
string StaticFileController::readFile(const string &path)
{
    // Bao ve chong lai "directory traversal" (../)
    if (path.find("..") != string::npos)
    {
        return "";
    }

    // Su dung webroot mac dinh la "."
    string fullPath = "./" + path;
    ifstream f(fullPath, ios::binary);
    if (!f)
        return "";
    ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

/**
 * @brief Tra ve Content-Type (MIME type) dua vao duoi file.
 */
string StaticFileController::getContentType(const string &path)
{
    if (path.find(".html") != string::npos)
        return "text/html";
    if (path.find(".css") != string::npos)
        return "text/css";
    if (path.find(".js") != string::npos)
        return "application/javascript";
    if (path.find(".mp4") != string::npos)
        return "video/mp4";
    return "text/plain";
}

/**
 * @brief Xu ly yeu cau file.
 */
void StaticFileController::handleFileRequest(SOCKET client, const string &path, const string &clientId)
{
    string effectivePath = path;
    if (path == "/")
    {
        effectivePath = "/index.html";
    }

    string content = readFile(effectivePath);

    if (content.empty())
    {
        // Su dung 'clientIP'
        logConsole(clientId, "File not found: " + effectivePath);
        sendFileResponse(client, "404 File Not Found", "text/plain", 404);
    }
    else
    {
        string contentType = getContentType(effectivePath);
        sendFileResponse(client, content, contentType, 200);

        if (contentType == "video/mp4")
        {
            string fullPath = "./" + effectivePath;
            if (DeleteFileA(fullPath.c_str()))
            {
                // logConsole(clientId, "Da gui va xoa file: " + effectivePath);
            }
        }
    }
}