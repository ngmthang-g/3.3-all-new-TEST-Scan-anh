## v4.9 — Unified AUTO core + simplified dungeon scheduler

- AUTO PHÓ BẢN không còn dùng DungeonAllStable như một engine trạng thái thứ hai; client/state/Đầu Thai/AutoPath/AutoFight tiếp tục do Account CORE AUTO quản lý.
- Team dùng RoleID làm identity bền vững; PID chỉ là cache runtime và được refresh không phá STEP.
- PRECHECK/GATHER/WAIT ENTER/WAIT EXIT/STEP thiếu state tạm thời đều WAIT thay vì tự STOP do rào riêng của Dungeon.
- FIGHT-CLEAR scan toàn bộ GMonster VERIFIED mà scanner trả về; bỏ Name/ResID/Group/Radius/X,Y khỏi quyết định PASS. GMonster X,Y=? nhưng HP còn vẫn chặn STEP.
- Scan fail, HP chưa chắc chắn hoặc scanner truncated đều WAIT; chỉ scan thành công và mọi GMonster VERIFIED sống = 0 mới PASS.
- ReadDungeonProgress/TASK không còn chạy trong hot FIGHT loop; bảng TASK/Monster chỉ là chẩn đoán thủ công.
- Giữ nguyên toàn bộ route/STEP/Parallel/Queue đã duyệt từ v4.7-v4.8.

## v4.8 — Stable core restoration + approved v4.7 routes

- Phó Bản dùng cùng ReadState freeze/recovery với Tab AUTO; một nhịp bridge lỗi không còn xóa KEY/PID.
- Rebind theo RoleID giữ nguyên Slot 1..6 và PID cũ khi state tạm thời chưa đọc được; RUN/STEP chuyển WAIT thay vì STOP.
- Sửa Life Guard: dungeonOwned được Đầu Thai bằng cùng Priority Revive core dù runtime.running=false.
- AutoPath của Phó Bản giữ nguyên thú cưỡi tại đích; không phát Dismount cho dungeon-owned account.
- Sửa STEP selection: STOP giữ lựa chọn người dùng để BẮT ĐẦU TỪ STEP ĐÃ CHỌN hoạt động.
- Khôi phục Activity LIVE reader/controller behavior và TickDungeonFight từ v4.6 ổn định; giữ nguyên route/Parallel v4.7.
- Giữ toàn bộ STEP đã duyệt: Hoàng Kim 9394, Huyền Phật 5881,2901, Trúc Lâm MOVE-only, Dã Ngoại final clear, Dung Nham 27 điểm.

# v4.7 — approved DOCX dungeon routes and runtime barriers

- Hoàng Kim Chi Liên: approved `9394,3009`; multi-waypoint parallel lanes for Slot 1-2 vs 3-6.
- Huyền Phật Châu: approved `5881,2901`.
- Trúc Lâm: `3279,2774` is MOVE ONLY.
- Dã Ngoại Trại Phỉ: ALL -> `2039,2099` -> AutoFight -> verified GMonster area-clear.
- Dung Nham Chi Địa: full 27-point move/clear route.
- Ác Tặc Tạo Phản: STEP coordinates are present as a non-startable placeholder; NPC/entry/map remain intentionally blank.
- WAIT ENTER / WAIT EXIT no longer auto-STOP on time alone.
- Dungeon death is a local revive/resume pause and does not consume STEP timeout.
- Activity board filters Unity property rows and keeps the last good LIVE counters as CACHED.
- Telegram per-run completion names the actual next run/dungeon.
- Parallel Group now supports sequential waypoints inside each participant lane.

## v4.6 — Parallel Stage + START từ STEP + Activity/Monster UX

- Sửa Activity Board: không còn tách mất dấu `/` trong tỷ lệ `current/target`; parser chấp nhận dòng mục tiêu có chữ dù client không hiện đúng một nhóm động từ hardcode.
- Slot tổ đội được chuẩn hóa bền vững: **Slot 1 luôn là KEY**; khi tạo/sửa/nạp đội, RoleID/PID của KEY được đưa lên đầu. UI ghi rõ tick = thêm, bỏ tick = xóa và hiển thị Slot 2–6.
- Thêm **BẮT ĐẦU TỪ STEP ĐÃ CHỌN**. Nếu toàn đội đã ở đúng DungeonMap thì chạy trực tiếp; nếu chưa ở trong PB vẫn đi qua entry pipeline rồi giữ đúng STEP đã chọn. Chọn giữa một Parallel Group tự chuẩn hóa về đầu group.
- Thêm `Parallel Group`: các STEP TỌA ĐỘ liên tiếp cùng P# được dispatch đồng thời cho các mask slot không chồng nhau. Thêm `AutoFight khi đến tọa` để mỗi lane có thể tự bật đánh khi đến đích.
- Điền lại canonical **Biên Giới Tống Liêu** theo kế hoạch đã duyệt: hội quân 1765,2665; tách 1–2 / 3–6 song song qua hai cặp điểm; hội quân 2213,1665 đánh sạch; tách 1279,1627 / 2503,1425 bật AutoFight; sau 10 giây hội quân 2071,1746 đánh sạch rồi chờ game đưa ra map.
- FIGHT cho phép để trống Monster/ResID; completion vẫn fail-closed theo verified GMonster presence trong radius.
- Thêm cửa sổ **MONSTER ĐÃ LƯU** hiển thị Name/ResID và cho xóa từng dòng; catalog vẫn dùng chung với combobox editor.
- Editor STEP có `Parallel Group (0=tuần tự)` và `Bật AutoFight khi đến tọa`; INI export/import/preset override lưu đầy đủ hai trường mới.
- Protocol EXE↔Bridge `0x00040600`; product/EXE/source/dist đồng bộ v4.6.

## v4.5 — Multi-phó-bản queue + live Activity board

- Mỗi tổ đội có tên hiển thị, sửa thành viên/KEY và danh sách nhiều phó bản; mỗi dòng chạy 1–10 lượt.
- Queue chạy tuần tự: chứng minh rời dungeon map → Telegram từng lượt → dùng chung Auto Sell → lượt/phó bản tiếp theo.
- AUTO-ĐÁNH/DỪNG/WAIT có thể kế thừa tọa độ hợp lệ gần nhất phía trên cùng Map; START fail-closed nếu kế thừa không hợp lệ.
- BẢNG HOẠT ĐỘNG PB đọc semantic text trực tiếp từ active client UI; không OCR và không dùng GMonster/STEP để bịa current/target.
- Xuất/nhập toàn bộ cấu hình phó bản, RoleID bền vững; PID/HWND/snapshot/RUN state không được persist.
- Các nút v4.5 có hàng riêng ở đáy tab; ghi chú an toàn được dời xuống, không chồng control.

# v4.4 — 2026-08-30

- Clean bỏ toàn bộ logic đếm kill cũ của AUTO PHÓ BẢN; giữ strict GMonster scanner.
- FIGHT hoàn thành theo monster presence: AutoFight, scan 5 giây/lần, còn GMonster sống thì đánh tiếp, scan 0 GMonster sống thì sang STEP; scan fail/HP unknown không được PASS.
- Tích hợp bảng Monster/HP/Name và catalog Monster lưu Name/ResID từ v0.6.2 vào UI v4.4.
- Nâng `GetDoingTasks` parser và chẩn đoán TaskID/Name/Parameters.
- Thêm bảng tiến trình realtime, highlight STEP và countdown timeout.
- Nâng TEST TASK + QUÁI thành CHẨN ĐOÁN TASK + QUÁI.

# Changelog

## 4.3

- AUTO PHÓ BẢN nhận ownership khi vào tab: dừng AUTO + AUTO PK ngay, không để ba scheduler cạnh tranh cùng PID.
- FIGHT/STOP-FIGHT tái dùng InputSync AUTO→ĐÁNH QUÁI và DỪNG/Travel Guard của AUTO, có authoritative AutoFight verify trước khi tiến step.
- Thêm QUÉT CLIENT thật + RoleID→PID rebind; chỉ lỗi binding/client được tự phục hồi ERROR→STOP.
- Giao diện tổ đội dạng cây: KEY dòng đầu nền xám, member mỗi người một dòng; row mapping dùng lParam/team-id tránh START/STOP nhầm đội.
- Sửa boxed IEnumerator trong ReadDungeonProgress/GetDoingTasks và Parameters bằng ManagedThis.
- Sửa ACC BÁO CÁO dùng đúng Telegram tab index 3, không còn chồng lên AUTO PHÓ BẢN.

## 4.2

- Thêm mạng lưới đường đi Xa Truyền theo graph/descriptor riêng `travel_network_logic.h`, không nhồi thêm chuỗi if route vào controller.
- Mở rộng `TravelSemantic`: Nam Hải, Miêu Cương, Hoàng Long Phủ, Thạch Lâm, Đại Lý; Bridge exact-match semantic text, không phụ thuộc dòng thứ N.
- Tái sử dụng duy nhất Xa Truyền Bình ID 387 và tọa hiện có trong `sellNpcPositions_`.
- Lâu Lan M5 đi trực tiếp Nam Hải M85, Miêu Cương M64, Hoàng Long Phủ M49; các đích M60/M61/M63/M65/M66 đi tắt tới Thạch Lâm M60 rồi handoff AutoPath cũ.
- Thêm Nam Hải M85 → Đại Lý M2 qua Xa Truyền Tín ID 522 tại 7236,1908.
- Không hardcode ResID chưa xác minh của Xa Truyền Chỉ/Sảng/Quy; planner không sinh cạnh NPC chưa có bằng chứng.
- Route planner xét CurrentMap: đang ở mạng M60/M61/M63/M65/M66 thì không vòng về Lâu Lan.
- Nâng protocol lên `0x00040200`, thêm `travel_network_logic_tests`, CI chạy 15 logic tests.

## 4.1

- Thêm `ReadDungeonProgress` đọc read-only `Game.GetDoingTasks()` → `dbTaskData.TaskID/Parameters` để dùng bảng tiến độ server, không OCR.
- FIGHT hợp nhất TASK + strict GMonster: TASK fresh/bound là authority; scanner là verifier/fallback; timeout không bao giờ tạo PASS.
- Thêm TASK + MONSTER LEARN và persistence INI theo preset/step; bind chỉ khi `GMonster.ResID` giao với key `Task.Parameters`.
- Thêm `dungeon_progress_logic` + test policy server-sync/conflict/fallback.
- Protocol 4.1 (`0x00040100`). FuBen packet 200168–200174 chỉ được giữ như proof slot, chưa replay khi direction/response store chưa được chứng minh.

## 4.0

- Thêm tab AUTO PHÓ BẢN theo kiến trúc multi-account của Pro, không thay core AUTO/AUTO PK.
- Mỗi bảng tổ đội 1–6 acc, leader riêng, preset riêng, số lượt 1–3; hỗ trợ nhiều đội chạy đồng thời trên các PID khác nhau.
- Start/Pause/Resume/Stop độc lập từng đội; một PID bị khóa không cho tham gia hai đội RUN/PAUSE.
- Entry barrier toàn đội trước khi leader ClickNPC/callback dialog exact; mỗi client tự chứng minh MapID/MapReady/Position/AutoPath.
- Thêm 19 preset phó bản canonical từ DATA và danh sách route/kill/boss tương ứng.
- Transplant strict GMonster scanner + DeathTracker lifecycle từ donor 0.6.2, không transplant bridge/protocol/FSM đơn acc cũ.
- Thêm semantic `ClickDialogText` fail-closed và protocol v4.0.
- Thêm `dungeon_logic_tests` và CI build/artifact v4.0.
- Transplant/adapt nốt persistence/editor/diagnostics từ source donor `ThanLongItemConsolidator_Source_v0.6.2(3)`: editor step + GET tọa + participant mask 1–6, team RoleID persistence, daily counter và skip diagnostics.
- Sửa FIGHT step dùng đúng X/Y của khu tương ứng; loại bỏ cơ chế thử nghiệm “3 scan rỗng = PASS”. AOI disappearance không còn là completion proof.
- Post-run Auto Sell fail-closed khi thiếu RoleID/FreeBagSpace; STOP/FAIL đóng background sell trước khi nhả ownership.
- Q3 Tô Châu gather fallback đồng bộ canonical ENTRY NPC thành `4500,8190`; portal có settle guard 1.8 giây để tránh race step kế tiếp.
- Giữ nguyên các logic 3.3 ngoài phạm vi tích hợp phó bản.

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
