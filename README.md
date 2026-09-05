# Auto dồn đồ thần Long phiên bản PRO MAX by Thắng Nguyễn S2

Phiên bản: **9.9 ĐẶC BIỆT**

Bản khách hàng tập trung vào một workflow duy nhất: CON1–CON12 train và đưa đồ về cho MAIN; MAIN đứng im tại TỌA GD, bán tại chỗ bằng Bước 5 và thực hiện chuỗi giao dịch.

## Điểm chốt của v9.9

- DỒN ĐỒ bắt buộc bật trong session.
- Tối đa bốn CON full được rời bãi cùng lúc.
- CON chỉ nhận vé FIFO sau khi đã tới đúng TỌA GD.
- MAIN không train, không đi NPC và không mở/đóng shop.
- MAIN dưới 9 ô bán ngay ở biên an toàn; kể cả pass chỉ nhận 0–8 món, CON đang giao dịch dở vẫn được giữ nguyên vì MAIN đầy làm kết quả đó chưa đủ kết luận CON hết đồ.
- Lần bán đầu dùng Repeat Bước 5 (mặc định 90).
- Sau mỗi pass, FreeBagSpace MAIN được ghi làm dữ liệu số click bán tiếp theo.
- Auto PK và xoay/đổi bãi đã bị loại bỏ.
- TELE / LOG ghi báo cáo local dù Telegram không được cấu hình.

Chi tiết nghiệm thu: [IMPLEMENTATION_REPORT_V9.9.md](IMPLEMENTATION_REPORT_V9.9.md).

## Thiết lập nhanh

1. Gán đúng một MAIN và tối đa CON1–CON12.
2. Chọn bãi train riêng cho từng CON.
3. Lưu TỌA GD tại vị trí MAIN sẽ đứng.
4. Mở sẵn UI bán trên MAIN.
5. Trong **BƯỚC 5 BÁN**, lấy tọa độ item và đặt Repeat lần đầu (mặc định 90).
6. Cấu hình chuỗi GD dùng chung cho CON.
7. Tick các tài khoản và bấm START.

MAIN không cần bãi train. Các CON vẫn cần bãi train và có thể dùng hệ thống đường tắt.

## Build

Yêu cầu Windows x64, Visual Studio 2022 và CMake 3.24+.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
```

File chạy cần đặt cạnh Bridge DLL:

- `Auto_don_do_Than_Long_PRO_MAX_v9.9_DAC_BIET.exe`
- `ThanLongCleanRouteBridge.dll`

GitHub Actions tạo sẵn gói `Auto-don-do-Than-Long-PRO-MAX-v9.9-DAC-BIET-win-x64.zip` trong thư mục `dist/`.
