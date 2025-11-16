// utils/helpers.cpp
#include "helpers.h"
#include <fstream>
#include <array>
#include <stdexcept>
#include <shlwapi.h>
#include <cstdint> // MOI: Them vao

using namespace Gdiplus;
using namespace std;

// (Cac ham sendTcp, sendStreamFrame, jsonEscape, Base64Encode khong doi)
bool sendTcp(SOCKET s, const std::string &data)
{
    // (Xoa lock)
    if (send(s, data.c_str(), data.length(), 0) == SOCKET_ERROR)
        return false;
    if (send(s, "\n", 1, 0) == SOCKET_ERROR)
        return false;
    return true;
}
bool sendStreamFrame(SOCKET s, const std::string &data)
{
    // (Xoa lock)
    uint32_t len = data.size();
    if (send(s, (const char *)&len, 4, 0) == SOCKET_ERROR)
        return false;
    if (send(s, data.c_str(), len, 0) == SOCKET_ERROR)
        return false;
    return true;
}
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
std::string Base64Encode(const std::string &data)
{
    static const char *base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string ret;
    int i = 0, j = 0;
    unsigned char char_array_3[3], char_array_4[4];
    const unsigned char *bytes_to_encode = (const unsigned char *)data.c_str();
    int in_len = data.length();
    while (in_len--)
    {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if (i)
    {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];
        while ((i++ < 3))
            ret += '=';
    }
    return ret;
}

// --- MOI: Ham phan tich doi so (thay cho stringstream) ---
std::vector<std::string> splitArgs(const std::string &s)
{
    std::vector<std::string> result;
    std::string current_arg;
    bool in_quote = false;

    for (size_t i = 0; i < s.length(); ++i)
    {
        char c = s[i];

        if (c == '"')
        {
            in_quote = !in_quote;
            // Neu la dau " bat dau, va current_arg rong (lenh dau tien)
            if (in_quote && current_arg.empty())
                continue;

            // Neu la dau " ket thuc
            if (!in_quote)
            {
                result.push_back(current_arg);
                current_arg = "";
            }
        }
        else if (c == ' ' && !in_quote)
        {
            if (!current_arg.empty())
            {
                result.push_back(current_arg);
                current_arg = "";
            }
        }
        else
        {
            current_arg += c;
        }
    }

    // Them引 so cuoi cung
    if (!current_arg.empty())
    {
        result.push_back(current_arg);
    }

    return result;
}

// (Cac ham system: exec, getLocalIPv4Addresses, getProcessPath khong doi)
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
std::string readBinaryFile(const std::string &filename)
{
    // Mo file o che do binary va di chuyen con tro ve cuoi file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file)
        return "";

    // Lay kich thuoc file
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg); // Di chuyen con tro ve dau file

    // Tao mot string voi kich thuoc da biet va doc du lieu vao
    std::string buffer(size, '\0');
    if (file.read(&buffer[0], size))
        return buffer;

    return ""; // Tra ve rong neu doc loi
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

// (Ham GDI+ GetEncoderClsid khong doi)
int GetEncoderClsid(const WCHAR *format, CLSID *pClsid)
{
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