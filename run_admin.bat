@echo off
TITLE Admin Panel Launcher

ECHO.
ECHO =================================================
ECHO  KHOI DONG ADMIN PANEL SERVER (CONG 8000)
ECHO =================================================
ECHO.
ECHO Mo mot cua so console moi de chay server...
:: 'start "Tieu de"' la de mo cua so moi
start "Admin Panel Server (Node.js)" node admin-panel.js

ECHO.
ECHO Dang cho server khoi dong (2 giay)...
:: Cho 2 giay de server Node.js khoi dong xong
timeout /t 2 /nobreak > nul

ECHO.
ECHO Mo giao dien Admin Panel tren trinh duyet...
:: Mo URL trong trinh duyet mac dinh
start http://localhost:8000

ECHO.
ECHO Hoan tat. Cua so nay se tu dong dong.
timeout /t 2 /nobreak > nul
exit