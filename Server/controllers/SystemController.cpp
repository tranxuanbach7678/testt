// controllers/SystemController.cpp
#include "SystemController.h"
#include <cstdlib> // cho system()

using namespace std;

string SystemController::powerCommand(const string &action)
{
    if (action == "shutdown")
        system("shutdown /s /t 5");
    else if (action == "restart")
        system("shutdown /r /t 5");
    else
        return "{\"ok\":false, \"error\":\"Unknown command\"}";

    return "{\"ok\":true}";
}