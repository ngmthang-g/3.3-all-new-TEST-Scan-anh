# Changelog

## 9.9 ĐẶC BIỆT

- Đổi tên sản phẩm thành **Auto dồn đồ thần Long phiên bản PRO MAX by Thắng Nguyễn S2**.
- Tách admission tối đa 4 CON khỏi FIFO; vé chỉ được cấp khi CON tới đúng TỌA GD.
- MAIN đứng im, bỏ toàn bộ route/mở NPC/đóng shop của seller cũ và chỉ click Bước 5.
- Thêm SellPause giữ nguyên CON/vé/pass khi MAIN dưới 9 ô; không hủy giao dịch dở.
- Ưu tiên kiểm tra sức chứa sau pass trước quy tắc delta: MAIN dưới 9 ô luôn giữ CON và bán; chỉ dùng delta 0–8 để kết thúc khi MAIN vẫn còn ít nhất 9 ô.
- Chuyển điểm học số click bán sang FreeBagSpace ổn định sau từng pass giao dịch.
- Loại Auto PK, logic xoay bãi và các test/source liên quan.
- Tab AUTO và AUTO PHÓ BẢN chỉ hiện popup tính năng đã cắt bỏ.
- Mở TELE / LOG và ghi báo cáo local kể cả khi Telegram tắt hoặc lỗi.

## 3.5

- Chuyển dòng trạng thái `ĐIỀU PHỐI` khỏi hàng tab xuống cuối cửa sổ; chế độ thu gọn cũng giữ trạng thái ở hàng cuối.
- Mở thanh tab rộng 870 px, ép 5 tab proxy có cùng độ rộng và dùng chữ Segoe UI semibold lớn hơn để không còn nút cuộn/cắt chữ.
- Nâng hardening bản Release MSVC: LTCG, CFG, CET compatibility, ASLR high-entropy, NX, tối ưu loại code/data thừa, gộp hàm trùng và build tái lập.
- Nhúng cảnh báo bản quyền tiếng Anh dưới dạng `RCDATA` trong EXE và thêm metadata `LegalCopyright`.
- Giữ nguyên license Supabase fail-closed; không đưa secret quản trị vào EXE.

## 3.3

- Xóa sạch `CHUYỂN ĐỒ kind=1` khỏi data model, runtime trade, INI trade sequence, portable config, editor UI, validation và test.
- Macro giao dịch chỉ còn MAIN/CON + tọa độ + Delay + Repeat + Nhóm lặp; mỗi pass luôn chạy đủ từ index 0.
- Giữ nguyên `ReceivedSlots()` và `DecidePass()` kiểu v1.4: delta `<=8` kết thúc CON, delta `>8` giữ cùng CON.
- Tăng ổn định snapshot MAIN `600 → 1500 ms`; verify max `2200 → 3200 ms`.
- MAIN chỉ được mở pass khi `FreeBagSpace >=9`; khi `<9`, nhả MAIN cho Auto Sell mà không xóa các CON khác đang chờ FIFO.
- Nếu pass vừa nhận `>=9` item nhưng MAIN còn `<9`, active CON về bãi train, các CON FIFO khác tiếp tục đứng đợi.
- Xóa `MainSellThreshold`; khi Dồn đồ bật, Auto Sell MAIN dùng cùng gate `<9` ô.
- Nâng `TLCLICKCFG` lên v2 và bỏ cột `kind` khỏi dòng CHILD.


## 3.2

- Xóa sạch cơ chế pre-click/hợp đồng đặc biệt của dòng đầu trong chuỗi Dồn đồ.
- Luồng mới là TỌA GD → auto-target/xác minh MAIN → chạy toàn bộ macro động từ index 0.
- Xóa/chèn/di chuyển dòng không còn làm FSM ép hoặc bỏ qua dòng đầu mới.
- Quy tắc lặp dùng delta FreeBagSpace bất kỳ: giảm `>=9` thì giữ CON và lặp khi MAIN còn `>=9` ô; giảm `1..8` mới là pass cuối.
- Delta lớn hơn 9 không còn bị coi là snapshot lỗi; snapshot một phần dưới 9 phải ổn định lâu hơn trước khi kết thúc CON.

## 3.1

- Sửa lỗi pass Dồn đồ thứ hai bị bỏ dòng #1 trong chuỗi autoclick.
- Khi MAIN FreeBagSpace đổi `40 → 31`, CON được xác định đã giao đủ 9 item và chạy lại toàn bộ chuỗi từ dòng #1 nếu MAIN vẫn còn ít nhất 9 ô trống.
- Giữ dòng #1 ở lần khởi động ban đầu theo cơ chế hiện có; chỉ thay đổi điểm bắt đầu của pass lặp.


## 3.0

- Thống nhất tên sản phẩm: **AUTO Thần Long đa tính năng Pro**.
- Rời Côn Lôn Sơn chuyển hoàn toàn sang 3 `TryClickUI` do người dùng F8: mở NPC, Đại Lý, Xác nhận.
- Mỗi click có Time/Delay riêng; chỉ kiểm tra MapID sau click thứ ba.
- Xóa riêng resolver/callback Đại Lý `Tag=200001`; các callback semantic khác giữ nguyên.
- Thêm route Tần Hoàng Địa Cung tuần tự M10000 → M10014 ↔ M10015 ↔ M10016 ↔ M10017.
- Khóa từng cổng vào đúng map nguồn; M10015 và M10016 mỗi map có hai cổng lên/xuống riêng.
- Chỉ xác nhận popup THĐC sau khi đã vào và ổn định ở M10014.
- Thêm 7 tọa mặc định có thể sửa/lấy lại trong Tùy chỉnh.
- Nâng XUẤT/NHẬP TẤT CẢ lên TLMASTERCFG v2 để chứa 7 cổng và 3 click + Time/Delay.

## 1.7

- Sửa đúng nhánh Lâu Lan → Hỏa Diệm: sau `Đến các môn phái → Tinh Túc`, game tự sang Tinh Túc Hải M12; FSM bỏ hoàn toàn callback/chờ popup Xác nhận và chuyển thẳng sang kiểm tra MapID.
- Tách rõ hai NPC Côn Lôn: NPC M75 dùng để **rời Côn Lôn → Đại Lý**, còn Xa Truyền Bình ID387 ở M5 dùng để **đi vào Côn Lôn**.
- Nhánh rời Côn Lôn có resolver riêng theo identity đã chạy thành công ở v0.1.8: ACTIVE Title `Xa Truyền…` + `UIButton` + `Tag/selectionID=200001`; ưu tiên parent `GameDialog/ButtonList`, vẫn fail-closed nếu Tag không duy nhất.
- Mở rộng vai trò giao dịch từ CON1-CON6 thành CON1-CON12; toàn bộ validation, migration và scheduler đều nhận đủ 12 slot.
- Đồng bộ cơ chế v0.1.8 vào đường tắt NPC: bỏ giả định byte `disposed` tại offset cố định, chụp ACTIVE UI baseline trước callback và xác nhận bằng UI-delta + semantic.
- Ngải Ni Ngoã Nhĩ chọn tuần tự `Đến các môn phái` rồi `Tinh Túc` trước popup xác nhận.
- M10000 tự arm cổng liên-server dù checkbox ĐƯỜNG TẮT tắt; route xa chờ 5 giây để quan sát AutoPath/movement proof, không còn reset StartPath liên tục mỗi 1.5 giây.
- Mọi StartPath bắt buộc snapshot `IsRiding=1`; loại bỏ hoàn toàn nhánh chạy bộ AutoPath sau khi lên ngựa thất bại.
- Auto Train khởi động ưu tiên `HandleShortcutTravel` trước route thường, đồng bộ với đường quay về sau Auto Sell/Recovery.
- Shortcut waypoint tái sử dụng FSM lên ngựa chuẩn; bỏ nhánh StartPath chạy bộ trực tiếp sau khi tắt AutoFight.
- Xác nhận chuyển map của NPC/cổng dùng resolver `ConfirmTravelSemantic` theo donor v0.1.8; `Đồng ý` không bị hiểu nhầm thành `Đóng`.
- Callback `Đại Lý / Côn Lôn Sơn / Đến các môn phái / Tinh Túc` quét toàn bộ ACTIVE UIObject có `HandleClickEvent`, exact semantic, fail-closed và log candidate khi không khớp.
- Sau khi mở NPC, controller chờ 1000 ms trước lần quét đầu và giữa các lần thử; bỏ `NPC REOPEN` để không reset GameDialog đang dựng. Dòng semantic được chờ tối đa 12 lần.
- Sau callback điểm đến, controller chờ/poll popup Xác nhận theo nhịp 1000 ms, tối đa 20 lần; giữ đúng nguyên tắc v0.1.8 là mỗi lần quét là một request riêng ngoài game UI thread.
- Scanner bổ sung fallback đọc `Text` từ mọi node con UI và cả `Children`/`CoreChildren`, giúp nhận dòng `Đại Lý` khi chữ nằm trên UIText con của UIButton.
- Xác nhận static DATA: Xa Truyền Bình `387` và Ngải Ni Ngoã Nhĩ `913` đều thuộc Lâu Lan M5; giữ nguyên ResID.
- Đồng bộ lại callback NPC theo probe v0.1.8: đọc `Tag/selectionID`, fallback getter động (`get_Item/GetValue/Get/RawGet`), dùng `ManagedThis` và không chặn callback bởi `SafeForAction` khi popup đang chuyển map.
- Tăng nhịp chờ sau mở NPC, sau chọn dòng và trước Xác nhận lên 2000 ms cho cả Đại Lý, Tinh Túc và menu trung gian; timeout semantic riêng là 7 giây, confirm là 5 giây.
- Xa Truyền Bình dùng duy nhất tọa NPC bán ngoài màn hình chính; shortcut `xaTruyenX/Y` cũ trở thành legacy/inert.
- Bỏ phần điều khiển `NPC BÁN / LẤY TỌA NPC BÁN` trùng trong cửa sổ TÙY CHỈNH; màn hình chính là nguồn tọa bán duy nhất.
- M10000 giữ mapping 10005/10004/10007 tương ứng Thanh Liên/Phàm Liên/Khô Vinh và chờ MapID + MapReady sau confirm semantic.

## 1.6

- Đổi tên hiển thị thành `AUTO Thần Long đa tính năng Pro v1.6`.
- Sửa trạng thái StartPath: chỉ báo AutoPath thành công khi Bridge thực sự PASS; lỗi Bridge không còn tạo trạng thái/cooldown giả như đã gửi đường đi.
- Thêm log tọa độ runtime `CURRENT/TARGET/DX/DY/DISTANCE/TOL` khi StartPath PASS và log raw coordinate khi LẤY TỌA.
- Giữ nguyên domain/scale MapID/X/Y hiện tại; không thêm phép nhân/chia tọa độ chưa có runtime proof.
- Tách tolerance theo mục đích: Auto Train/xoay bãi giữ tolerance profile; TỌA GD, NPC bán và NPC trị liệu dùng tolerance `20`.
- Waypoint đường tắt cùng map gửi StartPath thật và phải verify AutoPath runtime; retry có giới hạn, fail-closed nếu client không nhận waypoint.
- M10000 -> Thanh Liên Trại / Phàm Liên Trại / Khô Vinh Đạo dùng portal state riêng: không StopPath sớm theo tolerance 120, xử lý tới sát cổng hoặc stall khoảng 3 giây, ConfirmMap rồi chờ MapID + MapReady trước khi tiếp tục bãi train.
- Không thay đổi F8/click UI, trade macro hoặc các subsystem ngoài phạm vi travel/tọa độ đã duyệt.
- GitHub Actions build Windows x64 đã chạy verifier source và 12 nhóm logic test; `dist/` trên `main` được publish trực tiếp từ source hiện tại.

## 1.5.4

- Fix Auto Sell vendor-entry callbacks by ResID: Mã Kiêu Minh (373) uses `Mua thú cưỡi`; Dược Đại Phu (279) and Ba Nhĩ (328) use `Mua thuốc`; direct SellTab remains only a fallback if NPCShop is already open.
- Add one CON background pre-click at trade-row #1 coordinate after both accounts arrive at TỌA GD and before target/verify MAIN RoleID; the normal trade macro then still starts at row #1.
- Add portable XUẤT MAP / NHẬP MAP (`.tlmap`) for shared saved train-map presets.
- Trade-sequence `+ THÊM` now inserts after the selected row; MAIN insert repairs CON references.
- No other workflow intentionally changed.

## 1.5.3

- Add portable export/import click configuration (`.tlcfg`).
- Reduce Target MAIN stutter by verify-first SelectTarget and 500 ms retry backoff.

## 1.5.2 CLEAN

- Removed the selected-player face/header interaction path and all popup/Trade-button callback code from the tool.
- Production item consolidation targets MAIN by RoleID after both accounts are verified at the trade coordinate, then starts the coordinate trade macro.
- Removed the TEST GD UI/state machine.
- Removed legacy Confirm Map and Revive F8 capture/test rows; production semantic callbacks remain unchanged.
- Added verified sell-NPC presets: Ba Nhĩ 328, Xa Truyền Bình 387, Uông Diên 159, A Lạp Bá Nhân 275; preserved Dược Đại Phu 279 and existing Mã Kiêu Minh 373.
- Added compact main-window mode.
