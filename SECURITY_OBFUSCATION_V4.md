# Security Obfuscation v4

Phạm vi của lớp v4 chỉ là tăng chi phí phân tích tĩnh và patch license/action guard; không thay đổi logic trade, FIFO, route, train hay cấu hình gameplay.

## Lớp bổ sung

- Chuỗi nhạy cảm cho network/fingerprint được mã hóa tại compile-time và chỉ giải mã tạm thời khi runtime cần dùng.
- Public license API URL được CMake chuyển thành mảng byte XOR trước khi sinh header C++; URL không còn được emit như một wide-string nguyên văn.
- Hai đường kiểm tra license độc lập (`linear` và state-machine) phải đồng thời hợp lệ trước khi action gate mở.
- Runtime canary gắn với session token + lần sync server + thời hạn sync; canary sai thì fail-closed.
- Controller/Bridge proof protocol tăng lên v3.2 và các proof constants được tách thành nhiều phần volatile, không còn một static pepper rõ ràng.
- Release build tắt RTTI (`/GR-`) nếu source không dùng RTTI và giữ nguyên LTO/CFG/CET/ASLR/NX/stack hardening hiện có.
- CI kiểm tra một số marker network/fingerprint không còn xuất hiện nguyên văn trong EXE, trong khi copyright/anti-crack notice vẫn bắt buộc tồn tại.

## Giới hạn thực tế

Đây là native obfuscation/hardening, không phải mã hóa tuyệt đối. Một binary chạy trên máy khách luôn có thể bị phân tích nếu đối phương đủ thời gian và công cụ. Server-side license, machine binding và fail-closed action enforcement vẫn là lớp bảo vệ quan trọng nhất.
