# License/UI v2 — v9.9 ĐẶC BIỆT

- Ẩn LOG/BỘ ĐIỀU PHỐI, dòng điều phối, mô tả MAIN/FIFO/batch, F8/InputSync và BotToken/DPAPI khỏi UI khách hàng; runtime/log bên dưới giữ nguyên.
- Title tool hiển thị thời gian key còn lại theo `remainingSeconds` server.
- Fingerprint PC v2: MachineGuid, system volume, CPU, system product/model, baseboard, BIOS, firmware; chỉ truyền/lưu SHA-256.
- Giữ legacy fingerprint để nâng key cũ đúng PC sang v2.
- PC khác dùng key đã bind: backend tự block ngay với `machine_mismatch`.
- Thêm key 30 ngày.
- Web Admin hiển thị fingerprint/block reason và cho xóa key đã bị chặn.
- Production Supabase đã migrate v2, Edge Function v2 ACTIVE và smoke-test mismatch bằng transaction rollback.
