// controllers/ScreenController.cpp
#include "ScreenController.h"
#include "../utils/helpers.h" // Can sendFileResponse, GetEncoderClsid
#include "../utils/logging.h" // Can logConsole
#include <sstream>

using namespace Gdiplus;
using namespace std;

/**
 * @brief Chup anh TOAN BO man hinh (bao gom ca man hinh phu)
 * va nen thanh JPEG trong bo nho (RAM).
 * @return Chuoi string (dang binary) chua du lieu anh JPEG.
 */
string ScreenController::captureScreenToRam()
{
    // Lay kich thuoc toan bo "man hinh ao" (virtual screen)
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // Tao mot "tam canvas" trong bo nho
    HDC scrdc = GetDC(NULL), memdc = CreateCompatibleDC(scrdc);
    HBITMAP bmp = CreateCompatibleBitmap(scrdc, w, h);
    HGDIOBJ oldbmp = SelectObject(memdc, bmp);

    // Copy diem anh tu man hinh that vao canvas trong bo nho
    BitBlt(memdc, 0, 0, w, h, scrdc, x, y, SRCCOPY | CAPTUREBLT);

    // Chuyen doi HBITMAP (cua GDI) thanh Bitmap (cua GDI+)
    Bitmap bitmap(bmp, NULL);
    CLSID clsid;
    GetEncoderClsid(L"image/jpeg", &clsid); // Lay bo ma hoa JPEG

    // Thiet lap chat luong JPEG (50%) de giam dung luong
    ULONG quality = 50;
    EncoderParameters eps;
    eps.Count = 1;
    eps.Parameter[0].Guid = EncoderQuality;
    eps.Parameter[0].Type = EncoderParameterValueTypeLong;
    eps.Parameter[0].NumberOfValues = 1;
    eps.Parameter[0].Value = &quality;

    // Luu anh vao mot luong (stream) trong RAM
    IStream *pStream = NULL;
    CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    bitmap.Save(pStream, &clsid, &eps);

    // Lay du lieu tu stream trong RAM ra bien string
    LARGE_INTEGER liZero = {};
    ULARGE_INTEGER pos = {};
    pStream->Seek(liZero, STREAM_SEEK_CUR, &pos);
    pStream->Seek(liZero, STREAM_SEEK_SET, NULL);
    string data;
    data.resize((size_t)pos.QuadPart);
    ULONG bytesRead = 0;
    pStream->Read(&data[0], (ULONG)pos.QuadPart, &bytesRead);

    // Giai phong tai nguyen
    pStream->Release();
    SelectObject(memdc, oldbmp);
    DeleteObject(bmp);
    DeleteDC(memdc);
    ReleaseDC(NULL, scrdc);

    return data;
}

// --- Public Handlers ---
void ScreenController::handleScreenshot(SOCKET client, const string &path, const string &clientId)
{
    string imgData = captureScreenToRam();
    if (!imgData.empty())
    {
        if (path.find("auto=1") == string::npos)
            logConsole(clientId, "Yeu cau chup man hinh");
        sendFileResponse(client, imgData, "image/jpeg");
    }
    else
    {
        sendFileResponse(client, "{}", "application/json");
    }
}

void ScreenController::handleScreenStream(SOCKET client, const string &clientId)
{
    // logConsole(clientId, "Yeu cau livestream man hinh (MJPEG)...");
    string boundary = "--frame";
    string header = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: multipart/x-mixed-replace; boundary=" +
                    boundary + "\r\n"
                               "Connection: keep-alive\r\n\r\n";
    sendAll(client, header);

    while (true)
    {
        string jpgData = captureScreenToRam();
        stringstream frameHeader;
        frameHeader << boundary << "\r\n"
                    << "Content-Type: image/jpeg\r\n"
                    << "Content-Length: " << jpgData.size() << "\r\n\r\n";

        if (sendAll(client, frameHeader.str()) == SOCKET_ERROR)
            break;
        if (sendAll(client, jpgData) == SOCKET_ERROR)
            break;

        Sleep(40); // 25 FPS
    }
    // logConsole(clientId, "Da dung livestream man hinh.");
}