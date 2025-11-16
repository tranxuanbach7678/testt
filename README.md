1. Chạy lần đầu:

- Mở cmd, vào thư mục này, nhập:
  npm install

2. Chạy bình thường:

- Chạy file "run_admin.bat" là oke.
- Có thể chạy "start.bat" để dùng phiên bản không cần giao diện (Còn chạy console là còn server)

3. Lệnh build trên VSCode:

- Vào thư mục Server.
- Nhập:
  g++ server.cpp CommandRouter.cpp controllers/_.cpp utils/_.cpp -o ../server.exe -lws2_32 -lgdiplus -lgdi32 -liphlpapi -lpsapi -lshell32 -lshlwapi -lole32 -static
