// controllers/ScreenController.cpp
#include "ScreenController.h"
#include "../utils/helpers.h" // Can sendStreamFrame, GetEncoderClsid
#include "../utils/logging.h" // Can logConsole
#include <sstream>
#include <winsock2.h>

using namespace Gdiplus;
using namespace std;

// ham captureScreenToRam() KHONG THAY DOI
string ScreenController::captureScreenToRam()
{
    // ... (Toan bo code chup man hinh va ve chuot)
    // ... (giong het code cu)
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    HDC scrdc = GetDC(NULL), memdc = CreateCompatibleDC(scrdc);
    HBITMAP bmp = CreateCompatibleBitmap(scrdc, w, h);
    HGDIOBJ oldbmp = SelectObject(memdc, bmp);
    BitBlt(memdc, 0, 0, w, h, scrdc, x, y, SRCCOPY | CAPTUREBLT);
    CURSORINFO cursorInfo = {0};
    cursorInfo.cbSize = sizeof(CURSORINFO);
    if (GetCursorInfo(&cursorInfo))
    {
        if (cursorInfo.flags == CURSOR_SHOWING)
        {
            int memCursorX = cursorInfo.ptScreenPos.x - x;
            int memCursorY = cursorInfo.ptScreenPos.y - y;
            DrawIcon(memdc, memCursorX, memCursorY, cursorInfo.hCursor);
        }
    }
    Bitmap bitmap(bmp, NULL);
    CLSID clsid;
    GetEncoderClsid(L"image/jpeg", &clsid);
    ULONG quality = 50;
    EncoderParameters eps;
    eps.Count = 1;
    eps.Parameter[0].Guid = EncoderQuality;
    eps.Parameter[0].Type = EncoderParameterValueTypeLong;
    eps.Parameter[0].NumberOfValues = 1;
    eps.Parameter[0].Value = &quality;
    IStream *pStream = NULL;
    CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    bitmap.Save(pStream, &clsid, &eps);
    LARGE_INTEGER liZero = {};
    ULARGE_INTEGER pos = {};
    pStream->Seek(liZero, STREAM_SEEK_CUR, &pos);
    pStream->Seek(liZero, STREAM_SEEK_SET, NULL);
    string data;
    data.resize((size_t)pos.QuadPart);
    ULONG bytesRead = 0;
    pStream->Read(&data[0], (ULONG)pos.QuadPart, &bytesRead);
    pStream->Release();
    SelectObject(memdc, oldbmp);
    DeleteObject(bmp);
    DeleteDC(memdc);
    ReleaseDC(NULL, scrdc);
    return data;
}

// --- Public Handlers (DA THAY DOI) ---

/**
 * @brief Tra ve 1 khung hinh screenshot (da ma hoa Base64)
 */
std::string ScreenController::getScreenshotBase64()
{
    string jpgData = captureScreenToRam();
    string base64Data = Base64Encode(jpgData);

    // --- SUA LOI: Khong them 'command' ---
    return "{\"payload\":\"" + base64Data + "\"}";
}

/**
 * @brief Xu ly vong lap stream man hinh
 */
void ScreenController::handleScreenStream(SOCKET client, const string &clientIP)
{
    logConsole(clientIP, "Bat dau stream man hinh.");

    if (!sendTcp(client, "STREAM_START"))
    {
        logConsole(clientIP, "Gateway ngat ket noi truoc khi stream.");
        return;
    }

    while (true)
    {
        // === THEM LAI: Kiem tra lenh moi ===
        u_long bytes_available = 0;
        ioctlsocket(client, FIONREAD, &bytes_available);
        if (bytes_available > 0)
        {
            logConsole(clientIP, "Nhan duoc lenh moi -> Dung stream.");
            break; // Thoat vong lap
        }
        // === KET THUC THEM ===

        string jpgData = captureScreenToRam();
        if (!sendStreamFrame(client, jpgData))
        {
            break; // Loi, client ngat ket noi
        }
        Sleep(3);
    }
    logConsole(clientIP, "Da dung stream man hinh.");
}