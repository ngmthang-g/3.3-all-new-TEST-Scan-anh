# AUTO Thần Long PRO MAX v9.9 — Supabase License v2 status

## Nguồn chuẩn

Source chuẩn là repository khách hàng `ngmthang-g/Du-an-AUTO-cho-kHach-`. Không copy ngược `controller.cpp`/`bridge.cpp` từ source cũ; lớp license chỉ được ghép vào lõi AUTO hiện tại.

## Production đã live

Supabase project `AUTO-Than-Long-Pro` (`dklpxxxgapbnjixvbzai`) đang ACTIVE.

Đã thực hiện trên production:

- schema license gốc v3.4;
- migration security v2;
- key **30 ngày**;
- RPC `license_validate_v2` / `license_heartbeat_v2`;
- Edge Function `license-api` v2 ACTIVE;
- machine fingerprint v2 nhiều thành phần, chỉ lưu hash;
- compatibility legacy fingerprint để nâng key cũ đúng PC;
- mismatch PC khác → tự `blocked` ngay ở validate/heartbeat;
- lưu `blocked_reason`, fingerprint/machine lạ để admin tra cứu;
- route xóa key chỉ cho phép khi key đang `blocked`;
- thu hồi EXECUTE public khỏi helper SECURITY DEFINER `rls_auto_enable()` cũ.

Smoke-test production đã chạy trong transaction rollback: PC A kích hoạt → PC B fingerprint khác → key chuyển `blocked/machine_mismatch`; sau rollback DB còn 0 key test.

## Tool/EXE v2

License launcher v2:

- dùng nhiều nguồn máy: MachineGuid, system volume, CPU, system product/model, baseboard, BIOS, firmware;
- SHA-256 từng thành phần trước khi gửi/lưu;
- gửi fingerprint tổng v2 + legacy fingerprint;
- nhận `remainingSeconds` từ server;
- export thời gian còn lại cho controller để title tool đếm ngược theo ngày/giờ/phút;
- heartbeat tiếp tục fail-closed khi key block/hết hạn/mismatch.

## UI tool

Theo yêu cầu khách hàng, các nội dung kỹ thuật sau được ẩn khỏi giao diện nhưng runtime bên dưới vẫn hoạt động:

- `LOG / BỘ ĐIỀU PHỐI` và vùng log;
- dòng `ĐIỀU PHỐI`;
- dòng mô tả MAIN/FIFO/batch;
- dòng mô tả 3 điểm F8/InputSync/callback;
- dòng mô tả BotTokenProtected/Windows DPAPI.

Title tool hiển thị thời gian key còn lại, ví dụ `KEY: Còn 29 ngày 22 giờ`.

## Web Admin v2

Source Web Admin hiện hỗ trợ:

- key 3 / 15 / 30 / 60 / 90 / 365 ngày;
- allow/block/note/report;
- xem fingerprint v2 dạng hash;
- xem nguyên nhân tự chặn do PC khác;
- xóa vĩnh viễn **key đã bị chặn**; backend từ chối xóa key còn `allowed`.

Web public config đã trỏ đúng Supabase production bằng publishable key; không có service-role/backend secret trong frontend.

## Security advisor

Sau migration hardening, cảnh báo EXECUTE public của `rls_auto_enable()` đã hết. Advisor còn:

- INFO: `license_sessions` bật RLS và không có browser policy — phù hợp vì bảng chỉ dùng qua backend/service role;
- WARN: Supabase Auth leaked-password protection chưa bật — đây là Auth project setting, không phải secret/source EXE.

## Điều còn phụ thuộc hạ tầng ngoài source

Static Web Admin cần được publish bằng hosting đã chọn (Firebase Hosting/GitHub Pages/Cloudflare Pages/Netlify/Vercel). Source repo chứa `supabase/firebase-hosting.json`, nhưng việc deploy Firebase cần phiên đăng nhập/token/project hosting ngoài GitHub source.

GitHub build cần Repository Variable `TL_LICENSE_API_URL` trỏ tới:

`https://dklpxxxgapbnjixvbzai.supabase.co/functions/v1/license-api`
