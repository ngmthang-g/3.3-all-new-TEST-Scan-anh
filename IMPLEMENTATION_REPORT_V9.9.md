# BÁO CÁO CHỐT LOGIC — VER 9.9 ĐẶC BIỆT

Tên sản phẩm: **Auto dồn đồ thần Long phiên bản PRO MAX by Thắng Nguyễn S2**

## 1. Phạm vi bản khách hàng

- Chức năng chính duy nhất là điều phối CON train, đưa đồ về và dồn đồ cho MAIN.
- DỒN ĐỒ luôn bật trong session, không có nút tắt. F4 và nút DỪNG ACC vẫn là cơ chế an toàn toàn cục.
- Tab AUTO và AUTO PHÓ BẢN chỉ hiện thông báo tính năng đã được cắt bỏ.
- Auto PK và logic xoay/đổi bãi đã bị loại khỏi source, CMake và bộ test.
- TELE / LOG luôn ghi báo cáo local dù chưa bật Telegram, chưa nhập bot hoặc gửi Telegram lỗi.

## 2. Vai trò

- Tối đa một MAIN.
- Tối đa CON1–CON12.
- MAIN đứng im tại TỌA GD, không train, không AutoPath tới NPC và không tự mở/đóng NPC bán.
- CON vẫn train, đầu thai, quay đúng bãi đã chọn và dùng toàn bộ đường tắt cần thiết như logic nền.

## 3. Giới hạn bốn CON và FIFO tại điểm đến

1. CON full túi chưa có số thứ tự FIFO.
2. Nếu còn slot, CON full được nhận một trong tối đa bốn slot workflow và bắt đầu đi TỌA GD.
3. Khi đã có bốn CON đang chạy/đứng chờ/giao dịch, CON full tiếp theo phải ở lại bãi.
4. Slot chỉ được giải phóng khi CON hoàn tất workflow, bị dừng/đổi role/mất cửa sổ hoặc workflow bị hủy do lỗi nghiêm trọng.
5. Chỉ khi CON đã tới đúng TỌA GD, dừng AutoPath, xuống ngựa và đứng ổn định thì mới được cấp vé FIFO.
6. Nếu nhiều CON được xác nhận tới trong cùng tick, CON có số nhỏ hơn được cấp vé trước.

## 4. Chuỗi giao dịch

- Trước mỗi pass, MAIN phải có tối thiểu 9 ô trống và cả MAIN/CON phải ở trạng thái an toàn.
- CON target lại MAIN bằng RoleID trước từng pass.
- Mỗi pass luôn chạy chuỗi click đã lưu từ dòng 1.
- Mỗi dòng giữ nguyên Delay, Repeat và Group Repeat.
- Không có dòng đặc biệt, không kiểm tra lại full túi CON giữa các pass.
- Sau khi túi MAIN ổn định:
  - Nếu MAIN còn dưới 9 ô: luôn giữ CON hiện tại, bán trước rồi target lại MAIN và chạy pass tiếp theo; delta 0–8 lúc này chưa đủ để kết luận CON hết đồ.
  - Nếu MAIN vẫn còn ít nhất 9 ô và nhận từ 0–8 ô: kết thúc CON hiện tại và giải phóng slot.
  - Nếu MAIN vẫn còn ít nhất 9 ô và nhận từ 9 ô trở lên: giữ nguyên CON và chạy pass tiếp theo.
- Snapshot X→X chỉ được chờ trong cửa sổ verify hữu hạn; hết timeout với FreeBagSpace hợp lệ thì tính nhận=0 và xử lý Finish/Capacity bình thường. Snapshot không hợp lệ quá timeout phải Abort an toàn, không được giữ transaction vô hạn.
- Chuỗi GD dùng raw macro tọa độ: vẫn qua InputSync/Bridge/license nhưng không dùng raycast, tên UI hay `_uiDragging` làm điều kiện nghiệp vụ để sang dòng tiếp theo.

## 5. MAIN bán tại chỗ và tiếp tục giao dịch dở

- UI bán phải được mở sẵn bằng tay trước khi START.
- MAIN chỉ click tọa item của Bước 5 bằng click điểm nội bộ thuần.
- Luồng mới không phụ thuộc phiên shop nội bộ, stage tab Trang bị hay giới hạn cứng 90 callback của seller cũ.
- Khi MAIN còn dưới 9 ô tại một biên thao tác an toàn, MAIN bán ngay.
- Nếu một CON đang trong workflow, CON vẫn giữ nguyên slot, vé FIFO, vị trí và số pass; không hủy giao dịch và không quay bãi, kể cả delta pass cuối chỉ là 0–8 khi MAIN đã xuống dưới 9 ô.
- Một dòng click đang chạy không bị cắt ngang. Việc bán bắt đầu sau khi pass hoàn tất và mốc túi đã được ghi nhận.
- Khi MAIN có lại ít nhất 9 ô, workflow tiếp tục với đúng CON đang giữ và target lại MAIN trước pass tiếp theo.

## 6. Số click bán thích nghi — logic đã duyệt

- Lần bán đầu của một lần START dùng Repeat tại Bước 5, mặc định 90 và có thể sửa.
- Sau **mỗi pass giao dịch hoàn tất**, tool ghi `FreeBagSpace` ổn định của MAIN.
- Các lần bán sau dùng mốc sau pass gần nhất làm số click, giới hạn tối thiểu 1 và không vượt Repeat Bước 5.
- Bán xong không cập nhật mốc học. Điểm ghi nhận duy nhất là sau mỗi pass giao dịch.
- Nếu chạy đủ số click mà MAIN vẫn dưới 9 ô, chu kỳ Bước 5 lặp lại vô hạn cho tới khi đạt điều kiện.

## 7. Báo cáo local / Telegram

- Đồng hồ session bắt đầu khi người dùng bấm START, không tính từ lúc mở EXE.
- Log local ghi START, thời lượng, chết/sống lại, CON tới FIFO, giao dịch hoàn tất, lượt bán, client freeze, WorldFlow timeout và các mốc vàng khóa.
- Báo cáo định kỳ và báo cáo cuối session vẫn được tạo trong TELE / LOG khi Telegram tắt hoặc cấu hình lỗi.
- Vàng khóa dùng mẫu đầu tiên của session làm baseline và hiển thị chênh lệch so với mốc đó.

## 8. Tiêu chí nghiệm thu tự động

- Build Windows x64 Release.
- Chạy toàn bộ logic tests còn hiệu lực.
- Verifier v9.9 kiểm tra giới hạn bốn CON, FIFO khi tới nơi, MAIN đứng im, SellPause giữ workflow, học dữ liệu sau pass, loại Auto PK/xoay bãi và báo cáo local.
- GitHub Actions đóng gói EXE, Bridge DLL, ZIP chạy và source ZIP kèm SHA-256.
