// utils/helpers.cpp
#include "helpers.h"
#include <fstream>
#include <array>
#include <stdexcept> // cho runtime_error
#include <shlwapi.h> // cho getProcessPath

// --- Socket Helpers (DA BI THIEU TRUOC DAY) ---

/**
 * @brief Dam bao gui di TOAN BO du lieu qua socket.
 * @return Tra ve SOCKET_ERROR neu that bai, hoac so byte da gui neu thanh cong.
 */
int sendAll(SOCKET s, const std::string &data)
{
    int total = 0, len = (int)data.size();
    while (total < len)
    {
        int sent = send(s, data.c_str() + total, len - total, 0);
        if (sent == SOCKET_ERROR)
            return SOCKET_ERROR; // Tra ve loi
        total += sent;
    }
    return total; // Tra ve thanh cong
}

/**
 * @brief Ham chuan de gui phan hoi HTTP (vi du: file HTML, JSON).
 */
void sendFileResponse(SOCKET client, const std::string &content, const std::string &contentType, int statusCode)
{
    std::ostringstream resp;
    const char *statusText = (statusCode == 200) ? "OK" : "Not Found"; // Co the mo rong
    resp << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n"
         << "Content-Type: " << contentType << "\r\n"
         << "Content-Length: " << content.size() << "\r\n"
         << "Connection: close\r\n\r\n";
    sendAll(client, resp.str() + content);
}

// --- String Helpers ---

std::string jsonEscape(const std::string &s)
{
    std::string out;
    for (char c : s)
    {
        if (c == '"')
            out += "\\\"";
        else if (c == '\\')
            out += "\\\\";
        else if (c == '\n')
            out += "\\n";
        else
            out += c;
    }
    return out;
}

std::string urlDecode(const std::string &str)
{
    std::string result;
    for (size_t i = 0; i < str.length(); ++i)
    {
        if (str[i] == '%')
        {
            if (i + 2 < str.length())
            {
                int value;
                std::istringstream(str.substr(i + 1, 2)) >> std::hex >> value;
                result += static_cast<char>(value);
                i += 2;
            }
        }
        else if (str[i] == '+')
        {
            result += ' ';
        }
        else
        {
            result += str[i];
        }
    }
    return result;
}

/**
 * @brief Lay mot tham so (query param) tu mot duong dan URL hoac JSON body.
 * @param isStringValue Neu la 'true', tim gia tri trong dau "". Neu 'false', tim gia tri la so.
 */
// Xoa bo phan "= true" o day (vi da co trong file .h)
std::string getQueryParam(const std::string &path, const std::string &key, bool isStringValue)
{
    // 1. Thu tim trong URL (?key=value)
    size_t keyPos = path.find(key + "=");
    if (keyPos != std::string::npos)
    {
        keyPos += key.length() + 1;
        size_t endPos = path.find('&', keyPos);
        if (endPos == std::string::npos)
        {
            return path.substr(keyPos);
        }
        return path.substr(keyPos, endPos - keyPos);
    }

    // 2. Thu tim trong JSON ("key":"value" hoac "key":value)
    std::string keyToFind = "\"" + key + "\":";
    keyPos = path.find(keyToFind);
    if (keyPos == std::string::npos)
        return "";

    keyPos += keyToFind.length();

    // Bo qua khoang trang
    while (keyPos < path.length() && isspace(path[keyPos]))
    {
        keyPos++;
    }

    size_t endPos;
    if (isStringValue)
    {
        // Tim gia tri la chuoi ("value")
        if (path[keyPos] != '"')
            return ""; // Khong phai chuoi
        keyPos++;      // Bo qua dau " mo dau
        endPos = path.find('"', keyPos);
    }
    else
    {
        // Tim gia tri la so (value)
        endPos = path.find_first_of(",}", keyPos); // Tim dau , hoac }
    }

    if (endPos == std::string::npos)
        return "";
    return path.substr(keyPos, endPos - keyPos);
}

std::string getHeader(const std::string &req, const std::string &key)
{
    size_t pos = req.find(key + ": ");
    if (pos == std::string::npos)
        return "";
    size_t end = req.find("\r\n", pos);
    return req.substr(pos + key.length() + 2, end - (pos + key.length() + 2));
}

// --- System Helpers ---

std::string exec(const char *cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    FILE *pipe = _popen(cmd, "r");
    if (!pipe)
        throw std::runtime_error("_popen() failed!");
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        result += buffer.data();
    }
    _pclose(pipe);
    return result;
}

std::vector<std::string> getLocalIPv4Addresses()
{
    std::vector<std::string> res;
    ULONG len = 15000;
    PIP_ADAPTER_ADDRESSES pAddrs = (PIP_ADAPTER_ADDRESSES)malloc(len);
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, NULL, pAddrs, &len) == NO_ERROR)
    {
        for (PIP_ADAPTER_ADDRESSES cur = pAddrs; cur; cur = cur->Next)
        {
            if (cur->OperStatus == IfOperStatusUp)
            {
                for (PIP_ADAPTER_UNICAST_ADDRESS ua = cur->FirstUnicastAddress; ua; ua = ua->Next)
                {
                    if (ua->Address.lpSockaddr->sa_family == AF_INET)
                    {
                        char buf[128];
                        inet_ntop(AF_INET, &((sockaddr_in *)ua->Address.lpSockaddr)->sin_addr, buf, sizeof(buf));
                        std::string s(buf);
                        if (s != "127.0.0.1" && !s.empty())
                            res.push_back(s);
                    }
                }
            }
        }
    }
    free(pAddrs);
    return res;
}

std::string getProcessPath(DWORD pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess)
        return "";
    char path[MAX_PATH];
    if (GetModuleFileNameExA(hProcess, NULL, path, MAX_PATH) == 0)
    {
        DWORD size = MAX_PATH;
        QueryFullProcessImageNameA(hProcess, 0, path, &size);
    }
    CloseHandle(hProcess);
    return std::string(path);
}

// --- GDI+ Helpers ---

int GetEncoderClsid(const WCHAR *format, CLSID *pClsid)
{
    using namespace Gdiplus;
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;
    ImageCodecInfo *p = (ImageCodecInfo *)(malloc(size));
    GetImageEncoders(num, size, p);
    for (UINT j = 0; j < num; ++j)
    {
        if (wcscmp(p[j].MimeType, format) == 0)
        {
            *pClsid = p[j].Clsid;
            free(p);
            return j;
        }
    }
    free(p);
    return -1;
}