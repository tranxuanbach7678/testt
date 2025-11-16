@echo off
ECHO Khoi dong C++ Logic Server (Cong 9000)...
:: 'start "Tieu de Cua So"' la lenh de mo mot cua so moi
start "C++ Logic Server" server.exe

ECHO Khoi dong Node.js Gateway (Cong 8080)...
:: Cho 1 giay de C++ server khoi dong xong
timeout /t 1 /nobreak > nul

:: Chay Node.js trong mot cua so moi
start "Node.js Gateway" node gateway.js

ECHO Da khoi dong ca hai server.