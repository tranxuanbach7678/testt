// controllers/SystemController.cpp
#include "SystemController.h"
#include "../utils/helpers.h" // Can sendFileResponse
#include <cstdlib>            // cho system()

using namespace std;

void SystemController::handlePower(SOCKET client, const string &body)
{
    string act = (body.find("shutdown") != string::npos) ? "SHUTDOWN" : "RESTART";
    if (act == "SHUTDOWN")
        system("shutdown /s /t 5");
    else
        system("shutdown /r /t 5");
    sendFileResponse(client, "{\"ok\":true}", "application/json");
}