// utils/helpers.h
#ifndef HELPERS_H
#define HELPERS_H

// Day la file header lon, bao gom hau het cac thu vien Windows
// de cac file controller khac khong can include nhieu
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <gdiplus.h>
#include <iphlpapi.h>
#include <psapi.h>
#include <string>
#include <vector>
#include <sstream>

// --- Socket Helpers ---
int sendAll(SOCKET s, const std::string &data);
void sendFileResponse(SOCKET client, const std::string &content, const std::string &contentType, int statusCode = 200);

// --- String Helpers ---
std::string jsonEscape(const std::string &s);
std::string urlDecode(const std::string &str);
// *** START: SUA LOI O DAY ***
// Them tham so thu 3 (isStringValue) giong nhu trong file .cpp
std::string getQueryParam(const std::string &path, const std::string &key, bool isStringValue = true);
// *** END: SUA LOI O DAY ***
std::string getHeader(const std::string &req, const std::string &key);

// --- System Helpers ---
std::string exec(const char *cmd);
std::vector<std::string> getLocalIPv4Addresses();
std::string getProcessPath(DWORD pid);

// --- GDI+ Helpers ---
int GetEncoderClsid(const WCHAR *format, CLSID *pClsid);

#endif // HELPERS_H