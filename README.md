# AUTO Thần Long đa tính năng Pro v4.4

Tool Windows x64 quản lý nhiều client Thần Long Mobile. EXE và DLL phải luôn được thay cùng nhau vì giao thức bridge được khóa theo phiên bản.

## Mới trong v4.4 — scan monster/HP + FIGHT theo trạng thái còn quái

- **Bỏ hoàn toàn bộ đếm kill cũ** (`DeathTracker`, `requiredKills`, TASK+SCAN proof theo số kill). FIGHT không còn phụ thuộc 6/6, 1/1 hay alive→dead counter.
- FIGHT dùng đúng cơ chế mới: AutoFight ON → chờ 5 giây → scan strict `GMonster` mỗi 5 giây. Còn bất kỳ GMonster sống trong radius thì tiếp tục đánh; `GMonster sống=0` mới PASS và chuyển STEP. Scan lỗi hoặc HP chưa chắc chắn luôn WAIT, không được suy diễn là hết quái.
- Khôi phục bảng scan từ nền v0.6.2 vào UI mới: **Tên Monster, RoleID, ResID, HP/MaxHP, chết, X,Y, class + nguồn HP**.
- Khôi phục luôn **catalog Monster lưu** kiểu 0.6.2: chọn một dòng scan → `LƯU MONSTER ĐÃ CHỌN`; Name/ResID persist trong INI và xuất hiện trong combobox Monster của bảng cấu hình STEP.
- `GetDoingTasks()` được nâng parser nhiều nhánh: managed array, Dictionary.Values, IEnumerable/explicit-interface method và List/Collection index fallback. TASK chỉ là observer/chẩn đoán, không gate STEP.
- Thêm **BẢNG TIẾN TRÌNH** realtime: đội/state, lượt/phase, STEP hiện tại, mục tiêu, tọa độ, AutoFight, proof, scan, quyết định, TASK, WAIT/FAIL và countdown timeout.
- `CHẨN ĐOÁN TASK + QUÁI` hiển thị **TaskID, tên task, toàn bộ parameter key=value** và danh sách GMonster kèm Name/ResID/HP.
- Dòng STEP đang chạy được tự động highlight.

## Mới trong v4.3 — ổn định AUTO PHÓ BẢN + giao diện tổ đội

- Vào tab **AUTO PHÓ BẢN** lập tức STOP toàn bộ AUTO và AUTO PK trước khi trao ownership cho dungeon.
- FIGHT/STOP-FIGHT dùng lại đúng core InputSync `AUTO → ĐÁNH QUÁI` và `DỪNG/Travel Guard` đã chạy ổn ở tab AUTO; luôn verify `Snapshot.AutoFight`.
- Thêm nút **QUÉT CLIENT** thật; tổ đội vẫn persist theo RoleID và bind lại PID sau scan. Lỗi mất client/bind có thể tự về STOP khi bind đủ, lỗi runtime khác vẫn giữ ERROR.
- Bảng tổ đội hiển thị KEY ở dòng đầu nền xám nhẹ, từng thành viên một dòng thụt vào; mọi thao tác START/PAUSE/STOP/XÓA map theo team-id thay vì số dòng UI.
- Sửa TASK `GetDoingTasks()` với boxed value-type enumerator (`ManagedThis`) để Dictionary/List runtime không còn fail chỉ vì gọi `MoveNext/get_Current` trên boxed object.
- Sửa **ACC BÁO CÁO**: tab Telegram thực tế là index 3, không còn render đè lên tab AUTO PHÓ BẢN; sidebar báo cáo chỉ xuất hiện trong Telegram.

## Mới trong v4.2 — Mạng lưới đường đi Xa Truyền

- Lâu Lan M5 dùng lại đúng **Xa Truyền Bình ID 387** và đúng tọa đang lưu trong `sellNpcPositions_`; không tạo NPC/tọa Xa Truyền Bình thứ hai.
- Thêm exact semantic cho **Nam Hải / Miêu Cương / Hoàng Long Phủ / Thạch Lâm / Đại Lý**; không click theo số thứ tự dòng.
- Planner chọn đường tắt: Lâu Lan → Nam Hải/Miêu Cương/Hoàng Long Phủ trực tiếp; các đích M60/M61/M63/M65/M66 đi Xa Truyền Bình → Thạch Lâm M60 rồi bàn giao lại AutoPath portal cũ.
- Nếu đang ở M60/M61/M63/M65/M66 thì không vòng về Lâu Lan; AutoPath xử lý portal trực tiếp từ current map.
- Thêm chiều Nam Hải M85 → Đại Lý M2 qua **Xa Truyền Tín ID 522, 7236,1908**.
- Không đoán ResID của Xa Truyền Chỉ / Xa Truyền Sảng / Xa Truyền Quy; các chiều về chưa đủ bằng chứng không được hardcode.
- Protocol EXE↔Bridge: `0x00040200`; thêm `travel_network_logic_tests`; CI chạy 15 logic tests.

## Mới trong v4.1 — Bảng tiến độ server + scanner đồng bộ

- Auto Phó Bản đọc semantic `LuaSystemAPI_Game.GetDoingTasks()` và `dbTaskData.Parameters` làm nguồn tiến độ server, không OCR.
- FIGHT dùng policy: TASK server đã bind > strict GMonster alive→dead fallback. Scanner thấy đủ kill nhưng TASK fresh chưa đủ thì chờ `UpdateTask`, không tự PASS.
- `TASK + MONSTER LEARN`: đối chiếu `Task.Parameters[MonsterID]` với `GMonster.ResID`, lưu TaskID/ParameterKey theo preset+step vào INI cục bộ; mapping live không còn hợp lệ thì tự bỏ và học lại.
- DATA `CMD_FUBEN_* 200168–200174` đã được nghiên cứu nhưng v4.1 **không gửi packet mù**: FuBen proof chỉ được reserved cho khi chứng minh được inbound/runtime response store trên client.

- Thêm tab **AUTO PHÓ BẢN** nằm cạnh AUTO PK.
- Danh sách acc dùng chính tập client GameAssembly.dll mà Tool đã quét; không dùng scan người xung quanh.
- Mỗi bảng tổ đội chọn **1–6 acc**, gán **đội trưởng riêng**, chọn preset phó bản và số lượt 1–3.
- Có thể tạo nhiều bảng tổ đội. Các bảng dùng PID khác nhau được scheduler điều phối độc lập; một PID không thể thuộc hai đội RUN/PAUSE cùng lúc.
- Mỗi đội có **BẮT ĐẦU / TẠM DỪNG-TIẾP / STOP** riêng. Pause giữ nguyên phase/step nhưng dừng Path/AutoFight của đúng đội đó.
- Cấu hình tổ đội được lưu theo **RoleID ổn định** rồi bind lại PID sau mỗi lần quét client; restart tool không phụ thuộc PID cũ.
- Mỗi preset có cửa sổ **CẤU HÌNH PHÓ BẢN**: thêm/xóa/nhân bản/đổi thứ tự step, GET tọa, Map/X/Y/tolerance, kill/radius/timeout/delay, boss/filter, và mask slot 1–6. Preset đang RUN được đóng băng snapshot; sửa cấu hình chỉ áp dụng lần START sau.
- Số lượt được ghi theo **RoleID + preset + ngày local PC**, tối đa 3/ngày; chỉ cộng sau khi đã chứng minh toàn đội rời DungeonMapID.
- Sau mỗi lượt, từng acc có ngưỡng túi riêng. Nếu bật Auto Sell và FreeBagSpace dưới ngưỡng, v4.1 tái dùng nguyên Auto Sell v3.3; thiếu snapshot túi/NPC/macro thì fail-closed.
- Entry pipeline: toàn đội tập kết NPC → barrier per-PID → leader ClickNPC → callback dialog text exact → chờ đủ acc vào DungeonMapID.
- Tái sử dụng core v3.3: snapshot per-PID, robust Travel Guard, AutoPath, Start/Stop AutoFight, semantic UI, serialized Bridge action. Không tạo engine di chuyển/đánh riêng.
- Thêm strict dungeon scanner từ donor 0.6.2: chỉ nhận exact `GMonster`, loại `GRole`, đọc live RoleID/ResID/HP/death/position và đếm vòng đời alive→dead không trùng.
- Giữ đúng donor 0.6.2: xác chết thấy lần đầu, xác lặp và quái biến mất khỏi AOI **không** được suy diễn thành kill; stage chỉ PASS khi counter đạt `requiredKills`.
- MOVE/FIGHT cùng một khu dùng cùng tâm X/Y/radius; sửa lỗi prototype từng lọc FIGHT quanh `0,0`. `UsePortal` được biểu diễn bằng tới đúng tọa portal + settle guard trước step kế tiếp, không click portal mù.
- Bundled **19 scenario canonical** từ DATA, gồm Thủy Lao, Q1/Q2/Q3 Tô Châu, 3 Trân Long Kỳ Cuộc, 3 Lâu Lan Tầm Bảo, 3 Túc Cầu, Q1/Q2/Q3 Lâu Lan, Thập Nhị Sát Tinh, Yến Tử Ô và Diệt Phỉ Kính Hồ.
- Timeout chỉ là FAIL guard; không được dùng timeout để giả hoàn thành.
- Protocol EXE↔Bridge hiện tại là `0x00040200`; source/EXE/DLL phải đi cùng bộ v4.2.

### Lưu ý runtime

Preset và route là static canonical; HP/death/RoleID/spawn/dialog/map readiness vẫn lấy live. v4.1 đọc bảng nhiệm vụ runtime qua Task API, không OCR. Build/logic-test PASS không đồng nghĩa mọi phó bản đã được chứng minh runtime trên client thực.

## Sửa v3.3 — macro giao dịch thuần MAIN/CON + điều phối túi MAIN

- Xóa hoàn toàn loại dòng `CHUYỂN ĐỒ`: chuỗi giao dịch chỉ còn `ACC thực hiện (MAIN/CON) + tọa độ + Delay + Repeat + Nhóm lặp`.
- Không còn `kind`, `HasChildTransferStep`, cột/combobox Loại, validation bắt buộc CHUYỂN ĐỒ hay dữ liệu `Kind` trong chuỗi trade.
- `Repeat` của từng tọa độ được chạy đúng như cấu hình; không có auto-cap theo loại dòng.
- Giữ nguyên lõi v1.4: `delta = FreeBagSpace trước - FreeBagSpace sau`; `delta <= 8` kết thúc CON, `delta > 8` giữ cùng CON.
- Mỗi pass luôn chạy đủ toàn bộ macro từ index 0; capacity MAIN không được cắt ngang một Sequence đang chạy.
- Trước pass mới, MAIN phải có `FreeBagSpace >= 9`. Nếu `<9`, MAIN được nhả cho Auto Sell; các CON đã vào FIFO vẫn đứng chờ.
- Nếu một pass vừa nhận `>=9` item nhưng MAIN còn `<9` ô, CON đang giao dịch được nhả về bãi train; các CON khác trong FIFO vẫn giữ TỌA GD để chờ MAIN bán xong.
- Auto Sell MAIN khi Dồn đồ bật dùng cùng một ngưỡng duy nhất: `<9` ô thì bán; xóa `MainSellThreshold` riêng.
- Tăng cửa sổ ổn định snapshot túi MAIN từ `600 ms` lên `1500 ms`, verify max từ `2200 ms` lên `3200 ms`.
- Portable click config nâng lên `TLCLICKCFG v2`, bỏ cột `kind` khỏi dòng `CHILD`.

## Nền v3.0 được giữ nguyên

### Rời Côn Lôn Sơn

- NPC rời Côn Lôn Sơn là NPC trên M75, hoàn toàn khác Xa Truyền Bình ID387 ở M5 dùng để đi vào Côn Lôn.
- Sau khi AutoPath tới NPC rời Côn Lôn, tool chạy đúng 3 điểm `TryClickUI` do người dùng gán: **Mở NPC → Đại Lý → Xác nhận**.
- Mỗi click có `Time (ms)` (chờ trước click) và `Delay (ms)` (chờ sau click) riêng.
- Cơ chế callback semantic `Đại Lý/Tag=200001` đã bị loại bỏ riêng khỏi nhánh này.
- Tool chỉ bắt đầu kiểm tra MapID sau khi đã chạy xong cả 3 click. Khi MapID đổi và map ổn định, luồng AutoPath thông thường tiếp tục tới tọa đích.

### Tần Hoàng Địa Cung

Các tọa dưới đây thuộc đúng map nguồn đang chứa cổng:

| Map nguồn | Hướng cổng | X,Y | Map kế tiếp |
|---|---|---:|---:|
| M10000 | Vào THĐC tầng 1 | 8257,148110 | M10014 |
| M10014 | Lên tầng 2 | 890,6895 | M10015 |
| M10015 | Lên tầng 3 | 3080,2900 | M10016 |
| M10015 | Xuống tầng 1 | 7450,966 | M10014 |
| M10016 | Lên tầng 4 | 4256,7120 | M10017 |
| M10016 | Xuống tầng 2 | 7620,1242 | M10015 |
| M10017 | Xuống tầng 3 | 690,7200 | M10016 |

- Luồng luôn đi qua từng tầng liền kề và phải xác nhận MapID tầng kế trước khi chọn cổng tiếp theo.
- Từ M10000, chỉ đi tới cổng để game tự chuyển vào M10014. Sau khi M10014 ổn định, tool mới callback nút **Xác nhận** của popup “Chú ý”.
- Các cổng giữa tầng không có bước xác nhận.
- Bảy tọa cổng được điền sẵn, có thể sửa tay hoặc lấy lại bằng nút **LẤY TỌA** trong bảng Tùy chỉnh.
- **XUẤT/NHẬP TẤT CẢ** dùng `TLMASTERCFG v2` và đồng bộ cả 7 cổng cùng 3 click Côn Lôn + Time/Delay.

### Giữ nguyên

- Nhánh Hỏa Diệm/Tinh Túc: chọn Tinh Túc là tự chuyển map, không bấm Xác nhận.
- Các route liên-server khác, AutoPath, lên ngựa, bán đồ, giao dịch, Auto PK, F8, Telegram và lọc đồ giữ nguyên ngoài thay đổi được nêu trên.
- Danh sách vai trò hỗ trợ CON1 đến CON12.

## Build

GitHub Actions trên `windows-latest` tạo:

- `dist/AUTO_Than_Long_da_tinh_nang_Pro_v4.4.exe`
- `dist/ThanLongCleanRouteBridge.dll`
- `dist/AUTO-Than-Long-da-tinh-nang-Pro-v4.4-win-x64.zip`
- `dist/AUTO-Than-Long-da-tinh-nang-Pro-v4.4-source.zip`
- `dist/SHA256SUMS.txt`
