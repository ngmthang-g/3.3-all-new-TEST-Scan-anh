# Supabase License Backend — AUTO Thần Long PRO MAX v9.9

## Trạng thái production hiện tại

Project Supabase đang dùng: `AUTO-Than-Long-Pro` (`dklpxxxgapbnjixvbzai`).

- Project URL: `https://dklpxxxgapbnjixvbzai.supabase.co`
- Edge Function: `license-api`
- Function URL: `https://dklpxxxgapbnjixvbzai.supabase.co/functions/v1/license-api`
- `license-api` production đã nâng lên contract v2.
- Migration license v2 đã áp dụng trên production.
- Web public config chỉ chứa Project URL + publishable key + Function URL; không chứa service-role/backend secret.

## Nguyên tắc bảo mật

- EXE chỉ biết URL HTTPS công khai của `license-api`.
- KHÔNG đưa `sb_secret_*`, `service_role`, database password hoặc khóa có quyền cấp/sửa license vào EXE/Web.
- Web chỉ chứa Project URL + publishable key công khai.
- Các bảng license bật RLS; browser/EXE không được quyền sửa trực tiếp.
- Chỉ Edge Function dùng backend secret để truy cập dữ liệu license.
- Route public `/validate` và `/heartbeat` tự kiểm tra key + machine fingerprint phía server.
- Route `/admin/*` tự xác minh Supabase Auth JWT và bắt buộc user có `license_admins.enabled=true`.
- Các fingerprint thành phần được SHA-256 ở client trước khi gửi/lưu; DB không cần lưu serial phần cứng thô.

## License v2

Loại key hợp lệ: **3 / 15 / 30 / 60 / 90 / 365 ngày**.

Hạn bắt đầu từ lần kích hoạt hợp lệ đầu tiên. Tool nhận `remainingSeconds` từ server và hiển thị đếm ngược trên title, ví dụ `KEY: Còn 29 ngày 22 giờ`. Heartbeat định kỳ đồng bộ lại thời gian còn lại.

Machine fingerprint v2 kết hợp nhiều nguồn ổn định hơn một trường HWID đơn:

- MachineGuid;
- system volume;
- CPU;
- system product/model;
- baseboard;
- BIOS;
- firmware.

Mỗi thành phần được băm SHA-256, sau đó tạo fingerprint tổng. EXE vẫn gửi thêm fingerprint legacy v1 để key đang kích hoạt đúng PC có thể nâng từ client cũ sang v2 mà không tự khóa nhầm.

Nếu key đã bind mà `/validate` hoặc `/heartbeat` nhận fingerprint của PC khác, server fail-closed và tự chuyển key sang `blocked` với `blocked_reason=machine_mismatch`; đồng thời lưu fingerprint/machine name của máy lạ để Web Admin hiển thị nguyên nhân.

## Web Admin v2

Web Admin hỗ trợ:

- tạo key 3 / 15 / 30 / 60 / 90 / 365 ngày;
- cho phép/chặn key;
- ghi chú;
- xem ngày kích hoạt, hạn, last-seen, số lần mở và thời gian sử dụng;
- xem fingerprint v2 dạng hash;
- hiển thị `Tự chặn: phát hiện PC khác` khi backend phát hiện mismatch;
- **xóa vĩnh viễn key đã bị chặn**. Backend từ chối xóa key vẫn đang `allowed`.

## Database migrations

Schema gốc được giữ nguyên để tương thích. Các migration bổ sung của v2:

- `migrations/20260828_license_security_v2.sql`
- `migrations/20260828_harden_rls_helper_execute.sql`

Migration v2 thêm 30 ngày, machine profile/hash v2, legacy hash, thông tin auto-block/mismatch và RPC `license_validate_v2` + `license_heartbeat_v2`. Migration hardening thu hồi quyền public execute khỏi helper `rls_auto_enable()` cũ.

## Edge Function

Deploy `functions/license-api`. `config.toml` giữ `verify_jwt=false` vì EXE gọi `/validate` và `/heartbeat` không có user JWT; các route admin vẫn bắt buộc Auth JWT và kiểm tra bảng `license_admins` trong function.

## Web Hosting

Thư mục static là `supabase/web/`. Có thể deploy bằng Firebase Hosting với `supabase/firebase-hosting.json`. Hosting chỉ phục vụ HTML/JS/CSS tĩnh; toàn bộ dữ liệu và quyền admin vẫn nằm ở Supabase.

## GitHub build

Repository cần Variable:

`TL_LICENSE_API_URL=https://dklpxxxgapbnjixvbzai.supabase.co/functions/v1/license-api`

Workflow Windows x64 chạy contract verifier, build, logic tests, PE hardening và đóng gói source/EXE. Không nhét backend secret vào artifact.

## Kiểm tra production đã thực hiện

Đã smoke-test RPC v2 trong transaction có `ROLLBACK`: PC A kích hoạt key test, sau đó PC B dùng fingerprint khác làm key chuyển sang `blocked/machine_mismatch`; transaction rollback và xác nhận không còn key test trong production.
