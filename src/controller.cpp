#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <tlhelp32.h>
#include <cstdint>
#include <cstddef>
#include <climits>
#include <cstring>
#include <cwchar>
#include <cmath>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <utility>
#include <functional>
#include <deque>
#include <map>
#include <set>
#include "protocol.h"
#include "route_logic.h"
#include "rotation_logic.h"
#include "trade_coordinator_logic.h"
#include "telegram_notifier.h"
#include "telegram_logic.h"
#include "fixed_slot_sell_logic.h"
#include "internal_ui_click_logic.h"
#include "travel_fight_guard_logic.h"
#include "auto_fight_retry_logic.h"
#include "auto_pk_logic.h"
#include "inventory_filter_logic.h"
#include "thdc_route_logic.h"
#include "travel_network_logic.h"
#include "dungeon_logic.h"
#include "dungeon_progress_logic.h"
#include "dungeon_presets.h"

using namespace cleanroute;
using namespace cleanroute_logic;
using namespace cleanroute_rotation;
using namespace itemtrade_coordinator;
using inventory_filter_logic::RuleAction;

namespace {

constexpr wchar_t kTitle[] = L"AUTO Thần Long đa tính năng Pro v4.3";
constexpr int kTreatmentNpcResId = 339;
constexpr wchar_t kGameModule[] = L"GameAssembly.dll";
constexpr UINT_PTR kTimer = 1;
constexpr UINT_PTR kRecordTimer = 2;
constexpr int kCaptureHotkeyId = 9001;
constexpr int kPauseHotkeyId = 9002;
constexpr DWORD kClientStableResumeMs = 2000;
constexpr DWORD kBridgeNudgeMs = 750;
constexpr DWORD kReadFailLogIntervalMs = 2000;
constexpr UINT kWindowResponsiveProbeMs = 120;
constexpr DWORD kTrainPositionCheckMs = 60000;
constexpr DWORD kAutoFightRecheckMs = 60000;
constexpr wchar_t kUpcomingFeaturesText[] =
    L"Các chức năng/ tính năng của AUTO thần long do Thắng Nguyễn ( ĐỒ LONG )  xây dựng và Phát triển ĐỘC QUYỀN CHƯA TỪNG NƠI NÀO CÓ\r\n"
    L"\r\n"
    L"- Các acc được quản lý bởi bộ não ảo thông minh\r\n"
    L"\r\n"
    L"1. Giúp điều phối và giúp đỡ lẫn nhau giữa các acc\r\n"
    L"  - Ví dụ như acc 1 đang yếu máu thì dù ở cách xa vạn dặm acc 2 nếu là NM  cũng có thể tự động chạy đến buff rồi chạy về\r\n"
    L"  - Hoặc 1 acc đang train mà bị PK chết quá nhiều lần thì các acc ở các Map khác nhau sẽ cùng chạy về tọa độ acc đó để dọn dẹp rồi tự động về map train bình thường\r\n"
    L"  - 1 acc đang thiếu đói vàng khóa thì các acc còn lại sẽ cùng train và đem vàng khóa về giao lại cho\r\n"
    L"\r\n"
    L"2. Cùng nhau đi boss tự phân chia nhiệm vụ\r\n"
    L"  - ví dụ Bộ não sẽ chỉ đạo acc Võ đang tự bế Lý thu thủy khi cần thiết, và lúc nào cần bế. Nếu thấy skill chưa hồi có thể gọi các acc khác cùng đợi khi nào hồi thì cùng vào ăn boss\r\n"
    L"  - Hoặc đi QTC khi mà sót con quái , các acc tự động bảo nhau đi tìm 6 hướng khác nhau. Khi 1 acc tìm thấy và giết được quái thì sẽ bảo 5 đứa kia để về tọa ăn boss\r\n"
    L"\r\n"
    L"3. Tính năng PK\r\n"
    L"  - các acc clone đi với nhau sẽ không bao giờ pk lẻ tẻ. chỉ đợi khi các acc tụ đông đủ mới tự động lao vào bãi pk. 1 vòng lặp luân hồi\r\n"
    L"\r\n"
    L"4. Check trạng thái nhân vật theo real time thời gian thực để đưa ra những gợi ý hành động cho các acc.\r\n"
    L"\r\n"
    L"5. Bộ não cũng sẽ tự động gửi tin nhắn về điện thoại thông báo tình hình acc khỏe hay yếu , buồn hay vui , để bạn kịp thời để ý\r\n"
    L"\r\n"
    L"Đủ các loại auto mà bạn chưa từng nghĩ tới và chính mình cũng chưa từng nghĩ tới\r\n"
    L"Tất cả các hành động đều dựa theo bộ não điều khiển, không hành động như robot mà scrip từng làm .\r\n"
    L"Rất nhiều tình năng sắp ra mắt. hihi";
constexpr DWORD kMountRetryWaitMs = 5000;
constexpr DWORD kMountFightBoostMs = 10000;
constexpr DWORD kPriorityAutoVerifyMs = 1300;
constexpr int kUnderworldMapId = 87;
constexpr int kLauLanMapId = 5;
constexpr int kXaTruyenBinhNpcId = 387; // DATA verified: M5 Lâu Lan
constexpr int kNgaiNiNgoaNhiNpcId = 913; // DATA verified: M5 Lâu Lan
constexpr DWORD kLauLanGateStallMs = 3000;
constexpr DWORD kLauLanConfirmRetryMs = 3000;
constexpr DWORD kAutoPathFightConflictRetryMs = 1000;
constexpr DWORD kRouteOwnershipStopRetryMs = 1200;
constexpr int kRouteOwnershipStopMaxAttempts = 3;
constexpr DWORD kTradeBagStableMs = 1500;      // v3.3: allow MAIN bag snapshot enough time to settle after a completed pass.
constexpr DWORD kTradeBagVerifyMaxMs = 3200;    // v3.3: preserve a real 1500 ms stable window after a delayed bag update.
constexpr DWORD kTradeTargetTimeoutMs = 4500;
constexpr DWORD kTradeTargetRetryMs = 500;
constexpr int kPreciseWorldTolerance = 20; // v1.6: GD/NPC gần như tuyệt đối; train vẫn dùng profile tolerance.
constexpr DWORD kShortcutPathAcceptMs = 5000; // Cho snapshot đủ thời gian chứng minh AutoPath/movement thực.
constexpr int kShortcutPathMaxDispatch = 5;
// The game creates/rebuilds the NPC GameDialog on a later UI frame. The
// successful v0.1.8 probe starts with an already-open dialog; automated
// ClickNPC/TryClickUI needs a slower controller-side handoff. Keep all waits
// outside the game UI thread and give every semantic stage 2 seconds to settle.
constexpr DWORD kShortcutNpcUiReadyMs = 2000;
constexpr DWORD kShortcutSemanticRetryMs = 2000;
constexpr DWORD kShortcutConfirmUiReadyMs = 2000;
constexpr DWORD kShortcutConfirmRetryMs = 2000;
constexpr int kShortcutSemanticMaxAttempts = 12;
constexpr int kShortcutConfirmMaxAttempts = 20;
constexpr int kMainTradeRole = 1;
constexpr int kFirstChildTradeRole = 2;
constexpr int kChildTradeCount = 12;
constexpr int kLastChildTradeRole = kFirstChildTradeRole + kChildTradeCount - 1;
constexpr int kRotateDeathLimitDefault = 10;
constexpr int kRotateDeathWindowMinDefault = 10;
constexpr int kRotateNoFullBagMinDefault = 15;
constexpr int kRotateDeathLimitMin = 1;
constexpr int kRotateDeathLimitMax = 100;
constexpr int kRotateWindowMin = 1;
constexpr int kRotateWindowMax = 180;






constexpr int IDC_CLIENT_LIST = 100;
constexpr int IDC_SCAN = 101;
constexpr int IDC_START_CHECKED = 102;
constexpr int IDC_STOP_CHECKED = 103;
constexpr int IDC_SELECTED = 104;
constexpr int IDC_LIVE = 105;
constexpr int IDC_TARGET_NAME = 110;
constexpr int IDC_SAVE_TARGET = 111;
constexpr int IDC_TARGET_TEXT = 112;
constexpr int IDC_TOLERANCE = 113;
constexpr int IDC_SPOT_COMBO = 114;
constexpr int IDC_DELETE_SPOT = 115;
constexpr int IDC_ENABLE_REVIVE = 120;
constexpr int IDC_ENABLE_CONFIRM = 121;
constexpr int IDC_ENABLE_FIGHT = 122;
constexpr int IDC_ENABLE_SELL = 123;
constexpr int IDC_SELL_NPC = 124;
constexpr int IDC_SELL_NPC_X = 125;
constexpr int IDC_SELL_NPC_Y = 126;
constexpr int IDC_SELL_NPC_CAPTURE = 127;
constexpr int IDC_SELL_NPC_POS = 128;
constexpr int IDC_CAPTURE_AUTO = 132;
constexpr int IDC_CAPTURE_ATTACK = 133;
constexpr int IDC_CAPTURE_STOP_AUTO_2 = 135;
constexpr int IDC_POINT_AUTO = 142;
constexpr int IDC_POINT_ATTACK = 143;
constexpr int IDC_POINT_STOP_AUTO_2 = 145;
constexpr int IDC_TEST_AUTO = 152;
constexpr int IDC_TEST_ATTACK = 153;
constexpr int IDC_TEST_STOP_AUTO_2 = 155;
constexpr int IDC_SELL_MACRO_LIST = 170;
constexpr int IDC_SELL_ADD = 171;
constexpr int IDC_SELL_DELETE = 172;
constexpr int IDC_SELL_DESC = 173;
constexpr int IDC_SELL_DELAY = 174;
constexpr int IDC_SELL_REPEAT = 175;
constexpr int IDC_SELL_SAVE = 176;
constexpr int IDC_SELL_CAPTURE = 177;
constexpr int IDC_SELL_TEST = 178;
constexpr int IDC_LOG = 160;
constexpr int IDC_ROTATION_LIST = 186;
constexpr int IDC_ROTATE_DEATH_LIMIT = 187;
constexpr int IDC_ROTATE_DEATH_WINDOW = 188;
constexpr int IDC_ROTATE_NO_BAG = 189;
constexpr int IDC_TRADE_ROLE = 190;
constexpr int IDC_TRADE_RENDEZVOUS_CAPTURE = 195;
constexpr int IDC_SELL_SEQUENCE = 197;
constexpr int IDC_MAIN_TRADE_SEQUENCE = 198;
constexpr int IDC_CHILD_TRADE_SEQUENCE = 199;
constexpr int IDC_COPY_CLICKS = 200;
constexpr int IDC_SELL_REC = 201;
constexpr int IDC_SELL_COPY = 202;
constexpr int IDC_SELL_PASTE = 203;
constexpr int IDC_SELL_COPY_ACCOUNT = 204;
constexpr int IDC_CONSOLIDATE_TOGGLE = 205;
constexpr int IDC_MAIN_TAB = 207;
constexpr int IDC_BAG_FILTER_OPEN = 208;
constexpr int IDC_COMPACT_TOGGLE = 209;
constexpr int IDC_EXPORT_CLICK_CONFIG = 210;
constexpr int IDC_IMPORT_CLICK_CONFIG = 211;
constexpr int IDC_EXPORT_MAP_CONFIG = 212;
constexpr int IDC_IMPORT_MAP_CONFIG = 213;
constexpr int IDC_ENABLE_SHORTCUT = 214;
constexpr int IDC_SHORTCUT_SETTINGS = 215;
// Shortcut settings secondary window. 630-699 is isolated from inventory/Telegram IDs.
constexpr int IDC_SC_THEME = 630;
constexpr int IDC_SC_KUNLUN_X = 631;
constexpr int IDC_SC_KUNLUN_Y = 632;
constexpr int IDC_SC_XA_X = 633;
constexpr int IDC_SC_XA_Y = 634;
constexpr int IDC_SC_NGAI_X = 635;
constexpr int IDC_SC_NGAI_Y = 636;
constexpr int IDC_SC_TINHTUC_X = 637;
constexpr int IDC_SC_TINHTUC_Y = 638;
constexpr int IDC_SC_THANHLIEN_X = 639;
constexpr int IDC_SC_THANHLIEN_Y = 640;
constexpr int IDC_SC_PHAMLIEN_X = 641;
constexpr int IDC_SC_PHAMLIEN_Y = 642;
constexpr int IDC_SC_KHOVINH_X = 643;
constexpr int IDC_SC_KHOVINH_Y = 644;
constexpr int IDC_SC_SAVE = 646;
constexpr int IDC_SC_CLOSE = 647;
constexpr int IDC_SC_CAPTURE_COORD_0 = 651;
constexpr int IDC_SC_CAPTURE_COORD_1 = 652;
constexpr int IDC_SC_CAPTURE_COORD_2 = 653;
constexpr int IDC_SC_CAPTURE_COORD_3 = 654;
constexpr int IDC_SC_CAPTURE_COORD_4 = 655;
constexpr int IDC_SC_CAPTURE_COORD_5 = 656;
constexpr int IDC_SC_CAPTURE_COORD_6 = 657;
constexpr int IDC_SC_SELLER_COMBO = 658;
constexpr int IDC_SC_SELLER_CAPTURE = 659;
constexpr int IDC_SC_SELLER_LABEL = 660;
constexpr int IDC_SC_THDC_X_0 = 661;
constexpr int IDC_SC_THDC_Y_0 = 662;
constexpr int IDC_SC_THDC_X_1 = 663;
constexpr int IDC_SC_THDC_Y_1 = 664;
constexpr int IDC_SC_THDC_X_2 = 665;
constexpr int IDC_SC_THDC_Y_2 = 666;
constexpr int IDC_SC_THDC_X_3 = 667;
constexpr int IDC_SC_THDC_Y_3 = 668;
constexpr int IDC_SC_THDC_X_4 = 669;
constexpr int IDC_SC_THDC_Y_4 = 670;
constexpr int IDC_SC_THDC_X_5 = 671;
constexpr int IDC_SC_THDC_Y_5 = 672;
constexpr int IDC_SC_THDC_X_6 = 673;
constexpr int IDC_SC_THDC_Y_6 = 674;
constexpr int IDC_SC_CAPTURE_THDC_0 = 675;
constexpr int IDC_SC_CAPTURE_THDC_1 = 676;
constexpr int IDC_SC_CAPTURE_THDC_2 = 677;
constexpr int IDC_SC_CAPTURE_THDC_3 = 678;
constexpr int IDC_SC_CAPTURE_THDC_4 = 679;
constexpr int IDC_SC_CAPTURE_THDC_5 = 680;
constexpr int IDC_SC_CAPTURE_THDC_6 = 681;
constexpr int IDC_SC_CAPTURE_KUNLUN_CLICK_0 = 682;
constexpr int IDC_SC_CAPTURE_KUNLUN_CLICK_1 = 683;
constexpr int IDC_SC_CAPTURE_KUNLUN_CLICK_2 = 684;
constexpr int IDC_SC_KUNLUN_TIME_0 = 685;
constexpr int IDC_SC_KUNLUN_TIME_1 = 686;
constexpr int IDC_SC_KUNLUN_TIME_2 = 687;
constexpr int IDC_SC_KUNLUN_DELAY_0 = 688;
constexpr int IDC_SC_KUNLUN_DELAY_1 = 689;
constexpr int IDC_SC_KUNLUN_DELAY_2 = 690;
// v1.3 inventory-filter secondary window. 600-629 is isolated from main/AUTO PK/Telegram IDs.
constexpr int IDC_IF_BAG_LIST = 600;
constexpr int IDC_IF_RULE_LIST = 601;
constexpr int IDC_IF_REFRESH = 602;
constexpr int IDC_IF_ADD_KEEP = 603;
constexpr int IDC_IF_ADD_DROP = 604;
constexpr int IDC_IF_ADD_SELL = 605;
constexpr int IDC_IF_DELETE_RULE = 606;
constexpr int IDC_IF_ENABLED = 607;
constexpr int IDC_IF_PROTECT_BOUND = 608;
constexpr int IDC_IF_KEEP_WEAPON = 609;
constexpr int IDC_IF_DROP_EQUIP_NONWEAPON = 610;
constexpr int IDC_IF_SELL_EQUIP_NONWEAPON = 611;
constexpr int IDC_IF_DROP_COMMON = 612;
constexpr int IDC_IF_SELL_COMMON = 613;
constexpr int IDC_IF_DROP_GEM = 614;
constexpr int IDC_IF_SELL_GEM = 615;
constexpr int IDC_IF_DROP_MEDICINE = 616;
constexpr int IDC_IF_SELL_MEDICINE = 617;
constexpr int IDC_IF_DROP_PETEQUIP = 618;
constexpr int IDC_IF_SELL_PETEQUIP = 619;
constexpr int IDC_IF_STATUS = 620;
// v1.0 Telegram transplant: 500+ is isolated from AUTO/AutoPK/editor control IDs.
constexpr int IDC_TG_ENABLED = 500;
constexpr int IDC_TG_TOKEN = 501;
constexpr int IDC_TG_SHOW_TOKEN = 502;
constexpr int IDC_TG_CHAT_ID = 503;
constexpr int IDC_TG_SAVE = 504;
constexpr int IDC_TG_TEST_BOT = 505;
constexpr int IDC_TG_DISCOVER_CHAT = 506;
constexpr int IDC_TG_SEND_TEST = 507;
constexpr int IDC_TG_SEND_SUMMARY = 508;
constexpr int IDC_TG_LOG = 509;
constexpr int IDC_TG_CLEAR_LOG = 510;
constexpr int IDC_TG_COPY_LOG = 511;
constexpr int IDC_TG_NOTIFY_DEATH = 520;
constexpr int IDC_TG_NOTIFY_REVIVE = 521;
constexpr int IDC_TG_NOTIFY_SELL_COMPLETE = 522;
constexpr int IDC_TG_NOTIFY_SELL_SUMMARY = 523;
constexpr int IDC_TG_NOTIFY_TRADE = 524;
constexpr int IDC_TG_NOTIFY_FREEZE = 525;
constexpr int IDC_TG_NOTIFY_FIFO = 526;
constexpr int IDC_TG_NOTIFY_LAULAN = 527;
constexpr int IDC_TG_NOTIFY_WORLDFLOW_TIMEOUT = 528;
constexpr int IDC_TG_NOTIFY_TOOL_STATE = 529;
constexpr int IDC_TG_NOTIFY_SESSION_SUMMARY = 530;
constexpr int IDC_TG_INTERVAL_ENABLED = 540;
constexpr int IDC_TG_INTERVAL_MINUTES = 541;
constexpr int IDC_TG_DAILY_ENABLED = 542;
constexpr int IDC_TG_DAILY_TIME1 = 543;
constexpr int IDC_TG_DAILY_TIME2 = 544;
constexpr int IDC_TG_DAILY_TIME3 = 545;
constexpr int IDC_TG_DAILY_TIME4 = 546;
constexpr int IDC_TG_WORLDFLOW_TIMEOUT_SEC = 547;
constexpr int IDC_TG_STATUS = 548;
constexpr int IDC_TG_NOTIFY_FUN_ALERTS = 549;
constexpr int IDC_TG_MONEY_1M = 550;
constexpr int IDC_TG_MONEY_5M = 551;
constexpr int IDC_TG_MONEY_60M = 552;
constexpr int IDC_TG_MONEY_6H = 553;
constexpr int IDC_TG_MONEY_24H = 554;
constexpr UINT kTelegramResultMessage = WM_APP + 0x61;
constexpr int IDC_SEQ_LIST = 300;
constexpr int IDC_SEQ_TARGET = 301;
constexpr int IDC_SEQ_DESC = 303;
constexpr int IDC_SEQ_DELAY = 304;
constexpr int IDC_SEQ_REPEAT = 305;
constexpr int IDC_SEQ_ADD = 306;
constexpr int IDC_SEQ_DELETE = 307;
constexpr int IDC_SEQ_UP = 308;
constexpr int IDC_SEQ_DOWN = 309;
constexpr int IDC_SEQ_SAVE = 310;
constexpr int IDC_SEQ_CAPTURE = 311;
constexpr int IDC_SEQ_TEST = 312;
constexpr int IDC_SEQ_CLOSE = 313;
constexpr int IDC_SEQ_REC = 314;
constexpr int IDC_SEQ_COPY = 315;
constexpr int IDC_SEQ_PASTE = 316;
constexpr int IDC_SEQ_GROUP_REPEAT = 317;
constexpr int IDC_SEQ_GROUP_SELECTED = 318;
constexpr int IDC_SEQ_UNGROUP = 319;


// AUTO PHÓ BẢN v4.0 — isolated 700-749 control range.
constexpr int IDC_DG_ACCOUNT_LIST=700;
constexpr int IDC_DG_LEADER=701;
constexpr int IDC_DG_PRESET=702;
constexpr int IDC_DG_RUNS=703;
constexpr int IDC_DG_CREATE_TEAM=704;
constexpr int IDC_DG_TEAM_LIST=705;
constexpr int IDC_DG_START=706;
constexpr int IDC_DG_PAUSE=707;
constexpr int IDC_DG_STOP=708;
constexpr int IDC_DG_DELETE_TEAM=709;
constexpr int IDC_DG_STEP_LIST=710;
constexpr int IDC_DG_STATUS=711;
constexpr int IDC_DG_REFRESH=712;
constexpr int IDC_DG_SCAN_MONSTER=713;
constexpr int IDC_DG_MONSTER_STATUS=714;
constexpr int IDC_DG_CONFIG=715;
constexpr int IDC_DG_SELL_THRESHOLD=716;
constexpr int IDC_DG_SAVE_THRESHOLD=717;
constexpr int IDC_DG_SCAN_CLIENT=718;
// Dungeon preset editor window (740-779 reserved).
constexpr int IDC_DGE_LIST=740;
constexpr int IDC_DGE_KIND=741;
constexpr int IDC_DGE_LABEL=742;
constexpr int IDC_DGE_MAP=743;
constexpr int IDC_DGE_X=744;
constexpr int IDC_DGE_Y=745;
constexpr int IDC_DGE_TOL=746;
constexpr int IDC_DGE_KILLS=747;
constexpr int IDC_DGE_RADIUS=748;
constexpr int IDC_DGE_DELAY=749;
constexpr int IDC_DGE_TIMEOUT=750;
constexpr int IDC_DGE_MONSTER=751;
constexpr int IDC_DGE_RESID=752;
constexpr int IDC_DGE_GROUP=753;
constexpr int IDC_DGE_BOSS=754;
constexpr int IDC_DGE_ANY=755;
constexpr int IDC_DGE_SLOT1=756;
constexpr int IDC_DGE_SLOT2=757;
constexpr int IDC_DGE_SLOT3=758;
constexpr int IDC_DGE_SLOT4=759;
constexpr int IDC_DGE_SLOT5=760;
constexpr int IDC_DGE_SLOT6=761;
constexpr int IDC_DGE_ADD=762;
constexpr int IDC_DGE_DELETE=763;
constexpr int IDC_DGE_UP=764;
constexpr int IDC_DGE_DOWN=765;
constexpr int IDC_DGE_DUP=766;
constexpr int IDC_DGE_GET=767;
constexpr int IDC_DGE_SAVE=768;
constexpr int IDC_DGE_RESET=769;
constexpr int IDC_DGE_CLOSE=770;
constexpr int IDC_DGE_GATHER_MAP=771;
constexpr int IDC_DGE_NPC=772;
constexpr int IDC_DGE_GATHER_X=773;
constexpr int IDC_DGE_GATHER_Y=774;
constexpr int IDC_DGE_DUNGEON_MAP=775;
constexpr int IDC_DGE_DIALOG=776;
constexpr int IDC_DGE_SAVE_HEADER=777;

constexpr int IDC_PK_START = 400;
constexpr int IDC_PK_STOP = 401;
constexpr int IDC_PK_LIFE = 402;
constexpr int IDC_PK_LOOP = 403;
constexpr int IDC_PK_STEP_LIST = 410;
constexpr int IDC_PK_STEP_UP = 411;
constexpr int IDC_PK_STEP_DOWN = 412;
constexpr int IDC_PK_TARGET_CAPTURE = 413;
constexpr int IDC_PK_STEP_DESC = 414;
constexpr int IDC_PK_TOLERANCE = 415;
constexpr int IDC_PK_STEP_DELAY = 416;
constexpr int IDC_PK_STEP_REPEAT = 417;
constexpr int IDC_PK_STEP_SAVE = 418;
constexpr int IDC_PK_CLICK_LIST = 420;
constexpr int IDC_PK_CLICK_ADD = 421;
constexpr int IDC_PK_CLICK_DELETE = 422;
constexpr int IDC_PK_CLICK_UP = 423;
constexpr int IDC_PK_CLICK_DOWN = 424;
constexpr int IDC_PK_CLICK_PHASE = 425;
constexpr int IDC_PK_CLICK_DESC = 426;
constexpr int IDC_PK_CLICK_DELAY = 427;
constexpr int IDC_PK_CLICK_REPEAT = 428;
constexpr int IDC_PK_CLICK_SAVE = 429;
constexpr int IDC_PK_CLICK_CAPTURE = 430;
constexpr int IDC_PK_CLICK_TEST = 431;

constexpr std::array<const wchar_t*, 5> kClickKeys = {
    L"Confirm", L"Revive", L"AutoMenu", L"Attack", L"StopAuto2"
};
constexpr std::array<const wchar_t*, 5> kClickLabels = {
    L"XÁC NHẬN RA MAP", L"ĐẦU THAI", L"AUTO", L"ĐÁNH QUÁI", L"DỪNG AUTO 2"
};

enum class ClickSlot : int {
    None = -1,
    Confirm = 0,
    Revive = 1,
    AutoMenu = 2, // one saved UI point named AUTO replaces old DỪNG AUTO 1
    Attack = 3,
    StopAuto2 = 4,
};

enum class PriorityAutoOwner : int {
    None = 0,
    TravelGuardStop,
    TravelGuardReset,
    Train,
    MountRecovery,
    Dungeon,
};

struct ClickPoint {
    int x = 0;
    int y = 0;
    int baseW = 0;
    int baseH = 0;
    bool valid = false;
};

struct TimedClickPoint {
    ClickPoint point{};
    // Controller-side wait before dispatching TryClickUI. Keeping this wait out
    // of the game thread prevents a configurable timing value from freezing Unity.
    int timeMs = 0;
    // Wait after this completed TryClickUI before the next step/map check.
    int delayMs = 2000;
};

struct ShortcutSettings {
    bool enabled = false;
    int theme = 0; // 0=system/light, 1=dark for the shortcut settings panel.
    // v4: legacy shortcut points remain user-supplied; THĐC gates have measured defaults.
    int kunlunNpcX = 0, kunlunNpcY = 0;
    int xaTruyenX = 0, xaTruyenY = 0; // legacy v3 fields: v3.2 runtime NEVER reads these; ID387 uses sellNpcPositions_.
    int ngaiX = 0, ngaiY = 0;
    int tinhTucX = 0, tinhTucY = 0;
    int thanhLienGateX = 0, thanhLienGateY = 0;
    int phamLienGateX = 0, phamLienGateY = 0;
    int khoVinhGateX = 0, khoVinhGateY = 0;
    std::array<TimedClickPoint, 3> kunlunExitClicks{};

    // THĐC coordinates belong to the exact source map containing each gate.
    int thdcEntryX = 8257, thdcEntryY = 148110;             // M10000 -> M10014
    int thdcFloor1UpX = 890, thdcFloor1UpY = 6895;          // M10014 -> M10015
    int thdcFloor2UpX = 3080, thdcFloor2UpY = 2900;         // M10015 -> M10016
    int thdcFloor2DownX = 7450, thdcFloor2DownY = 966;      // M10015 -> M10014
    int thdcFloor3UpX = 4256, thdcFloor3UpY = 7120;         // M10016 -> M10017
    int thdcFloor3DownX = 7620, thdcFloor3DownY = 1242;     // M10016 -> M10015
    int thdcFloor4DownX = 690, thdcFloor4DownY = 7200;      // M10017 -> M10016
};

enum class ShortcutKind : int {
    None = 0,
    KunLunExit = 1,
    KunLunEnter = 2,
    FireEnter = 3,
    FireExit = 4,
    InterserverGate = 5,
    ThdcRoute = 6,
    TravelNetwork = 7,
};

TravelSemantic ToProtocolTravelSemantic(travel_network_logic::Semantic semantic) {
    using S = travel_network_logic::Semantic;
    switch (semantic) {
        case S::NamHai: return TravelSemantic::NamHai;
        case S::MieuCuong: return TravelSemantic::MieuCuong;
        case S::HoangLongPhu: return TravelSemantic::HoangLongPhu;
        case S::ThachLam: return TravelSemantic::ThachLam;
        case S::DaiLy: return TravelSemantic::DaiLy;
        default: return TravelSemantic::None;
    }
}

int AutoSellerPresetForTrainingMap(int mapID) {
    switch (mapID) {
        case 71: case 72: return 2; // Ba Nhĩ
        case 68: case 69: case 73: case 74: case 75: case 76: return 0; // Mã Kiêu Minh
        case 55: case 70: return 1; // Dược Đại Phu
        default: return -1;
    }
}

struct SellMacroStep {
    std::wstring description;
    ClickPoint point{};
    int delayMs = 600;
    int repeat = 1;
};

struct TradeSequenceStep {
    // v0.2.7 child workflow semantics:
    // target=0 => active CON uses this row's own point.
    // target=1 => MAIN executes shared step mainRef from the MAIN common sequence.
    int target = 0;
    int mainRef = -1;
    std::wstring description;
    ClickPoint point{};
    int delayMs = 500;
    int repeat = 1;       // repeat this individual row
    int groupId = 0;      // 0=not grouped; >0=contiguous mini-sequence
    int groupRepeat = 1;  // repeat the whole mini-sequence before continuing
};

enum class RecorderMode : int { None = 0, Sell = 1, TradeMain = 2, TradeChild = 3 };

struct RecordedClick {
    DWORD pid = 0;
    ClickPoint point{};
    DWORD tick = 0;
};

struct SellNpcPreset {
    const wchar_t* name;
    int mapID;
    int npcID;
};

constexpr std::array<SellNpcPreset, 6> kSellNpcs = {{
    {L"Mã Kiêu Minh • M5 • ID 373", 5, 373},
    {L"Dược Đại Phu • Hỏa Diệm Sơn M55 • ID 279", 55, 279},
    {L"Ba Nhĩ • Lâu Lan M5 • ID 328", 5, 328},
    {L"Xa Truyền Bình • Lâu Lan M5 • ID 387", 5, 387},
    {L"Uông Diên • Đôn Hoàng M22 • ID 159", 22, 159},
    {L"A Lạp Bá Nhân • Hỏa Diệm Sơn M55 • ID 275", 55, 275},
}};

struct SellNpcPosition {
    int x = 0;
    int y = 0;
    bool valid = false;
};

struct TargetProfile {
    std::wstring name;
    int mapID = 0;
    int x = 0;
    int y = 0;
    bool valid = false;
};

struct AutoPkClickStep {
    auto_pk_logic::ClickPhase phase = auto_pk_logic::ClickPhase::Normal;
    std::wstring description;
    ClickPoint point{};
    int delayMs = 500;
    int repeat = 1;
};

struct AutoPkStep {
    bool enabled = true;
    auto_pk_logic::StepKind kind = auto_pk_logic::StepKind::Custom;
    std::wstring description;
    TargetProfile target{};
    int tolerance = 120;
    int delayMs = 500;
    int repeat = 1;
    std::vector<AutoPkClickStep> clicks{};
};

struct AutoPkAccountRuntime {
    bool active = false;
    bool stepDone = false;
    bool preDone = false;
    bool arrived = false;
    bool postDone = false;
    bool sawDead = false;
    DWORD dueTick = 0;
    DWORD lastReviveTick = 0;
    std::size_t clickIndex = 0;
    int clickRepeatDone = 0;
    int stepRepeatDone = 0;
    int treatmentStage = 0;
    int treatmentCloseAttempts = 0;
    int postVerifyAttempts = 0;
};


struct DungeonTeamRuntime {
    cleanroute_dungeon::TeamConfig config{};
    // Stable identities persist across client restarts; pids are rebound after each scan.
    std::vector<std::int32_t> memberRoleIDs{};
    std::int32_t leaderRoleID = 0;
    cleanroute_dungeon::TeamState state = cleanroute_dungeon::TeamState::Stopped;
    cleanroute_dungeon::TeamPhase phase = cleanroute_dungeon::TeamPhase::Precheck;
    int runIndex = 1;
    int stepIndex = 0;
    bool fightStopPending = false;
    bool bindingError = false;
    int kills = 0;
    DWORD phaseTick = 0;
    DWORD dueTick = 0;
    DWORD lastScanTick = 0;
    DWORD lastProgressTick = 0;
    DWORD lastProgressAttemptTick = 0;
    DWORD serverSyncWaitTick = 0;
    bool progressReadOk = false;
    DungeonProgressSnapshot progressSnapshot{};
    int boundTaskID = 0;
    std::wstring boundTaskName{};
    std::set<int> boundTaskParameterKeys{};
    std::map<int, int> taskParameterBaselines{};
    int taskStepProgress = 0;
    int npcAttempts = 0;
    int dialogAttempts = 0;
    bool seenMatchingAlive = false;
    cleanroute_dungeon::DeathTracker deathTracker{};
    cleanroute_dungeon::Preset activePreset{};
    bool activePresetValid = false;
    std::set<std::uint32_t> postSellDone{};
    std::set<std::wstring> loggedDiagnostics{};
    std::wstring status = L"STOP";
};

struct TelegramSettings {
    bool enabled = false;
    std::wstring botToken{};
    std::wstring chatId{};
    bool notifyDeath = true;
    bool notifyRevive = true;
    bool notifySellComplete = false; // intentionally OFF by default to avoid spam.
    bool notifySellSummary = true;
    bool notifyTradeComplete = true;
    bool notifyClientFreeze = true;
    bool notifyFifoEnter = false;
    bool notifyLauLanConfirm = false;
    bool notifyWorldFlowTimeout = true;
    bool notifyToolState = false;
    bool notifySessionSummary = true;
    bool notifyFunAlerts = true;
    // Currency milestones are independently selectable. 1m/5m are test-friendly and OFF by default.
    std::array<bool, 5> currencyMilestones{false, false, true, true, true};
    bool intervalEnabled = true;
    int intervalMinutes = 60;
    bool dailyEnabled = true;
    std::array<std::wstring, 4> dailyTimes{L"08:00", L"12:00", L"18:00", L"23:00"};
    int worldFlowTimeoutSec = 120;
};

struct TelegramStats {
    bool active = false;
    SYSTEMTIME startedLocal{};
    ULONGLONG startedTick = 0;
    int sellTotal = 0;
    int tradeTotal = 0;
    int deathTotal = 0;
    int reviveTotal = 0;
    int fifoTotal = 0;
    int lauLanConfirmTotal = 0;
    int clientFreezeTotal = 0;
    int worldFlowTimeoutTotal = 0;
    std::map<DWORD, int> sellsByPid{};
    std::map<DWORD, int> tradesByChildPid{};
};

struct TelegramReportBaseline {
    int sellTotal = 0;
    int tradeTotal = 0;
    int deathTotal = 0;
    int reviveTotal = 0;
    int fifoTotal = 0;
    int lauLanConfirmTotal = 0;
    int clientFreezeTotal = 0;
    int worldFlowTimeoutTotal = 0;
    std::map<DWORD, int> sellsByPid{};
    std::map<DWORD, int> tradesByChildPid{};
};

struct TelegramAccountWatch {
    bool lifeKnown = false;
    bool lastDead = false;
    DWORD deathStartedTick = 0;
    std::deque<DWORD> recentDeathTicks{};
    DWORD deathBurstLastAlertTick = 0;
    DWORD autoTrainOffStartedTick = 0;
    bool autoTrainOffAlertSent = false;
    DWORD currencyNextReadTick = 0;
    ULONGLONG currencyBaselineTick = 0;
    unsigned currencyMilestoneMask = 0;
    bool moneyKnown = false;
    bool boundMoneyKnown = false;
    bool moneyBaselineKnown = false;
    bool boundMoneyBaselineKnown = false;
    std::int64_t money = 0;
    std::int64_t boundMoney = 0;
    std::int64_t moneyBaseline = 0;
    std::int64_t boundMoneyBaseline = 0;
    std::wstring boundMoneyBaselineTime{};
    std::uint64_t lastWorkflowTicket = 0;
    DWORD worldFlowTravelStartedTick = 0;
    bool worldFlowTimeoutSent = false;
    bool criticalFreezeNotified = false;
};

struct AccountProfile {
    std::wstring section;
    // 0=NONE, 1=MAIN, 2..13=CON1..CON12. Persisted by RoleID profile.
    int tradeRole = 0;
    // Per-CON release condition: after each full trade click sequence, keep the same
    // child in the workflow until FreeBagSpace reaches this target. Default = 30.
    std::wstring selectedSpot;
    int tolerance = 120;
    bool enableRevive = true;
    bool enableConfirm = true;
    bool enableFight = true;
    bool enableSell = false;
    int sellNpcPreset = 0;
    std::vector<std::wstring> rotationSpots{};
    int rotateDeathLimit = kRotateDeathLimitDefault;
    int rotateDeathWindowMin = kRotateDeathWindowMinDefault;
    int rotateNoFullBagMin = kRotateNoFullBagMinDefault;
    TargetProfile target{};
    std::array<ClickPoint, 5> points{};
    std::vector<SellMacroStep> sellMacro{};
    // Legacy v0.2.3-v0.2.6 per-CON workflow kept only for one-time v0.2.7 migration.
    // Active v0.2.7 uses one global childTradeSequence_ shared by every CON.
    std::vector<TradeSequenceStep> childTradeSequence{};
};

struct GameClient {
    DWORD pid = 0;
    DWORD threadId = 0;
    HWND window = nullptr;
    std::wstring title;
};


struct InventoryBagRow {
    inventory_filter_logic::ItemView item{};
    std::wstring name{};
    std::wstring equipType{};
};

struct RuntimeState {
    bool running = false;
    std::wstring status = L"Đã dừng";
    int qualifiedMap = 0;
    int candidateMap = 0;
    int candidateCount = 0;
    DWORD lastActionTick = 0;
    Action lastAction = Action::Wait;
    DWORD lastStartPathPassTick = 0;

    DWORD deadSinceTick = 0;
    int revivePhase = 0;
    DWORD revivePhaseTick = 0;
    DWORD lastReviveClickTick = 0;

    int lastObservedMap = 0;
    int lastObservedX = 0;
    int lastObservedY = 0;
    DWORD lastMovementTick = 0;
    bool crossMapSeenAutoPath = false;
    DWORD stallSinceTick = 0;
    int confirmAttempts = 0;
    DWORD lastLauLanConfirmTick = 0;

    int fightPhase = 0;
    DWORD fightPhaseTick = 0;
    int fightAttempts = 0;
    DWORD fightRetryWaitTick = 0;
    bool wasAtTarget = false;

    // Once AUTO fight is confirmed at the training spot, position is intentionally
    // checked every 1 minute. Death/bag state are still observed every tick.
    bool trainPositionMonitorArmed = false;
    DWORD lastTrainPositionCheckTick = 0;
    DWORD lastAutoFightCheckTick = 0;
    int trainRecoveryPhase = 0;

    // Dedicated trade-rendezvous state. AutoFight stopping is no longer duplicated here;
    // every StartPath is protected by the shared v0.3 AutoFight Travel Guard.
    int tradeTravelPhase = 0;
    DWORD tradeTravelTick = 0;
    bool tradeTravelReady = false;
    std::uint64_t tradeWorkflowEntrySeq = 0; // R7: immutable FIFO ticket while staged in workflow.

    // Priority-AUTO request/result mailbox. v0.6.1.6 dispatches configured points
    // through InputSyncManager. An Attack request owns both AUTO then ĐÁNH QUÁI
    // phases and publishes one result only after click 2 finishes.
    ClickSlot priorityAutoRequestSlot = ClickSlot::None;
    PriorityAutoOwner priorityAutoRequestOwner = PriorityAutoOwner::None;
    ClickSlot priorityAutoCompletedSlot = ClickSlot::None;
    PriorityAutoOwner priorityAutoCompletedOwner = PriorityAutoOwner::None;
    bool priorityAutoCompletedOk = false;
    DWORD priorityAutoCompletedTick = 0;
    int priorityAutoPointPhase = 0;
    DWORD priorityAutoPointTick = 0;

    // v0.3 shared AutoFight Travel Guard. Any StartPath must prove authoritative
    // AutoFight OFF. Two failed stop cycles trigger AUTO->Attack reset, then stop retries.
    int travelFightGuardPhase = 0;
    DWORD travelFightGuardTick = 0;
    int travelFightStopAttempts = 0;

    // Hard runtime invariant. If a snapshot ever exposes AutoPath=ON together
    // with AutoFight=ON, stop the path first, finish the normal two-stop/reset
    // Travel Guard, and require both states OFF before any route may resume.
    bool autoPathFightConflictLatched = false;
    DWORD autoPathFightConflictTick = 0;
    int autoPathFightConflictStopAttempts = 0;

    // Shared robust-travel helper: mount x2 -> fight 10s -> stop fight -> mount x2.
    // If still not mounted, reset and repeat; StartPath on foot is forbidden.
    int travelMountAttempts = 0;
    DWORD travelMountTick = 0;
    int travelMountCycle = 0; // 0=before fight boost, 1=after 10s fight boost
    int travelFightBoostPhase = 0;
    DWORD travelFightBoostTick = 0;
    bool travelFootFallback = false;
    DWORD travelFootTick = 0;

    bool crossMapRouteArmed = false;
    bool crossMapRouteMoved = false;

    // Optional user-enabled shortcut router. It wraps the existing robust travel helper;
    // unrelated travel remains unchanged when shortcutKind=None or the global checkbox is off.
    ShortcutKind shortcutKind = ShortcutKind::None;
    int shortcutPhase = 0;
    int shortcutFinalMap = 0;
    int shortcutExpectedMap = 0;
    int shortcutSourceMap = 0;
    int shortcutClickIndex = 0;
    DWORD shortcutTick = 0;
    int shortcutAttempts = 0;

    int sellPhase = 0;
    DWORD sellPhaseTick = 0;
    int sellOpenAttempts = 0;
    int sellMacroIndex = 0;
    int sellMacroRepeatDone = 0;
    DWORD sellMacroNextTick = 0;
    DWORD sellMacroCompletionDueTick = 0; // R6: keep SELL macro completion state through final configured delay.
    int sellMacroPass = 0;
    int sellLastFreeBag = -1;
    DWORD sellBagStableSince = 0;

    // v1.3 semantic bag-filter mutation gate. One current instance per account at a time.
    std::int64_t bagFilterPendingInstance = 0;
    DWORD bagFilterPendingTick = 0;

    // Global per-PID transition/unresponsive safety gate. While active, no mutable
    // gameplay/window action may be dispatched. Read-only state polling continues
    // until the client is continuously healthy for kClientStableResumeMs.
    bool clientFreezeActive = false;
    DWORD clientFreezeSinceTick = 0;
    DWORD clientStableSinceTick = 0;
    int readStateFailStreak = 0;
    DWORD lastReadFailureLogTick = 0;

    // Map 87 = Địa Phủ uses the same v0.3 AutoFight Travel Guard as every other
    // movement flow. This flag is log-only; there is no separate M87 stop state machine.
    bool underworldGuardLogged = false;

    // A tool-runtime reset does not stop the game client's real AutoPath. After a
    // fresh Start or revive cold-start, reacquire route ownership by forcing any
    // stale AutoPath OFF and verifying it before a new StartPath may arm Confirm.
    bool routeOwnershipResetPending = false;
    DWORD routeOwnershipStopTick = 0;
    int routeOwnershipStopAttempts = 0;
    bool routeOwnershipResetLogged = false;
};

template <typename T>
bool ResolveProc(HMODULE module, const char* name, T& out) {
    out = nullptr;
    FARPROC raw = GetProcAddress(module, name);
    if (!raw) return false;
    static_assert(sizeof(raw) == sizeof(out), "pointer size mismatch");
    std::memcpy(&out, &raw, sizeof(out));
    return out != nullptr;
}

std::wstring ExeDir() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, _countof(path));
    if (wchar_t* slash = wcsrchr(path, L'\\')) *slash = 0;
    return path;
}

std::wstring LegacyConfigPath() { return ExeDir() + L"\\ThanLongCleanRoute.accounts.ini"; }

std::wstring ConfigDir() {
    wchar_t localAppData[4096]{};
    const DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, _countof(localAppData));
    if (n > 0 && n < _countof(localAppData)) {
        std::wstring dir = std::wstring(localAppData) + L"\\ThanLongCleanRoute";
        (void)CreateDirectoryW(dir.c_str(), nullptr);
        return dir;
    }
    return ExeDir();
}

std::wstring ConfigPath() {
    static const std::wstring path = ConfigDir() + L"\\ThanLongCleanRoute.accounts.ini";
    return path;
}

void MigrateLegacyConfigIfNeeded() {
    const std::wstring current = ConfigPath();
    const std::wstring legacy = LegacyConfigPath();
    if (current == legacy) return;
    if (GetFileAttributesW(current.c_str()) != INVALID_FILE_ATTRIBUTES) return;
    if (GetFileAttributesW(legacy.c_str()) == INVALID_FILE_ATTRIBUTES) return;
    (void)CopyFileW(legacy.c_str(), current.c_str(), TRUE);
}

void FlushIni() {
    (void)WritePrivateProfileStringW(nullptr, nullptr, nullptr, ConfigPath().c_str());
}

void EnsureUnicodeIni() {
    const std::wstring path = ConfigPath();
    if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) return;
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    const BYTE bom[2] = {0xFF, 0xFE};
    DWORD done = 0;
    (void)WriteFile(h, bom, 2, &done, nullptr);
    CloseHandle(h);
}

int ReadIniInt(const std::wstring& section, const std::wstring& key, int fallback) {
    return static_cast<int>(GetPrivateProfileIntW(section.c_str(), key.c_str(), fallback, ConfigPath().c_str()));
}

void WriteIniInt(const std::wstring& section, const std::wstring& key, int value) {
    wchar_t text[32]{};
    wsprintfW(text, L"%d", value);
    WritePrivateProfileStringW(section.c_str(), key.c_str(), text, ConfigPath().c_str());
}

std::wstring ReadIniText(const std::wstring& section, const std::wstring& key) {
    wchar_t text[512]{};
    GetPrivateProfileStringW(section.c_str(), key.c_str(), L"", text, _countof(text), ConfigPath().c_str());
    return text;
}

void WriteIniText(const std::wstring& section, const std::wstring& key, const std::wstring& value) {
    WritePrivateProfileStringW(section.c_str(), key.c_str(), value.c_str(), ConfigPath().c_str());
}

std::wstring Utf8ToWide(const std::string& input);
std::string WideToUtf8(const std::wstring& input);

std::wstring EscapePortableField(const std::wstring& input) {
    std::wstring out;
    out.reserve(input.size());
    for (wchar_t ch : input) {
        if (ch == L'\\') out += L"\\\\";
        else if (ch == L'\t') out += L"\\t";
        else if (ch == L'\r') out += L"\\r";
        else if (ch == L'\n') out += L"\\n";
        else out += ch;
    }
    return out;
}

bool UnescapePortableField(const std::wstring& input, std::wstring& out) {
    out.clear(); out.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        wchar_t ch = input[i];
        if (ch != L'\\') { out += ch; continue; }
        if (++i >= input.size()) return false;
        const wchar_t e = input[i];
        if (e == L'\\') out += L'\\';
        else if (e == L't') out += L'\t';
        else if (e == L'r') out += L'\r';
        else if (e == L'n') out += L'\n';
        else return false;
    }
    return true;
}

std::vector<std::wstring> SplitPortableLine(const std::wstring& line) {
    std::vector<std::wstring> fields;
    std::size_t start = 0;
    while (true) {
        const std::size_t pos = line.find(L'\t', start);
        fields.push_back(line.substr(start, pos == std::wstring::npos ? std::wstring::npos : pos - start));
        if (pos == std::wstring::npos) break;
        start = pos + 1;
    }
    return fields;
}

bool ParsePortableInt(const std::wstring& text, int& value) {
    if (text.empty()) return false;
    wchar_t* end = nullptr;
    const long v = wcstol(text.c_str(), &end, 10);
    if (!end || *end != 0 || v < INT_MIN || v > INT_MAX) return false;
    value = static_cast<int>(v);
    return true;
}

bool WriteUtf8File(const std::wstring& path, const std::wstring& text, std::wstring& error) {
    const std::string bytes = WideToUtf8(text);
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) { error = L"Không tạo được file • Win32=" + std::to_wstring(GetLastError()); return false; }
    DWORD written = 0;
    const bool ok = bytes.empty() || (WriteFile(h, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) && written == bytes.size());
    const DWORD last = ok ? ERROR_SUCCESS : GetLastError();
    CloseHandle(h);
    if (!ok) { error = L"Ghi file không đủ dữ liệu • Win32=" + std::to_wstring(last); return false; }
    return true;
}

bool ReadUtf8File(const std::wstring& path, std::wstring& text, std::wstring& error) {
    text.clear();
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) { error = L"Không mở được file • Win32=" + std::to_wstring(GetLastError()); return false; }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(h, &size) || size.QuadPart < 0 || size.QuadPart > 8 * 1024 * 1024) {
        CloseHandle(h); error = L"File cấu hình quá lớn/không hợp lệ"; return false;
    }
    std::string bytes(static_cast<std::size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    const bool ok = bytes.empty() || (ReadFile(h, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) && read == bytes.size());
    const DWORD last = ok ? ERROR_SUCCESS : GetLastError();
    CloseHandle(h);
    if (!ok) { error = L"Đọc file thất bại • Win32=" + std::to_wstring(last); return false; }
    if (bytes.size() >= 3 && static_cast<unsigned char>(bytes[0]) == 0xEF && static_cast<unsigned char>(bytes[1]) == 0xBB && static_cast<unsigned char>(bytes[2]) == 0xBF)
        bytes.erase(0, 3);
    text = Utf8ToWide(bytes);
    if (!bytes.empty() && text.empty()) { error = L"File không phải UTF-8 hợp lệ"; return false; }
    return true;
}

bool PickPortableConfigPath(HWND owner, bool save, std::wstring& path) {
    wchar_t file[4096]{};
    if (save) wcscpy_s(file, _countof(file), L"ThanLong-click-config.tlcfg");
    const wchar_t filter[] = L"Thần Long click config (*.tlcfg)\0*.tlcfg\0Tất cả file (*.*)\0*.*\0\0";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = owner; ofn.lpstrFilter = filter;
    ofn.lpstrFile = file; ofn.nMaxFile = _countof(file); ofn.lpstrDefExt = L"tlcfg";
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    const BOOL ok = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    if (!ok) return false;
    path = file; return true;
}

bool PickMasterConfigPath(HWND owner, bool save, std::wstring& path) {
    wchar_t file[4096]{};
    if (save) wcscpy_s(file, _countof(file), L"ThanLong-all-coordinates.tlmaster");
    const wchar_t filter[] = L"Thần Long ALL config (*.tlmaster)\0*.tlmaster\0Tất cả file (*.*)\0*.*\0\0";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = owner; ofn.lpstrFilter = filter;
    ofn.lpstrFile = file; ofn.nMaxFile = _countof(file); ofn.lpstrDefExt = L"tlmaster";
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    const BOOL ok = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    if (!ok) return false;
    path = file; return true;
}

bool PickMapConfigPath(HWND owner, bool save, std::wstring& path) {
    wchar_t file[4096]{};
    if (save) wcscpy_s(file, _countof(file), L"ThanLong-map-config.tlmap");
    const wchar_t filter[] = L"Thần Long map config (*.tlmap)\0*.tlmap\0Tất cả file (*.*)\0*.*\0\0";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = owner; ofn.lpstrFilter = filter;
    ofn.lpstrFile = file; ofn.nMaxFile = _countof(file); ofn.lpstrDefExt = L"tlmap";
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    const BOOL ok = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    if (!ok) return false;
    path = file; return true;
}


inventory_filter_logic::Settings LoadInventoryFilterSettings() {
    inventory_filter_logic::Settings f{};
    const std::wstring section = L"InventoryFilter";
    f.enabled = ReadIniInt(section, L"Enabled", 0) != 0;
    f.protectBound = ReadIniInt(section, L"ProtectBound", 1) != 0;
    f.keepWeapons = ReadIniInt(section, L"KeepWeapons", 1) != 0;
    f.dropEquipNonWeapon = ReadIniInt(section, L"DropEquipNonWeapon", 0) != 0;
    f.sellEquipNonWeapon = ReadIniInt(section, L"SellEquipNonWeapon", 0) != 0;
    f.dropCommon = ReadIniInt(section, L"DropCommon", 0) != 0;
    f.sellCommon = ReadIniInt(section, L"SellCommon", 0) != 0;
    f.dropGem = ReadIniInt(section, L"DropGem", 0) != 0;
    f.sellGem = ReadIniInt(section, L"SellGem", 0) != 0;
    f.dropMedicine = ReadIniInt(section, L"DropMedicine", 0) != 0;
    f.sellMedicine = ReadIniInt(section, L"SellMedicine", 0) != 0;
    f.dropPetEquip = ReadIniInt(section, L"DropPetEquip", 0) != 0;
    f.sellPetEquip = ReadIniInt(section, L"SellPetEquip", 0) != 0;
    const int count = std::clamp(ReadIniInt(section, L"RuleCount", 0), 0, 512);
    for (int i = 0; i < count; ++i) {
        const std::wstring prefix = L"Rule_" + std::to_wstring(i) + L"_";
        const int itemID = ReadIniInt(section, prefix + L"ItemID", 0);
        const int action = ReadIniInt(section, prefix + L"Action", 0);
        if (itemID <= 0 || action < 0 || action > 2) continue;
        inventory_filter_logic::ItemRule r{};
        r.itemID = itemID; r.action = static_cast<RuleAction>(action); r.name = ReadIniText(section, prefix + L"Name");
        f.rules.push_back(std::move(r));
    }
    return f;
}

void SaveInventoryFilterSettings(const inventory_filter_logic::Settings& f) {
    EnsureUnicodeIni();
    const std::wstring section = L"InventoryFilter";
    const int oldCount = std::clamp(ReadIniInt(section, L"RuleCount", 0), 0, 512);
    WriteIniInt(section, L"Enabled", f.enabled ? 1 : 0);
    WriteIniInt(section, L"ProtectBound", f.protectBound ? 1 : 0);
    WriteIniInt(section, L"KeepWeapons", f.keepWeapons ? 1 : 0);
    WriteIniInt(section, L"DropEquipNonWeapon", f.dropEquipNonWeapon ? 1 : 0);
    WriteIniInt(section, L"SellEquipNonWeapon", f.sellEquipNonWeapon ? 1 : 0);
    WriteIniInt(section, L"DropCommon", f.dropCommon ? 1 : 0);
    WriteIniInt(section, L"SellCommon", f.sellCommon ? 1 : 0);
    WriteIniInt(section, L"DropGem", f.dropGem ? 1 : 0);
    WriteIniInt(section, L"SellGem", f.sellGem ? 1 : 0);
    WriteIniInt(section, L"DropMedicine", f.dropMedicine ? 1 : 0);
    WriteIniInt(section, L"SellMedicine", f.sellMedicine ? 1 : 0);
    WriteIniInt(section, L"DropPetEquip", f.dropPetEquip ? 1 : 0);
    WriteIniInt(section, L"SellPetEquip", f.sellPetEquip ? 1 : 0);
    WriteIniInt(section, L"RuleCount", static_cast<int>(f.rules.size()));
    for (std::size_t i = 0; i < f.rules.size(); ++i) {
        const auto& r = f.rules[i]; const std::wstring prefix = L"Rule_" + std::to_wstring(i) + L"_";
        WriteIniInt(section, prefix + L"ItemID", r.itemID);
        WriteIniInt(section, prefix + L"Action", static_cast<int>(r.action));
        WriteIniText(section, prefix + L"Name", r.name);
    }
    for (int i = static_cast<int>(f.rules.size()); i < oldCount; ++i) {
        const std::wstring prefix = L"Rule_" + std::to_wstring(i) + L"_";
        WritePrivateProfileStringW(section.c_str(), (prefix + L"ItemID").c_str(), nullptr, ConfigPath().c_str());
        WritePrivateProfileStringW(section.c_str(), (prefix + L"Action").c_str(), nullptr, ConfigPath().c_str());
        WritePrivateProfileStringW(section.c_str(), (prefix + L"Name").c_str(), nullptr, ConfigPath().c_str());
    }
    FlushIni();
}

TelegramSettings LoadTelegramSettings(std::wstring& warning) {
    TelegramSettings t{};
    const std::wstring section = L"Telegram";
    t.enabled = ReadIniInt(section, L"Enabled", 0) != 0;
    t.chatId = ReadIniText(section, L"ChatId");
    t.notifyDeath = ReadIniInt(section, L"NotifyDeath", 1) != 0;
    t.notifyRevive = ReadIniInt(section, L"NotifyRevive", 1) != 0;
    t.notifySellComplete = ReadIniInt(section, L"NotifySellComplete", 0) != 0;
    t.notifySellSummary = ReadIniInt(section, L"NotifySellSummary", 1) != 0;
    t.notifyTradeComplete = ReadIniInt(section, L"NotifyTradeComplete", 1) != 0;
    t.notifyClientFreeze = ReadIniInt(section, L"NotifyClientFreeze", 1) != 0;
    t.notifyFifoEnter = ReadIniInt(section, L"NotifyFifoEnter", 0) != 0;
    t.notifyLauLanConfirm = ReadIniInt(section, L"NotifyLauLanConfirm", 0) != 0;
    t.notifyWorldFlowTimeout = ReadIniInt(section, L"NotifyWorldFlowTimeout", 1) != 0;
    t.notifyToolState = ReadIniInt(section, L"NotifyToolState", 0) != 0;
    t.notifySessionSummary = ReadIniInt(section, L"NotifySessionSummary", 1) != 0;
    t.notifyFunAlerts = ReadIniInt(section, L"NotifyFunAlerts", 1) != 0;
    t.currencyMilestones[0] = ReadIniInt(section, L"MoneyMilestone1m", 0) != 0;
    t.currencyMilestones[1] = ReadIniInt(section, L"MoneyMilestone5m", 0) != 0;
    t.currencyMilestones[2] = ReadIniInt(section, L"MoneyMilestone60m", 1) != 0;
    t.currencyMilestones[3] = ReadIniInt(section, L"MoneyMilestone6h", 1) != 0;
    t.currencyMilestones[4] = ReadIniInt(section, L"MoneyMilestone24h", 1) != 0;
    t.intervalEnabled = ReadIniInt(section, L"IntervalEnabled", 1) != 0;
    t.intervalMinutes = telegram_logic::ClampSummaryIntervalMinutes(ReadIniInt(section, L"IntervalMinutes", 60));
    t.dailyEnabled = ReadIniInt(section, L"DailyEnabled", 1) != 0;
    const std::array<std::wstring, 4> defaults{L"08:00", L"12:00", L"18:00", L"23:00"};
    for (int i = 0; i < 4; ++i) {
        const std::wstring raw = ReadIniText(section, L"DailyTime" + std::to_wstring(i + 1));
        t.dailyTimes[static_cast<std::size_t>(i)] = telegram_logic::NormalizeDailyTime(raw.empty() ? defaults[static_cast<std::size_t>(i)] : raw,
                                                                                       defaults[static_cast<std::size_t>(i)]);
    }
    t.worldFlowTimeoutSec = telegram_logic::ClampWorldFlowTimeoutSeconds(ReadIniInt(section, L"WorldFlowTimeoutSec", 120));

    const std::wstring protectedToken = ReadIniText(section, L"BotTokenProtected");
    if (!protectedToken.empty()) {
        std::wstring error;
        if (!telegram_notify::UnprotectTokenForCurrentUser(protectedToken, t.botToken, error)) {
            warning = L"Telegram Bot Token đã lưu không giải mã được bằng Windows DPAPI; Telegram tạm tắt. " + error;
            t.enabled = false;
            t.botToken.clear();
        }
    }
    return t;
}

bool SaveTelegramSettings(const TelegramSettings& t, std::wstring& error) {
    EnsureUnicodeIni();
    const std::wstring section = L"Telegram";
    std::wstring protectedToken;
    if (!telegram_notify::ProtectTokenForCurrentUser(t.botToken, protectedToken, error)) return false;
    WriteIniInt(section, L"Enabled", t.enabled ? 1 : 0);
    WriteIniText(section, L"BotTokenProtected", protectedToken);
    // Clean any accidental/plain legacy key if it ever existed during development.
    WritePrivateProfileStringW(section.c_str(), L"BotToken", nullptr, ConfigPath().c_str());
    WriteIniText(section, L"ChatId", t.chatId);
    WriteIniInt(section, L"NotifyDeath", t.notifyDeath ? 1 : 0);
    WriteIniInt(section, L"NotifyRevive", t.notifyRevive ? 1 : 0);
    WriteIniInt(section, L"NotifySellComplete", t.notifySellComplete ? 1 : 0);
    WriteIniInt(section, L"NotifySellSummary", t.notifySellSummary ? 1 : 0);
    WriteIniInt(section, L"NotifyTradeComplete", t.notifyTradeComplete ? 1 : 0);
    WriteIniInt(section, L"NotifyClientFreeze", t.notifyClientFreeze ? 1 : 0);
    WriteIniInt(section, L"NotifyFifoEnter", t.notifyFifoEnter ? 1 : 0);
    WriteIniInt(section, L"NotifyLauLanConfirm", t.notifyLauLanConfirm ? 1 : 0);
    WriteIniInt(section, L"NotifyWorldFlowTimeout", t.notifyWorldFlowTimeout ? 1 : 0);
    WriteIniInt(section, L"NotifyToolState", t.notifyToolState ? 1 : 0);
    WriteIniInt(section, L"NotifySessionSummary", t.notifySessionSummary ? 1 : 0);
    WriteIniInt(section, L"NotifyFunAlerts", t.notifyFunAlerts ? 1 : 0);
    WriteIniInt(section, L"MoneyMilestone1m", t.currencyMilestones[0] ? 1 : 0);
    WriteIniInt(section, L"MoneyMilestone5m", t.currencyMilestones[1] ? 1 : 0);
    WriteIniInt(section, L"MoneyMilestone60m", t.currencyMilestones[2] ? 1 : 0);
    WriteIniInt(section, L"MoneyMilestone6h", t.currencyMilestones[3] ? 1 : 0);
    WriteIniInt(section, L"MoneyMilestone24h", t.currencyMilestones[4] ? 1 : 0);
    WriteIniInt(section, L"IntervalEnabled", t.intervalEnabled ? 1 : 0);
    WriteIniInt(section, L"IntervalMinutes", telegram_logic::ClampSummaryIntervalMinutes(t.intervalMinutes));
    WriteIniInt(section, L"DailyEnabled", t.dailyEnabled ? 1 : 0);
    for (int i = 0; i < 4; ++i) WriteIniText(section, L"DailyTime" + std::to_wstring(i + 1), t.dailyTimes[static_cast<std::size_t>(i)]);
    WriteIniInt(section, L"WorldFlowTimeoutSec", telegram_logic::ClampWorldFlowTimeoutSeconds(t.worldFlowTimeoutSec));
    FlushIni();
    return true;
}

ShortcutSettings LoadShortcutSettings() {
    ShortcutSettings sc{};
    const std::wstring section = L"Shortcut";
    sc.enabled = ReadIniInt(section, L"Enabled", 0) != 0;
    sc.theme = std::clamp(ReadIniInt(section, L"Theme", 0), 0, 1);
    const int coordinateVersion = ReadIniInt(section, L"CoordinateVersion", 0);
    if (coordinateVersion >= 3) {
        sc.kunlunNpcX = ReadIniInt(section, L"KunLunNpcX", 0); sc.kunlunNpcY = ReadIniInt(section, L"KunLunNpcY", 0);
        sc.xaTruyenX = ReadIniInt(section, L"XaTruyenX", 0); sc.xaTruyenY = ReadIniInt(section, L"XaTruyenY", 0);
        sc.ngaiX = ReadIniInt(section, L"NgaiNiNgoaNhiX", 0); sc.ngaiY = ReadIniInt(section, L"NgaiNiNgoaNhiY", 0);
        sc.tinhTucX = ReadIniInt(section, L"TinhTucX", 0); sc.tinhTucY = ReadIniInt(section, L"TinhTucY", 0);
        sc.thanhLienGateX = ReadIniInt(section, L"ThanhLienGateX", 0); sc.thanhLienGateY = ReadIniInt(section, L"ThanhLienGateY", 0);
        sc.phamLienGateX = ReadIniInt(section, L"PhamLienGateX", 0); sc.phamLienGateY = ReadIniInt(section, L"PhamLienGateY", 0);
        sc.khoVinhGateX = ReadIniInt(section, L"KhoVinhGateX", 0); sc.khoVinhGateY = ReadIniInt(section, L"KhoVinhGateY", 0);
    }
    if (coordinateVersion >= 4) {
        for (std::size_t i = 0; i < sc.kunlunExitClicks.size(); ++i) {
            const std::wstring prefix = L"KunLunExitClick" + std::to_wstring(i) + L"_";
            TimedClickPoint& click = sc.kunlunExitClicks[i];
            click.point.x = ReadIniInt(section, prefix + L"X", -1);
            click.point.y = ReadIniInt(section, prefix + L"Y", -1);
            click.point.baseW = ReadIniInt(section, prefix + L"W", 0);
            click.point.baseH = ReadIniInt(section, prefix + L"H", 0);
            click.point.valid = ReadIniInt(section, prefix + L"Valid", 0) != 0 &&
                                click.point.x >= 0 && click.point.y >= 0 &&
                                click.point.baseW > 0 && click.point.baseH > 0;
            click.timeMs = std::clamp(ReadIniInt(section, prefix + L"TimeMs", 0), 0, 60000);
            click.delayMs = std::clamp(ReadIniInt(section, prefix + L"DelayMs", 2000), 0, 60000);
        }
        sc.thdcEntryX = ReadIniInt(section, L"ThdcEntryX", sc.thdcEntryX);
        sc.thdcEntryY = ReadIniInt(section, L"ThdcEntryY", sc.thdcEntryY);
        sc.thdcFloor1UpX = ReadIniInt(section, L"ThdcFloor1UpX", sc.thdcFloor1UpX);
        sc.thdcFloor1UpY = ReadIniInt(section, L"ThdcFloor1UpY", sc.thdcFloor1UpY);
        sc.thdcFloor2UpX = ReadIniInt(section, L"ThdcFloor2UpX", sc.thdcFloor2UpX);
        sc.thdcFloor2UpY = ReadIniInt(section, L"ThdcFloor2UpY", sc.thdcFloor2UpY);
        sc.thdcFloor2DownX = ReadIniInt(section, L"ThdcFloor2DownX", sc.thdcFloor2DownX);
        sc.thdcFloor2DownY = ReadIniInt(section, L"ThdcFloor2DownY", sc.thdcFloor2DownY);
        sc.thdcFloor3UpX = ReadIniInt(section, L"ThdcFloor3UpX", sc.thdcFloor3UpX);
        sc.thdcFloor3UpY = ReadIniInt(section, L"ThdcFloor3UpY", sc.thdcFloor3UpY);
        sc.thdcFloor3DownX = ReadIniInt(section, L"ThdcFloor3DownX", sc.thdcFloor3DownX);
        sc.thdcFloor3DownY = ReadIniInt(section, L"ThdcFloor3DownY", sc.thdcFloor3DownY);
        sc.thdcFloor4DownX = ReadIniInt(section, L"ThdcFloor4DownX", sc.thdcFloor4DownX);
        sc.thdcFloor4DownY = ReadIniInt(section, L"ThdcFloor4DownY", sc.thdcFloor4DownY);
    } else if (coordinateVersion >= 3) {
        // One-time migration: old opener -> click #1; #2/#3 stay invalid.
        TimedClickPoint& first = sc.kunlunExitClicks[0];
        first.point.x = ReadIniInt(section, L"KunLunOpenClickX", -1);
        first.point.y = ReadIniInt(section, L"KunLunOpenClickY", -1);
        first.point.baseW = ReadIniInt(section, L"KunLunOpenClickW", 0);
        first.point.baseH = ReadIniInt(section, L"KunLunOpenClickH", 0);
        first.point.valid = first.point.x >= 0 && first.point.y >= 0 &&
                            first.point.baseW > 0 && first.point.baseH > 0;
    }
    // CoordinateVersion < 3 is intentionally discarded: incompatible coordinate system.
    return sc;
}

void SaveShortcutSettings(const ShortcutSettings& sc) {
    EnsureUnicodeIni();
    const std::wstring section = L"Shortcut";
    WriteIniInt(section, L"Enabled", sc.enabled ? 1 : 0); WriteIniInt(section, L"Theme", sc.theme);
    WriteIniInt(section, L"CoordinateVersion", 4);
    WriteIniInt(section, L"KunLunNpcX", sc.kunlunNpcX); WriteIniInt(section, L"KunLunNpcY", sc.kunlunNpcY);
    WriteIniInt(section, L"XaTruyenX", sc.xaTruyenX); WriteIniInt(section, L"XaTruyenY", sc.xaTruyenY);
    WriteIniInt(section, L"NgaiNiNgoaNhiX", sc.ngaiX); WriteIniInt(section, L"NgaiNiNgoaNhiY", sc.ngaiY);
    WriteIniInt(section, L"TinhTucX", sc.tinhTucX); WriteIniInt(section, L"TinhTucY", sc.tinhTucY);
    WriteIniInt(section, L"ThanhLienGateX", sc.thanhLienGateX); WriteIniInt(section, L"ThanhLienGateY", sc.thanhLienGateY);
    WriteIniInt(section, L"PhamLienGateX", sc.phamLienGateX); WriteIniInt(section, L"PhamLienGateY", sc.phamLienGateY);
    WriteIniInt(section, L"KhoVinhGateX", sc.khoVinhGateX); WriteIniInt(section, L"KhoVinhGateY", sc.khoVinhGateY);
    for (std::size_t i = 0; i < sc.kunlunExitClicks.size(); ++i) {
        const std::wstring prefix = L"KunLunExitClick" + std::to_wstring(i) + L"_";
        const TimedClickPoint& click = sc.kunlunExitClicks[i];
        WriteIniInt(section, prefix + L"Valid", click.point.valid ? 1 : 0);
        WriteIniInt(section, prefix + L"X", click.point.valid ? click.point.x : -1);
        WriteIniInt(section, prefix + L"Y", click.point.valid ? click.point.y : -1);
        WriteIniInt(section, prefix + L"W", click.point.valid ? click.point.baseW : 0);
        WriteIniInt(section, prefix + L"H", click.point.valid ? click.point.baseH : 0);
        WriteIniInt(section, prefix + L"TimeMs", std::clamp(click.timeMs, 0, 60000));
        WriteIniInt(section, prefix + L"DelayMs", std::clamp(click.delayMs, 0, 60000));
    }
    WriteIniInt(section, L"ThdcEntryX", sc.thdcEntryX); WriteIniInt(section, L"ThdcEntryY", sc.thdcEntryY);
    WriteIniInt(section, L"ThdcFloor1UpX", sc.thdcFloor1UpX); WriteIniInt(section, L"ThdcFloor1UpY", sc.thdcFloor1UpY);
    WriteIniInt(section, L"ThdcFloor2UpX", sc.thdcFloor2UpX); WriteIniInt(section, L"ThdcFloor2UpY", sc.thdcFloor2UpY);
    WriteIniInt(section, L"ThdcFloor2DownX", sc.thdcFloor2DownX); WriteIniInt(section, L"ThdcFloor2DownY", sc.thdcFloor2DownY);
    WriteIniInt(section, L"ThdcFloor3UpX", sc.thdcFloor3UpX); WriteIniInt(section, L"ThdcFloor3UpY", sc.thdcFloor3UpY);
    WriteIniInt(section, L"ThdcFloor3DownX", sc.thdcFloor3DownX); WriteIniInt(section, L"ThdcFloor3DownY", sc.thdcFloor3DownY);
    WriteIniInt(section, L"ThdcFloor4DownX", sc.thdcFloor4DownX); WriteIniInt(section, L"ThdcFloor4DownY", sc.thdcFloor4DownY);
    for (const wchar_t* key : {L"KunLunOpenClickX", L"KunLunOpenClickY", L"KunLunOpenClickW", L"KunLunOpenClickH"})
        WritePrivateProfileStringW(section.c_str(), key, nullptr, ConfigPath().c_str());
    FlushIni();
}

std::array<SellNpcPosition, kSellNpcs.size()> LoadSharedSellNpcPositions() {
    std::array<SellNpcPosition, kSellNpcs.size()> positions{};
    const std::wstring section = L"SellNpcPositions";
    const int coordinateVersion = ReadIniInt(section, L"CoordinateVersion", 0);
    if (coordinateVersion < 2) return positions; // discard every legacy/default coordinate
    for (std::size_t i = 0; i < kSellNpcs.size(); ++i) {
        const std::wstring prefix = L"SellNpcPos_" + std::to_wstring(i) + L"_";
        SellNpcPosition& pos = positions[i];
        pos.x = ReadIniInt(section, prefix + L"X", -1);
        pos.y = ReadIniInt(section, prefix + L"Y", -1);
        pos.valid = pos.x >= 0 && pos.y >= 0 && ReadIniInt(section, prefix + L"Valid", 0) != 0;
    }
    return positions;
}

void SaveSharedSellNpcPositions(const std::array<SellNpcPosition, kSellNpcs.size()>& positions) {
    EnsureUnicodeIni();
    const std::wstring section = L"SellNpcPositions";
    WriteIniInt(section, L"CoordinateVersion", 2);
    for (std::size_t i = 0; i < kSellNpcs.size(); ++i) {
        const std::wstring prefix = L"SellNpcPos_" + std::to_wstring(i) + L"_";
        const SellNpcPosition& pos = positions[i];
        WriteIniInt(section, prefix + L"X", pos.valid ? pos.x : -1);
        WriteIniInt(section, prefix + L"Y", pos.valid ? pos.y : -1);
        WriteIniInt(section, prefix + L"Valid", pos.valid ? 1 : 0);
    }
    FlushIni();
}

AccountProfile LoadProfile(const std::wstring& section) {
    AccountProfile p{};
    p.section = section;
    p.tradeRole = ReadIniInt(section, L"TradeRole", 0);
    if (p.tradeRole < 0 || p.tradeRole > kLastChildTradeRole) p.tradeRole = 0;
    p.tolerance = ReadIniInt(section, L"Tolerance", 120);
    if (p.tolerance < 20) p.tolerance = 20;
    if (p.tolerance > 2000) p.tolerance = 2000;
    p.enableRevive = ReadIniInt(section, L"EnableRevive", 1) != 0;
    p.enableConfirm = ReadIniInt(section, L"EnableConfirm", 1) != 0;
    p.enableFight = ReadIniInt(section, L"EnableFight", 1) != 0;
    p.enableSell = ReadIniInt(section, L"EnableSell", 0) != 0;
    p.sellNpcPreset = ReadIniInt(section, L"SellNpcPreset", 0);
    if (p.sellNpcPreset < 0 || p.sellNpcPreset >= static_cast<int>(kSellNpcs.size())) p.sellNpcPreset = 0;
    p.selectedSpot = ReadIniText(section, L"SelectedSpot");
    p.rotateDeathLimit = ReadIniInt(section, L"RotateDeathLimit", kRotateDeathLimitDefault);
    if (p.rotateDeathLimit < kRotateDeathLimitMin) p.rotateDeathLimit = kRotateDeathLimitMin;
    if (p.rotateDeathLimit > kRotateDeathLimitMax) p.rotateDeathLimit = kRotateDeathLimitMax;
    p.rotateDeathWindowMin = ReadIniInt(section, L"RotateDeathWindowMin", kRotateDeathWindowMinDefault);
    if (p.rotateDeathWindowMin < kRotateWindowMin) p.rotateDeathWindowMin = kRotateWindowMin;
    if (p.rotateDeathWindowMin > kRotateWindowMax) p.rotateDeathWindowMin = kRotateWindowMax;
    p.rotateNoFullBagMin = ReadIniInt(section, L"RotateNoFullBagMin", kRotateNoFullBagMinDefault);
    if (p.rotateNoFullBagMin < kRotateWindowMin) p.rotateNoFullBagMin = kRotateWindowMin;
    if (p.rotateNoFullBagMin > kRotateWindowMax) p.rotateNoFullBagMin = kRotateWindowMax;
    int rotationCount = ReadIniInt(section, L"RotationCount", 0);
    if (rotationCount < 0) rotationCount = 0;
    if (rotationCount > 64) rotationCount = 64;
    for (int i = 0; i < rotationCount; ++i) {
        std::wstring name = ReadIniText(section, L"RotationSpot_" + std::to_wstring(i));
        if (!name.empty() && std::none_of(p.rotationSpots.begin(), p.rotationSpots.end(), [&](const std::wstring& x){ return _wcsicmp(x.c_str(), name.c_str()) == 0; })) {
            p.rotationSpots.push_back(std::move(name));
        }
    }
    p.target.name = ReadIniText(section, L"TargetName");
    p.target.mapID = ReadIniInt(section, L"TargetMap", 0);
    p.target.x = ReadIniInt(section, L"TargetX", 0);
    p.target.y = ReadIniInt(section, L"TargetY", 0);
    p.target.valid = p.target.mapID > 0 && ReadIniInt(section, L"TargetValid", 0) != 0;
    if (p.selectedSpot.empty() && p.target.valid) p.selectedSpot = p.target.name;
    for (int i : {static_cast<int>(ClickSlot::AutoMenu), static_cast<int>(ClickSlot::Attack), static_cast<int>(ClickSlot::StopAuto2)}) {
        const std::wstring prefix = kClickKeys[static_cast<std::size_t>(i)];
        ClickPoint& c = p.points[static_cast<std::size_t>(i)];
        c.x = ReadIniInt(section, prefix + L"X", -1);
        c.y = ReadIniInt(section, prefix + L"Y", -1);
        c.baseW = ReadIniInt(section, prefix + L"W", 0);
        c.baseH = ReadIniInt(section, prefix + L"H", 0);
        c.valid = c.x >= 0 && c.y >= 0 && c.baseW > 0 && c.baseH > 0;
    }
    int macroCount = ReadIniInt(section, L"SellMacroCount", 0);
    if (macroCount < 0) macroCount = 0;
    if (macroCount > 64) macroCount = 64;
    for (int i = 0; i < macroCount; ++i) {
        SellMacroStep step{};
        const std::wstring prefix = L"Sell_" + std::to_wstring(i) + L"_";
        step.description = ReadIniText(section, prefix + L"Desc");
        step.point.x = ReadIniInt(section, prefix + L"X", -1);
        step.point.y = ReadIniInt(section, prefix + L"Y", -1);
        step.point.baseW = ReadIniInt(section, prefix + L"W", 0);
        step.point.baseH = ReadIniInt(section, prefix + L"H", 0);
        step.point.valid = step.point.x >= 0 && step.point.y >= 0 && step.point.baseW > 0 && step.point.baseH > 0;
        step.delayMs = ReadIniInt(section, prefix + L"Delay", 600);
        if (step.delayMs < 50) step.delayMs = 50;
        if (step.delayMs > 60000) step.delayMs = 60000;
        step.repeat = ReadIniInt(section, prefix + L"Repeat", 1);
        if (step.repeat < 1) step.repeat = 1;
        if (step.repeat > 999) step.repeat = 999;
        p.sellMacro.push_back(step);
    }
    int childTradeCount = ReadIniInt(section, L"ChildTradeCount", 0);
    childTradeCount = std::clamp(childTradeCount, 0, 64);
    for (int i = 0; i < childTradeCount; ++i) {
        TradeSequenceStep step{};
        const std::wstring prefix = L"ChildTrade_" + std::to_wstring(i) + L"_";
        step.target = std::clamp(ReadIniInt(section, prefix + L"Target", 0), 0, 1);
        step.mainRef = ReadIniInt(section, prefix + L"MainRef", -1);
        step.description = ReadIniText(section, prefix + L"Desc");
        step.point.x = ReadIniInt(section, prefix + L"X", -1);
        step.point.y = ReadIniInt(section, prefix + L"Y", -1);
        step.point.baseW = ReadIniInt(section, prefix + L"W", 0);
        step.point.baseH = ReadIniInt(section, prefix + L"H", 0);
        step.point.valid = step.point.x >= 0 && step.point.y >= 0 && step.point.baseW > 0 && step.point.baseH > 0;
        step.delayMs = std::clamp(ReadIniInt(section, prefix + L"Delay", 500), 50, 60000);
        step.repeat = std::clamp(ReadIniInt(section, prefix + L"Repeat", 1), 1, 999);
        step.groupId = std::max(0, ReadIniInt(section, prefix + L"GroupId", 0));
        step.groupRepeat = std::clamp(ReadIniInt(section, prefix + L"GroupRepeat", 1), 1, 999);
        p.childTradeSequence.push_back(step);
    }
    return p;
}

void SaveProfile(const AccountProfile& p) {
    EnsureUnicodeIni();
    WriteIniInt(p.section, L"TradeRole", p.tradeRole);
    WriteIniInt(p.section, L"Tolerance", p.tolerance);
    WriteIniInt(p.section, L"EnableRevive", p.enableRevive ? 1 : 0);
    WriteIniInt(p.section, L"EnableConfirm", p.enableConfirm ? 1 : 0);
    WriteIniInt(p.section, L"EnableFight", p.enableFight ? 1 : 0);
    WriteIniInt(p.section, L"EnableSell", p.enableSell ? 1 : 0);
    WriteIniInt(p.section, L"SellNpcPreset", p.sellNpcPreset);
    WriteIniText(p.section, L"SelectedSpot", p.selectedSpot);
    WriteIniInt(p.section, L"RotateDeathLimit", p.rotateDeathLimit);
    WriteIniInt(p.section, L"RotateDeathWindowMin", p.rotateDeathWindowMin);
    WriteIniInt(p.section, L"RotateNoFullBagMin", p.rotateNoFullBagMin);
    WriteIniInt(p.section, L"RotationCount", static_cast<int>(p.rotationSpots.size()));
    for (std::size_t i = 0; i < p.rotationSpots.size(); ++i) {
        WriteIniText(p.section, L"RotationSpot_" + std::to_wstring(i), p.rotationSpots[i]);
    }
    WriteIniText(p.section, L"TargetName", p.target.name);
    WriteIniInt(p.section, L"TargetMap", p.target.mapID);
    WriteIniInt(p.section, L"TargetX", p.target.x);
    WriteIniInt(p.section, L"TargetY", p.target.y);
    WriteIniInt(p.section, L"TargetValid", p.target.valid ? 1 : 0);
    for (int i : {static_cast<int>(ClickSlot::AutoMenu), static_cast<int>(ClickSlot::Attack), static_cast<int>(ClickSlot::StopAuto2)}) {
        const std::wstring prefix = kClickKeys[static_cast<std::size_t>(i)];
        const ClickPoint& c = p.points[static_cast<std::size_t>(i)];
        WriteIniInt(p.section, prefix + L"X", c.valid ? c.x : -1);
        WriteIniInt(p.section, prefix + L"Y", c.valid ? c.y : -1);
        WriteIniInt(p.section, prefix + L"W", c.valid ? c.baseW : 0);
        WriteIniInt(p.section, prefix + L"H", c.valid ? c.baseH : 0);
    }
    WriteIniInt(p.section, L"SellMacroCount", static_cast<int>(p.sellMacro.size()));
    for (std::size_t i = 0; i < p.sellMacro.size(); ++i) {
        const SellMacroStep& step = p.sellMacro[i];
        const std::wstring prefix = L"Sell_" + std::to_wstring(i) + L"_";
        WriteIniText(p.section, prefix + L"Desc", step.description);
        WriteIniInt(p.section, prefix + L"X", step.point.valid ? step.point.x : -1);
        WriteIniInt(p.section, prefix + L"Y", step.point.valid ? step.point.y : -1);
        WriteIniInt(p.section, prefix + L"W", step.point.valid ? step.point.baseW : 0);
        WriteIniInt(p.section, prefix + L"H", step.point.valid ? step.point.baseH : 0);
        WriteIniInt(p.section, prefix + L"Delay", step.delayMs);
        WriteIniInt(p.section, prefix + L"Repeat", step.repeat);
    }
    WriteIniInt(p.section, L"ChildTradeCount", static_cast<int>(p.childTradeSequence.size()));
    for (std::size_t i = 0; i < p.childTradeSequence.size(); ++i) {
        const TradeSequenceStep& step = p.childTradeSequence[i];
        const std::wstring prefix = L"ChildTrade_" + std::to_wstring(i) + L"_";
        WriteIniInt(p.section, prefix + L"Target", step.target);
        WriteIniInt(p.section, prefix + L"MainRef", step.mainRef);
        WriteIniText(p.section, prefix + L"Desc", step.description);
        WriteIniInt(p.section, prefix + L"X", step.point.valid ? step.point.x : -1);
        WriteIniInt(p.section, prefix + L"Y", step.point.valid ? step.point.y : -1);
        WriteIniInt(p.section, prefix + L"W", step.point.valid ? step.point.baseW : 0);
        WriteIniInt(p.section, prefix + L"H", step.point.valid ? step.point.baseH : 0);
        WriteIniInt(p.section, prefix + L"Delay", step.delayMs);
        WriteIniInt(p.section, prefix + L"Repeat", step.repeat);
        WriteIniInt(p.section, prefix + L"GroupId", step.groupId);
        WriteIniInt(p.section, prefix + L"GroupRepeat", step.groupRepeat);
    }
    FlushIni();
}


std::wstring SpotsPath() { return ExeDir() + L"\\ThanLongCleanRoute.spots.tsv"; }

std::wstring Utf8ToWide(const std::string& input) {
    if (input.empty()) return {};
    const int needed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(),
                                           static_cast<int>(input.size()), nullptr, 0);
    if (needed <= 0) return {};
    std::wstring out(static_cast<std::size_t>(needed), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()),
                        out.data(), needed);
    return out;
}

std::string WideToUtf8(const std::wstring& input) {
    if (input.empty()) return {};
    const int needed = WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()),
                                           nullptr, 0, nullptr, nullptr);
    if (needed <= 0) return {};
    std::string out(static_cast<std::size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), out.data(), needed,
                        nullptr, nullptr);
    return out;
}

std::wstring SanitizeSpotName(std::wstring name) {
    for (wchar_t& c : name) {
        if (c == L'\t' || c == L'\r' || c == L'\n') c = L' ';
    }
    while (!name.empty() && name.front() == L' ') name.erase(name.begin());
    while (!name.empty() && name.back() == L' ') name.pop_back();
    return name;
}

std::vector<std::wstring> SplitSpotLine(const std::wstring& line) {
    wchar_t separator = L'\t';
    if (line.find(L'\t') == std::wstring::npos) {
        if (line.find(L'|') != std::wstring::npos) separator = L'|';
        else if (line.find(L';') != std::wstring::npos) separator = L';';
        else return {};
    }
    std::vector<std::wstring> fields;
    std::size_t start = 0;
    while (start <= line.size()) {
        const std::size_t pos = line.find(separator, start);
        if (pos == std::wstring::npos) {
            fields.push_back(line.substr(start));
            break;
        }
        fields.push_back(line.substr(start, pos - start));
        start = pos + 1;
    }
    return fields;
}

int FindSpotIndex(const std::vector<TargetProfile>& spots, const std::wstring& name);

std::vector<TargetProfile> LoadSharedSpots() {
    std::vector<TargetProfile> out;
    const std::wstring path = SpotsPath();
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return out;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(h, &size) || size.QuadPart <= 0 || size.QuadPart > 4 * 1024 * 1024) {
        CloseHandle(h);
        return out;
    }
    std::string bytes(static_cast<std::size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    if (!ReadFile(h, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr)) {
        CloseHandle(h);
        return out;
    }
    CloseHandle(h);
    bytes.resize(read);

    std::wstring text;
    if (bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0xFF &&
        static_cast<unsigned char>(bytes[1]) == 0xFE) {
        const std::size_t wcharCount = (bytes.size() - 2) / 2;
        text.resize(wcharCount);
        for (std::size_t i = 0; i < wcharCount; ++i) {
            const unsigned char lo = static_cast<unsigned char>(bytes[2 + i * 2]);
            const unsigned char hi = static_cast<unsigned char>(bytes[3 + i * 2]);
            text[i] = static_cast<wchar_t>(lo | (static_cast<unsigned int>(hi) << 8));
        }
    } else {
        if (bytes.size() >= 3 && static_cast<unsigned char>(bytes[0]) == 0xEF &&
            static_cast<unsigned char>(bytes[1]) == 0xBB && static_cast<unsigned char>(bytes[2]) == 0xBF) {
            bytes.erase(0, 3);
        }
        text = Utf8ToWide(bytes);
    }
    std::size_t lineStart = 0;
    while (lineStart <= text.size()) {
        std::size_t lineEnd = text.find(L'\n', lineStart);
        if (lineEnd == std::wstring::npos) lineEnd = text.size();
        std::wstring line = text.substr(lineStart, lineEnd - lineStart);
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        if (!line.empty() && line[0] != L'#') {
            const auto f = SplitSpotLine(line);
            if (f.size() >= 4) {
                TargetProfile t{};
                const bool firstNumeric = !f[0].empty() && (f[0][0] >= L'0' && f[0][0] <= L'9');
                if (firstNumeric) {
                    t.mapID = _wtoi(f[0].c_str());
                    t.x = _wtoi(f[1].c_str());
                    t.y = _wtoi(f[2].c_str());
                    t.name = SanitizeSpotName(f[3]);
                } else {
                    t.name = SanitizeSpotName(f[0]);
                    t.mapID = _wtoi(f[1].c_str());
                    t.x = _wtoi(f[2].c_str());
                    t.y = _wtoi(f[3].c_str());
                }
                t.valid = !t.name.empty() && t.mapID > 0;
                if (t.valid && FindSpotIndex(out, t.name) < 0) out.push_back(std::move(t));
            }
        }
        if (lineEnd == text.size()) break;
        lineStart = lineEnd + 1;
    }
    return out;
}

void SaveSharedSpots(const std::vector<TargetProfile>& spots) {
    std::wstring wide;
    for (const auto& spot : spots) {
        if (!spot.valid || spot.mapID <= 0 || spot.name.empty()) continue;
        wide += SanitizeSpotName(spot.name) + L"\t" + std::to_wstring(spot.mapID) + L"\t" +
                std::to_wstring(spot.x) + L"\t" + std::to_wstring(spot.y) + L"\r\n";
    }
    const std::string bytes = WideToUtf8(wide);
    const std::wstring path = SpotsPath();
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    if (!bytes.empty()) (void)WriteFile(h, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr);
    CloseHandle(h);
}

int FindSpotIndex(const std::vector<TargetProfile>& spots, const std::wstring& name) {
    if (name.empty()) return -1;
    for (std::size_t i = 0; i < spots.size(); ++i) {
        if (_wcsicmp(spots[i].name.c_str(), name.c_str()) == 0) return static_cast<int>(i);
    }
    return -1;
}

std::wstring GetText(HWND h) {
    const int n = GetWindowTextLengthW(h);
    std::wstring out(static_cast<std::size_t>(n) + 1, L'\0');
    if (n > 0) GetWindowTextW(h, out.data(), n + 1);
    out.resize(static_cast<std::size_t>(n));
    return out;
}

void SetText(HWND h, const std::wstring& s) { SetWindowTextW(h, s.c_str()); }

bool HasModule(DWORD pid, const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return false;
    MODULEENTRY32W e{};
    e.dwSize = sizeof(e);
    bool found = false;
    if (Module32FirstW(snap, &e)) {
        do {
            if (_wcsicmp(e.szModule, name) == 0) { found = true; break; }
        } while (Module32NextW(snap, &e));
    }
    CloseHandle(snap);
    return found;
}

BOOL CALLBACK EnumGameWindows(HWND hwnd, LPARAM param) {
    if (!IsWindowVisible(hwnd) || GetWindowTextLengthW(hwnd) <= 0) return TRUE;
    DWORD pid = 0;
    const DWORD tid = GetWindowThreadProcessId(hwnd, &pid);
    if (!pid || !tid || !HasModule(pid, kGameModule)) return TRUE;
    auto* out = reinterpret_cast<std::vector<GameClient>*>(param);
    for (const auto& g : *out) if (g.pid == pid) return TRUE;
    wchar_t title[512]{};
    GetWindowTextW(hwnd, title, _countof(title));
    out->push_back({pid, tid, hwnd, title});
    return TRUE;
}

std::vector<GameClient> FindClients() {
    std::vector<GameClient> out;
    EnumWindows(EnumGameWindows, reinterpret_cast<LPARAM>(&out));
    std::sort(out.begin(), out.end(), [](const GameClient& a, const GameClient& b){ return a.pid < b.pid; });
    return out;
}

class BridgeClient {
public:
    BridgeClient() = default;
    BridgeClient(const BridgeClient&) = delete;
    BridgeClient& operator=(const BridgeClient&) = delete;
    ~BridgeClient() { Close(); }

    bool Attach(const GameClient& game, std::wstring& error) {
        Close();
        game_ = game;
        wchar_t mappingName[96]{};
        MappingName(game.pid, mappingName, _countof(mappingName));
        mapping_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                                      sizeof(SharedBlock), mappingName);
        if (!mapping_) { error = L"Không tạo được shared memory"; return false; }
        shared_ = reinterpret_cast<SharedBlock*>(MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedBlock)));
        if (!shared_) { error = L"Không map được shared memory"; Close(); return false; }
        ZeroMemory(shared_, sizeof(*shared_));
        shared_->magic = kMagic;
        shared_->protocolVersion = kProtocolVersion;
        shared_->targetPid = game.pid;
        shared_->targetWindowThreadId = game.threadId;

        const std::wstring path = ExeDir() + L"\\ThanLongCleanRouteBridge.dll";
        if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
            error = L"Thiếu ThanLongCleanRouteBridge.dll cạnh EXE";
            Close();
            return false;
        }
        SetLastError(ERROR_SUCCESS);
        localDll_ = LoadLibraryW(path.c_str());
        const DWORD loadError = GetLastError();
        if (!localDll_) {
            error = L"Có Bridge DLL nhưng LoadLibrary thất bại Win32=" + std::to_wstring(loadError);
            Close();
            return false;
        }
        HOOKPROC proc = nullptr;
        if (!ResolveProc(localDll_, "TlcGetMessageHook", proc)) {
            error = L"Bridge DLL thiếu TlcGetMessageHook";
            Close();
            return false;
        }
        hook_ = SetWindowsHookExW(WH_GETMESSAGE, proc, localDll_, game.threadId);
        if (!hook_) {
            error = L"Không hook được game; hãy chạy tool cùng quyền với game";
            Close();
            return false;
        }
        if (!PostThreadMessageW(game.threadId, kWakeMessage, 0, 0)) {
            error = L"Không đánh thức được message thread game";
            Close();
            return false;
        }
        attached_ = true;
        return true;
    }

    void Close() {
        if (hook_) UnhookWindowsHookEx(hook_);
        if (localDll_) FreeLibrary(localDll_);
        if (shared_) UnmapViewOfFile(shared_);
        if (mapping_) CloseHandle(mapping_);
        hook_ = nullptr;
        localDll_ = nullptr;
        shared_ = nullptr;
        mapping_ = nullptr;
        attached_ = false;
        pendingSeq_ = 0;
        pendingWakeTick_ = 0;
    }

    bool AttachedTo(DWORD pid) const { return attached_ && game_.pid == pid; }
    bool Attached() const { return attached_; }

    bool Call(Command command, int a0, int a1, int a2, Response& out,
              std::wstring& error, DWORD timeoutMs = 1000, const wchar_t* text = nullptr) {
        if (!attached_ || !shared_) { error = L"Bridge chưa attach"; return false; }

        // Never overwrite a request that timed out on the controller side but may still
        // be executing on the game thread. A late completion is discarded safely; until
        // then only re-post the SAME wake message when the bridge is not busy.
        if (pendingSeq_ > 0) {
            if (shared_->completedSeq == pendingSeq_) {
                MemoryBarrier();
                pendingSeq_ = 0;
                pendingWakeTick_ = 0;
            } else {
                const DWORD now = GetTickCount();
                if (shared_->bridgeBusy == 0 &&
                    (pendingWakeTick_ == 0 || now - pendingWakeTick_ >= kBridgeNudgeMs)) {
                    (void)PostThreadMessageW(game_.threadId, kWakeMessage, 0, 0);
                    pendingWakeTick_ = now;
                }
                error = L"Bridge còn bận sau timeout; không gửi chồng request";
                return false;
            }
        }
        if (shared_->bridgeBusy != 0) {
            error = L"Bridge busy; không gửi chồng request";
            return false;
        }

        const LONG next = shared_->requestSeq + 1;
        shared_->request = {};
        shared_->request.command = static_cast<std::uint32_t>(command);
        shared_->request.arg0 = a0;
        shared_->request.arg1 = a1;
        shared_->request.arg2 = a2;
        if (text && *text) wcsncpy_s(shared_->request.text, _countof(shared_->request.text), text, _TRUNCATE);
        MemoryBarrier();
        InterlockedExchange(&shared_->requestSeq, next);
        if (!PostThreadMessageW(game_.threadId, kWakeMessage, 0, 0)) {
            error = L"Không đánh thức được game thread";
            return false;
        }
        const DWORD begin = GetTickCount();
        while (GetTickCount() - begin < timeoutMs) {
            if (shared_->completedSeq == next) {
                MemoryBarrier();
                pendingSeq_ = 0;
                pendingWakeTick_ = 0;
                out = shared_->response;
                if (!out.ok) {
                    error = out.detail[0] ? out.detail : L"Bridge trả lỗi";
                    return false;
                }
                return true;
            }
            Sleep(2);
        }
        pendingSeq_ = next;
        pendingWakeTick_ = GetTickCount();
        error = L"Bridge timeout; fail-closed";
        return false;
    }

    bool CallText(Command command, const std::wstring& text, Response& out,
                  std::wstring& error, DWORD timeoutMs = 1800) {
        return Call(command, 0, 0, 0, out, error, timeoutMs, text.c_str());
    }

private:
    GameClient game_{};
    HANDLE mapping_ = nullptr;
    SharedBlock* shared_ = nullptr;
    HMODULE localDll_ = nullptr;
    HHOOK hook_ = nullptr;
    bool attached_ = false;
    LONG pendingSeq_ = 0;
    DWORD pendingWakeTick_ = 0;
};

struct Account {
    GameClient game{};
    BridgeClient bridge{};
    Snapshot snapshot{};
    bool snapshotValid = false;
    std::wstring displayName;
    AccountProfile profile{};
    RuntimeState runtime{};
    AutoPkAccountRuntime pk{};
    bool dungeonOwned = false; // one managed PID may belong to only one active dungeon team.

    // Lifecycle latch intentionally lives OUTSIDE RuntimeState. ResetRuntime() may wipe
    // every automation phase at death/alive boundaries without forgetting that both
    // snapshots still belong to the same death session.
    bool deathSessionLatched = false;

    // Rotation metrics intentionally live OUTSIDE RuntimeState so death/alive cold
    // resets do not erase the rolling death window or productive-train timer.
    std::vector<DWORD> rotationDeathTicks{};
    DWORD rotationMetricTick = 0;
    std::uint64_t rotationActiveTrainMs = 0;
    bool rotationBagWasFull = false;

    // Trade coordinator owns only the paired MAIN/CON while a transaction is active.
    // Snapshot polling continues; normal route/death FSM resumes immediately after abort/release.
    bool tradeHeld = false;

    // Adaptive Step 5 value restored from v0.5 for the fixed-slot internal seller.
    // It intentionally lives outside RuntimeState so death/alive cold resets and the
    // next completed train trip retain the count learned from stable FreeBagSpace.
    int sellStep5LearnedRepeat = -1;
};


std::wstring TradeRoleLabel(int role) {
    if (role == kMainTradeRole) return L"MAIN";
    if (role >= kFirstChildTradeRole && role <= kLastChildTradeRole)
        return L"CON " + std::to_wstring(role - 1);
    return L"-";
}

std::wstring PointDescription(const ClickPoint& p) {
    if (!p.valid) return L"CHƯA LẤY";
    return std::to_wstring(p.x) + L"," + std::to_wstring(p.y) + L" @ " +
           std::to_wstring(p.baseW) + L"x" + std::to_wstring(p.baseH);
}

bool ScaleClickPoint(const GameClient& game, const ClickPoint& saved, POINT& point, std::wstring& error) {
    if (!saved.valid) { error = L"Chưa lấy tọa độ click"; return false; }
    if (!game.window || !IsWindow(game.window)) { error = L"Cửa sổ game không còn tồn tại"; return false; }
    RECT rc{};
    if (!GetClientRect(game.window, &rc)) { error = L"Không đọc được client rect"; return false; }
    const int width = rc.right - rc.left;
    const int height = rc.bottom - rc.top;
    if (width <= 0 || height <= 0 || saved.baseW <= 0 || saved.baseH <= 0) {
        error = L"Kích thước cửa sổ không hợp lệ";
        return false;
    }
    point.x = MulDiv(saved.x, width, saved.baseW);
    point.y = MulDiv(saved.y, height, saved.baseH);
    if (point.x < 0 || point.y < 0 || point.x >= width || point.y >= height) {
        error = L"Tọa độ sau scale nằm ngoài cửa sổ";
        return false;
    }
    return true;
}

bool NormalizeClickPointForBridge(const GameClient& game, const ClickPoint& saved,
                                  int& normalizedX, int& normalizedY,
                                  std::wstring& error) {
    POINT point{};
    if (!ScaleClickPoint(game, saved, point, error)) return false;
    RECT rc{};
    if (!GetClientRect(game.window, &rc)) {
        error = L"Không đọc được client rect để chuẩn hóa tọa độ";
        return false;
    }
    const int width = rc.right - rc.left;
    const int height = rc.bottom - rc.top;
    normalizedX = fixed_slot_sell_logic::NormalizeClientCoordinate(point.x, width);
    normalizedY = fixed_slot_sell_logic::NormalizeClientCoordinate(point.y, height);
    if (normalizedX < 0 || normalizedY < 0) {
        error = L"Không chuẩn hóa được tọa độ UI nội bộ";
        return false;
    }
    return true;
}

bool Elapsed(DWORD now, DWORD since, DWORD delay) {
    return since != 0 && now - since >= delay;
}

void ResetRuntime(RuntimeState& r) {
    const bool running = r.running;
    r = RuntimeState{};
    r.running = running;
    r.status = running ? L"Đang giám sát" : L"Đã dừng";
}

enum class TradePhase { Idle, Rendezvous, TargetMain, Sequence };

class App {
public:
    bool Create(HINSTANCE instance) {
        instance_ = instance;
        MigrateLegacyConfigIfNeeded();
        EnsureUnicodeIni();
        LoadTradeSettings();
        inventoryFilter_ = LoadInventoryFilterSettings();
        telegramSettings_ = LoadTelegramSettings(telegramLoadWarning_);
        LoadTradeSequence();
        LoadAutoPkSettings();
        spots_ = LoadSharedSpots();
        INITCOMMONCONTROLSEX ic{sizeof(ic), ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES};
        InitCommonControlsEx(&ic);
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = WndProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = L"ThanLongCleanRouteMultiWindow";
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
        hwnd_ = CreateWindowExW(0, wc.lpszClassName, kTitle,
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                CW_USEDEFAULT, CW_USEDEFAULT, 1060, 1030,
                                nullptr, nullptr, instance, this);
        return hwnd_ != nullptr;
    }

    void Show(int cmd) {
        ShowWindow(hwnd_, cmd);
        UpdateWindow(hwnd_);
    }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        App* self = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
            self = reinterpret_cast<App*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->hwnd_ = hwnd;
        }
        return self ? self->Handle(msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
    }

    static LRESULT CALLBACK TradeEditorWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        App* self = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
            self = reinterpret_cast<App*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            if (self) self->tradeEditor_ = hwnd;
        }
        if (!self) return DefWindowProcW(hwnd, msg, wp, lp);
        return self->HandleTradeEditor(hwnd, msg, wp, lp);
    }


    static LRESULT CALLBACK InventoryFilterWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        App* self = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
            self = reinterpret_cast<App*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        return self ? self->HandleInventoryFilterWindow(hwnd, msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
    }

    static LRESULT CALLBACK ShortcutWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        App* self = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
            self = reinterpret_cast<App*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            if (self) self->shortcutWindow_ = hwnd;
        }
        return self ? self->HandleShortcutWindow(hwnd, msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
    }

    static LRESULT CALLBACK TradeSequenceListSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                                         UINT_PTR, DWORD_PTR refData) {
        App* self = reinterpret_cast<App*>(refData);
        if (!self) return DefSubclassProc(hwnd, msg, wp, lp);
        switch (msg) {
            case WM_LBUTTONDOWN: {
                const LRESULT result = DefSubclassProc(hwnd, msg, wp, lp);
                LVHITTESTINFO hit{};
                hit.pt.x = GET_X_LPARAM(lp);
                hit.pt.y = GET_Y_LPARAM(lp);
                const int row = ListView_HitTest(hwnd, &hit);
                if (row >= 0) {
                    self->tradeSeqDragSelecting_ = true;
                    self->tradeSeqDragStartRow_ = row;
                    self->SelectTradeSequenceDragRange(row, row);
                    SetCapture(hwnd);
                }
                return result;
            }
            case WM_MOUSEMOVE:
                if (self->tradeSeqDragSelecting_ && (wp & MK_LBUTTON)) {
                    LVHITTESTINFO hit{};
                    hit.pt.x = GET_X_LPARAM(lp);
                    hit.pt.y = GET_Y_LPARAM(lp);
                    int row = ListView_HitTest(hwnd, &hit);
                    if (row < 0) {
                        const int count = ListView_GetItemCount(hwnd);
                        RECT rc{}; GetClientRect(hwnd, &rc);
                        if (count > 0 && hit.pt.y < rc.top) row = 0;
                        else if (count > 0 && hit.pt.y >= rc.bottom) row = count - 1;
                    }
                    if (row >= 0) self->SelectTradeSequenceDragRange(self->tradeSeqDragStartRow_, row);
                    return 0;
                }
                break;
            case WM_LBUTTONUP:
                if (self->tradeSeqDragSelecting_) {
                    self->tradeSeqDragSelecting_ = false;
                    self->tradeSeqDragStartRow_ = -1;
                    if (GetCapture() == hwnd) ReleaseCapture();
                }
                break;
            case WM_CAPTURECHANGED:
                self->tradeSeqDragSelecting_ = false;
                self->tradeSeqDragStartRow_ = -1;
                break;
            case WM_NCDESTROY:
                RemoveWindowSubclass(hwnd, TradeSequenceListSubclassProc, 1);
                break;
        }
        return DefSubclassProc(hwnd, msg, wp, lp);
    }

    HWND MakeIn(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style,
                int x, int y, int w, int h, int id) {
        HWND hWnd = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style,
                                    x, y, w, h, parent,
                                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_, nullptr);
        if (hWnd) SendMessageW(hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
        return hWnd;
    }

    HWND Make(const wchar_t* cls, const wchar_t* text, DWORD style,
              int x, int y, int w, int h, int id) {
        return CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style,
                               x, y, w, h, hwnd_,
                               reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
    }

    void AddListColumn(int index, int width, const wchar_t* text) {
        LVCOLUMNW c{};
        c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        c.pszText = const_cast<wchar_t*>(text);
        c.cx = width;
        c.iSubItem = index;
        ListView_InsertColumn(clientList_, index, &c);
    }

    void AddMacroColumn(int index, int width, const wchar_t* text) {
        LVCOLUMNW c{};
        c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        c.pszText = const_cast<wchar_t*>(text);
        c.cx = width;
        c.iSubItem = index;
        ListView_InsertColumn(sellMacroList_, index, &c);
    }

    void AddRotationColumn(int index, int width, const wchar_t* text) {
        LVCOLUMNW c{};
        c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        c.pszText = const_cast<wchar_t*>(text);
        c.cx = width;
        c.iSubItem = index;
        ListView_InsertColumn(rotationList_, index, &c);
    }

    void LoadAutoPkSettings() {
        const std::wstring section = L"AutoPK";
        autoPkLoop_ = ReadIniInt(section, L"Loop", 0) != 0;
        autoPkLifeCheck_ = ReadIniInt(section, L"LifeCheck", 1) != 0;
        const int count = std::clamp(ReadIniInt(section, L"StepCount", 0), 0, 32);
        autoPkSteps_.clear();
        for (int i = 0; i < count; ++i) {
            const std::wstring prefix = L"Step_" + std::to_wstring(i) + L"_";
            AutoPkStep step{};
            step.enabled = ReadIniInt(section, prefix + L"Enabled", 1) != 0;
            step.kind = static_cast<auto_pk_logic::StepKind>(std::clamp(ReadIniInt(section, prefix + L"Kind", 4), 0, 4));
            step.description = ReadIniText(section, prefix + L"Desc");
            step.target.mapID = ReadIniInt(section, prefix + L"Map", 0);
            step.target.x = ReadIniInt(section, prefix + L"X", 0);
            step.target.y = ReadIniInt(section, prefix + L"Y", 0);
            step.target.valid = ReadIniInt(section, prefix + L"TargetValid", 0) != 0 && step.target.mapID > 0;
            step.tolerance = std::clamp(ReadIniInt(section, prefix + L"Tolerance", 120), 20, 2000);
            step.delayMs = std::clamp(ReadIniInt(section, prefix + L"Delay", 500), 0, 60000);
            step.repeat = std::clamp(ReadIniInt(section, prefix + L"Repeat", 1), 1, 999);
            const int clickCount = std::clamp(ReadIniInt(section, prefix + L"ClickCount", 0), 0, 64);
            for (int j = 0; j < clickCount; ++j) {
                const std::wstring cp = prefix + L"Click_" + std::to_wstring(j) + L"_";
                AutoPkClickStep click{};
                click.phase = static_cast<auto_pk_logic::ClickPhase>(std::clamp(ReadIniInt(section, cp + L"Phase", 0), 0, 2));
                click.description = ReadIniText(section, cp + L"Desc");
                click.point.x = ReadIniInt(section, cp + L"X", -1);
                click.point.y = ReadIniInt(section, cp + L"Y", -1);
                click.point.baseW = ReadIniInt(section, cp + L"W", 0);
                click.point.baseH = ReadIniInt(section, cp + L"H", 0);
                click.point.valid = click.point.x >= 0 && click.point.y >= 0 && click.point.baseW > 0 && click.point.baseH > 0;
                click.delayMs = std::clamp(ReadIniInt(section, cp + L"Delay", 500), 0, 60000);
                click.repeat = std::clamp(ReadIniInt(section, cp + L"Repeat", 1), 1, 999);
                if (!auto_pk_logic::PhaseAllowed(step.kind, click.phase)) click.phase = auto_pk_logic::ClickPhase::Normal;
                step.clicks.push_back(std::move(click));
            }
            autoPkSteps_.push_back(std::move(step));
        }
        if (autoPkSteps_.empty()) {
            auto add = [&](auto_pk_logic::StepKind kind, const wchar_t* desc, bool enabled) {
                AutoPkStep s{}; s.kind = kind; s.description = desc; s.enabled = enabled; autoPkSteps_.push_back(std::move(s));
            };
            add(auto_pk_logic::StepKind::Treatment, L"1. Trị Liệu • Đỗ Thanh Đằng • ResID 339", true);
            add(auto_pk_logic::StepKind::Buff, L"2. Auto buff", true);
            add(auto_pk_logic::StepKind::Rally, L"3. Tụ tại điểm PK phụ", true);
            add(auto_pk_logic::StepKind::EnterPk, L"4. Lao vào PK", true);
            add(auto_pk_logic::StepKind::Custom, L"5. THAO TÁC TÙY CHỌN", false);
            add(auto_pk_logic::StepKind::Custom, L"6. THAO TÁC TÙY CHỌN", false);
            auto& enter = autoPkSteps_[3];
            for (const auto& seed : std::array<std::pair<auto_pk_logic::ClickPhase, const wchar_t*>, 4>{{
                {auto_pk_logic::ClickPhase::EnterPkMode, L"BẬT PK 1"},
                {auto_pk_logic::ClickPhase::EnterPkMode, L"BẬT PK 2"},
                {auto_pk_logic::ClickPhase::AutoPk, L"AUTO PK 1"},
                {auto_pk_logic::ClickPhase::AutoPk, L"AUTO PK 2"}}}) {
                AutoPkClickStep c{}; c.phase = seed.first; c.description = seed.second; enter.clicks.push_back(std::move(c));
            }
            SaveAutoPkSettings();
        }
    }

    void SaveAutoPkSettings() {
        EnsureUnicodeIni();
        const std::wstring section = L"AutoPK";
        WriteIniInt(section, L"Loop", autoPkLoop_ ? 1 : 0);
        WriteIniInt(section, L"LifeCheck", autoPkLifeCheck_ ? 1 : 0);
        WriteIniInt(section, L"StepCount", static_cast<int>(autoPkSteps_.size()));
        for (std::size_t i = 0; i < autoPkSteps_.size(); ++i) {
            const AutoPkStep& step = autoPkSteps_[i];
            const std::wstring prefix = L"Step_" + std::to_wstring(i) + L"_";
            WriteIniInt(section, prefix + L"Enabled", step.enabled ? 1 : 0);
            WriteIniInt(section, prefix + L"Kind", static_cast<int>(step.kind));
            WriteIniText(section, prefix + L"Desc", step.description);
            WriteIniInt(section, prefix + L"Map", step.target.valid ? step.target.mapID : 0);
            WriteIniInt(section, prefix + L"X", step.target.valid ? step.target.x : 0);
            WriteIniInt(section, prefix + L"Y", step.target.valid ? step.target.y : 0);
            WriteIniInt(section, prefix + L"TargetValid", step.target.valid ? 1 : 0);
            WriteIniInt(section, prefix + L"Tolerance", step.tolerance);
            WriteIniInt(section, prefix + L"Delay", step.delayMs);
            WriteIniInt(section, prefix + L"Repeat", step.repeat);
            WriteIniInt(section, prefix + L"ClickCount", static_cast<int>(step.clicks.size()));
            for (std::size_t j = 0; j < step.clicks.size(); ++j) {
                const AutoPkClickStep& click = step.clicks[j];
                const std::wstring cp = prefix + L"Click_" + std::to_wstring(j) + L"_";
                WriteIniInt(section, cp + L"Phase", static_cast<int>(click.phase));
                WriteIniText(section, cp + L"Desc", click.description);
                WriteIniInt(section, cp + L"X", click.point.valid ? click.point.x : -1);
                WriteIniInt(section, cp + L"Y", click.point.valid ? click.point.y : -1);
                WriteIniInt(section, cp + L"W", click.point.valid ? click.point.baseW : 0);
                WriteIniInt(section, cp + L"H", click.point.valid ? click.point.baseH : 0);
                WriteIniInt(section, cp + L"Delay", click.delayMs);
                WriteIniInt(section, cp + L"Repeat", click.repeat);
            }
        }
        FlushIni();
    }

    static int ParseEditInt(HWND edit, int fallback, int lo, int hi) {
        if (!edit) return fallback;
        wchar_t buf[64]{}; GetWindowTextW(edit, buf, _countof(buf));
        wchar_t* end = nullptr; long value = wcstol(buf, &end, 10);
        if (end == buf) value = fallback;
        return std::clamp(static_cast<int>(value), lo, hi);
    }

    void SetAutoPkStatus(const std::wstring& text) {
        if (autoPkStatus_) SetText(autoPkStatus_, L"AUTO PK • " + text);
    }

    int SelectedAutoPkStep() const {
        return autoPkStepList_ ? ListView_GetNextItem(autoPkStepList_, -1, LVNI_SELECTED) : -1;
    }
    int SelectedAutoPkClick() const {
        return autoPkClickList_ ? ListView_GetNextItem(autoPkClickList_, -1, LVNI_SELECTED) : -1;
    }

    void RefreshAutoPkStepList(int select = -1) {
        if (!autoPkStepList_) return;
        autoPkUiLoading_ = true;
        ListView_DeleteAllItems(autoPkStepList_);
        for (std::size_t i = 0; i < autoPkSteps_.size(); ++i) {
            const AutoPkStep& s = autoPkSteps_[i];
            LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = static_cast<int>(i);
            std::wstring num = std::to_wstring(i + 1); item.pszText = num.data(); ListView_InsertItem(autoPkStepList_, &item);
            ListView_SetCheckState(autoPkStepList_, static_cast<int>(i), s.enabled ? TRUE : FALSE);
            ListView_SetItemText(autoPkStepList_, static_cast<int>(i), 1, const_cast<wchar_t*>(auto_pk_logic::StepKindLabel(s.kind)));
            ListView_SetItemText(autoPkStepList_, static_cast<int>(i), 2, const_cast<wchar_t*>(s.description.c_str()));
            std::wstring target = auto_pk_logic::NeedsWorldTarget(s.kind)
                ? (s.target.valid ? L"M" + std::to_wstring(s.target.mapID) + L" • " + std::to_wstring(s.target.x) + L"," + std::to_wstring(s.target.y) : L"CHƯA GET")
                : L"—";
            ListView_SetItemText(autoPkStepList_, static_cast<int>(i), 3, target.data());
            std::wstring delay = std::to_wstring(s.delayMs); ListView_SetItemText(autoPkStepList_, static_cast<int>(i), 4, delay.data());
            std::wstring repeat = std::to_wstring(s.repeat); ListView_SetItemText(autoPkStepList_, static_cast<int>(i), 5, repeat.data());
        }
        autoPkUiLoading_ = false;
        if (select < 0 && !autoPkSteps_.empty()) select = 0;
        if (select >= 0 && select < static_cast<int>(autoPkSteps_.size())) {
            ListView_SetItemState(autoPkStepList_, select, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(autoPkStepList_, select, FALSE);
            LoadAutoPkStepEditor(select);
        } else {
            RefreshAutoPkClickList();
        }
    }

    void RefreshAutoPkClickList(int select = -1) {
        if (!autoPkClickList_) return;
        ListView_DeleteAllItems(autoPkClickList_);
        const int si = SelectedAutoPkStep();
        if (si < 0 || si >= static_cast<int>(autoPkSteps_.size())) return;
        const AutoPkStep& step = autoPkSteps_[static_cast<std::size_t>(si)];
        for (std::size_t i = 0; i < step.clicks.size(); ++i) {
            const AutoPkClickStep& c = step.clicks[i];
            LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = static_cast<int>(i);
            std::wstring num = std::to_wstring(i + 1); item.pszText = num.data(); ListView_InsertItem(autoPkClickList_, &item);
            ListView_SetItemText(autoPkClickList_, static_cast<int>(i), 1, const_cast<wchar_t*>(auto_pk_logic::ClickPhaseLabel(c.phase)));
            ListView_SetItemText(autoPkClickList_, static_cast<int>(i), 2, const_cast<wchar_t*>(c.description.c_str()));
            std::wstring point = c.point.valid ? std::to_wstring(c.point.x) + L"," + std::to_wstring(c.point.y) + L" @ " + std::to_wstring(c.point.baseW) + L"x" + std::to_wstring(c.point.baseH) : L"CHƯA F8";
            ListView_SetItemText(autoPkClickList_, static_cast<int>(i), 3, point.data());
            std::wstring delay = std::to_wstring(c.delayMs); ListView_SetItemText(autoPkClickList_, static_cast<int>(i), 4, delay.data());
            std::wstring repeat = std::to_wstring(c.repeat); ListView_SetItemText(autoPkClickList_, static_cast<int>(i), 5, repeat.data());
        }
        if (select < 0 && !step.clicks.empty()) select = 0;
        if (select >= 0 && select < static_cast<int>(step.clicks.size())) {
            ListView_SetItemState(autoPkClickList_, select, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            LoadAutoPkClickEditor(select);
        }
    }

    void LoadAutoPkStepEditor(int index) {
        if (index < 0 || index >= static_cast<int>(autoPkSteps_.size())) return;
        const AutoPkStep& s = autoPkSteps_[static_cast<std::size_t>(index)];
        SetText(autoPkStepDesc_, s.description);
        SetText(autoPkTolerance_, std::to_wstring(s.tolerance));
        SetText(autoPkStepDelay_, std::to_wstring(s.delayMs));
        SetText(autoPkStepRepeat_, std::to_wstring(s.repeat));
        if (autoPkTargetLabel_) {
            SetText(autoPkTargetLabel_, s.target.valid ? L"TARGET M" + std::to_wstring(s.target.mapID) + L" • " + std::to_wstring(s.target.x) + L"," + std::to_wstring(s.target.y) : L"TARGET: CHƯA GET");
        }
        RefreshAutoPkClickList();
    }

    void LoadAutoPkClickEditor(int index) {
        const int si = SelectedAutoPkStep();
        if (si < 0 || si >= static_cast<int>(autoPkSteps_.size())) return;
        const auto& clicks = autoPkSteps_[static_cast<std::size_t>(si)].clicks;
        if (index < 0 || index >= static_cast<int>(clicks.size())) return;
        const AutoPkClickStep& c = clicks[static_cast<std::size_t>(index)];
        if (autoPkClickPhase_) SendMessageW(autoPkClickPhase_, CB_SETCURSEL, static_cast<WPARAM>(static_cast<int>(c.phase)), 0);
        SetText(autoPkClickDesc_, c.description);
        SetText(autoPkClickDelay_, std::to_wstring(c.delayMs));
        SetText(autoPkClickRepeat_, std::to_wstring(c.repeat));
    }

    void SaveAutoPkStepEditor() {
        const int i = SelectedAutoPkStep();
        if (i < 0 || i >= static_cast<int>(autoPkSteps_.size())) return;
        AutoPkStep& s = autoPkSteps_[static_cast<std::size_t>(i)];
        s.description = GetText(autoPkStepDesc_);
        s.tolerance = ParseEditInt(autoPkTolerance_, s.tolerance, 20, 2000);
        s.delayMs = ParseEditInt(autoPkStepDelay_, s.delayMs, 0, 60000);
        s.repeat = ParseEditInt(autoPkStepRepeat_, s.repeat, 1, 999);
        SaveAutoPkSettings(); RefreshAutoPkStepList(i);
    }

    void SaveAutoPkClickEditor() {
        const int si = SelectedAutoPkStep(), ci = SelectedAutoPkClick();
        if (si < 0 || si >= static_cast<int>(autoPkSteps_.size())) return;
        AutoPkStep& step = autoPkSteps_[static_cast<std::size_t>(si)];
        if (ci < 0 || ci >= static_cast<int>(step.clicks.size())) return;
        AutoPkClickStep& c = step.clicks[static_cast<std::size_t>(ci)];
        int phase = autoPkClickPhase_ ? static_cast<int>(SendMessageW(autoPkClickPhase_, CB_GETCURSEL, 0, 0)) : 0;
        phase = std::clamp(phase, 0, 2);
        c.phase = static_cast<auto_pk_logic::ClickPhase>(phase);
        if (!auto_pk_logic::PhaseAllowed(step.kind, c.phase)) {
            c.phase = auto_pk_logic::ClickPhase::Normal;
            if (autoPkClickPhase_) SendMessageW(autoPkClickPhase_, CB_SETCURSEL, 0, 0);
        }
        c.description = GetText(autoPkClickDesc_);
        c.delayMs = ParseEditInt(autoPkClickDelay_, c.delayMs, 0, 60000);
        c.repeat = ParseEditInt(autoPkClickRepeat_, c.repeat, 1, 999);
        SaveAutoPkSettings(); RefreshAutoPkClickList(ci);
    }

    void AddAutoPkClick() {
        const int si = SelectedAutoPkStep(); if (si < 0 || si >= static_cast<int>(autoPkSteps_.size())) return;
        AutoPkStep& step = autoPkSteps_[static_cast<std::size_t>(si)];
        AutoPkClickStep c{};
        if (step.kind == auto_pk_logic::StepKind::EnterPk) c.phase = auto_pk_logic::ClickPhase::EnterPkMode;
        c.description = L"Click ẩn mới";
        step.clicks.push_back(std::move(c)); SaveAutoPkSettings(); RefreshAutoPkStepList(si); RefreshAutoPkClickList(static_cast<int>(step.clicks.size()) - 1);
    }
    void DeleteAutoPkClick() {
        const int si = SelectedAutoPkStep(), ci = SelectedAutoPkClick();
        if (si < 0 || si >= static_cast<int>(autoPkSteps_.size())) return;
        auto& v = autoPkSteps_[static_cast<std::size_t>(si)].clicks;
        if (ci < 0 || ci >= static_cast<int>(v.size())) return;
        v.erase(v.begin() + ci); SaveAutoPkSettings(); RefreshAutoPkClickList(std::min(ci, static_cast<int>(v.size()) - 1));
    }
    void MoveAutoPkClick(int delta) {
        const int si = SelectedAutoPkStep(), ci = SelectedAutoPkClick();
        if (si < 0 || si >= static_cast<int>(autoPkSteps_.size())) return;
        auto& v = autoPkSteps_[static_cast<std::size_t>(si)].clicks; const int ni = ci + delta;
        if (ci < 0 || ni < 0 || ci >= static_cast<int>(v.size()) || ni >= static_cast<int>(v.size())) return;
        std::swap(v[static_cast<std::size_t>(ci)], v[static_cast<std::size_t>(ni)]); SaveAutoPkSettings(); RefreshAutoPkClickList(ni);
    }
    void MoveAutoPkStepTo(int from, int to) {
        if (autoPkRunning_ || from < 0 || to < 0 || from >= static_cast<int>(autoPkSteps_.size()) || to >= static_cast<int>(autoPkSteps_.size()) || from == to) return;
        AutoPkStep moving = std::move(autoPkSteps_[static_cast<std::size_t>(from)]);
        autoPkSteps_.erase(autoPkSteps_.begin() + from);
        autoPkSteps_.insert(autoPkSteps_.begin() + to, std::move(moving));
        SaveAutoPkSettings(); RefreshAutoPkStepList(to);
    }
    void MoveAutoPkStep(int delta) { const int i = SelectedAutoPkStep(); MoveAutoPkStepTo(i, i + delta); }

    static LRESULT CALLBACK AutoPkStepSubclassProc(HWND h, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR ref) {
        App* self = reinterpret_cast<App*>(ref);
        if (self && msg == WM_LBUTTONDOWN) {
            LVHITTESTINFO hit{}; hit.pt.x = GET_X_LPARAM(lp); hit.pt.y = GET_Y_LPARAM(lp); ListView_HitTest(h, &hit);
            self->autoPkDragStartRow_ = hit.iItem;
        } else if (self && msg == WM_LBUTTONUP && self->autoPkDragStartRow_ >= 0) {
            LVHITTESTINFO hit{}; hit.pt.x = GET_X_LPARAM(lp); hit.pt.y = GET_Y_LPARAM(lp); ListView_HitTest(h, &hit);
            const int from = self->autoPkDragStartRow_; self->autoPkDragStartRow_ = -1;
            if (hit.iItem >= 0 && hit.iItem != from) self->MoveAutoPkStepTo(from, hit.iItem);
        }
        return DefSubclassProc(h, msg, wp, lp);
    }

    void CaptureAutoPkTarget() {
        const int si = SelectedAutoPkStep(); Account* a = SelectedAccount();
        if (si < 0 || si >= static_cast<int>(autoPkSteps_.size()) || !a) { Log(L"AUTO PK: chọn bước và acc trước khi GET tọa độ."); return; }
        std::wstring error;
        if (!ReadSnapshot(*a, error, 1200) || (a->snapshot.validMask & (ValidMap | ValidPosition)) != (ValidMap | ValidPosition)) {
            LogAccount(*a, L"AUTO PK GET TARGET FAIL • " + error); return;
        }
        AutoPkStep& s = autoPkSteps_[static_cast<std::size_t>(si)];
        if (!auto_pk_logic::NeedsWorldTarget(s.kind)) { Log(L"AUTO PK: bước này không dùng tọa độ thế giới."); return; }
        s.target.mapID = a->snapshot.mapID; s.target.x = a->snapshot.x; s.target.y = a->snapshot.y; s.target.valid = s.target.mapID > 0;
        s.target.name = s.description; SaveAutoPkSettings(); RefreshAutoPkStepList(si);
        Log(L"AUTO PK GET TARGET • " + s.description + L" → M" + std::to_wstring(s.target.mapID) + L" • " + std::to_wstring(s.target.x) + L"," + std::to_wstring(s.target.y));
    }

    void BeginAutoPkClickCapture() {
        const int si = SelectedAutoPkStep(), ci = SelectedAutoPkClick(); Account* a = SelectedAccount();
        if (!a || si < 0 || ci < 0 || si >= static_cast<int>(autoPkSteps_.size()) || ci >= static_cast<int>(autoPkSteps_[static_cast<std::size_t>(si)].clicks.size())) {
            Log(L"AUTO PK F8: chọn acc + bước + dòng click trước."); return;
        }
        captureSlot_ = ClickSlot::None; captureMacroIndex_ = -1; captureTradeSequenceIndex_ = -1;
        capturePkStepIndex_ = si; capturePkClickIndex_ = ci; capturePid_ = a->game.pid;
        SetAutoPkStatus(L"CAPTURE F8 • đưa chuột lên UI game của acc " + AccountTag(*a) + L" rồi nhấn F8");
    }

    void TestAutoPkClick() {
        const int si = SelectedAutoPkStep(), ci = SelectedAutoPkClick(); Account* a = SelectedAccount();
        if (!a || si < 0 || ci < 0 || si >= static_cast<int>(autoPkSteps_.size())) return;
        auto& clicks = autoPkSteps_[static_cast<std::size_t>(si)].clicks; if (ci >= static_cast<int>(clicks.size())) return;
        std::wstring error; if (!EnsureAttach(*a, error)) { LogAccount(*a, L"AUTO PK TEST FAIL • " + error); return; }
        const bool ok = CoordinatorInternalPointAction(*a, clicks[static_cast<std::size_t>(ci)].point, L"AUTO PK TEST", error);
        LogAccount(*a, ok ? L"AUTO PK TEST PASS • click ẩn InputSync" : L"AUTO PK TEST FAIL • " + error);
    }

    bool IsAutoPkControl(HWND h) const { return std::find(autoPkControls_.begin(), autoPkControls_.end(), h) != autoPkControls_.end(); }
    void ShowAutoPkControls(bool show) { for (HWND h : autoPkControls_) if (h) ShowWindow(h, show ? SW_SHOW : SW_HIDE); }
    void ShowAutoPkShared(bool show) {
        for (HWND h : {clientList_, selected_, live_}) if (h) ShowWindow(h, show ? SW_SHOW : SW_HIDE);
    }

    void BuildAutoPkUi() {
        HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        auto add = [&](HWND h) { if (h) { SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE); autoPkControls_.push_back(h); } return h; };
        autoPkStatus_ = add(Make(L"STATIC", L"AUTO PK • STOP", SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 330, 6, 693, 27, 0));
        add(Make(L"BUTTON", L"QUÉT CLIENT", BS_PUSHBUTTON, 18, 205, 120, 30, IDC_SCAN));
        add(Make(L"BUTTON", L"BẮT ĐẦU AUTO PK", BS_DEFPUSHBUTTON, 148, 205, 175, 30, IDC_PK_START));
        add(Make(L"BUTTON", L"STOP AUTO PK", BS_PUSHBUTTON, 333, 205, 145, 30, IDC_PK_STOP));
        autoPkLife_ = add(Make(L"BUTTON", L"CHECK SỐNG/CHẾT + HỒI SINH", BS_AUTOCHECKBOX, 490, 205, 240, 30, IDC_PK_LIFE));
        autoPkLoopCheck_ = add(Make(L"BUTTON", L"LẶP CHUỖI", BS_AUTOCHECKBOX, 742, 205, 125, 30, IDC_PK_LOOP));
        SendMessageW(autoPkLife_, BM_SETCHECK, autoPkLifeCheck_ ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(autoPkLoopCheck_, BM_SETCHECK, autoPkLoop_ ? BST_CHECKED : BST_UNCHECKED, 0);
        add(Make(L"STATIC", L"BỘ ĐIỀU PHỐI CHIẾN TRANH • tick acc ở bảng trên • kéo thả bước để đổi thứ tự", SS_LEFT | SS_CENTERIMAGE, 18, 288, 1005, 24, 0));

        autoPkStepList_ = add(Make(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER, 18, 314, 1005, 190, IDC_PK_STEP_LIST));
        ListView_SetExtendedListViewStyle(autoPkStepList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);
        const std::array<std::pair<int,const wchar_t*>,6> sc{{{40,L"#"},{120,L"Loại"},{370,L"Thao tác"},{250,L"Tọa độ"},{105,L"Delay"},{80,L"Lặp"}}};
        for (int i=0;i<6;++i){ LVCOLUMNW c{}; c.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM; c.cx=sc[static_cast<std::size_t>(i)].first; c.iSubItem=i; c.pszText=const_cast<wchar_t*>(sc[static_cast<std::size_t>(i)].second); ListView_InsertColumn(autoPkStepList_,i,&c); }
        SetWindowSubclass(autoPkStepList_, AutoPkStepSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
        add(Make(L"BUTTON", L"↑ BƯỚC", BS_PUSHBUTTON, 18, 510, 82, 27, IDC_PK_STEP_UP)); add(Make(L"BUTTON", L"↓ BƯỚC", BS_PUSHBUTTON, 106, 510, 82, 27, IDC_PK_STEP_DOWN));
        add(Make(L"BUTTON", L"GET TỌA ĐỘ ĐANG ĐỨNG", BS_PUSHBUTTON, 194, 510, 190, 27, IDC_PK_TARGET_CAPTURE));
        autoPkTargetLabel_ = add(Make(L"STATIC", L"TARGET: CHƯA GET", SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 392, 510, 250, 27, 0));
        autoPkStepDesc_ = add(Make(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, 18, 543, 370, 27, IDC_PK_STEP_DESC));
        add(Make(L"STATIC", L"Sai số:", SS_LEFT|SS_CENTERIMAGE, 398,543,55,27,0)); autoPkTolerance_=add(Make(L"EDIT",L"120",WS_BORDER|ES_NUMBER|ES_CENTER,453,543,65,27,IDC_PK_TOLERANCE));
        add(Make(L"STATIC", L"Delay:", SS_LEFT|SS_CENTERIMAGE, 527,543,48,27,0)); autoPkStepDelay_=add(Make(L"EDIT",L"500",WS_BORDER|ES_NUMBER|ES_CENTER,575,543,70,27,IDC_PK_STEP_DELAY));
        add(Make(L"STATIC", L"Lặp:", SS_LEFT|SS_CENTERIMAGE, 653,543,38,27,0)); autoPkStepRepeat_=add(Make(L"EDIT",L"1",WS_BORDER|ES_NUMBER|ES_CENTER,691,543,55,27,IDC_PK_STEP_REPEAT));
        add(Make(L"BUTTON", L"LƯU BƯỚC", BS_PUSHBUTTON, 756, 543, 110, 27, IDC_PK_STEP_SAVE));

        autoPkClickList_ = add(Make(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER, 18, 578, 1005, 180, IDC_PK_CLICK_LIST));
        ListView_SetExtendedListViewStyle(autoPkClickList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        const std::array<std::pair<int,const wchar_t*>,6> cc{{{40,L"#"},{100,L"Pha"},{355,L"Mô tả click ẩn"},{280,L"Tọa độ UI"},{105,L"Delay"},{80,L"Lặp"}}};
        for (int i=0;i<6;++i){ LVCOLUMNW c{}; c.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM; c.cx=cc[static_cast<std::size_t>(i)].first; c.iSubItem=i; c.pszText=const_cast<wchar_t*>(cc[static_cast<std::size_t>(i)].second); ListView_InsertColumn(autoPkClickList_,i,&c); }
        add(Make(L"BUTTON", L"+ CLICK", BS_PUSHBUTTON, 18, 765, 78, 27, IDC_PK_CLICK_ADD)); add(Make(L"BUTTON", L"- CLICK", BS_PUSHBUTTON, 102, 765, 78, 27, IDC_PK_CLICK_DELETE));
        add(Make(L"BUTTON", L"↑", BS_PUSHBUTTON, 186, 765, 38, 27, IDC_PK_CLICK_UP)); add(Make(L"BUTTON", L"↓", BS_PUSHBUTTON, 230, 765, 38, 27, IDC_PK_CLICK_DOWN));
        autoPkClickPhase_ = add(Make(WC_COMBOBOXW,L"",CBS_DROPDOWNLIST|WS_VSCROLL,276,765,115,120,IDC_PK_CLICK_PHASE));
        for (const wchar_t* text : {L"THƯỜNG",L"BẬT PK",L"AUTO PK"}) SendMessageW(autoPkClickPhase_,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text));
        autoPkClickDesc_ = add(Make(L"EDIT", L"", WS_BORDER|ES_AUTOHSCROLL, 399,765,245,27,IDC_PK_CLICK_DESC));
        autoPkClickDelay_=add(Make(L"EDIT",L"500",WS_BORDER|ES_NUMBER|ES_CENTER,652,765,70,27,IDC_PK_CLICK_DELAY)); autoPkClickRepeat_=add(Make(L"EDIT",L"1",WS_BORDER|ES_NUMBER|ES_CENTER,730,765,52,27,IDC_PK_CLICK_REPEAT));
        add(Make(L"BUTTON", L"LƯU CLICK", BS_PUSHBUTTON, 790,765,90,27,IDC_PK_CLICK_SAVE)); add(Make(L"BUTTON", L"LẤY F8", BS_PUSHBUTTON, 888,765,65,27,IDC_PK_CLICK_CAPTURE)); add(Make(L"BUTTON", L"TEST", BS_PUSHBUTTON, 959,765,64,27,IDC_PK_CLICK_TEST));
        add(Make(L"STATIC", L"Trị liệu dùng NPC Đỗ Thanh Đằng ResID 339. Bước LAO PK: BẬT PK (barrier) → AutoPath tới điểm chính → AUTO PK → xác nhận AutoFight ON.", SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,800,1005,34,0));
        ShowAutoPkControls(false); RefreshAutoPkStepList();
    }

    void ResetAutoPkAccountStep(Account& a) {
        a.pk.stepDone=false; a.pk.preDone=false; a.pk.arrived=false; a.pk.postDone=false; a.pk.dueTick=0;
        a.pk.clickIndex=0; a.pk.clickRepeatDone=0; a.pk.treatmentStage=0; a.pk.treatmentCloseAttempts=0; a.pk.postVerifyAttempts=0;
        ResetRobustTravel(a.runtime); ResetTravelFightGuard(a.runtime);
    }

    int FirstEnabledAutoPkStep() const {
        for (std::size_t i=0;i<autoPkSteps_.size();++i) if (autoPkSteps_[i].enabled) return static_cast<int>(i); return -1;
    }
    int NextEnabledAutoPkStep(int current) const {
        for (int i=current+1;i<static_cast<int>(autoPkSteps_.size());++i) if (autoPkSteps_[static_cast<std::size_t>(i)].enabled) return i; return -1;
    }
    bool AllActivePk(std::function<bool(const Account&)> pred) const {
        bool any=false; for (const auto& p:accounts_) if(p->pk.active){ any=true; if(!pred(*p)) return false; } return any;
    }

    bool ValidateAutoPk(std::wstring& error) {
        const int first=FirstEnabledAutoPkStep(); if(first<0){error=L"chưa bật bước nào";return false;}
        int checked=0; const int count=ListView_GetItemCount(clientList_);
        for(int i=0;i<count && i<static_cast<int>(accounts_.size());++i) if(ListView_GetCheckState(clientList_,i)){
            ++checked; Account& a=*accounts_[static_cast<std::size_t>(i)]; if(a.dungeonOwned){error=AccountTag(a)+L": đang thuộc AUTO PHÓ BẢN";return false;} std::wstring e; if(!EnsureAttach(a,e)){error=AccountTag(a)+L": "+e;return false;}
            for(ClickSlot slot:{ClickSlot::AutoMenu,ClickSlot::Attack,ClickSlot::StopAuto2}) if(!a.profile.points[static_cast<std::size_t>(static_cast<int>(slot))].valid){ error=AccountTag(a)+L": thiếu điểm AUTO/ĐÁNH/DỪNG AUTO dùng chung Travel Guard"; return false; }
        }
        if(!checked){error=L"chưa tick acc nào";return false;}
        for(const AutoPkStep& s:autoPkSteps_) if(s.enabled){
            if(auto_pk_logic::NeedsWorldTarget(s.kind) && !s.target.valid){error=s.description+L": chưa GET tọa độ";return false;}
            if(auto_pk_logic::RequiresClicks(s.kind)){
                std::size_t pre=0,post=0; for(const auto& c:s.clicks){ if(!c.point.valid){error=s.description+L": còn click CHƯA F8";return false;} if(c.phase==auto_pk_logic::ClickPhase::EnterPkMode)++pre; if(c.phase==auto_pk_logic::ClickPhase::AutoPk)++post; }
                if(s.kind==auto_pk_logic::StepKind::EnterPk && !auto_pk_logic::EnterPkLayoutReady(pre,post)){error=s.description+L": cần tối thiểu 2 click BẬT PK và 2 click AUTO PK";return false;}
                if(s.kind!=auto_pk_logic::StepKind::EnterPk && s.clicks.empty()){error=s.description+L": chưa có click";return false;}
            }
        }
        return true;
    }

    void StopAutoPk(const std::wstring& reason=L"người dùng STOP") {
        const bool was=autoPkRunning_;
        if(capturePkClickIndex_ >= 0){ capturePkStepIndex_=-1; capturePkClickIndex_=-1; capturePid_=0; }
        autoPkRunning_=false; autoPkRecoveryActive_=false; autoPkEnterPhase_=0; autoPkStepPass_=1; autoPkDueTick_=0;
        for(auto& p:accounts_){ Account& a=*p; if(a.pk.active && a.bridge.Attached()){ Response r{}; std::wstring e; (void)a.bridge.Call(Command::StopPath,0,0,0,r,e,700); } a.pk={}; if(!a.runtime.running) a.runtime.status=L"Đã dừng Auto PK"; }
        if(was) Log(L"AUTO PK STOP • "+reason); SetAutoPkStatus(L"STOP • "+reason);
    }

    void EnterAutoPkTabSafetyStop() {
        if(recorderMode_!=RecorderMode::None) StopRecorder(true);
        if(tradeTxn_.phase!=TradePhase::Idle) AbortTrade(L"chuyển sang tab AUTO PK",GetTickCount());
        StopAutoPk(L"vào tab Auto PK");
        for(auto& p:accounts_) if(p->runtime.running) StopAccount(*p);
        for(std::size_t i=0;i<accounts_.size();++i) UpdateAccountRow(static_cast<int>(i),*accounts_[i]);
        SetAutoPkStatus(L"STOP • AUTO cũ đã hủy để tránh xung đột");
        MessageBoxW(hwnd_, L"AUTO PK đang ở trạng thái STOP.\r\nMọi workflow đang chạy của tab AUTO đã được dừng/hủy để tránh xung đột.\r\nHãy tick acc và bấm BẮT ĐẦU AUTO PK khi đã kiểm tra cấu hình.", L"Cảnh báo Auto PK", MB_OK | MB_ICONWARNING);
    }

    void StartAutoPk() {
        autoPkLifeCheck_ = autoPkLife_ && SendMessageW(autoPkLife_,BM_GETCHECK,0,0)==BST_CHECKED;
        autoPkLoop_ = autoPkLoopCheck_ && SendMessageW(autoPkLoopCheck_,BM_GETCHECK,0,0)==BST_CHECKED; SaveAutoPkSettings();
        std::wstring error; if(!ValidateAutoPk(error)){ Log(L"AUTO PK KHÔNG START • "+error); SetAutoPkStatus(L"LỖI • "+error); return; }
        if(tradeTxn_.phase!=TradePhase::Idle) AbortTrade(L"khởi động AUTO PK",GetTickCount());
        for(auto& p:accounts_) if(p->runtime.running) StopAccount(*p);
        autoPkCurrentStep_=FirstEnabledAutoPkStep(); autoPkStepPass_=1; autoPkEnterPhase_=0; autoPkDueTick_=0; autoPkRecoveryActive_=false; autoPkRunning_=true;
        const int count=ListView_GetItemCount(clientList_);
        for(int i=0;i<count && i<static_cast<int>(accounts_.size());++i){ Account& a=*accounts_[static_cast<std::size_t>(i)]; a.pk={}; if(!ListView_GetCheckState(clientList_,i)) continue; ResetRuntime(a.runtime); a.runtime.running=false; a.runtime.status=L"AUTO PK • chuẩn bị"; a.pk.active=true; ResetAutoPkAccountStep(a); }
        Log(L"AUTO PK START • barrier theo nhóm • dùng hidden InputSync + Travel Guard của tab AUTO."); SetAutoPkStatus(L"RUN • bước "+std::to_wstring(autoPkCurrentStep_+1)+L"/"+std::to_wstring(autoPkSteps_.size()));
    }

    bool TickAutoPkClicks(Account& a, const AutoPkStep& step, auto_pk_logic::ClickPhase phase, DWORD now) {
        if(a.pk.dueTick && static_cast<LONG>(now-a.pk.dueTick)<0) return false;
        std::vector<std::size_t> indices; for(std::size_t i=0;i<step.clicks.size();++i) if(step.clicks[i].phase==phase) indices.push_back(i);
        if(a.pk.clickIndex>=indices.size()){ a.pk.clickIndex=0; a.pk.clickRepeatDone=0; return true; }
        const AutoPkClickStep& c=step.clicks[indices[a.pk.clickIndex]]; std::wstring error;
        if(!CoordinatorInternalPointAction(a,c.point,L"AUTO PK • "+step.description+L" • "+c.description,error)){ a.runtime.status=L"AUTO PK chờ click • "+error; a.pk.dueTick=now+1000; return false; }
        ++a.pk.clickRepeatDone; if(a.pk.clickRepeatDone>=c.repeat){a.pk.clickRepeatDone=0;++a.pk.clickIndex;} a.pk.dueTick=now+static_cast<DWORD>(c.delayMs); return false;
    }

    bool TickAutoPkTreatment(Account& a, const AutoPkStep& step, DWORD now) {
        bool arrived=false; if(a.pk.treatmentStage==0){ HandleRobustTravel(a,now,step.target,L"NPC Đỗ Thanh Đằng ResID 339",arrived,kPreciseWorldTolerance); if(!arrived)return false; a.pk.treatmentStage=1; a.pk.dueTick=now+300; return false; }
        if(a.pk.dueTick && static_cast<LONG>(now-a.pk.dueTick)<0) return false;
        Response r{}; std::wstring error;
        if(a.pk.treatmentStage==1){ if(!a.bridge.Call(Command::BeginBackgroundTreatment,kTreatmentNpcResId,0,0,r,error,1800)){a.runtime.status=L"TRỊ LIỆU mở NPC fail • "+error;a.pk.dueTick=now+1200;return false;} a.pk.treatmentStage=2;a.pk.dueTick=now+500;return false; }
        if(a.pk.treatmentStage==2){ if(!a.bridge.Call(Command::AdvanceBackgroundTreatment,0,0,0,r,error,1800)){a.runtime.status=L"TRỊ LIỆU callback fail • "+error;a.pk.dueTick=now+900;return false;} if(r.resultCode==static_cast<int>(ActionResult::StageReady)) a.pk.treatmentStage=3; a.pk.dueTick=now+500;return false; }
        if(a.pk.treatmentStage==3){ if(!a.bridge.Call(Command::CloseBackgroundTreatment,0,0,0,r,error,1800)){a.runtime.status=L"TRỊ LIỆU đóng UI fail • "+error;a.pk.dueTick=now+900;return false;} ++a.pk.treatmentCloseAttempts; if(r.resultCode==static_cast<int>(ActionResult::NothingToClose)||a.pk.treatmentCloseAttempts>=3){a.runtime.status=L"AUTO PK • trị liệu xong";return true;} a.pk.dueTick=now+350; }
        return false;
    }

    void TickAutoPk(DWORD now) {
        if(!autoPkRunning_||globalPaused_)return;
        if(autoPkDueTick_ && static_cast<LONG>(now-autoPkDueTick_) < 0) return;
        autoPkDueTick_ = 0;
        bool blocked=false;
        for(auto& p:accounts_) if(p->pk.active) {
            if(!p->snapshotValid || HoldUntilClientStable(*p, now)) blocked=true;
        }
        if(blocked){ SetAutoPkStatus(L"GIỮ BARRIER • chờ client/map ổn định"); return; }
        bool anyDead=false, anyRecovery=false;
        for(auto& p:accounts_) if(p->pk.active && p->snapshotValid && (p->snapshot.validMask&ValidLifeState)){
            if(p->snapshot.dead){anyDead=true;p->pk.sawDead=true; if(autoPkLifeCheck_ && (!p->pk.lastReviveTick||Elapsed(now,p->pk.lastReviveTick,5000))){Response r{};std::wstring e;if(p->bridge.Call(Command::Revive,0,0,0,r,e,1500)){p->pk.lastReviveTick=now;LogAccount(*p,L"AUTO PK LIFE GUARD • semantic ĐẦU THAI");}}}
            if(p->pk.sawDead) anyRecovery=true;
        }
        if(anyDead){autoPkRecoveryActive_=true;SetAutoPkStatus(autoPkLifeCheck_?L"LIFE GUARD • chờ toàn nhóm sống":L"CÓ ACC CHẾT • đang giữ barrier");return;}
        if(autoPkRecoveryActive_&&anyRecovery){autoPkCurrentStep_=FirstEnabledAutoPkStep();autoPkStepPass_=1;autoPkEnterPhase_=0;autoPkDueTick_=0;for(auto& p:accounts_)if(p->pk.active){p->pk.sawDead=false;ResetAutoPkAccountStep(*p);}autoPkRecoveryActive_=false;Log(L"AUTO PK LIFE GUARD • toàn nhóm sống → chạy lại chuỗi từ đầu.");}
        if(autoPkCurrentStep_<0||autoPkCurrentStep_>=static_cast<int>(autoPkSteps_.size())){StopAutoPk(L"hết chuỗi");return;}
        AutoPkStep& step=autoPkSteps_[static_cast<std::size_t>(autoPkCurrentStep_)]; if(!step.enabled){autoPkCurrentStep_=NextEnabledAutoPkStep(autoPkCurrentStep_);return;}
        for(auto& p:accounts_) if(p->pk.active && p->snapshotValid){ Account& a=*p; if(a.pk.stepDone)continue;
            if(step.kind==auto_pk_logic::StepKind::Treatment){ if(TickAutoPkTreatment(a,step,now))a.pk.stepDone=true; }
            else if(step.kind==auto_pk_logic::StepKind::Rally){ bool arrived=false;HandleRobustTravel(a,now,step.target,L"điểm tập kết PK phụ",arrived,step.tolerance);if(arrived){a.pk.stepDone=true;a.runtime.status=L"AUTO PK • đã tới điểm tập kết";} }
            else if(step.kind==auto_pk_logic::StepKind::Buff||step.kind==auto_pk_logic::StepKind::Custom){ if(TickAutoPkClicks(a,step,auto_pk_logic::ClickPhase::Normal,now)){a.pk.stepDone=true;a.runtime.status=L"AUTO PK • xong "+step.description;} }
            else if(step.kind==auto_pk_logic::StepKind::EnterPk){
                if(autoPkEnterPhase_==0){ if(TickAutoPkClicks(a,step,auto_pk_logic::ClickPhase::EnterPkMode,now))a.pk.preDone=true; }
                else if(autoPkEnterPhase_==1){ bool arrived=false;HandleRobustTravel(a,now,step.target,L"điểm PK chính",arrived,step.tolerance);if(arrived)a.pk.arrived=true; }
                else { if(!a.pk.postDone){ if(TickAutoPkClicks(a,step,auto_pk_logic::ClickPhase::AutoPk,now)){a.pk.postDone=true;a.pk.dueTick=now+1000;} } else if(!a.pk.dueTick||static_cast<LONG>(now-a.pk.dueTick)>=0){ if((a.snapshot.validMask&ValidAutoFight)&&a.snapshot.autoFight){a.pk.stepDone=true;a.runtime.status=L"AUTO PK • AutoFight ON xác nhận";} else if(a.pk.postVerifyAttempts<3){++a.pk.postVerifyAttempts;a.pk.postDone=false;a.pk.clickIndex=0;a.pk.clickRepeatDone=0;a.pk.dueTick=now+500;} else a.runtime.status=L"AUTO PK • chờ AutoFight ON"; } }
            }
        }
        if(step.kind==auto_pk_logic::StepKind::EnterPk){
            if(autoPkEnterPhase_==0 && AllActivePk([](const Account&a){return a.pk.preDone;})){autoPkEnterPhase_=1;for(auto& p:accounts_)if(p->pk.active){p->pk.clickIndex=0;p->pk.clickRepeatDone=0;p->pk.dueTick=0;}Log(L"AUTO PK BARRIER • tất cả đã bật PK → đồng loạt vào điểm PK chính.");}
            else if(autoPkEnterPhase_==1 && AllActivePk([](const Account&a){return a.pk.arrived;})){autoPkEnterPhase_=2;for(auto& p:accounts_)if(p->pk.active){p->pk.clickIndex=0;p->pk.clickRepeatDone=0;p->pk.dueTick=0;}Log(L"AUTO PK BARRIER • tất cả đã tới điểm chính → mở Auto PK và xác nhận AutoFight ON.");}
        }
        if(!AllActivePk([](const Account&a){return a.pk.stepDone;})){SetAutoPkStatus(L"RUN • bước "+std::to_wstring(autoPkCurrentStep_+1)+L" • "+step.description);return;}
        if(autoPkStepPass_<step.repeat){++autoPkStepPass_;autoPkEnterPhase_=0;for(auto& p:accounts_)if(p->pk.active)ResetAutoPkAccountStep(*p);Log(L"AUTO PK • lặp lại bước "+std::to_wstring(autoPkCurrentStep_+1)+L" lần "+std::to_wstring(autoPkStepPass_));return;}
        const int next=NextEnabledAutoPkStep(autoPkCurrentStep_); if(next>=0){autoPkCurrentStep_=next;autoPkStepPass_=1;autoPkEnterPhase_=0;autoPkDueTick_=now+static_cast<DWORD>(step.delayMs);for(auto& p:accounts_)if(p->pk.active)ResetAutoPkAccountStep(*p);return;}
        if(autoPkLoop_){autoPkCurrentStep_=FirstEnabledAutoPkStep();autoPkStepPass_=1;autoPkEnterPhase_=0;autoPkDueTick_=now+static_cast<DWORD>(step.delayMs);for(auto& p:accounts_)if(p->pk.active)ResetAutoPkAccountStep(*p);Log(L"AUTO PK LOOP • quay về bước đầu.");} else StopAutoPk(L"hoàn tất toàn bộ chuỗi");
    }

    void BuildUi() {
        HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        auto addFont = [font](HWND h){ if (h) SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE); };

        mainTab_ = Make(WC_TABCONTROLW, L"", WS_CLIPSIBLINGS, 18, 4, 420, 32, IDC_MAIN_TAB); addFont(mainTab_);
        if (mainTab_) {
            TCITEMW tab{}; tab.mask = TCIF_TEXT;
            tab.pszText = const_cast<wchar_t*>(L"AUTO"); TabCtrl_InsertItem(mainTab_, 0, &tab);
            tab.pszText = const_cast<wchar_t*>(L"AUTO PK"); TabCtrl_InsertItem(mainTab_, 1, &tab);
            tab.pszText = const_cast<wchar_t*>(L"AUTO PHÓ BẢN"); TabCtrl_InsertItem(mainTab_, 2, &tab);
            tab.pszText = const_cast<wchar_t*>(L"TELEGRAM"); TabCtrl_InsertItem(mainTab_, 3, &tab);
            tab.pszText = const_cast<wchar_t*>(L"GIỚI THIỆU"); TabCtrl_InsertItem(mainTab_, 4, &tab);
            TabCtrl_SetCurSel(mainTab_, 0);
        }
        tradeStatus_ = Make(L"STATIC", L"ĐIỀU PHỐI: khởi động...", SS_LEFT | SS_CENTERIMAGE | WS_BORDER,
                            430, 6, 458, 27, 0); addFont(tradeStatus_);
        compactButton_ = Make(L"BUTTON", L"THU NHỎ", BS_PUSHBUTTON, 896, 6, 127, 27, IDC_COMPACT_TOGGLE); addFont(compactButton_);
        clientList_ = Make(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER,
                           18, 40, 1005, 157, IDC_CLIENT_LIST);
        addFont(clientList_);
        ListView_SetExtendedListViewStyle(clientList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);
        AddListColumn(0, 170, L"Nhân vật / RoleID");
        AddListColumn(1, 72, L"Vai trò");
        AddListColumn(2, 62, L"PID");
        AddListColumn(3, 220, L"Trạng thái");
        AddListColumn(4, 190, L"Map / X,Y / Túi");
        AddListColumn(5, 280, L"Bãi train");

        scanButton_ = Make(L"BUTTON", L"QUÉT CLIENT", BS_PUSHBUTTON, 18, 205, 120, 30, IDC_SCAN); addFont(scanButton_);
        startCheckedButton_ = Make(L"BUTTON", L"BẮT ĐẦU ACC TICK", BS_DEFPUSHBUTTON, 148, 205, 175, 30, IDC_START_CHECKED); addFont(startCheckedButton_);
        stopCheckedButton_ = Make(L"BUTTON", L"DỪNG ACC TICK", BS_PUSHBUTTON, 333, 205, 155, 30, IDC_STOP_CHECKED); addFont(stopCheckedButton_);
        addFont(Make(L"STATIC", L"Vai trò:", SS_LEFT | SS_CENTERIMAGE, 500, 205, 55, 30, 0));
        tradeRoleCombo_ = Make(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL, 558, 205, 105, 330, IDC_TRADE_ROLE); addFont(tradeRoleCombo_);
        for (const wchar_t* r : {L"KHÔNG", L"MAIN"})
            SendMessageW(tradeRoleCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(r));
        for (int child = 1; child <= kChildTradeCount; ++child) {
            const std::wstring role = L"CON " + std::to_wstring(child);
            SendMessageW(tradeRoleCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(role.c_str()));
        }
        selected_ = Make(L"STATIC", L"ACC ĐANG CHỈNH: chưa chọn", SS_LEFT | SS_CENTERIMAGE | WS_BORDER,
                         675, 205, 348, 30, IDC_SELECTED); addFont(selected_);

        live_ = Make(L"STATIC", L"STATE: chưa có", SS_LEFT | SS_CENTERIMAGE | WS_BORDER,
                     18, 243, 510, 38, IDC_LIVE); addFont(live_);
        tradeEnable_ = Make(L"BUTTON", tradeEnabled_ ? L"DỒN ĐỒ: BẬT" : L"DỒN ĐỒ: TẮT",
                            BS_PUSHBUTTON, 538, 247, 120, 27, IDC_CONSOLIDATE_TOGGLE); addFont(tradeEnable_);
        addFont(Make(L"STATIC", L"CON FULL = 0 ô", 0, 663, 252, 92, 22, 0));
        addFont(Make(L"STATIC", L"MAIN < 9 ô → BÁN", 0, 758, 252, 112, 22, 0));
        sellSequenceButton_ = Make(L"BUTTON", L"MACRO BÁN CŨ", BS_PUSHBUTTON, 878, 247, 145, 27, IDC_SELL_SEQUENCE); addFont(sellSequenceButton_);
        tradeRendezvousCaptureButton_ = Make(L"BUTTON", L"TỌA GD • LẤY", BS_PUSHBUTTON, 538, 273, 110, 24, IDC_TRADE_RENDEZVOUS_CAPTURE); addFont(tradeRendezvousCaptureButton_);
        tradeRendezvousLabel_ = Make(L"STATIC", L"CHƯA LẤY TỌA GD", SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 655, 273, 110, 24, 0); addFont(tradeRendezvousLabel_);
        mainTradeSequenceButton_ = Make(L"BUTTON", L"CHUỖI GD MAIN", BS_PUSHBUTTON, 772, 273, 120, 24, IDC_MAIN_TRADE_SEQUENCE); addFont(mainTradeSequenceButton_);
        childTradeSequenceButton_ = Make(L"BUTTON", L"CHUỖI GD ACC CON", BS_PUSHBUTTON, 900, 273, 123, 24, IDC_CHILD_TRADE_SEQUENCE); addFont(childTradeSequenceButton_);

        addFont(Make(L"STATIC", L"SETTING RIÊNG ACC", 0, 18, 290, 150, 20, 0));
        addFont(Make(L"STATIC", L"GD CON: tự đổi acc khi pass cuối làm MAIN nhận ≤8 slot", 0, 560, 288, 463, 20, 0));
        addFont(Make(L"STATIC", L"Bãi:", 0, 18, 316, 45, 22, 0));
        spotCombo_ = Make(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL, 63, 312, 220, 240, IDC_SPOT_COMBO); addFont(spotCombo_);
        addFont(Make(L"STATIC", L"Tên lưu:", 0, 292, 316, 60, 22, 0));
        targetName_ = Make(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, 352, 312, 135, 27, IDC_TARGET_NAME); addFont(targetName_);
        addFont(Make(L"BUTTON", L"LƯU/CẬP NHẬT", BS_PUSHBUTTON, 497, 312, 150, 28, IDC_SAVE_TARGET));
        addFont(Make(L"BUTTON", L"XÓA BÃI", BS_PUSHBUTTON, 657, 312, 90, 28, IDC_DELETE_SPOT));
        targetText_ = Make(L"STATIC", L"CHƯA CHỌN", SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 757, 312, 266, 28, IDC_TARGET_TEXT); addFont(targetText_);

        addFont(Make(L"STATIC", L"Sai số:", 0, 18, 350, 55, 22, 0));
        tolerance_ = Make(L"EDIT", L"120", WS_BORDER | ES_NUMBER | ES_CENTER, 73, 346, 70, 27, IDC_TOLERANCE); addFont(tolerance_);
        enableRevive_ = Make(L"BUTTON", L"Tự Đầu thai", BS_AUTOCHECKBOX, 160, 347, 125, 24, IDC_ENABLE_REVIVE); addFont(enableRevive_);
        enableConfirm_ = Make(L"BUTTON", L"XN Lâu Lan mắc cổng", BS_AUTOCHECKBOX, 300, 347, 175, 24, IDC_ENABLE_CONFIRM); addFont(enableConfirm_);
        enableShortcut_ = Make(L"BUTTON", L"ĐƯỜNG TẮT", BS_AUTOCHECKBOX, 480, 347, 112, 24, IDC_ENABLE_SHORTCUT); addFont(enableShortcut_);
        SendMessageW(enableShortcut_, BM_SETCHECK, shortcutSettings_.enabled ? BST_CHECKED : BST_UNCHECKED, 0);
        shortcutSettingsButton_ = Make(L"BUTTON", L"TÙY CHỈNH", BS_PUSHBUTTON, 596, 345, 124, 27, IDC_SHORTCUT_SETTINGS); addFont(shortcutSettingsButton_);
        enableFight_ = Make(L"BUTTON", L"AUTO → Đánh quái", BS_AUTOCHECKBOX, 730, 347, 145, 24, IDC_ENABLE_FIGHT); addFont(enableFight_);
        addFont(Make(L"STATIC", L"Không foreground/không chiếm chuột", 0, 878, 350, 145, 22, 0));

        addFont(Make(L"STATIC", L"XOAY BÃI TRAIN — mặc định chỉ bãi đang chọn; chỉ bật xoay khi tự tick thêm bãi thứ 2", 0, 18, 382, 1005, 20, 0));
        rotationList_ = Make(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER,
                             18, 404, 1005, 90, IDC_ROTATION_LIST);
        addFont(rotationList_);
        ListView_SetExtendedListViewStyle(rotationList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);
        AddRotationColumn(0, 515, L"Bãi train");
        AddRotationColumn(1, 95, L"Map");
        AddRotationColumn(2, 160, L"X,Y");
        AddRotationColumn(3, 220, L"Ghi chú");

        addFont(Make(L"STATIC", L"Đổi bãi nếu chết quá", 0, 18, 500, 120, 22, 0));
        rotateDeathLimit_ = Make(L"EDIT", L"10", WS_BORDER | ES_NUMBER | ES_CENTER, 140, 497, 45, 27, IDC_ROTATE_DEATH_LIMIT); addFont(rotateDeathLimit_);
        addFont(Make(L"STATIC", L"lần /", 0, 190, 500, 38, 22, 0));
        rotateDeathWindow_ = Make(L"EDIT", L"10", WS_BORDER | ES_NUMBER | ES_CENTER, 230, 497, 45, 27, IDC_ROTATE_DEATH_WINDOW); addFont(rotateDeathWindow_);
        addFont(Make(L"STATIC", L"phút", 0, 280, 500, 38, 22, 0));
        addFont(Make(L"STATIC", L"• Đổi bãi nếu chưa FULL túi trong", 0, 335, 500, 190, 22, 0));
        rotateNoFullBag_ = Make(L"EDIT", L"15", WS_BORDER | ES_NUMBER | ES_CENTER, 530, 497, 45, 27, IDC_ROTATE_NO_BAG); addFont(rotateNoFullBag_);
        addFont(Make(L"STATIC", L"phút train thực • 1 bãi = không đổi • nhiều bãi = vòng lại bãi 1", 0, 580, 500, 443, 22, 0));

        addFont(Make(L"STATIC", L"3 ĐIỂM F8 — AUTO/ĐÁNH QUÁI/DỪNG dùng InputSync; XN/Đầu thai callback semantic không cần tọa độ", 0, 18, 530, 760, 20, 0));
        addFont(Make(L"BUTTON", L"LẤY 3 CLICK CỦA ACC...", BS_PUSHBUTTON, 755, 526, 268, 27, IDC_COPY_CLICKS));
        const int visibleSlots[3] = {static_cast<int>(ClickSlot::AutoMenu), static_cast<int>(ClickSlot::Attack), static_cast<int>(ClickSlot::StopAuto2)};
        const int rowY[3] = {552, 578, 604};
        const int pointIds[3] = {IDC_POINT_AUTO, IDC_POINT_ATTACK, IDC_POINT_STOP_AUTO_2};
        const int captureIds[3] = {IDC_CAPTURE_AUTO, IDC_CAPTURE_ATTACK, IDC_CAPTURE_STOP_AUTO_2};
        const int testIds[3] = {IDC_TEST_AUTO, IDC_TEST_ATTACK, IDC_TEST_STOP_AUTO_2};
        for (int row = 0; row < 3; ++row) {
            const int i = visibleSlots[row];
            addFont(Make(L"STATIC", kClickLabels[static_cast<std::size_t>(i)], SS_LEFT | SS_CENTERIMAGE, 18, rowY[row], 150, 24, 0));
            pointLabels_[static_cast<std::size_t>(i)] = Make(L"STATIC", L"CHƯA LẤY", SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 172, rowY[row], 430, 24, pointIds[row]);
            addFont(pointLabels_[static_cast<std::size_t>(i)]);
            addFont(Make(L"BUTTON", L"LẤY F8", BS_PUSHBUTTON, 612, rowY[row], 115, 24, captureIds[row]));
            addFont(Make(L"BUTTON", L"TEST", BS_PUSHBUTTON, 737, rowY[row], 90, 24, testIds[row]));
        }
        addFont(Make(L"STATIC", L"1 FILE DUY NHẤT: MAP + NPC bán + Đường tắt + TỌA GD + 3 F8 + chuỗi GD động + macro bán", 0, 18, 632, 655, 22, 0));
        addFont(Make(L"BUTTON", L"XUẤT TẤT CẢ", BS_PUSHBUTTON, 678, 628, 145, 27, IDC_EXPORT_CLICK_CONFIG));
        addFont(Make(L"BUTTON", L"NHẬP TẤT CẢ", BS_PUSHBUTTON, 831, 628, 145, 27, IDC_IMPORT_CLICK_CONFIG));
        inventoryFilterOpenButton_ = Make(L"BUTTON", L"LỌC ĐỒ TAY NẢI", BS_PUSHBUTTON, 837, 656, 186, 24, IDC_BAG_FILTER_OPEN); addFont(inventoryFilterOpenButton_);

        enableSell_ = Make(L"BUTTON", L"AUTO BÁN ĐỒ KHI TÚI FULL", BS_AUTOCHECKBOX, 18, 712, 220, 25, IDC_ENABLE_SELL); addFont(enableSell_);
        addFont(Make(L"STATIC", L"NPC bán:", 0, 250, 715, 65, 22, 0));
        sellNpcCombo_ = Make(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL, 315, 710, 250, 180, IDC_SELL_NPC); addFont(sellNpcCombo_);
        for (const auto& npc : kSellNpcs) SendMessageW(sellNpcCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(npc.name));
        addFont(Make(L"STATIC", L"X:", 0, 574, 715, 18, 22, 0));
        sellNpcX_ = Make(L"EDIT", L"", WS_BORDER | ES_NUMBER | ES_CENTER, 592, 710, 58, 27, IDC_SELL_NPC_X); addFont(sellNpcX_);
        addFont(Make(L"STATIC", L"Y:", 0, 658, 715, 18, 22, 0));
        sellNpcY_ = Make(L"EDIT", L"", WS_BORDER | ES_NUMBER | ES_CENTER, 676, 710, 58, 27, IDC_SELL_NPC_Y); addFont(sellNpcY_);
        addFont(Make(L"BUTTON", L"LẤY VỊ TRÍ", BS_PUSHBUTTON, 742, 710, 112, 27, IDC_SELL_NPC_CAPTURE));
        sellNpcPosText_ = Make(L"STATIC", L"CHƯA LẤY", SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 862, 710, 161, 27, IDC_SELL_NPC_POS); addFont(sellNpcPosText_);

        sellMacroList_ = Make(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER, 18, 742, 1005, 72, IDC_SELL_MACRO_LIST);
        addFont(sellMacroList_);
        ListView_SetExtendedListViewStyle(sellMacroList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        AddMacroColumn(0, 36, L"#");
        AddMacroColumn(1, 400, L"Mô tả bước bán");
        AddMacroColumn(2, 230, L"Tọa độ");
        AddMacroColumn(3, 110, L"Delay ms");
        AddMacroColumn(4, 90, L"Lặp");

        sellMacroControls_.push_back(sellMacroList_);
        sellMacroControls_.push_back(Make(L"BUTTON", L"+ THÊM", BS_PUSHBUTTON, 18, 818, 82, 27, IDC_SELL_ADD));
        sellMacroControls_.push_back(Make(L"BUTTON", L"- XÓA", BS_PUSHBUTTON, 108, 818, 82, 27, IDC_SELL_DELETE));
        sellDesc_ = Make(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, 202, 818, 260, 27, IDC_SELL_DESC); addFont(sellDesc_); sellMacroControls_.push_back(sellDesc_);
        sellDelay_ = Make(L"EDIT", L"600", WS_BORDER | ES_NUMBER | ES_CENTER, 470, 818, 75, 27, IDC_SELL_DELAY); addFont(sellDelay_); sellMacroControls_.push_back(sellDelay_);
        sellRepeat_ = Make(L"EDIT", L"1", WS_BORDER | ES_NUMBER | ES_CENTER, 553, 818, 55, 27, IDC_SELL_REPEAT); addFont(sellRepeat_); sellMacroControls_.push_back(sellRepeat_);
        sellMacroControls_.push_back(Make(L"BUTTON", L"LƯU DÒNG", BS_PUSHBUTTON, 616, 818, 100, 27, IDC_SELL_SAVE));
        sellMacroControls_.push_back(Make(L"BUTTON", L"LẤY DÒNG (F8)", BS_PUSHBUTTON, 724, 818, 130, 27, IDC_SELL_CAPTURE));
        sellMacroControls_.push_back(Make(L"BUTTON", L"TEST DÒNG", BS_PUSHBUTTON, 862, 818, 112, 27, IDC_SELL_TEST));
        sellRecordButton_ = Make(L"BUTTON", L"REC", BS_PUSHBUTTON, 18, 850, 92, 27, IDC_SELL_REC); addFont(sellRecordButton_); sellMacroControls_.push_back(sellRecordButton_);
        sellMacroControls_.push_back(Make(L"BUTTON", L"SAO CHÉP", BS_PUSHBUTTON, 118, 850, 104, 27, IDC_SELL_COPY));
        sellMacroControls_.push_back(Make(L"BUTTON", L"DÁN", BS_PUSHBUTTON, 230, 850, 80, 27, IDC_SELL_PASTE));
        sellMacroControls_.push_back(Make(L"BUTTON", L"LẤY CHUỖI CỦA ACC...", BS_PUSHBUTTON, 318, 850, 190, 27, IDC_SELL_COPY_ACCOUNT));
        sellRecordStatus_ = Make(L"STATIC", L"REC: sẵn sàng • chọn một hoặc nhiều dòng để SAO CHÉP", SS_LEFT | SS_CENTERIMAGE, 322, 850, 652, 27, 0); addFont(sellRecordStatus_); sellMacroControls_.push_back(sellRecordStatus_);
        for (HWND h : sellMacroControls_) if (h) ShowWindow(h, SW_HIDE);

        logCaption_ = Make(L"STATIC", L"LOG / BỘ ĐIỀU PHỐI", 0, 18, 742, 190, 20, 0); addFont(logCaption_);
        log_ = Make(L"EDIT", L"", WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, 18, 764, 1005, 159, IDC_LOG); addFont(log_);

        BuildAutoPkUi();
        BuildDungeonUi();

        // v0.6 TELEGRAM is a pure observer/output tab. None of these controls participate
        // in click lease, World Flow, scanner, P1/P2/P3, Sell or Trade state machines.
        auto tg = [&](const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int w, int h, int id) {
            HWND control = Make(cls, text, style, x, y, w, h, id);
            addFont(control);
            telegramControls_.push_back(control);
            if (control) ShowWindow(control, SW_HIDE);
            return control;
        };
        tg(L"BUTTON", L"CẤU HÌNH TELEGRAM BOT", BS_GROUPBOX, 18, 48, 1005, 190, 0);
        telegramEnabled_ = tg(L"BUTTON", L"BẬT THÔNG BÁO TELEGRAM", BS_AUTOCHECKBOX, 35, 70, 220, 24, IDC_TG_ENABLED);
        tg(L"STATIC", L"Bot Token:", SS_LEFT | SS_CENTERIMAGE, 35, 103, 75, 25, 0);
        telegramToken_ = tg(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD, 112, 101, 655, 28, IDC_TG_TOKEN);
        telegramShowToken_ = tg(L"BUTTON", L"HIỆN", BS_PUSHBUTTON, 775, 101, 72, 28, IDC_TG_SHOW_TOKEN);
        tg(L"STATIC", L"Chat ID:", SS_LEFT | SS_CENTERIMAGE, 35, 137, 75, 25, 0);
        telegramChatId_ = tg(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, 112, 135, 350, 28, IDC_TG_CHAT_ID);
        tg(L"BUTTON", L"LƯU CẤU HÌNH", BS_DEFPUSHBUTTON, 475, 135, 128, 28, IDC_TG_SAVE);
        tg(L"BUTTON", L"TEST BOT", BS_PUSHBUTTON, 611, 135, 92, 28, IDC_TG_TEST_BOT);
        tg(L"BUTTON", L"LẤY CHAT ID", BS_PUSHBUTTON, 711, 135, 108, 28, IDC_TG_DISCOVER_CHAT);
        tg(L"BUTTON", L"GỬI TIN THỬ", BS_PUSHBUTTON, 827, 135, 110, 28, IDC_TG_SEND_TEST);
        tg(L"BUTTON", L"GỬI BÁO CÁO NGAY", BS_PUSHBUTTON, 827, 169, 160, 28, IDC_TG_SEND_SUMMARY);
        telegramStatus_ = tg(L"STATIC", L"Telegram worker: khởi động...", SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 35, 169, 775, 28, IDC_TG_STATUS);
        tg(L"STATIC", L"HƯỚNG DẪN: 1) Telegram → @BotFather → /newbot → dán Token.  2) Mở bot và bấm Start / gửi 1 tin.  3) Bấm TEST BOT → LẤY CHAT ID → GỬI TIN THỬ.  Group: thêm bot vào group rồi gửi /test trước khi LẤY CHAT ID. Nếu bot đang dùng webhook thì getUpdates không lấy được Chat ID; có thể nhập Chat ID thủ công.",
           SS_LEFT, 35, 203, 950, 30, 0);

        tg(L"BUTTON", L"SỰ KIỆN / BÁO CÁO", BS_GROUPBOX, 18, 245, 1005, 285, 0);
        telegramNotifyDeath_ = tg(L"BUTTON", L"Acc chết", BS_AUTOCHECKBOX, 38, 272, 155, 24, IDC_TG_NOTIFY_DEATH);
        telegramNotifyRevive_ = tg(L"BUTTON", L"Sống lại / Đầu thai xong", BS_AUTOCHECKBOX, 38, 300, 210, 24, IDC_TG_NOTIFY_REVIVE);
        telegramNotifySellComplete_ = tg(L"BUTTON", L"Mỗi lượt bán xong (dễ spam)", BS_AUTOCHECKBOX, 38, 328, 230, 24, IDC_TG_NOTIFY_SELL_COMPLETE);
        telegramNotifySellSummary_ = tg(L"BUTTON", L"Báo cáo bán đồ định kỳ", BS_AUTOCHECKBOX, 38, 356, 220, 24, IDC_TG_NOTIFY_SELL_SUMMARY);

        telegramNotifyTrade_ = tg(L"BUTTON", L"GD CON → MAIN hoàn tất", BS_AUTOCHECKBOX, 300, 272, 220, 24, IDC_TG_NOTIFY_TRADE);
        telegramNotifyFreeze_ = tg(L"BUTTON", L"Client Freeze + hồi phục", BS_AUTOCHECKBOX, 300, 300, 220, 24, IDC_TG_NOTIFY_FREEZE);
        telegramNotifyFifo_ = tg(L"BUTTON", L"CON FULL → vào FIFO (dễ spam)", BS_AUTOCHECKBOX, 300, 328, 240, 24, IDC_TG_NOTIFY_FIFO);
        telegramNotifyLauLan_ = tg(L"BUTTON", L"Lâu Lan đã auto XN", BS_AUTOCHECKBOX, 300, 356, 200, 24, IDC_TG_NOTIFY_LAULAN);

        telegramNotifyWorldFlowTimeout_ = tg(L"BUTTON", L"WorldFlow đi TỌA GD quá lâu", BS_AUTOCHECKBOX, 565, 272, 245, 24, IDC_TG_NOTIFY_WORLDFLOW_TIMEOUT);
        telegramNotifyToolState_ = tg(L"BUTTON", L"Start / F4 Pause / Resume", BS_AUTOCHECKBOX, 565, 300, 220, 24, IDC_TG_NOTIFY_TOOL_STATE);
        telegramNotifySessionSummary_ = tg(L"BUTTON", L"Tổng kết khi session kết thúc", BS_AUTOCHECKBOX, 565, 328, 250, 24, IDC_TG_NOTIFY_SESSION_SUMMARY);
        telegramNotifyFunAlerts_ = tg(L"BUTTON", L"Cảnh báo vui + mốc vàng", BS_AUTOCHECKBOX, 565, 356, 250, 24, IDC_TG_NOTIFY_FUN_ALERTS);

        telegramIntervalEnabled_ = tg(L"BUTTON", L"Báo cáo mỗi", BS_AUTOCHECKBOX, 38, 402, 112, 24, IDC_TG_INTERVAL_ENABLED);
        telegramIntervalMinutes_ = tg(L"EDIT", L"60", WS_BORDER | ES_NUMBER | ES_CENTER, 150, 400, 55, 27, IDC_TG_INTERVAL_MINUTES);
        tg(L"STATIC", L"phút (5–1440)", SS_LEFT | SS_CENTERIMAGE, 212, 402, 105, 24, 0);
        telegramDailyEnabled_ = tg(L"BUTTON", L"Mốc cố định:", BS_AUTOCHECKBOX, 330, 402, 110, 24, IDC_TG_DAILY_ENABLED);
        const int timeIds[4] = {IDC_TG_DAILY_TIME1, IDC_TG_DAILY_TIME2, IDC_TG_DAILY_TIME3, IDC_TG_DAILY_TIME4};
        for (int i = 0; i < 4; ++i) telegramDailyTime_[static_cast<std::size_t>(i)] = tg(L"EDIT", L"", WS_BORDER | ES_CENTER, 445 + i * 70, 400, 62, 27, timeIds[i]);
        tg(L"STATIC", L"WorldFlow timeout:", SS_LEFT | SS_CENTERIMAGE, 38, 442, 125, 24, 0);
        telegramWorldFlowTimeoutSec_ = tg(L"EDIT", L"120", WS_BORDER | ES_NUMBER | ES_CENTER, 165, 440, 60, 27, IDC_TG_WORLDFLOW_TIMEOUT_SEC);
        tg(L"STATIC", L"giây (30–3600) • CHỈ CẢNH BÁO Telegram, tuyệt đối không reset/stop workflow", SS_LEFT | SS_CENTERIMAGE, 232, 442, 560, 24, 0);
        tg(L"STATIC", L"Mốc vàng:", SS_LEFT | SS_CENTERIMAGE, 38, 482, 75, 24, 0);
        const int moneyIds[5] = {IDC_TG_MONEY_1M, IDC_TG_MONEY_5M, IDC_TG_MONEY_60M, IDC_TG_MONEY_6H, IDC_TG_MONEY_24H};
        const wchar_t* moneyLabels[5] = {L"1 phút (test)", L"5 phút (test)", L"60 phút", L"6 tiếng", L"24 tiếng"};
        const int moneyX[5] = {112, 240, 368, 478, 588};
        const int moneyW[5] = {120, 120, 100, 100, 110};
        for (int i = 0; i < 5; ++i) telegramMoneyMilestone_[static_cast<std::size_t>(i)] =
            tg(L"BUTTON", moneyLabels[i], BS_AUTOCHECKBOX, moneyX[i], 482, moneyW[i], 24, moneyIds[i]);
        tg(L"STATIC", L"1m/5m mặc định tắt; bật riêng để test nhanh.", SS_LEFT | SS_CENTERIMAGE, 710, 482, 270, 24, 0);

        tg(L"BUTTON", L"TELEGRAM LOG — queue tối đa 200 event • UI giữ 500 dòng gần nhất • network retry tối đa 3 lần", BS_GROUPBOX, 18, 540, 1005, 386, 0);
        telegramLog_ = tg(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER, 35, 568, 970, 306, IDC_TG_LOG);
        ListView_SetExtendedListViewStyle(telegramLog_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        auto addTgCol = [&](int index, int width, const wchar_t* text) {
            LVCOLUMNW c{}; c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM; c.pszText = const_cast<wchar_t*>(text); c.cx = width; c.iSubItem = index;
            ListView_InsertColumn(telegramLog_, index, &c);
        };
        addTgCol(0, 82, L"Time"); addTgCol(1, 150, L"Event"); addTgCol(2, 250, L"Account"); addTgCol(3, 72, L"Result"); addTgCol(4, 405, L"Detail");
        tg(L"BUTTON", L"XÓA LOG", BS_PUSHBUTTON, 35, 884, 105, 28, IDC_TG_CLEAR_LOG);
        tg(L"BUTTON", L"SAO CHÉP LOG", BS_PUSHBUTTON, 150, 884, 130, 28, IDC_TG_COPY_LOG);
        tg(L"STATIC", L"Bot Token không bao giờ ghi vào log. INI chỉ lưu BotTokenProtected bằng Windows DPAPI theo user Windows hiện tại.", SS_LEFT | SS_CENTERIMAGE, 300, 884, 700, 28, 0);

        LoadTelegramSettingsToUi();
        if (!telegramWorker_.Start(hwnd_, kTelegramResultMessage)) SetTelegramStatus(L"Telegram worker START FAIL");
        else SetTelegramStatus(L"Telegram worker READY • network tách khỏi toàn bộ auto");
        if (!telegramLoadWarning_.empty()) AddTelegramLog(L"CONFIG", L"-", L"WARN", telegramLoadWarning_);


        aboutHeadingFont_ = CreateFontW(-25, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        aboutNameFont_ = CreateFontW(-20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                     DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        aboutUpcomingFont_ = CreateFontW(-32, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                         DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        aboutBodyFont_ = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                     DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        HWND aboutHeading = Make(L"STATIC", L"GIỚI THIỆU", SS_CENTER | SS_CENTERIMAGE, 55, 62, 950, 42, 0);
        HWND aboutName = Make(L"STATIC", L"Thiết kế và phát triển bởi Thắng Nguyễn - ĐỒ LONG",
                              SS_CENTER | SS_CENTERIMAGE | WS_BORDER, 55, 112, 950, 46, 0);
        HWND aboutUpcoming = Make(L"STATIC", L"CÁC TÍNH NĂNG SẮP RA MẮT",
                                  SS_CENTER | SS_CENTERIMAGE | WS_BORDER, 55, 170, 950, 66, 0);
        HWND aboutVersion = Make(L"STATIC", L"AUTO Thần Long đa tính năng Pro • v4.0",
                                 SS_CENTER | SS_CENTERIMAGE, 55, 242, 950, 28, 0);
        HWND aboutBody = Make(L"EDIT", kUpcomingFeaturesText,
                              WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL,
                              55, 280, 950, 642, 0);
        if (aboutHeading && aboutHeadingFont_) SendMessageW(aboutHeading, WM_SETFONT, reinterpret_cast<WPARAM>(aboutHeadingFont_), TRUE);
        if (aboutName && aboutNameFont_) SendMessageW(aboutName, WM_SETFONT, reinterpret_cast<WPARAM>(aboutNameFont_), TRUE);
        if (aboutUpcoming && aboutUpcomingFont_) SendMessageW(aboutUpcoming, WM_SETFONT, reinterpret_cast<WPARAM>(aboutUpcomingFont_), TRUE);
        if (aboutVersion && aboutNameFont_) SendMessageW(aboutVersion, WM_SETFONT, reinterpret_cast<WPARAM>(aboutNameFont_), TRUE);
        if (aboutBody && aboutBodyFont_) {
            SendMessageW(aboutBody, WM_SETFONT, reinterpret_cast<WPARAM>(aboutBodyFont_), TRUE);
            SendMessageW(aboutBody, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(12, 12));
        }
        aboutControls_ = {aboutHeading, aboutName, aboutUpcoming, aboutVersion, aboutBody};
        for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_HIDE);

        if (!RegisterHotKey(hwnd_, kCaptureHotkeyId, MOD_NOREPEAT, VK_F8)) {
            Log(L"CẢNH BÁO: không đăng ký được F8 global.");
        }
        if (!RegisterHotKey(hwnd_, kPauseHotkeyId, MOD_NOREPEAT, VK_F4)) {
            Log(L"CẢNH BÁO: không đăng ký được F4 global.");
        }
        Log(L"HIDDEN ACTION ENGINE ON • auto-click dùng InputSync nội bộ; không chiếm chuột Windows.");
        SetTimer(hwnd_, kTimer, 250, nullptr);
        UpdateTradeRendezvousLabel();
        UpdateRoleActionButtons();
        ScanClients();
    }

#include "dungeon_app_methods.inl"

    bool IsAboutControl(HWND h) const {
        return std::find(aboutControls_.begin(), aboutControls_.end(), h) != aboutControls_.end();
    }

    bool IsTelegramControl(HWND h) const {
        return std::find(telegramControls_.begin(), telegramControls_.end(), h) != telegramControls_.end();
    }

    void EnterDungeonTabSafetyStop() {
        if (recorderMode_ != RecorderMode::None) StopRecorder(true);
        if (tradeTxn_.phase != TradePhase::Idle) AbortTrade(L"chuyển sang tab AUTO PHÓ BẢN", GetTickCount());
        StopAutoPk(L"vào tab AUTO PHÓ BẢN");
        for (auto& p : accounts_) {
            if (p && p->runtime.running && !p->dungeonOwned) StopAccount(*p);
        }
        ReleaseTradeHolds();
        for (std::size_t i = 0; i < accounts_.size(); ++i) UpdateAccountRow(static_cast<int>(i), *accounts_[i]);
        Log(L"AUTO PHÓ BẢN • TAB OWNERSHIP: đã STOP toàn bộ AUTO + AUTO PK trước khi điều phối đội.");
    }

    void SwitchMainTab(int index) {
        if (!mainTab_) return;
        index = std::clamp(index, 0, 4);
        if (index == mainTabIndex_) return;

        if (mainTabIndex_ == 0) {
            autoTabVisibility_.clear();
            for (HWND child = GetWindow(hwnd_, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
                if (child == mainTab_ || IsAboutControl(child) || IsAutoPkControl(child) || IsDungeonControl(child) || IsTelegramControl(child)) continue;
                autoTabVisibility_.push_back({child, IsWindowVisible(child) != FALSE});
                ShowWindow(child, SW_HIDE);
            }
        } else if (mainTabIndex_ == 1) {
            StopAutoPk(L"rời tab Auto PK"); ShowAutoPkControls(false); ShowAutoPkShared(false);
        } else if (mainTabIndex_ == 2) {
            ShowDungeonControls(false); // teams keep running when the user views another tab.
        } else if (mainTabIndex_ == 3) {
            for (HWND h : telegramControls_) if (h) ShowWindow(h, SW_HIDE);
        } else {
            for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_HIDE);
        }

        ShowAutoPkControls(false); ShowAutoPkShared(false); ShowDungeonControls(false);
        for (HWND h : telegramControls_) if (h) ShowWindow(h, SW_HIDE);
        for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_HIDE);
        if (index == 0) {
            for (const auto& saved : autoTabVisibility_) if (saved.first && IsWindow(saved.first)) ShowWindow(saved.first, saved.second ? SW_SHOW : SW_HIDE);
            autoTabVisibility_.clear();
        } else if (index == 1) {
            ShowAutoPkControls(true); ShowAutoPkShared(true); EnterAutoPkTabSafetyStop();
        } else if (index == 2) {
            EnterDungeonTabSafetyStop();
            ShowDungeonControls(true); RefreshDungeonAccountList(); RefreshDungeonTeamList(); RefreshDungeonStepList();
        } else if (index == 3) {
            for (HWND h : telegramControls_) if (h) ShowWindow(h, SW_SHOW);
        } else {
            for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_SHOW);
        }
        mainTabIndex_ = index;
    }

    bool IsCompactKeepControl(HWND h) const {
        return h == tradeStatus_ || h == clientList_ || h == scanButton_ ||
               h == startCheckedButton_ || h == stopCheckedButton_ || h == compactButton_;
    }

    void ToggleCompactMode() {
        if (!compactMode_) {
            // Compact mode is intentionally the AUTO overview only: account rows,
            // coordinator status and the three existing scan/start/pause controls.
            if (mainTabIndex_ != 0) SwitchMainTab(0);
            compactVisibility_.clear();
            for (HWND child = GetWindow(hwnd_, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
                compactVisibility_.push_back({child, IsWindowVisible(child) != FALSE});
                if (!IsCompactKeepControl(child)) ShowWindow(child, SW_HIDE);
            }
            if (tradeStatus_) SetWindowPos(tradeStatus_, nullptr, 18, 6, 870, 27, SWP_NOZORDER | SWP_NOACTIVATE);
            if (compactButton_) {
                SetWindowTextW(compactButton_, L"MỞ RỘNG");
                ShowWindow(compactButton_, SW_SHOW);
            }
            if (clientList_) ShowWindow(clientList_, SW_SHOW);
            if (scanButton_) ShowWindow(scanButton_, SW_SHOW);
            if (startCheckedButton_) ShowWindow(startCheckedButton_, SW_SHOW);
            if (stopCheckedButton_) ShowWindow(stopCheckedButton_, SW_SHOW);
            if (tradeStatus_) ShowWindow(tradeStatus_, SW_SHOW);
            SetWindowPos(hwnd_, nullptr, 0, 0, 1060, 285, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            compactMode_ = true;
            return;
        }

        for (const auto& saved : compactVisibility_) {
            if (saved.first && IsWindow(saved.first)) ShowWindow(saved.first, saved.second ? SW_SHOW : SW_HIDE);
        }
        compactVisibility_.clear();
        if (tradeStatus_) SetWindowPos(tradeStatus_, nullptr, 430, 6, 458, 27, SWP_NOZORDER | SWP_NOACTIVATE);
        if (compactButton_) {
            SetWindowTextW(compactButton_, L"THU NHỎ");
            ShowWindow(compactButton_, SW_SHOW);
        }
        SetWindowPos(hwnd_, nullptr, 0, 0, 1060, 1030, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        compactMode_ = false;
    }

    void Log(const std::wstring& text) {
        if (!log_) return;
        SYSTEMTIME st{};
        GetLocalTime(&st);
        wchar_t prefix[32]{};
        wsprintfW(prefix, L"[%02u:%02u:%02u] ", st.wHour, st.wMinute, st.wSecond);
        std::wstring line = prefix + text + L"\r\n";
        const int len = GetWindowTextLengthW(log_);
        SendMessageW(log_, EM_SETSEL, len, len);
        SendMessageW(log_, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(line.c_str()));
        SendMessageW(log_, EM_SCROLLCARET, 0, 0);
    }

    std::wstring AccountTag(const Account& a) const {
        if (!a.displayName.empty()) return a.displayName + L"/PID " + std::to_wstring(a.game.pid);
        return L"PID " + std::to_wstring(a.game.pid);
    }

    void LogAccount(const Account& a, const std::wstring& text) {
        Log(L"[" + AccountTag(a) + L"] " + text);
    }

    int SelectedIndex() const {
        if (!clientList_) return -1;
        return ListView_GetNextItem(clientList_, -1, LVNI_SELECTED);
    }

    Account* SelectedAccount() {
        const int i = SelectedIndex();
        if (i < 0 || i >= static_cast<int>(accounts_.size())) return nullptr;
        return accounts_[static_cast<std::size_t>(i)].get();
    }

    Account* AccountByPid(DWORD pid) {
        for (auto& a : accounts_) if (a->game.pid == pid) return a.get();
        return nullptr;
    }

    static std::wstring TrimWs(std::wstring text) {
        auto ws = [](wchar_t c){ return c == L' ' || c == L'\t' || c == L'\r' || c == L'\n'; };
        while (!text.empty() && ws(text.front())) text.erase(text.begin());
        while (!text.empty() && ws(text.back())) text.pop_back();
        return text;
    }

    static std::wstring LocalDateTimeText() {
        SYSTEMTIME st{}; GetLocalTime(&st);
        wchar_t buf[64]{};
        wsprintfW(buf, L"%02u/%02u/%04u %02u:%02u:%02u", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
        return buf;
    }

    static std::wstring LocalTimeText() {
        SYSTEMTIME st{}; GetLocalTime(&st);
        wchar_t buf[32]{};
        wsprintfW(buf, L"%02u:%02u:%02u", st.wHour, st.wMinute, st.wSecond);
        return buf;
    }

    static std::wstring LocalHourMinuteText() {
        SYSTEMTIME st{}; GetLocalTime(&st);
        wchar_t buf[16]{};
        wsprintfW(buf, L"%02u:%02u", st.wHour, st.wMinute);
        return buf;
    }

    static std::wstring FormatDurationSeconds(ULONGLONG ms) {
        const ULONGLONG sec = ms / 1000ULL;
        const ULONGLONG h = sec / 3600ULL;
        const ULONGLONG m = (sec % 3600ULL) / 60ULL;
        const ULONGLONG s = sec % 60ULL;
        if (h > 0) return std::to_wstring(h) + L"h " + std::to_wstring(m) + L"m " + std::to_wstring(s) + L"s";
        if (m > 0) return std::to_wstring(m) + L"m " + std::to_wstring(s) + L"s";
        return std::to_wstring(s) + L"s";
    }

    std::wstring TelegramAccountLabel(const Account& a) const {
        std::wstring role = TradeRoleLabel(a.profile.tradeRole);
        if (role == L"-") role = L"ACC";
        // Telegram v1.3 is intentionally human-readable only: never append RoleID/PID.
        // displayName includes numeric identity in the main UI, so read the plain runtime name instead.
        if ((a.snapshot.validMask & ValidIdentity) && a.snapshot.characterName[0] != 0)
            return role + L" • " + std::wstring(a.snapshot.characterName);
        return role;
    }

    bool TelegramCheck(HWND h) const { return h && SendMessageW(h, BM_GETCHECK, 0, 0) == BST_CHECKED; }
    void SetTelegramCheck(HWND h, bool value) { if (h) SendMessageW(h, BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED, 0); }

    void SetTelegramStatus(const std::wstring& text) {
        if (telegramStatus_) SetText(telegramStatus_, text);
    }

    void AddTelegramLog(const std::wstring& eventType, const std::wstring& account,
                        const std::wstring& result, const std::wstring& detail) {
        if (!telegramLog_) return;
        if (ListView_GetItemCount(telegramLog_) >= 500) ListView_DeleteItem(telegramLog_, 0);
        const int row = ListView_GetItemCount(telegramLog_);
        std::wstring t = LocalTimeText();
        LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = row; item.iSubItem = 0; item.pszText = t.data();
        ListView_InsertItem(telegramLog_, &item);
        ListView_SetItemText(telegramLog_, row, 1, const_cast<wchar_t*>(eventType.c_str()));
        ListView_SetItemText(telegramLog_, row, 2, const_cast<wchar_t*>(account.c_str()));
        ListView_SetItemText(telegramLog_, row, 3, const_cast<wchar_t*>(result.c_str()));
        ListView_SetItemText(telegramLog_, row, 4, const_cast<wchar_t*>(detail.c_str()));
        ListView_EnsureVisible(telegramLog_, row, FALSE);
    }

    void ClearTelegramLog() {
        if (telegramLog_) ListView_DeleteAllItems(telegramLog_);
        SetTelegramStatus(L"Telegram log đã xóa");
    }

    void CopyTelegramLog() {
        if (!telegramLog_) return;
        std::wstring text = L"Time\tEvent\tAccount\tResult\tDetail\r\n";
        wchar_t cell[1024]{};
        const int rows = ListView_GetItemCount(telegramLog_);
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < 5; ++c) {
                cell[0] = 0;
                ListView_GetItemText(telegramLog_, r, c, cell, _countof(cell));
                if (c) text += L"\t";
                text += cell;
            }
            text += L"\r\n";
        }
        if (!OpenClipboard(hwnd_)) { SetTelegramStatus(L"Không mở được Clipboard"); return; }
        EmptyClipboard();
        const SIZE_T bytes = (text.size() + 1) * sizeof(wchar_t);
        HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (mem) {
            void* dst = GlobalLock(mem);
            if (dst) { std::memcpy(dst, text.c_str(), bytes); GlobalUnlock(mem); SetClipboardData(CF_UNICODETEXT, mem); mem = nullptr; }
        }
        if (mem) GlobalFree(mem);
        CloseClipboard();
        SetTelegramStatus(L"Đã sao chép Telegram log");
    }

    void LoadTelegramSettingsToUi() {
        SetTelegramCheck(telegramEnabled_, telegramSettings_.enabled);
        SetText(telegramToken_, telegramSettings_.botToken);
        if (telegramToken_) { SendMessageW(telegramToken_, EM_SETPASSWORDCHAR, static_cast<WPARAM>(L'●'), 0); InvalidateRect(telegramToken_, nullptr, TRUE); }
        telegramTokenVisible_ = false;
        SetText(telegramChatId_, telegramSettings_.chatId);
        SetTelegramCheck(telegramNotifyDeath_, telegramSettings_.notifyDeath);
        SetTelegramCheck(telegramNotifyRevive_, telegramSettings_.notifyRevive);
        SetTelegramCheck(telegramNotifySellComplete_, telegramSettings_.notifySellComplete);
        SetTelegramCheck(telegramNotifySellSummary_, telegramSettings_.notifySellSummary);
        SetTelegramCheck(telegramNotifyTrade_, telegramSettings_.notifyTradeComplete);
        SetTelegramCheck(telegramNotifyFreeze_, telegramSettings_.notifyClientFreeze);
        SetTelegramCheck(telegramNotifyFifo_, telegramSettings_.notifyFifoEnter);
        SetTelegramCheck(telegramNotifyLauLan_, telegramSettings_.notifyLauLanConfirm);
        SetTelegramCheck(telegramNotifyWorldFlowTimeout_, telegramSettings_.notifyWorldFlowTimeout);
        SetTelegramCheck(telegramNotifyToolState_, telegramSettings_.notifyToolState);
        SetTelegramCheck(telegramNotifySessionSummary_, telegramSettings_.notifySessionSummary);
        SetTelegramCheck(telegramNotifyFunAlerts_, telegramSettings_.notifyFunAlerts);
        for (std::size_t i = 0; i < telegramMoneyMilestone_.size(); ++i)
            SetTelegramCheck(telegramMoneyMilestone_[i], telegramSettings_.currencyMilestones[i]);
        SetTelegramCheck(telegramIntervalEnabled_, telegramSettings_.intervalEnabled);
        SetText(telegramIntervalMinutes_, std::to_wstring(telegramSettings_.intervalMinutes));
        SetTelegramCheck(telegramDailyEnabled_, telegramSettings_.dailyEnabled);
        for (std::size_t i = 0; i < telegramDailyTime_.size(); ++i) if (telegramDailyTime_[i]) SetText(telegramDailyTime_[i], telegramSettings_.dailyTimes[i]);
        SetText(telegramWorldFlowTimeoutSec_, std::to_wstring(telegramSettings_.worldFlowTimeoutSec));
    }

    bool PersistTelegramSettingsFromUi(bool feedback) {
        if (!telegramToken_) return false;
        TelegramSettings next = telegramSettings_;
        next.enabled = TelegramCheck(telegramEnabled_);
        next.botToken = TrimWs(GetText(telegramToken_));
        next.chatId = TrimWs(GetText(telegramChatId_));
        next.notifyDeath = TelegramCheck(telegramNotifyDeath_);
        next.notifyRevive = TelegramCheck(telegramNotifyRevive_);
        next.notifySellComplete = TelegramCheck(telegramNotifySellComplete_);
        next.notifySellSummary = TelegramCheck(telegramNotifySellSummary_);
        next.notifyTradeComplete = TelegramCheck(telegramNotifyTrade_);
        next.notifyClientFreeze = TelegramCheck(telegramNotifyFreeze_);
        next.notifyFifoEnter = TelegramCheck(telegramNotifyFifo_);
        next.notifyLauLanConfirm = TelegramCheck(telegramNotifyLauLan_);
        next.notifyWorldFlowTimeout = TelegramCheck(telegramNotifyWorldFlowTimeout_);
        next.notifyToolState = TelegramCheck(telegramNotifyToolState_);
        next.notifySessionSummary = TelegramCheck(telegramNotifySessionSummary_);
        next.notifyFunAlerts = TelegramCheck(telegramNotifyFunAlerts_);
        for (std::size_t i = 0; i < next.currencyMilestones.size(); ++i)
            next.currencyMilestones[i] = TelegramCheck(telegramMoneyMilestone_[i]);
        next.intervalEnabled = TelegramCheck(telegramIntervalEnabled_);
        next.intervalMinutes = telegram_logic::ClampSummaryIntervalMinutes(_wtoi(GetText(telegramIntervalMinutes_).c_str()));
        next.dailyEnabled = TelegramCheck(telegramDailyEnabled_);
        const std::array<std::wstring, 4> defaults{L"08:00", L"12:00", L"18:00", L"23:00"};
        for (std::size_t i = 0; i < next.dailyTimes.size(); ++i) {
            next.dailyTimes[i] = telegram_logic::NormalizeDailyTime(TrimWs(GetText(telegramDailyTime_[i])), defaults[i]);
            SetText(telegramDailyTime_[i], next.dailyTimes[i]);
        }
        next.worldFlowTimeoutSec = telegram_logic::ClampWorldFlowTimeoutSeconds(_wtoi(GetText(telegramWorldFlowTimeoutSec_).c_str()));
        SetText(telegramIntervalMinutes_, std::to_wstring(next.intervalMinutes));
        SetText(telegramWorldFlowTimeoutSec_, std::to_wstring(next.worldFlowTimeoutSec));

        std::wstring error;
        if (!SaveTelegramSettings(next, error)) {
            AddTelegramLog(L"CONFIG", L"-", L"FAIL", error);
            SetTelegramStatus(L"Lưu cấu hình Telegram thất bại");
            return false;
        }
        telegramSettings_ = std::move(next);
        if (feedback) {
            AddTelegramLog(L"CONFIG", L"-", L"OK", L"Đã lưu • Bot Token được mã hóa bằng Windows DPAPI");
            SetTelegramStatus(L"Đã lưu cấu hình Telegram bằng DPAPI");
        }
        return true;
    }

    void ToggleTelegramTokenVisible() {
        if (!telegramToken_) return;
        telegramTokenVisible_ = !telegramTokenVisible_;
        SendMessageW(telegramToken_, EM_SETPASSWORDCHAR, telegramTokenVisible_ ? 0 : static_cast<WPARAM>(L'●'), 0);
        InvalidateRect(telegramToken_, nullptr, TRUE);
        if (telegramShowToken_) SetText(telegramShowToken_, telegramTokenVisible_ ? L"ẨN" : L"HIỆN");
    }

    bool QueueTelegramRequest(telegram_notify::TaskKind kind, const std::wstring& message,
                              const std::wstring& eventType, const std::wstring& account,
                              bool forceManual = false) {
        if (!forceManual && !telegramSettings_.enabled) return false;
        if (!telegram_logic::LooksLikeBotToken(telegramSettings_.botToken)) {
            if (forceManual) AddTelegramLog(eventType, account, L"FAIL", L"Bot Token chưa hợp lệ");
            SetTelegramStatus(L"Telegram: Bot Token chưa hợp lệ");
            return false;
        }
        if (kind == telegram_notify::TaskKind::SendMessage && !telegram_logic::LooksLikeTelegramChatId(telegramSettings_.chatId)) {
            if (forceManual) AddTelegramLog(eventType, account, L"FAIL", L"Chat ID chưa hợp lệ");
            SetTelegramStatus(L"Telegram: Chat ID chưa hợp lệ");
            return false;
        }
        telegram_notify::Request req{};
        req.id = ++telegramRequestCounter_;
        req.kind = kind;
        req.botToken = telegramSettings_.botToken;
        req.chatId = telegramSettings_.chatId;
        req.message = message;
        req.eventType = eventType;
        req.account = account;
        if (!telegramWorker_.Enqueue(std::move(req))) {
            AddTelegramLog(eventType, account, L"DROP", L"Worker dừng hoặc queue đã đầy 200 event");
            SetTelegramStatus(L"Telegram queue đầy/dừng • event bị DROP");
            return false;
        }
        SetTelegramStatus(L"Telegram queue: " + std::to_wstring(telegramWorker_.Pending()) + L" pending");
        return true;
    }

    void TelegramTestBot() {
        if (!PersistTelegramSettingsFromUi(false)) return;
        (void)QueueTelegramRequest(telegram_notify::TaskKind::TestBot, L"", L"TEST BOT", L"-", true);
    }

    void TelegramDiscoverChatId() {
        if (!PersistTelegramSettingsFromUi(false)) return;
        (void)QueueTelegramRequest(telegram_notify::TaskKind::DiscoverChatId, L"", L"LẤY CHAT ID", L"-", true);
    }

    void TelegramSendTest() {
        if (!PersistTelegramSettingsFromUi(false)) return;
        const std::wstring msg = L"✅ Thần Long Item Consolidator v1.5\nTelegram kết nối thành công.\nThời gian: " + LocalDateTimeText();
        (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, msg, L"TIN THỬ", L"-", true);
    }

    void HandleTelegramWorkerResult(LPARAM lp) {
        std::unique_ptr<telegram_notify::Result> result(reinterpret_cast<telegram_notify::Result*>(lp));
        if (!result) return;
        if (result->kind == telegram_notify::TaskKind::DiscoverChatId && result->ok && !result->discoveredChatId.empty()) {
            telegramSettings_.chatId = result->discoveredChatId;
            SetText(telegramChatId_, telegramSettings_.chatId);
            std::wstring saveError;
            if (!SaveTelegramSettings(telegramSettings_, saveError)) result->detail += L" • lấy được Chat ID nhưng lưu config FAIL: " + saveError;
        }
        AddTelegramLog(result->eventType.empty() ? L"TELEGRAM" : result->eventType,
                       result->account.empty() ? L"-" : result->account,
                       result->ok ? L"OK" : L"FAIL", result->detail);
        SetTelegramStatus(std::wstring(result->ok ? L"Telegram OK • " : L"Telegram FAIL • ") + result->detail);
    }

    bool AnyRunningAccount() const {
        return std::any_of(accounts_.begin(), accounts_.end(), [](const std::unique_ptr<Account>& a){ return a && a->runtime.running; });
    }

    void ResetTelegramReportBaseline() {
        telegramReportBaselineTime_ = LocalDateTimeText();
        telegramReportBaseline_.sellTotal = telegramStats_.sellTotal;
        telegramReportBaseline_.tradeTotal = telegramStats_.tradeTotal;
        telegramReportBaseline_.deathTotal = telegramStats_.deathTotal;
        telegramReportBaseline_.reviveTotal = telegramStats_.reviveTotal;
        telegramReportBaseline_.fifoTotal = telegramStats_.fifoTotal;
        telegramReportBaseline_.lauLanConfirmTotal = telegramStats_.lauLanConfirmTotal;
        telegramReportBaseline_.clientFreezeTotal = telegramStats_.clientFreezeTotal;
        telegramReportBaseline_.worldFlowTimeoutTotal = telegramStats_.worldFlowTimeoutTotal;
        telegramReportBaseline_.sellsByPid = telegramStats_.sellsByPid;
        telegramReportBaseline_.tradesByChildPid = telegramStats_.tradesByChildPid;
    }

    void BeginTelegramSession() {
        telegramStats_ = TelegramStats{};
        telegramStats_.active = true;
        telegramStats_.startedTick = GetTickCount64();
        GetLocalTime(&telegramStats_.startedLocal);
        telegramWatch_.clear();
        telegramLastIntervalSummaryTick_ = GetTickCount();
        telegramLastDailyKeys_.fill(0);
        ResetTelegramReportBaseline();
        if (telegramSettings_.enabled && telegramSettings_.notifyToolState) {
            (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage,
                L"▶️ AUTO SESSION START\nThời gian: " + LocalDateTimeText(), L"SESSION START", L"-");
        }
    }

    static std::wstring FormatInt64Grouped(std::int64_t value) {
        std::wstring raw = std::to_wstring(value);
        const std::size_t begin = (!raw.empty() && raw[0] == L'-') ? 1u : 0u;
        for (std::ptrdiff_t i = static_cast<std::ptrdiff_t>(raw.size()) - 3;
             i > static_cast<std::ptrdiff_t>(begin); i -= 3) {
            raw.insert(static_cast<std::size_t>(i), 1, L'.');
        }
        return raw;
    }

    static std::wstring SignedMoneyDelta(std::int64_t value) {
        return (value >= 0 ? L"+" : L"") + FormatInt64Grouped(value);
    }

    static std::wstring FunnySellText(const std::wstring& label, int count, std::uint64_t seed) {
        switch (telegram_logic::PhraseIndex(seed, 6)) {
            case 0: return L"Hôm nay " + label + L" chăm chỉ phết: đã bán " + std::to_wstring(count) + L" lượt đồ rồi 💰";
            case 1: return label + L" vừa được phong danh hiệu đồng nát: " + std::to_wstring(count) + L" lượt bán từ lúc bật tool 😆";
            case 2: return L"Kho ve chai của " + label + L" chạy tốt: " + std::to_wstring(count) + L" lượt bán rồi anh.";
            case 3: return label + L" dọn túi như dọn nhà: tổng " + std::to_wstring(count) + L" lượt bán hôm nay (từ lúc bật tool).";
            case 4: return L"Báo cáo tài chính kiểu Thần Long: " + label + L" đã quăng hàng ra shop " + std::to_wstring(count) + L" lượt.";
            default:return label + L" đang làm giàu cho NPC: " + std::to_wstring(count) + L" lượt bán rồi, cái túi chắc nhẹ hẳn 😄";
        }
    }

    static std::wstring FunnyTradeText(int count, std::uint64_t seed) {
        switch (telegram_logic::PhraseIndex(seed, 5)) {
            case 0: return L"Đàn em đã mớm cho acc chính " + std::to_wstring(count) + L" lần rồi anh 🍚";
            case 1: return L"Acc chính hôm nay được các em bón tận miệng " + std::to_wstring(count) + L" lần 😆";
            case 2: return L"Hệ thống tiếp tế hoạt động ngon: CON → MAIN " + std::to_wstring(count) + L" chuyến.";
            case 3: return L"Các acc con chăm MAIN như chăm em bé: mớm đủ " + std::to_wstring(count) + L" lần rồi.";
            default:return L"MAIN lại há mồm nhận hàng: tổng cộng " + std::to_wstring(count) + L" lần tiếp tế.";
        }
    }

    static std::wstring FunnyDeathBurstText(const std::wstring& label, std::size_t deaths, std::uint64_t seed) {
        const std::wstring n = std::to_wstring(deaths);
        switch (telegram_logic::PhraseIndex(seed, 6)) {
            case 0: return L"Acc " + label + L" đang bị thằng mả mẹ nào hành chết " + n + L" lần trong 10 phút kìa anh, vào bem lại nó 😤";
            case 1: return label + L" vừa nằm sàn " + n + L" lần/10 phút. Thằng nào đang lấy nó làm bao cát vậy anh?";
            case 2: return label + L" chết " + n + L" mạng trong 10 phút rồi. Có mùi bị hội đồng, vào đòi công đạo thôi.";
            case 3: return L"Báo động nghĩa địa: " + label + L" ghé thăm Địa Phủ " + n + L" lần trong 10 phút 😭";
            case 4: return label + L" đang phát vé free kill à anh? " + n + L" lần chết/10 phút rồi.";
            default:return L"Thằng nào thương " + label + L" quá mà tiễn nó về làng " + n + L" lần/10 phút vậy?";
        }
    }

    static std::wstring FunnyAutoTrainOffText(const std::wstring& label, int minutes, std::uint64_t seed) {
        const std::wstring m = std::to_wstring(minutes);
        switch (telegram_logic::PhraseIndex(seed, 6)) {
            case 0: return L"Acc " + label + L" nằm chơi hơn " + m + L" phút rồi kìa anh, vào đá đít nó bật train lại đi 😆";
            case 1: return L"Báo động lười biếng: " + label + L" tắt Auto Train " + m + L" phút. Nó đang lĩnh lương mà ngồi uống trà rồi.";
            case 2: return label + L" đình công " + m + L" phút rồi anh ơi. Vào vặn tai nó cho đánh quái tiếp thôi.";
            case 3: return L"Con hàng " + label + L" đứng ngắm cảnh " + m + L" phút rồi. Auto Train vẫn OFF, xử lý nó cái anh 😅";
            case 4: return label + L" hình như xin nghỉ phép không lương: Auto Train OFF " + m + L" phút liên tục.";
            default:return L"Anh ơi, " + label + L" trốn việc " + m + L" phút rồi. Vào đạp nhẹ một phát cho nó train lại.";
        }
    }

    static std::wstring FunnyCurrencyText(const std::wstring& label, const std::wstring& milestone,
                                          const TelegramAccountWatch& watch, std::uint64_t seed) {
        const std::int64_t currentGold = telegram_logic::WholeGoldFromRaw(watch.boundMoney);
        const std::int64_t deltaGold = watch.boundMoneyBaselineKnown
            ? telegram_logic::WholeGoldDeltaFromRaw(watch.boundMoney, watch.boundMoneyBaseline) : 0;
        const std::wstring baseTime = watch.boundMoneyBaselineTime.empty() ? L"?" : watch.boundMoneyBaselineTime;
        std::wstring deltaLine;
        if (!watch.boundMoneyBaselineKnown || deltaGold == 0) deltaLine = L"Không đổi so với mốc " + baseTime;
        else if (deltaGold > 0) deltaLine = L"Tăng +" + FormatInt64Grouped(deltaGold) + L" vàng so với mốc " + baseTime;
        else deltaLine = L"Giảm " + FormatInt64Grouped(-deltaGold) + L" vàng so với mốc " + baseTime;

        std::wstring tail;
        switch (telegram_logic::PhraseIndex(seed, 5)) {
            case 0: tail = L"Ví khóa đang béo lên, cứ thế mà vắt máy 😆"; break;
            case 1: tail = L"Acc này biết làm kinh tế hơn chủ rồi đấy 😏"; break;
            case 2: tail = L"Chăm thế này NPC sắp gọi bằng cổ đông rồi 😂"; break;
            case 3: tail = L"Đà này tối khỏi kiểm ví, nó tự đẻ vàng rồi 💸"; break;
            default: tail = L"Nếu số tụt thì xem thằng nào vừa tiêu hộ nhé 😄"; break;
        }
        return L"💰 " + label + L" • " + milestone + L"\nVàng khóa: " +
            FormatInt64Grouped(currentGold) + L" vàng\n" + deltaLine + L"\n" + tail;
    }

    void ObserveTelegramCurrency(Account& a, TelegramAccountWatch& watch, DWORD now) {
        if (!telegramSettings_.enabled || !a.bridge.Attached() || a.runtime.clientFreezeActive) return;
        constexpr DWORD kCurrencySampleMs = 30000;
        if (watch.currencyNextReadTick != 0 && !Elapsed(now, watch.currencyNextReadTick, kCurrencySampleMs)) return;
        watch.currencyNextReadTick = now;
        Response r{}; std::wstring ignored;
        if (!a.bridge.Call(Command::ReadCurrency, 0, 0, 0, r, ignored, 700) || r.value0 == 0) return;

        const ULONGLONG now64 = GetTickCount64();
        if (watch.currencyBaselineTick == 0) watch.currencyBaselineTick = now64;
        if (r.value0 & 1) {
            watch.money = r.value64_0; watch.moneyKnown = true;
            if (!watch.moneyBaselineKnown) { watch.moneyBaseline = watch.money; watch.moneyBaselineKnown = true; }
        }
        if (r.value0 & 2) {
            watch.boundMoney = r.value64_1; watch.boundMoneyKnown = true;
            if (!watch.boundMoneyBaselineKnown) {
                watch.boundMoneyBaseline = watch.boundMoney;
                watch.boundMoneyBaselineKnown = true;
                watch.boundMoneyBaselineTime = LocalHourMinuteText();
            }
        }
        if (!telegramSettings_.notifyFunAlerts || !watch.boundMoneyKnown) return;

        const ULONGLONG elapsed = now64 - watch.currencyBaselineTick;
        constexpr const wchar_t* labels[5] = {L"1 phút", L"5 phút", L"60 phút", L"6 tiếng", L"24 tiếng"};
        const unsigned enabledMask = telegram_logic::CurrencyMilestoneEnabledMask(
            telegramSettings_.currencyMilestones.data(), telegramSettings_.currencyMilestones.size());
        const int due = telegram_logic::CurrencyMilestoneDue(elapsed, watch.currencyMilestoneMask, enabledMask);
        if (due >= 0) {
            // If the PC slept or the tool was busy across several selected thresholds, emit only
            // the largest crossed milestone and consume lower crossed thresholds to avoid bursts.
            const unsigned crossedMask = (1u << (static_cast<unsigned>(due) + 1u)) - 1u;
            const std::wstring account = TelegramAccountLabel(a);
            const std::wstring msg = FunnyCurrencyText(account, labels[due], watch,
                now64 ^ (static_cast<std::uint64_t>(a.game.pid) << 20) ^ static_cast<std::uint64_t>(due));
            if (!QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, msg, L"MONEY MILESTONE", account)) return;
            watch.currencyMilestoneMask |= crossedMask;
        }
        if (elapsed >= telegram_logic::kCurrencyMilestoneThresholds[4]) {
            // 24h is the accounting-cycle boundary even when its notification checkbox is OFF.
            watch.currencyBaselineTick = now64;
            watch.currencyMilestoneMask = 0;
            // Keep the bound-gold reference fixed at the first valid sample of this AUTO session.
            // Only the milestone schedule rolls over every 24h.
        }
    }

    std::wstring BuildTelegramSummary(const std::wstring& reason, bool finalSession) const {
        const int sellDelta = telegramStats_.sellTotal - telegramReportBaseline_.sellTotal;
        const int tradeDelta = telegramStats_.tradeTotal - telegramReportBaseline_.tradeTotal;
        const int deathDelta = telegramStats_.deathTotal - telegramReportBaseline_.deathTotal;
        const int reviveDelta = telegramStats_.reviveTotal - telegramReportBaseline_.reviveTotal;
        const int freezeDelta = telegramStats_.clientFreezeTotal - telegramReportBaseline_.clientFreezeTotal;
        const int fifoDelta = telegramStats_.fifoTotal - telegramReportBaseline_.fifoTotal;
        const int llDelta = telegramStats_.lauLanConfirmTotal - telegramReportBaseline_.lauLanConfirmTotal;
        const int wfDelta = telegramStats_.worldFlowTimeoutTotal - telegramReportBaseline_.worldFlowTimeoutTotal;
        const std::wstring nowText = LocalDateTimeText();
        std::wstring msg = finalSession ? L"📊 TỔNG KẾT SESSION" : L"📦 BÁO CÁO ĐỊNH KỲ";
        msg += L"\nMốc: " + reason + L"\nThời gian: " + nowText;
        if (!telegramReportBaselineTime_.empty()) msg += L"\nKhoảng: " + telegramReportBaselineTime_ + L" → " + nowText;
        if (telegramStats_.active) msg += L"\nUptime: " + FormatDurationSeconds(GetTickCount64() - telegramStats_.startedTick);
        msg += L"\n\nTừ báo cáo trước:";
        msg += L"\n• Bán đồ: " + std::to_wstring(sellDelta) + L" lượt";
        for (const auto& kv : telegramStats_.sellsByPid) {
            int before = 0;
            const auto it = telegramReportBaseline_.sellsByPid.find(kv.first);
            if (it != telegramReportBaseline_.sellsByPid.end()) before = it->second;
            const int delta = kv.second - before;
            if (delta <= 0) continue;
            const Account* a = nullptr;
            for (const auto& item : accounts_) if (item && item->game.pid == kv.first) { a = item.get(); break; }
            const std::wstring label = a ? TelegramAccountLabel(*a) : L"ACC";
            msg += L"\n  - " + label + L": +" + std::to_wstring(delta);
        }
        msg += L"\n• GD CON→MAIN: " + std::to_wstring(tradeDelta);
        msg += L"\n• Chết/Sống lại: " + std::to_wstring(deathDelta) + L"/" + std::to_wstring(reviveDelta);
        msg += L"\n• FIFO vào hàng: " + std::to_wstring(fifoDelta);
        msg += L"\n• XN Lâu Lan: " + std::to_wstring(llDelta);
        msg += L"\n• Client Freeze: " + std::to_wstring(freezeDelta);
        msg += L"\n• WorldFlow timeout: " + std::to_wstring(wfDelta);
        msg += L"\n\nCộng dồn session:";
        msg += L"\n• Bán đồ: " + std::to_wstring(telegramStats_.sellTotal) + L" lượt";
        for (const auto& kv : telegramStats_.sellsByPid) {
            const Account* a = nullptr;
            for (const auto& item : accounts_) if (item && item->game.pid == kv.first) { a = item.get(); break; }
            std::wstring label = a ? TelegramAccountLabel(*a) : L"ACC";
            msg += L"\n  - " + label + L": " + std::to_wstring(kv.second);
        }
        msg += L"\n• GD hoàn tất: " + std::to_wstring(telegramStats_.tradeTotal);
        msg += L"\n• Chết: " + std::to_wstring(telegramStats_.deathTotal) + L" • sống lại: " + std::to_wstring(telegramStats_.reviveTotal);
        msg += L"\n• FIFO vào hàng: " + std::to_wstring(telegramStats_.fifoTotal);
        msg += L"\n• XN Lâu Lan: " + std::to_wstring(telegramStats_.lauLanConfirmTotal);
        msg += L"\n• Client Freeze: " + std::to_wstring(telegramStats_.clientFreezeTotal);
        msg += L"\n• WorldFlow timeout: " + std::to_wstring(telegramStats_.worldFlowTimeoutTotal);

        if (telegramSettings_.notifyFunAlerts) {
            msg += L"\n\n🎭 Báo cáo kiểu cà khịa:";
            for (const auto& kv : telegramStats_.sellsByPid) {
                if (kv.second <= 0) continue;
                const Account* a = nullptr;
                for (const auto& item : accounts_) if (item && item->game.pid == kv.first) { a = item.get(); break; }
                const std::wstring label = a ? TelegramAccountLabel(*a) : L"ACC";
                msg += L"\n• " + FunnySellText(label, kv.second, GetTickCount64() ^ (static_cast<std::uint64_t>(kv.first) << 24) ^ static_cast<std::uint64_t>(kv.second));
            }
            if (telegramStats_.tradeTotal > 0)
                msg += L"\n• " + FunnyTradeText(telegramStats_.tradeTotal, GetTickCount64() ^ static_cast<std::uint64_t>(telegramStats_.tradeTotal));
        }

        bool currencyHeader = false;
        for (const auto& item : accounts_) {
            if (!item) continue;
            const auto it = telegramWatch_.find(item->game.pid);
            if (it == telegramWatch_.end() || !it->second.boundMoneyKnown) continue;
            if (!currencyHeader) { msg += L"\n\n💰 Vàng khóa hiện tại (cache <=30s):"; currencyHeader = true; }
            const auto& w = it->second;
            const std::int64_t currentGold = telegram_logic::WholeGoldFromRaw(w.boundMoney);
            const std::int64_t deltaGold = w.boundMoneyBaselineKnown
                ? telegram_logic::WholeGoldDeltaFromRaw(w.boundMoney, w.boundMoneyBaseline) : 0;
            msg += L"\n• " + TelegramAccountLabel(*item) + L": " + FormatInt64Grouped(currentGold) + L" vàng";
            if (w.boundMoneyBaselineKnown) {
                msg += L" • ";
                if (deltaGold > 0) msg += L"+" + FormatInt64Grouped(deltaGold);
                else msg += FormatInt64Grouped(deltaGold);
                msg += L" so với mốc " + (w.boundMoneyBaselineTime.empty() ? L"?" : w.boundMoneyBaselineTime);
            }
        }
        return msg;
    }

    bool SendTelegramSummary(const std::wstring& reason, bool finalSession, bool forceManual = false) {
        if (!forceManual) {
            if (!telegramStats_.active || !telegramSettings_.enabled) return false;
            if (finalSession ? !telegramSettings_.notifySessionSummary : !telegramSettings_.notifySellSummary) return false;
        }
        if (!PersistTelegramSettingsFromUi(false) && forceManual) return false;
        const std::wstring msg = BuildTelegramSummary(reason, finalSession);
        if (!QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, msg,
                                  finalSession ? L"SESSION SUMMARY" : L"SELL SUMMARY", L"-", forceManual)) return false;
        ResetTelegramReportBaseline();
        return true;
    }

    void TelegramSendSummaryNow() {
        if (!telegramStats_.active) {
            if (!PersistTelegramSettingsFromUi(false)) return;
            const std::wstring msg = L"📦 BÁO CÁO THỦ CÔNG\nChưa có AUTO session đang chạy.\nThời gian: " + LocalDateTimeText();
            (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, msg, L"SELL SUMMARY", L"-", true);
            return;
        }
        (void)SendTelegramSummary(L"GỬI THỦ CÔNG", false, true);
    }

    void TelegramRecordSellCompleted(Account& a, int freeBagSpace) {
        if (!telegramStats_.active) return;
        ++telegramStats_.sellTotal;
        ++telegramStats_.sellsByPid[a.game.pid];
        if (!telegramSettings_.enabled || !telegramSettings_.notifySellComplete) return;
        const int accountSellCount = telegramStats_.sellsByPid[a.game.pid];
        const std::wstring label = TelegramAccountLabel(a);
        std::wstring msg = telegramSettings_.notifyFunAlerts
            ? (L"📦 " + FunnySellText(label, accountSellCount, GetTickCount64() ^ (static_cast<std::uint64_t>(a.game.pid) << 18) ^ accountSellCount))
            : (L"📦 BÁN ĐỒ HOÀN TẤT\nAcc: " + label + L"\nTổng lượt bán acc session: " + std::to_wstring(accountSellCount));
        msg += L"\nThời gian: " + LocalDateTimeText() +
               L"\nFreeBagSpace sau bán: " + std::to_wstring(freeBagSpace);
        (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, msg, L"SELL COMPLETE", TelegramAccountLabel(a));
    }

    void TelegramRecordTradeCompleted(Account& main, Account& child, int receivedSlots, int passCount) {
        if (telegramStats_.active) { ++telegramStats_.tradeTotal; ++telegramStats_.tradesByChildPid[child.game.pid]; }
        if (!telegramSettings_.enabled || !telegramSettings_.notifyTradeComplete) return;
        std::wstring msg = telegramSettings_.notifyFunAlerts
            ? (L"🔄 " + FunnyTradeText(telegramStats_.tradeTotal, GetTickCount64() ^ static_cast<std::uint64_t>(telegramStats_.tradeTotal)))
            : L"🔄 GIAO DỊCH HOÀN TẤT";
        msg += L"\nCON: " + TelegramAccountLabel(child) +
               L"\nMAIN: " + TelegramAccountLabel(main) +
               L"\nReceivedSlots pass cuối: " + std::to_wstring(receivedSlots) +
               L"\nSố pass GD: " + std::to_wstring(passCount) +
               L"\nThời gian: " + LocalDateTimeText() +
               L"\nCON: nhả HOLD để quay train";
        (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, msg, L"TRADE COMPLETE", TelegramAccountLabel(child));
    }

    void TelegramRecordLauLanConfirm(Account& a, int attempt) {
        if (telegramStats_.active) ++telegramStats_.lauLanConfirmTotal;
        if (!telegramSettings_.enabled || !telegramSettings_.notifyLauLanConfirm) return;
        const std::wstring msg = L"🚪 LÂU LAN XÁC NHẬN CỔNG\nAcc: " + TelegramAccountLabel(a) +
            L"\nMapID: 5 • AutoPath ON nhưng đứng >=3s\nLần XN watchdog: " + std::to_wstring(attempt) +
            L"\nThời gian: " + LocalDateTimeText();
        (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, msg, L"LÂU LAN XN", TelegramAccountLabel(a));
    }

    static bool CriticalFreezeReason(const wchar_t* reason) {
        if (!reason) return false;
        const std::wstring r(reason);
        return r.find(L"timeout") != std::wstring::npos || r.find(L"Timeout") != std::wstring::npos ||
               r.find(L"ReadState") != std::wstring::npos || r.find(L"không phản hồi") != std::wstring::npos ||
               r.find(L"busy") != std::wstring::npos || r.find(L"Bridge") != std::wstring::npos;
    }

    void TelegramRecordCriticalFreeze(Account& a, const wchar_t* reason) {
        TelegramAccountWatch& watch = telegramWatch_[a.game.pid];
        if (watch.criticalFreezeNotified || !CriticalFreezeReason(reason)) return;
        watch.criticalFreezeNotified = true;
        if (telegramStats_.active) ++telegramStats_.clientFreezeTotal;
        if (!telegramSettings_.enabled || !telegramSettings_.notifyClientFreeze) return;
        const std::wstring msg = L"⚠️ CLIENT FREEZE\nAcc: " + TelegramAccountLabel(a) +
            L"\nLý do: " + std::wstring(reason ? reason : L"không rõ") +
            L"\nAutomation: fail-closed, không gửi action mới\nThời gian: " + LocalDateTimeText();
        (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, msg, L"CLIENT FREEZE", TelegramAccountLabel(a));
    }

    void TelegramRecordFreezeRecovered(Account& a) {
        TelegramAccountWatch& watch = telegramWatch_[a.game.pid];
        if (!watch.criticalFreezeNotified) return;
        watch.criticalFreezeNotified = false;
        if (!telegramSettings_.enabled || !telegramSettings_.notifyClientFreeze) return;
        const std::wstring msg = L"✅ CLIENT RECOVERED\nAcc: " + TelegramAccountLabel(a) +
            L"\nClient đã ổn định lại >=2s\nThời gian: " + LocalDateTimeText();
        (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, msg, L"CLIENT RECOVER", TelegramAccountLabel(a));
    }

    void ObserveTelegramAccountState(Account& a, DWORD now) {
        if (!a.runtime.running || !a.snapshotValid) return;
        TelegramAccountWatch& watch = telegramWatch_[a.game.pid];
        const Snapshot& st = a.snapshot;
        if (st.validMask & ValidLifeState) {
            auto emitDeath = [&]() {
                watch.deathStartedTick = now;
                if (telegramStats_.active) ++telegramStats_.deathTotal;
                while (!watch.recentDeathTicks.empty() && Elapsed(now, watch.recentDeathTicks.front(), 10u * 60u * 1000u))
                    watch.recentDeathTicks.pop_front();
                watch.recentDeathTicks.push_back(now);
                const bool deathBurst = telegram_logic::IsDeathBurst(watch.recentDeathTicks.size());
                if (telegramSettings_.enabled && telegramSettings_.notifyDeath) {
                    const std::wstring account = TelegramAccountLabel(a);
                    if (telegramSettings_.notifyFunAlerts && deathBurst) {
                        if (watch.deathBurstLastAlertTick == 0 || Elapsed(now, watch.deathBurstLastAlertTick, 10u * 60u * 1000u)) {
                            watch.deathBurstLastAlertTick = now;
                            const std::wstring msg = FunnyDeathBurstText(account, watch.recentDeathTicks.size(),
                                GetTickCount64() ^ (static_cast<std::uint64_t>(a.game.pid) << 16) ^ watch.recentDeathTicks.size());
                            (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, msg, L"DEATH BURST", account);
                        }
                        // Once the 10-minute burst threshold is reached, suppress per-death spam.
                        return;
                    }
                    const std::wstring context = a.tradeHeld ? L"WorldFlow/BĐPT đang giữ acc về TỌA GD" : L"Core auto bình thường";
                    const std::wstring msg = L"☠️ NHÂN VẬT CHẾT\nAcc: " + account +
                        L"\nThời gian: " + LocalDateTimeText() +
                        L"\nMapID: " + std::to_wstring(st.mapID) + L" • X,Y: " + std::to_wstring(st.x) + L"," + std::to_wstring(st.y) +
                        L"\nTrạng thái: " + context;
                    (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, msg, L"DEAD", account);
                }
            };
            auto emitRevived = [&]() {
                if (telegramStats_.active) ++telegramStats_.reviveTotal;
                const DWORD duration = watch.deathStartedTick ? now - watch.deathStartedTick : 0;
                if (telegramSettings_.enabled && telegramSettings_.notifyRevive) {
                    const std::wstring msg = L"✅ NHÂN VẬT SỐNG LẠI\nAcc: " + TelegramAccountLabel(a) +
                        L"\nThời gian: " + LocalDateTimeText() +
                        L"\nThời gian chết: " + FormatDurationSeconds(duration) +
                        L"\nWorldFlow: " + std::wstring(a.tradeHeld ? L"giữ FIFO/HOLD và tiếp tục route" : L"core cold-start tiếp tục");
                    (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, msg, L"REVIVED", TelegramAccountLabel(a));
                }
                watch.deathStartedTick = 0;
            };

            const bool deadNow = st.dead != 0;
            if (!watch.lifeKnown) {
                watch.lifeKnown = true;
                watch.lastDead = deadNow;
                // Starting monitoring while the character is already dead is still a real
                // actionable condition; report it once instead of waiting for another death edge.
                if (deadNow) emitDeath();
            } else if (deadNow != watch.lastDead) {
                watch.lastDead = deadNow;
                if (deadNow) emitDeath();
                else emitRevived();
            }
        }

        const bool autoTrainJudgable = telegramSettings_.enabled && telegramSettings_.notifyFunAlerts &&
            (st.validMask & ValidMapTransition) && (st.validMask & ValidLifeState) &&
            (st.validMask & ValidAutoFight) && (st.validMask & ValidAutoPath) &&
            st.mapReady && !st.waitingChangeMap && !st.dead && !st.autoPathing &&
            a.runtime.trainPositionMonitorArmed && !a.tradeHeld && !a.runtime.clientFreezeActive &&
            a.runtime.sellPhase == 0 && a.runtime.revivePhase == 0 && a.runtime.trainRecoveryPhase == 0;
        if (!autoTrainJudgable || st.autoFight) {
            watch.autoTrainOffStartedTick = 0;
            watch.autoTrainOffAlertSent = false;
        } else {
            if (watch.autoTrainOffStartedTick == 0) watch.autoTrainOffStartedTick = now;
            constexpr DWORD kAutoTrainOffAlertMs = 20u * 60u * 1000u;
            if (!watch.autoTrainOffAlertSent && Elapsed(now, watch.autoTrainOffStartedTick, kAutoTrainOffAlertMs)) {
                watch.autoTrainOffAlertSent = true;
                if (telegramSettings_.enabled) {
                    const int minutes = static_cast<int>((now - watch.autoTrainOffStartedTick) / 60000u);
                    const std::wstring account = TelegramAccountLabel(a);
                    const std::wstring msg = L"🛑 " + FunnyAutoTrainOffText(account, minutes,
                        GetTickCount64() ^ (static_cast<std::uint64_t>(a.game.pid) << 12));
                    (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, msg, L"AUTO TRAIN OFF 20M", account);
                }
            }
        }

        ObserveTelegramCurrency(a, watch, now);

        if (a.runtime.tradeWorkflowEntrySeq != 0 && a.runtime.tradeWorkflowEntrySeq != watch.lastWorkflowTicket) {
            watch.lastWorkflowTicket = a.runtime.tradeWorkflowEntrySeq;
            if (telegramStats_.active) ++telegramStats_.fifoTotal;
            if (telegramSettings_.enabled && telegramSettings_.notifyFifoEnter) {
                std::size_t pos = 0;
                for (std::size_t i = 0; i < tradeQueuePids_.size(); ++i) if (tradeQueuePids_[i] == a.game.pid) { pos = i + 1; break; }
                const std::wstring posText = pos > 0 ? (L"#" + std::to_wstring(pos) + L"/3") : L"đã nhận workflow ticket";
                const std::wstring msg = L"🧳 CON FULL → VÀO FIFO\nAcc: " + TelegramAccountLabel(a) +
                    L"\nVị trí FIFO: " + posText + L"\nVé workflow: #" + std::to_wstring(a.runtime.tradeWorkflowEntrySeq) +
                    L"\nThời gian: " + LocalDateTimeText();
                (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, msg, L"FIFO ENTER", TelegramAccountLabel(a));
            }
        } else if (a.runtime.tradeWorkflowEntrySeq == 0) {
            watch.lastWorkflowTicket = 0;
        }

        const bool lifeOk = (st.validMask & ValidLifeState) && !st.dead;
        const bool worldFlowTravel = a.tradeHeld && !a.runtime.tradeTravelReady && lifeOk && st.mapReady && !st.waitingChangeMap;
        if (!worldFlowTravel) {
            watch.worldFlowTravelStartedTick = 0;
            watch.worldFlowTimeoutSent = false;
        } else {
            if (watch.worldFlowTravelStartedTick == 0) watch.worldFlowTravelStartedTick = now;
            const DWORD timeoutMs = static_cast<DWORD>(telegramSettings_.worldFlowTimeoutSec) * 1000u;
            if (!watch.worldFlowTimeoutSent && Elapsed(now, watch.worldFlowTravelStartedTick, timeoutMs)) {
                watch.worldFlowTimeoutSent = true;
                if (telegramStats_.active) ++telegramStats_.worldFlowTimeoutTotal;
                if (telegramSettings_.enabled && telegramSettings_.notifyWorldFlowTimeout) {
                    const std::wstring msg = L"⚠️ WORLDFLOW TIMEOUT\nAcc: " + TelegramAccountLabel(a) +
                        L"\nĐã đi về TỌA GD >= " + std::to_wstring(telegramSettings_.worldFlowTimeoutSec) + L" giây nhưng chưa Ready\nMapID: " +
                        std::to_wstring(st.mapID) + L" • X,Y: " + std::to_wstring(st.x) + L"," + std::to_wstring(st.y) +
                        L"\nAutoPath: " + std::wstring((st.validMask & ValidAutoPath) && st.autoPathing ? L"ON" : L"OFF") +
                        L"\nChỉ cảnh báo Telegram; KHÔNG thay đổi workflow.\nThời gian: " + LocalDateTimeText();
                    (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, msg, L"WORLDFLOW TIMEOUT", TelegramAccountLabel(a));
                }
            }
        }
    }

    void TickTelegramSchedules(DWORD now) {
        if (!telegramStats_.active || !telegramSettings_.enabled) return;
        // Fixed daily milestones win over interval when both land on the same minute,
        // preventing duplicate zero-delta reports (e.g. 12:00 and a 60-minute interval).
        if (telegramSettings_.dailyEnabled && telegramSettings_.notifySellSummary) {
            SYSTEMTIME st{}; GetLocalTime(&st);
            const int currentMinute = static_cast<int>(st.wHour) * 60 + st.wMinute;
            const std::uint64_t day = static_cast<std::uint64_t>(st.wYear) * 10000ULL + st.wMonth * 100ULL + st.wDay;
            std::set<int> seenTargetMinutes; // duplicate configured HH:MM values must never produce duplicate sends
            for (std::size_t i = 0; i < telegramSettings_.dailyTimes.size(); ++i) {
                int targetMinute = -1;
                if (!telegram_logic::ParseDailyTimeMinutes(telegramSettings_.dailyTimes[i], targetMinute) || currentMinute != targetMinute) continue;
                if (!seenTargetMinutes.insert(targetMinute).second) continue;
                const std::uint64_t key = day * 1440ULL + static_cast<std::uint64_t>(targetMinute + 1);
                if (telegramLastDailyKeys_[i] == key) continue;
                telegramLastDailyKeys_[i] = key;
                if (SendTelegramSummary(L"MỐC " + telegramSettings_.dailyTimes[i], false)) {
                    telegramLastIntervalSummaryTick_ = now;
                    return;
                }
            }
        }
        if (telegramSettings_.intervalEnabled && telegramSettings_.notifySellSummary) {
            const DWORD intervalMs = static_cast<DWORD>(telegramSettings_.intervalMinutes) * 60u * 1000u;
            if (telegramLastIntervalSummaryTick_ == 0) telegramLastIntervalSummaryTick_ = now;
            if (Elapsed(now, telegramLastIntervalSummaryTick_, intervalMs)) {
                telegramLastIntervalSummaryTick_ = now;
                (void)SendTelegramSummary(L"MỖI " + std::to_wstring(telegramSettings_.intervalMinutes) + L" PHÚT", false);
            }
        }
    }



    static std::wstring ProfileSection(const Snapshot& s, DWORD pid) {
        if ((s.validMask & ValidIdentity) && s.roleID > 0) return L"Role_" + std::to_wstring(s.roleID);
        return L"PID_" + std::to_wstring(pid);
    }

    static std::wstring DisplayName(const Snapshot& s, DWORD pid) {
        std::wstring name = s.characterName[0] ? s.characterName : L"?";
        if ((s.validMask & ValidIdentity) && s.roleID > 0) {
            return name + L" • " + std::to_wstring(s.roleID);
        }
        return name + L" • PID " + std::to_wstring(pid);
    }

    bool EnsureAttach(Account& a, std::wstring& error) {
        if (a.bridge.AttachedTo(a.game.pid)) return true;
        if (!IsWindow(a.game.window)) { error = L"Cửa sổ game đã mất"; return false; }
        return a.bridge.Attach(a.game, error);
    }

    bool ReadSnapshot(Account& a, std::wstring& error, DWORD timeout = 850) {
        if (!EnsureAttach(a, error)) return false;
        Response r{};
        if (!a.bridge.Call(Command::ReadState, 0, 0, 0, r, error, timeout)) return false;
        a.snapshot = r.snapshot;
        a.snapshotValid = true;
        return true;
    }

    void ScanClients() {
        if (AnyDungeonActive()) { Log(L"QUÉT CLIENT bị chặn: đang có tổ đội phó bản RUN/PAUSE. STOP tổ đội trước để tránh PID bị tái tạo."); return; }
        ReleaseTradeHolds();
        tradeTxn_ = TradeTxn{};
        captureSlot_ = ClickSlot::None;
        captureMacroIndex_ = -1;
        capturePkStepIndex_ = -1;
        capturePkClickIndex_ = -1;
        capturePid_ = 0;
        for (auto& a : accounts_) a->bridge.Close();
        accounts_.clear();
        ListView_DeleteAllItems(clientList_);

        const auto found = FindClients();
        for (const auto& game : found) {
            auto a = std::make_unique<Account>();
            a->game = game;
            std::wstring error;
            if (a->bridge.Attach(game, error)) {
                Response r{};
                if (a->bridge.Call(Command::ReadState, 0, 0, 0, r, error, 1200)) {
                    a->snapshot = r.snapshot;
                    a->snapshotValid = true;
                }
            }
            if (!a->snapshotValid) {
                a->snapshot = {};
                a->displayName = L"? • PID " + std::to_wstring(game.pid);
                Log(L"PID " + std::to_wstring(game.pid) + L": chưa đọc được identity: " + error);
            } else {
                a->displayName = DisplayName(a->snapshot, game.pid);
            }
            a->profile = LoadProfile(ProfileSection(a->snapshot, game.pid));
            if (a->profile.tradeRole >= 2) a->profile.enableSell = false;
            MigrateLegacySpot(a->profile);
            a->runtime.status = L"Đã dừng";
            accounts_.push_back(std::move(a));
        }

        for (std::size_t i = 0; i < accounts_.size(); ++i) InsertAccountRow(static_cast<int>(i), *accounts_[i]);
        RefreshSpotCombo();
        if (!accounts_.empty()) {
            ListView_SetItemState(clientList_, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(clientList_, 0, FALSE);
            LoadSelectedProfileToUi();
        } else {
            ClearEditor();
        }
        Log(L"Quét thấy " + std::to_wstring(accounts_.size()) + L" client GameAssembly.dll.");
        RefreshDungeonAccountList();
    }

    void InsertAccountRow(int row, const Account& a) {
        LVITEMW item{};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = row;
        item.iSubItem = 0;
        item.pszText = const_cast<wchar_t*>(a.displayName.c_str());
        item.lParam = static_cast<LPARAM>(a.game.pid);
        ListView_InsertItem(clientList_, &item);
        SetRowText(row, 2, std::to_wstring(a.game.pid));
        UpdateAccountRow(row, a);
    }

    void SetRowText(int row, int sub, const std::wstring& text) {
        ListView_SetItemText(clientList_, row, sub, const_cast<wchar_t*>(text.c_str()));
    }

    void UpdateAccountRow(int row, const Account& a) {
        SetRowText(row, 0, a.displayName);
        SetRowText(row, 1, TradeRoleLabel(a.profile.tradeRole));
        SetRowText(row, 2, std::to_wstring(a.game.pid));
        SetRowText(row, 3, (a.pk.active ? L"PK • " : (a.runtime.running ? L"RUN • " : L"STOP • ")) + a.runtime.status);
        if (a.snapshotValid && (a.snapshot.validMask & (ValidMap | ValidPosition)) == (ValidMap | ValidPosition)) {
            std::wstring mapText = L"M" + std::to_wstring(a.snapshot.mapID) + L" • " +
                                   std::to_wstring(a.snapshot.x) + L"," + std::to_wstring(a.snapshot.y);
            if (a.snapshot.validMask & ValidBagSpace) mapText += L" • Trống " + std::to_wstring(a.snapshot.freeBagSpace);
            SetRowText(row, 4, mapText);
        } else {
            SetRowText(row, 4, L"?");
        }
        if (a.profile.target.valid) {
            SetRowText(row, 5, a.profile.target.name + L" • M" + std::to_wstring(a.profile.target.mapID) +
                             L" • " + std::to_wstring(a.profile.target.x) + L"," + std::to_wstring(a.profile.target.y) +
                             L" • vòng " + std::to_wstring(a.profile.rotationSpots.size()) + L" bãi");
        } else {
            SetRowText(row, 5, L"CHƯA CHỌN BÃI");
        }
    }


    void ResolveProfileTarget(AccountProfile& p) {
        const int index = FindSpotIndex(spots_, p.selectedSpot);
        if (index >= 0) {
            p.target = spots_[static_cast<std::size_t>(index)];
            p.target.valid = true;
        } else {
            p.target = {};
        }
    }

    bool RotationContains(const AccountProfile& p, const std::wstring& name) const {
        return std::any_of(p.rotationSpots.begin(), p.rotationSpots.end(), [&](const std::wstring& x){
            return _wcsicmp(x.c_str(), name.c_str()) == 0;
        });
    }

    void NormalizeRotationProfile(AccountProfile& p) {
        std::vector<std::wstring> clean;
        for (const auto& name : p.rotationSpots) {
            if (FindSpotIndex(spots_, name) < 0) continue;
            if (std::none_of(clean.begin(), clean.end(), [&](const std::wstring& x){ return _wcsicmp(x.c_str(), name.c_str()) == 0; })) {
                clean.push_back(name);
            }
        }
        p.rotationSpots = std::move(clean);
        if (p.rotationSpots.empty() && !p.selectedSpot.empty() && FindSpotIndex(spots_, p.selectedSpot) >= 0) {
            p.rotationSpots.push_back(p.selectedSpot);
        }
        if (!p.rotationSpots.empty() && (p.selectedSpot.empty() || !RotationContains(p, p.selectedSpot))) {
            p.selectedSpot = p.rotationSpots.front();
        }
        ResolveProfileTarget(p);
    }

    void RefreshRotationList() {
        if (!rotationList_) return;
        rotationUiLoading_ = true;
        ListView_DeleteAllItems(rotationList_);
        Account* a = SelectedAccount();
        for (std::size_t i = 0; i < spots_.size(); ++i) {
            const TargetProfile& spot = spots_[i];
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = static_cast<int>(i);
            item.pszText = const_cast<wchar_t*>(spot.name.c_str());
            ListView_InsertItem(rotationList_, &item);
            const std::wstring map = L"M" + std::to_wstring(spot.mapID);
            const std::wstring xy = std::to_wstring(spot.x) + L"," + std::to_wstring(spot.y);
            ListView_SetItemText(rotationList_, static_cast<int>(i), 1, const_cast<wchar_t*>(map.c_str()));
            ListView_SetItemText(rotationList_, static_cast<int>(i), 2, const_cast<wchar_t*>(xy.c_str()));
            const std::wstring note = (a && _wcsicmp(a->profile.selectedSpot.c_str(), spot.name.c_str()) == 0) ? L"BÃI HIỆN TẠI" : L"";
            ListView_SetItemText(rotationList_, static_cast<int>(i), 3, const_cast<wchar_t*>(note.c_str()));
            if (a && RotationContains(a->profile, spot.name)) ListView_SetCheckState(rotationList_, static_cast<int>(i), TRUE);
        }
        rotationUiLoading_ = false;
    }

    void PersistRotationListFromUi(Account& a) {
        if (!rotationList_) return;
        std::vector<std::wstring> selected;
        const int count = std::min(ListView_GetItemCount(rotationList_), static_cast<int>(spots_.size()));
        for (int i = 0; i < count; ++i) {
            if (ListView_GetCheckState(rotationList_, i)) selected.push_back(spots_[static_cast<std::size_t>(i)].name);
        }
        if (selected.empty() && !a.profile.selectedSpot.empty() && FindSpotIndex(spots_, a.profile.selectedSpot) >= 0) {
            selected.push_back(a.profile.selectedSpot);
        }
        a.profile.rotationSpots = std::move(selected);
        NormalizeRotationProfile(a.profile);
    }

    void MigrateLegacySpot(AccountProfile& p) {
        if (p.selectedSpot.empty() && p.target.valid) p.selectedSpot = p.target.name;
        if (p.target.valid && !p.selectedSpot.empty()) {
            int index = FindSpotIndex(spots_, p.selectedSpot);
            if (index >= 0) {
                const TargetProfile& existing = spots_[static_cast<std::size_t>(index)];
                if (existing.mapID != p.target.mapID || existing.x != p.target.x || existing.y != p.target.y) {
                    p.selectedSpot += L" [M" + std::to_wstring(p.target.mapID) + L" " +
                                      std::to_wstring(p.target.x) + L"," + std::to_wstring(p.target.y) + L"]";
                    index = FindSpotIndex(spots_, p.selectedSpot);
                }
            }
            if (index < 0) {
                TargetProfile migrated = p.target;
                migrated.name = p.selectedSpot;
                migrated.valid = true;
                spots_.push_back(std::move(migrated));
                SaveSharedSpots(spots_);
            }
        }
        NormalizeRotationProfile(p);
    }

    void ApplyAutoSellerForTrainingTarget(Account& a, bool writeLog = true) {
        if (!a.profile.target.valid) return;
        const int preset = AutoSellerPresetForTrainingMap(a.profile.target.mapID);
        if (preset < 0 || preset == a.profile.sellNpcPreset) return;
        a.profile.sellNpcPreset = preset;
        if (writeLog) {
            LogAccount(a, L"NPC BÁN TỰ ĐỔI THEO BÃI TRAIN M" + std::to_wstring(a.profile.target.mapID) +
                          L" → " + kSellNpcs[static_cast<std::size_t>(preset)].name +
                          L" • người dùng vẫn có thể đổi lại combo NPC bán thủ công.");
        }
    }

    void RefreshSpotCombo() {
        if (!spotCombo_) return;
        SendMessageW(spotCombo_, CB_RESETCONTENT, 0, 0);
        for (const auto& spot : spots_) {
            SendMessageW(spotCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(spot.name.c_str()));
        }
        Account* a = SelectedAccount();
        int select = -1;
        if (a) select = FindSpotIndex(spots_, a->profile.selectedSpot);
        SendMessageW(spotCombo_, CB_SETCURSEL, select, 0);
    }

    void SelectSharedSpotForAccount() {
        Account* a = SelectedAccount();
        if (!a) return;
        const LRESULT sel = SendMessageW(spotCombo_, CB_GETCURSEL, 0, 0);
        if (sel == CB_ERR || sel < 0 || static_cast<std::size_t>(sel) >= spots_.size()) return;
        const TargetProfile& spot = spots_[static_cast<std::size_t>(sel)];
        const std::wstring oldSpot = a->profile.selectedSpot;
        a->profile.selectedSpot = spot.name;
        a->profile.target = spot;
        ApplyAutoSellerForTrainingTarget(*a);
        // Selecting the top combobox defines the single default train spot. Rotation
        // becomes active only after the user explicitly checks at least one extra row below.
        a->profile.rotationSpots.clear();
        a->profile.rotationSpots.push_back(spot.name);
        NormalizeRotationProfile(a->profile);
        SetText(targetName_, spot.name);
        SaveProfile(a->profile);
        if (_wcsicmp(oldSpot.c_str(), spot.name.c_str()) != 0) {
            ResetRotationWindow(*a, GetTickCount());
            if (a->runtime.running) BeginTrainRecovery(*a, GetTickCount());
        }
        LoadSelectedProfileToUi();
        const int row = SelectedIndex();
        if (row >= 0) UpdateAccountRow(row, *a);
        LogAccount(*a, L"Đã chọn bãi chung: " + spot.name + L" • M" + std::to_wstring(spot.mapID) + L" • " +
                       std::to_wstring(spot.x) + L"," + std::to_wstring(spot.y));
    }

    void DeleteSelectedSharedSpot() {
        Account* a = SelectedAccount();
        if (!a) { Log(L"Chưa chọn acc"); return; }
        const LRESULT sel = SendMessageW(spotCombo_, CB_GETCURSEL, 0, 0);
        if (sel == CB_ERR || sel < 0 || static_cast<std::size_t>(sel) >= spots_.size()) {
            Log(L"Chưa chọn bãi chung để xóa");
            return;
        }
        const std::wstring name = spots_[static_cast<std::size_t>(sel)].name;
        spots_.erase(spots_.begin() + sel);
        SaveSharedSpots(spots_);
        for (auto& item : accounts_) {
            item->profile.rotationSpots.erase(std::remove_if(item->profile.rotationSpots.begin(), item->profile.rotationSpots.end(), [&](const std::wstring& x){
                return _wcsicmp(x.c_str(), name.c_str()) == 0;
            }), item->profile.rotationSpots.end());
            if (_wcsicmp(item->profile.selectedSpot.c_str(), name.c_str()) == 0) {
                item->profile.selectedSpot = item->profile.rotationSpots.empty() ? L"" : item->profile.rotationSpots.front();
            }
            NormalizeRotationProfile(item->profile);
            SaveProfile(item->profile);
        }
        RefreshSpotCombo();
        LoadSelectedProfileToUi();
        for (std::size_t i = 0; i < accounts_.size(); ++i) UpdateAccountRow(static_cast<int>(i), *accounts_[i]);
        Log(L"Đã xóa bãi chung: " + name);
    }

    int FocusedSelectedRow(HWND list) const {
        if (!list) return -1;
        const int focused = ListView_GetNextItem(list, -1, LVNI_FOCUSED);
        if (focused >= 0 && (ListView_GetItemState(list, focused, LVIS_SELECTED) & LVIS_SELECTED) != 0) return focused;
        return ListView_GetNextItem(list, -1, LVNI_SELECTED);
    }

    std::vector<int> SelectedRows(HWND list) const {
        std::vector<int> rows;
        if (!list) return rows;
        int row = -1;
        while ((row = ListView_GetNextItem(list, row, LVNI_SELECTED)) >= 0) rows.push_back(row);
        return rows;
    }

    void CopyClicksFromAnotherAccount() {
        Account* target = SelectedAccount();
        if (!target) { Log(L"LẤY 3 CLICK: chưa chọn acc đích."); return; }
        HMENU menu = CreatePopupMenu();
        if (!menu) return;
        std::vector<Account*> sources;
        for (auto& item : accounts_) {
            Account* source = item.get();
            if (!source || source->game.pid == target->game.pid) continue;
            int valid = 0;
            for (int i : {static_cast<int>(ClickSlot::AutoMenu), static_cast<int>(ClickSlot::Attack), static_cast<int>(ClickSlot::StopAuto2)})
                if (source->profile.points[static_cast<std::size_t>(i)].valid) ++valid;
            if (valid == 0) continue;
            sources.push_back(source);
            const std::wstring label = AccountTag(*source) + L" • có " + std::to_wstring(valid) + L"/3 điểm";
            AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(6000 + sources.size() - 1), label.c_str());
        }
        if (sources.empty()) {
            DestroyMenu(menu);
            LogAccount(*target, L"LẤY 3 CLICK: chưa có acc khác nào đã gán tọa độ.");
            return;
        }
        POINT screen{}; GetCursorPos(&screen);
        const int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
                                       screen.x, screen.y, 0, hwnd_, nullptr);
        DestroyMenu(menu);
        if (cmd < 6000 || static_cast<std::size_t>(cmd - 6000) >= sources.size()) return;
        Account* source = sources[static_cast<std::size_t>(cmd - 6000)];
        int copied = 0;
        for (int i : {static_cast<int>(ClickSlot::AutoMenu), static_cast<int>(ClickSlot::Attack), static_cast<int>(ClickSlot::StopAuto2)}) {
            if (!source->profile.points[static_cast<std::size_t>(i)].valid) continue;
            target->profile.points[static_cast<std::size_t>(i)] = source->profile.points[static_cast<std::size_t>(i)];
            ++copied;
        }
        SaveProfile(target->profile);
        LoadSelectedProfileToUi();
        LogAccount(*target, L"Đã lấy " + std::to_wstring(copied) + L"/3 CLICK từ " + AccountTag(*source) +
                           L" • điểm nguồn chưa gán không ghi đè điểm hiện tại.");
    }

    bool RecorderModeIsTrade(RecorderMode mode) const {
        return mode == RecorderMode::TradeMain || mode == RecorderMode::TradeChild;
    }

    int RecordedDelay(std::size_t index, int lastDefault) const {
        if (index + 1 >= recorderClicks_.size()) return lastDefault;
        const DWORD delta = recorderClicks_[index + 1].tick - recorderClicks_[index].tick;
        return std::clamp(static_cast<int>(delta), 50, 60000);
    }

    void UpdateRecorderUi(const std::wstring& status = L"") {
        const bool active = recorderMode_ != RecorderMode::None;
        if (sellRecordButton_) SetWindowTextW(sellRecordButton_, active && recorderMode_ == RecorderMode::Sell ? L"DỪNG REC" : L"REC");
        if (tradeRecordButton_) SetWindowTextW(tradeRecordButton_, active && RecorderModeIsTrade(recorderMode_) ? L"DỪNG REC" : L"REC");
        std::wstring text = status;
        if (text.empty()) text = active ? L"REC đang ghi thao tác tay..." : L"REC: sẵn sàng";
        if (sellRecordStatus_) SetWindowTextW(sellRecordStatus_, text.c_str());
        if (tradeRecordStatus_) SetWindowTextW(tradeRecordStatus_, text.c_str());
    }

    Account* RecorderAccountAtPoint(const POINT& screen) {
        HWND hit = WindowFromPoint(screen);
        HWND root = hit ? GetAncestor(hit, GA_ROOT) : nullptr;
        for (auto& item : accounts_) if (item && item->game.window == root) return item.get();
        return nullptr;
    }

    bool RecorderAllowsAccount(const Account& account) const {
        if (recorderMode_ == RecorderMode::Sell) return account.game.pid == recorderPrimaryPid_;
        if (recorderMode_ == RecorderMode::TradeMain) return account.profile.tradeRole == 1;
        if (recorderMode_ == RecorderMode::TradeChild) {
            return account.game.pid == recorderPrimaryPid_ || account.profile.tradeRole == 1;
        }
        return false;
    }

    void CaptureRecorderClick() {
        if (recorderMode_ == RecorderMode::None || recorderClicks_.size() >= 64) return;
        POINT screen{};
        if (!GetCursorPos(&screen)) return;
        Account* account = RecorderAccountAtPoint(screen);
        if (!account || !RecorderAllowsAccount(*account)) return; // Click on tool/other apps/other CON is intentionally ignored.
        POINT client = screen;
        if (!ScreenToClient(account->game.window, &client)) return;
        RECT rc{};
        if (!GetClientRect(account->game.window, &rc)) return;
        const int width = rc.right - rc.left, height = rc.bottom - rc.top;
        if (width <= 0 || height <= 0 || client.x < 0 || client.y < 0 || client.x >= width || client.y >= height) return;
        RecordedClick click{};
        click.pid = account->game.pid;
        click.point = ClickPoint{client.x, client.y, width, height, true};
        click.tick = GetTickCount();
        recorderClicks_.push_back(click);
        const std::wstring status = L"REC • " + std::to_wstring(recorderClicks_.size()) + L" click • vừa ghi " +
                                    AccountTag(*account) + L" @ " + PointDescription(click.point);
        UpdateRecorderUi(status);
        SetTradeStatus(L"RECORDING • FREEZE AUTO • " + status);
    }

    void PollRecorder() {
        if (recorderMode_ == RecorderMode::None) return;
        const bool down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (down) {
            recorderMouseDown_ = true;
            return;
        }
        if (recorderMouseDown_) {
            recorderMouseDown_ = false;
            CaptureRecorderClick();
        }
    }

    int FindSharedMainStepByPoint(const ClickPoint& point) const {
        for (std::size_t i = 0; i < mainTradeSequence_.size(); ++i) {
            const ClickPoint& p = mainTradeSequence_[i].point;
            if (p.valid && p.x == point.x && p.y == point.y && p.baseW == point.baseW && p.baseH == point.baseH) return static_cast<int>(i);
        }
        return -1;
    }

    void CommitRecordedSell(DWORD pid) {
        Account* account = AccountByPid(pid);
        if (!account) return;
        const std::size_t room = account->profile.sellMacro.size() < 64 ? 64 - account->profile.sellMacro.size() : 0;
        const std::size_t count = std::min(room, recorderClicks_.size());
        const std::size_t first = account->profile.sellMacro.size();
        for (std::size_t i = 0; i < count; ++i) {
            SellMacroStep step{};
            step.description = L"REC bước " + std::to_wstring(first + i + 1);
            step.point = recorderClicks_[i].point;
            step.delayMs = RecordedDelay(i, 600);
            step.repeat = 1;
            account->profile.sellMacro.push_back(step);
        }
        SaveProfile(account->profile);
        RefreshSellMacroList();
        if (count > 0 && sellMacroList_) {
            const int row = static_cast<int>(first);
            ListView_SetItemState(sellMacroList_, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(sellMacroList_, row, FALSE);
            LoadSelectedMacroEditor();
        }
        LogAccount(*account, L"REC BÁN ĐỒ → đã chuyển " + std::to_wstring(count) + L" click thành dòng tọa độ editable.");
    }

    void CommitRecordedTradeMain() {
        Account* main = AccountByTradeRole(1);
        if (!main) return;
        const std::size_t room = mainTradeSequence_.size() < 64 ? 64 - mainTradeSequence_.size() : 0;
        const std::size_t count = std::min(room, recorderClicks_.size());
        const std::size_t first = mainTradeSequence_.size();
        for (std::size_t i = 0; i < count; ++i) {
            if (recorderClicks_[i].pid != main->game.pid) continue;
            TradeSequenceStep step{};
            step.target = 1; step.mainRef = static_cast<int>(mainTradeSequence_.size());
            step.description = L"REC MAIN bước " + std::to_wstring(mainTradeSequence_.size() + 1);
            step.point = recorderClicks_[i].point;
            step.delayMs = RecordedDelay(i, 500); step.repeat = 1;
            mainTradeSequence_.push_back(step);
        }
        for (std::size_t i = 0; i < mainTradeSequence_.size(); ++i) mainTradeSequence_[i].mainRef = static_cast<int>(i);
        SaveMainTradeSequence();
        RefreshTradeSequenceList();
        PopulateTradeTargetCombo();
        if (mainTradeSequence_.size() > first && tradeSeqList_) {
            const int row = static_cast<int>(first);
            ListView_SetItemState(tradeSeqList_, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(tradeSeqList_, row, FALSE);
            LoadTradeSequenceRowToEditor(row);
        }
        LogAccount(*main, L"REC CHUỖI GD MAIN → click đã được chuyển thành thư viện MAIN editable dùng chung.");
    }

    void CommitRecordedTradeChild(DWORD childPid) {
        Account* child = AccountByPid(childPid);
        Account* main = AccountByTradeRole(1);
        if (!child || child->profile.tradeRole < 2 || !main) return;
        EnsureSharedChildTradeSequence();
        const std::size_t first = childTradeSequence_.size();
        for (std::size_t i = 0; i < recorderClicks_.size() && childTradeSequence_.size() < 64; ++i) {
            const RecordedClick& click = recorderClicks_[i];
            TradeSequenceStep row{};
            row.repeat = 1;
            if (click.pid == child->game.pid) {
                row.target = 0; row.mainRef = -1;
                row.description = L"REC ACC CON bước " + std::to_wstring(childTradeSequence_.size() + 1);
                row.point = click.point; row.delayMs = RecordedDelay(i, 500);
            } else if (click.pid == main->game.pid) {
                int ref = FindSharedMainStepByPoint(click.point);
                if (ref < 0) {
                    if (mainTradeSequence_.size() >= 64) {
                        LogAccount(*child, L"REC bỏ qua click MAIN mới vì CHUỖI GD MAIN đã đủ 64 dòng.");
                        continue;
                    }
                    TradeSequenceStep shared{};
                    shared.target = 1; shared.mainRef = static_cast<int>(mainTradeSequence_.size());
                    shared.description = L"REC MAIN bước " + std::to_wstring(mainTradeSequence_.size() + 1);
                    shared.point = click.point; shared.delayMs = RecordedDelay(i, 500); shared.repeat = 1;
                    mainTradeSequence_.push_back(shared);
                    ref = static_cast<int>(mainTradeSequence_.size() - 1);
                }
                row.target = 1; row.mainRef = ref;
            } else continue;
            childTradeSequence_.push_back(row);
        }
        for (std::size_t i = 0; i < mainTradeSequence_.size(); ++i) mainTradeSequence_[i].mainRef = static_cast<int>(i);
        SaveMainTradeSequence();
        SaveSharedChildTradeSequence();
        RefreshTradeSequenceList(); PopulateTradeTargetCombo();
        if (childTradeSequence_.size() > first && tradeSeqList_) {
            const int row = static_cast<int>(first);
            ListView_SetItemState(tradeSeqList_, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(tradeSeqList_, row, FALSE); LoadTradeSequenceRowToEditor(row);
        }
        LogAccount(*child, L"REC CHUỖI GD ACC CON dùng chung → click trên " + TradeRoleLabel(child->profile.tradeRole) +
                           L" được lưu cho mọi CON; click MAIN vẫn tham chiếu CHUỖI GD MAIN.");
    }

    bool RecorderBlocksAccount(const Account& a) const {
        if (recorderMode_ == RecorderMode::None) return false;
        if (recorderMode_ == RecorderMode::Sell) return a.game.pid == recorderPrimaryPid_;
        if (recorderMode_ == RecorderMode::TradeMain) {
            const Account* main = const_cast<App*>(this)->AccountByTradeRole(1);
            return main && a.game.pid == main->game.pid;
        }
        if (recorderMode_ == RecorderMode::TradeChild) {
            const Account* main = const_cast<App*>(this)->AccountByTradeRole(1);
            return a.game.pid == recorderPrimaryPid_ || (main && a.game.pid == main->game.pid);
        }
        return false;
    }

    void StopRecorder(bool commit) {
        if (recorderMode_ == RecorderMode::None) return;
        const RecorderMode mode = recorderMode_;
        const DWORD primaryPid = recorderPrimaryPid_;
        KillTimer(hwnd_, kRecordTimer);
        recorderMode_ = RecorderMode::None;
        recorderMouseDown_ = false;
        if (commit && !recorderClicks_.empty()) {
            if (mode == RecorderMode::Sell) CommitRecordedSell(primaryPid);
            else if (mode == RecorderMode::TradeMain) CommitRecordedTradeMain();
            else if (mode == RecorderMode::TradeChild) CommitRecordedTradeChild(primaryPid);
        }
        const std::size_t count = recorderClicks_.size();
        recorderClicks_.clear(); recorderPrimaryPid_ = 0;
        const std::wstring status = commit ? L"REC xong • đã chuyển " + std::to_wstring(count) + L" click thành dòng tọa độ" : L"REC đã hủy";
        UpdateRecorderUi(status);
        SetTradeStatus(L"BĐPT thoát RECORDING CỤC BỘ • acc bị giữ tiếp tục auto");
    }

    void StartRecorder(RecorderMode mode) {
        if (recorderMode_ != RecorderMode::None) { StopRecorder(true); return; }
        // REC is now scoped to the window(s) being captured. It must never freeze
        // unrelated accounts merely because point capture uses the physical mouse.
        Account* primary = nullptr;
        if (mode == RecorderMode::Sell) {
            primary = SelectedAccount();
            if (!primary) { Log(L"REC BÁN ĐỒ: chưa chọn acc."); return; }
        } else if (mode == RecorderMode::TradeMain) {
            primary = AccountByTradeRole(1);
            if (!primary || tradeEditorMode_ != 1) { Log(L"REC MAIN: không có MAIN/editor MAIN."); return; }
        } else if (mode == RecorderMode::TradeChild) {
            primary = TradeEditorChild();
            if (!primary || !AccountByTradeRole(1)) { Log(L"REC CON cần cả MAIN và CON đang mở editor."); return; }
        } else return;
        recorderClicks_.clear(); recorderMode_ = mode; recorderPrimaryPid_ = primary->game.pid;
        recorderMouseDown_ = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        SetTimer(hwnd_, kRecordTimer, 10, nullptr);
        UpdateRecorderUi(L"REC ĐANG GHI • chỉ khóa acc/cặp acc đang capture • các acc khác tiếp tục auto");
        SetTradeStatus(L"RECORDING CỤC BỘ • chỉ giữ cửa sổ liên quan; scheduler acc khác tiếp tục");
        LogAccount(*primary, L"BĐPT vào RECORDING CỤC BỘ • chỉ acc/cặp acc capture bị giữ; acc khác vẫn auto độc lập.");
    }

    void ToggleSellRecorder() { StartRecorder(RecorderMode::Sell); }
    void ToggleTradeRecorder() { StartRecorder(tradeEditorMode_ == 1 ? RecorderMode::TradeMain : RecorderMode::TradeChild); }

    void CopySelectedSellRows() {
        Account* account = SelectedAccount();
        const std::vector<int> rows = SelectedRows(sellMacroList_);
        if (!account || rows.empty()) { Log(L"SAO CHÉP BÁN: hãy chọn một hoặc nhiều dòng."); return; }
        sellClipboard_.clear();
        for (int row : rows) if (row >= 0 && row < static_cast<int>(account->profile.sellMacro.size())) sellClipboard_.push_back(account->profile.sellMacro[static_cast<std::size_t>(row)]);
        UpdateRecorderUi(L"Đã sao chép " + std::to_wstring(sellClipboard_.size()) + L" dòng bán • bấm DÁN để thêm vào cuối chuỗi");
    }

    void PasteSellRows() {
        Account* account = SelectedAccount();
        if (!account || sellClipboard_.empty()) { Log(L"DÁN BÁN: clipboard dòng đang rỗng."); return; }
        const std::size_t first = account->profile.sellMacro.size();
        for (const SellMacroStep& step : sellClipboard_) {
            if (account->profile.sellMacro.size() >= 64) break;
            account->profile.sellMacro.push_back(step);
        }
        SaveProfile(account->profile); RefreshSellMacroList();
        if (account->profile.sellMacro.size() > first) {
            const int row = static_cast<int>(first); ListView_SetItemState(sellMacroList_, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED); ListView_EnsureVisible(sellMacroList_, row, FALSE);
        }
        UpdateRecorderUi(L"Đã DÁN " + std::to_wstring(account->profile.sellMacro.size() - first) + L" dòng vào cuối chuỗi bán");
    }

    void CopySelectedTradeRows() {
        std::vector<TradeSequenceStep>* seq = EditorSequence();
        const std::vector<int> rows = SelectedRows(tradeSeqList_);
        if (!seq || rows.empty()) { Log(L"SAO CHÉP GD: hãy chọn một hoặc nhiều dòng."); return; }
        tradeClipboard_.clear(); tradeClipboardMode_ = tradeEditorMode_;
        for (int row : rows) if (row >= 0 && row < static_cast<int>(seq->size())) tradeClipboard_.push_back((*seq)[static_cast<std::size_t>(row)]);
        UpdateRecorderUi(L"Đã sao chép " + std::to_wstring(tradeClipboard_.size()) + L" dòng GD • bấm DÁN để thêm vào cuối chuỗi");
    }

    void PasteTradeRows() {
        std::vector<TradeSequenceStep>* seq = EditorSequence();
        if (!seq || tradeClipboard_.empty() || tradeClipboardMode_ != tradeEditorMode_) {
            Log(L"DÁN GD: clipboard rỗng hoặc khác loại editor MAIN/CON."); return;
        }
        const std::size_t first = seq->size();
        int nextGroupId = MaxTradeGroupId(*seq) + 1;
        std::vector<std::pair<int, int>> groupMap;
        for (TradeSequenceStep step : tradeClipboard_) {
            if (seq->size() >= 64) break;
            if (tradeEditorMode_ == 1) {
                step.target = 1;
                step.mainRef = static_cast<int>(seq->size());
                step.groupId = 0;
                step.groupRepeat = 1;
            } else if (step.groupId > 0) {
                int mapped = 0;
                for (const auto& entry : groupMap) {
                    if (entry.first == step.groupId) { mapped = entry.second; break; }
                }
                if (mapped == 0) {
                    mapped = nextGroupId++;
                    groupMap.emplace_back(step.groupId, mapped);
                }
                step.groupId = mapped;
                step.groupRepeat = std::clamp(step.groupRepeat, 1, 999);
            }
            seq->push_back(step);
        }
        if (tradeEditorMode_ == 1) {
            for (std::size_t i = 0; i < seq->size(); ++i) (*seq)[i].mainRef = static_cast<int>(i);
        } else {
            NormalizeTradeGroups(*seq);
        }
        SaveEditorSequence(); RefreshTradeSequenceList(); PopulateTradeTargetCombo();
        if (seq->size() > first) {
            const int row = static_cast<int>(first);
            ListView_SetItemState(tradeSeqList_, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(tradeSeqList_, row, FALSE);
            LoadTradeSequenceRowToEditor(row);
        }
        UpdateRecorderUi(L"Đã DÁN " + std::to_wstring(seq->size() - first) + L" dòng GD vào cuối chuỗi");
    }

    void GroupSelectedTradeRows() {
        if (tradeEditorMode_ != 2) { Log(L"GOM NHÓM chỉ dùng trong CHUỖI GD ACC CON."); return; }
        std::vector<TradeSequenceStep>* seq = EditorSequence();
        std::vector<int> rows = SelectedRows(tradeSeqList_);
        if (!seq || rows.empty()) { Log(L"GOM NHÓM: chọn một hoặc nhiều dòng liên tiếp."); return; }
        std::sort(rows.begin(), rows.end());
        for (std::size_t i = 1; i < rows.size(); ++i) {
            if (rows[i] != rows[i - 1] + 1) {
                Log(L"GOM NHÓM: các dòng phải liên tiếp nhau."); return;
            }
        }
        const int repeat = tradeSeqGroupRepeat_
            ? std::clamp(_wtoi(GetText(tradeSeqGroupRepeat_).c_str()), 1, 999)
            : 1;
        const int id = MaxTradeGroupId(*seq) + 1;
        for (int row : rows) {
            if (row < 0 || row >= static_cast<int>(seq->size())) continue;
            TradeSequenceStep& step = (*seq)[static_cast<std::size_t>(row)];
            step.groupId = id;
            step.groupRepeat = repeat;
        }
        NormalizeTradeGroups(*seq);
        SaveEditorSequence();
        RefreshTradeSequenceList();
        for (int row : rows) {
            ListView_SetItemState(tradeSeqList_, row, LVIS_SELECTED, LVIS_SELECTED);
        }
        if (!rows.empty()) {
            ListView_SetItemState(tradeSeqList_, rows.front(), LVIS_FOCUSED, LVIS_FOCUSED);
            LoadTradeSequenceRowToEditor(rows.front());
        }
        Log(L"Đã GOM " + std::to_wstring(rows.size()) + L" dòng thành mini-sequence • lặp nhóm " + std::to_wstring(repeat) + L" lần.");
    }

    void UngroupSelectedTradeRows() {
        if (tradeEditorMode_ != 2) { Log(L"BỎ NHÓM chỉ dùng trong CHUỖI GD ACC CON."); return; }
        std::vector<TradeSequenceStep>* seq = EditorSequence();
        const std::vector<int> rows = SelectedRows(tradeSeqList_);
        if (!seq || rows.empty()) { Log(L"BỎ NHÓM: chọn ít nhất một dòng thuộc nhóm."); return; }
        std::vector<int> groupIds;
        for (int row : rows) {
            if (row < 0 || row >= static_cast<int>(seq->size())) continue;
            const int id = (*seq)[static_cast<std::size_t>(row)].groupId;
            if (id > 0 && std::find(groupIds.begin(), groupIds.end(), id) == groupIds.end()) groupIds.push_back(id);
        }
        if (groupIds.empty()) { Log(L"BỎ NHÓM: các dòng đã chọn không thuộc nhóm nào."); return; }
        for (TradeSequenceStep& step : *seq) {
            if (std::find(groupIds.begin(), groupIds.end(), step.groupId) != groupIds.end()) {
                step.groupId = 0;
                step.groupRepeat = 1;
            }
        }
        NormalizeTradeGroups(*seq);
        SaveEditorSequence();
        RefreshTradeSequenceList();
        Log(L"Đã BỎ " + std::to_wstring(groupIds.size()) + L" nhóm khỏi chuỗi GD.");
    }

    int SelectedMacroIndex() const {
        return FocusedSelectedRow(sellMacroList_);
    }

    void RefreshSellMacroList() {
        if (!sellMacroList_) return;
        ListView_DeleteAllItems(sellMacroList_);
        Account* a = SelectedAccount();
        if (!a) return;
        for (std::size_t i = 0; i < a->profile.sellMacro.size(); ++i) {
            const SellMacroStep& step = a->profile.sellMacro[i];
            std::wstring no = std::to_wstring(i + 1);
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = static_cast<int>(i);
            item.pszText = const_cast<wchar_t*>(no.c_str());
            ListView_InsertItem(sellMacroList_, &item);
            ListView_SetItemText(sellMacroList_, static_cast<int>(i), 1,
                const_cast<wchar_t*>(step.description.empty() ? L"(chưa mô tả)" : step.description.c_str()));
            std::wstring point = PointDescription(step.point);
            ListView_SetItemText(sellMacroList_, static_cast<int>(i), 2, const_cast<wchar_t*>(point.c_str()));
            std::wstring delay = std::to_wstring(step.delayMs);
            std::wstring repeat = std::to_wstring(step.repeat);
            ListView_SetItemText(sellMacroList_, static_cast<int>(i), 3, const_cast<wchar_t*>(delay.c_str()));
            ListView_SetItemText(sellMacroList_, static_cast<int>(i), 4, const_cast<wchar_t*>(repeat.c_str()));
        }
    }

    void ClearSellMacroEditor() {
        SetText(sellDesc_, L"");
        SetText(sellDelay_, L"600");
        SetText(sellRepeat_, L"1");
    }

    void LoadSelectedMacroEditor() {
        Account* a = SelectedAccount();
        const int index = SelectedMacroIndex();
        if (!a || index < 0 || index >= static_cast<int>(a->profile.sellMacro.size())) {
            ClearSellMacroEditor();
            return;
        }
        const SellMacroStep& step = a->profile.sellMacro[static_cast<std::size_t>(index)];
        SetText(sellDesc_, step.description);
        SetText(sellDelay_, std::to_wstring(step.delayMs));
        SetText(sellRepeat_, std::to_wstring(step.repeat));
    }

    void AddSellMacroRow() {
        Account* a = SelectedAccount();
        if (!a) { Log(L"Chưa chọn acc để thêm bước bán"); return; }
        if (a->profile.sellMacro.size() >= 64) { LogAccount(*a, L"Macro bán tối đa 64 dòng."); return; }
        SellMacroStep step{};
        step.description = L"Bước " + std::to_wstring(a->profile.sellMacro.size() + 1);
        a->profile.sellMacro.push_back(step);
        SaveProfile(a->profile);
        RefreshSellMacroList();
        const int row = static_cast<int>(a->profile.sellMacro.size() - 1);
        ListView_SetItemState(sellMacroList_, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(sellMacroList_, row, FALSE);
        LoadSelectedMacroEditor();
    }

    void DeleteSellMacroRow() {
        Account* a = SelectedAccount();
        const int index = SelectedMacroIndex();
        if (!a || index < 0 || index >= static_cast<int>(a->profile.sellMacro.size())) {
            Log(L"Chưa chọn dòng macro để xóa"); return;
        }
        a->profile.sellMacro.erase(a->profile.sellMacro.begin() + index);
        SaveProfile(a->profile);
        RefreshSellMacroList();
        ClearSellMacroEditor();
    }

    void SaveSellMacroRow() {
        Account* a = SelectedAccount();
        const int index = SelectedMacroIndex();
        if (!a || index < 0 || index >= static_cast<int>(a->profile.sellMacro.size())) {
            Log(L"Chưa chọn dòng macro để lưu"); return;
        }
        SellMacroStep& step = a->profile.sellMacro[static_cast<std::size_t>(index)];
        step.description = GetText(sellDesc_);
        int delay = _wtoi(GetText(sellDelay_).c_str());
        int repeat = _wtoi(GetText(sellRepeat_).c_str());
        if (delay < 50) delay = 50;
        if (delay > 60000) delay = 60000;
        if (repeat < 1) repeat = 1;
        if (repeat > 999) repeat = 999;
        step.delayMs = delay;
        step.repeat = repeat;
        SaveProfile(a->profile);
        RefreshSellMacroList();
        ListView_SetItemState(sellMacroList_, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        LoadSelectedMacroEditor();
    }

    void BeginMacroCapture() {
        Account* a = SelectedAccount();
        const int index = SelectedMacroIndex();
        if (!a || index < 0 || index >= static_cast<int>(a->profile.sellMacro.size())) {
            Log(L"Chưa chọn dòng macro để lấy tọa độ"); return;
        }
        captureSlot_ = ClickSlot::None;
        captureMacroIndex_ = index;
        captureTradeSequenceIndex_ = -1;
        capturePkStepIndex_ = -1; capturePkClickIndex_ = -1;
        captureTradeSequenceMode_ = 0;
        captureTradeSequenceMainRef_ = -1;
        capturePkStepIndex_ = -1; capturePkClickIndex_ = -1;
        capturePid_ = a->game.pid;
        LogAccount(*a, L"Đang chờ F8 cho macro bán dòng " + std::to_wstring(index + 1));
        SetText(selected_, L"LẤY TỌA ĐỘ MACRO DÒNG " + std::to_wstring(index + 1) + L" • đưa chuột vào game rồi F8");
    }

    void TestSellMacroRow() {
        Account* a = SelectedAccount();
        const int index = SelectedMacroIndex();
        if (!a || index < 0 || index >= static_cast<int>(a->profile.sellMacro.size())) {
            Log(L"Chưa chọn dòng macro để TEST"); return;
        }
        const SellMacroStep& step = a->profile.sellMacro[static_cast<std::size_t>(index)];
        std::wstring error;
        if (!CoordinatorInternalPointAction(
                *a, step.point,
                L"TEST CHUỖI BÁN ĐỒ dòng " + std::to_wstring(index + 1),
                error)) {
            LogAccount(*a, L"TEST macro HIDDEN INPUT FAIL: " + error); return;
        }
        LogAccount(*a, L"TEST macro dòng " + std::to_wstring(index + 1) +
                       L" PASS • InputSync nội bộ • cursor không đổi");
    }

    void LoadSellNpcPositionToUi(const Account& a) {
        int index = a.profile.sellNpcPreset;
        if (index < 0 || index >= static_cast<int>(kSellNpcs.size())) index = 0;
        const SellNpcPosition& pos = sellNpcPositions_[static_cast<std::size_t>(index)];
        if (pos.valid) {
            SetText(sellNpcX_, std::to_wstring(pos.x));
            SetText(sellNpcY_, std::to_wstring(pos.y));
            SetText(sellNpcPosText_, L"M" + std::to_wstring(kSellNpcs[static_cast<std::size_t>(index)].mapID) + L" • " +
                                     std::to_wstring(pos.x) + L"," + std::to_wstring(pos.y));
        } else {
            SetText(sellNpcX_, L"");
            SetText(sellNpcY_, L"");
            SetText(sellNpcPosText_, L"CHƯA LẤY");
        }
    }

    void PersistSellNpcPositionEditor(Account& a) {
        int index = a.profile.sellNpcPreset;
        if (index < 0 || index >= static_cast<int>(kSellNpcs.size())) index = 0;
        const std::wstring xText = GetText(sellNpcX_);
        const std::wstring yText = GetText(sellNpcY_);
        SellNpcPosition& pos = sellNpcPositions_[static_cast<std::size_t>(index)];
        if (xText.empty() || yText.empty()) {
            pos = SellNpcPosition{};
            SaveSharedSellNpcPositions(sellNpcPositions_);
            return;
        }
        const int x = _wtoi(xText.c_str());
        const int y = _wtoi(yText.c_str());
        if (x < 0 || y < 0) {
            pos = SellNpcPosition{};
            SaveSharedSellNpcPositions(sellNpcPositions_);
            return;
        }
        pos.x = x;
        pos.y = y;
        pos.valid = true;
        SaveSharedSellNpcPositions(sellNpcPositions_);
    }

    void OnSellNpcSelectionChanged() {
        Account* a = SelectedAccount();
        if (!a) return;
        PersistSellNpcPositionEditor(*a);
        const LRESULT sellSel = SendMessageW(sellNpcCombo_, CB_GETCURSEL, 0, 0);
        if (sellSel != CB_ERR && sellSel >= 0 && sellSel < static_cast<LRESULT>(kSellNpcs.size())) {
            a->profile.sellNpcPreset = static_cast<int>(sellSel);
        }
        SaveProfile(a->profile);
        LoadSellNpcPositionToUi(*a);
    }

    void CaptureSellNpcPosition() {
        Account* a = SelectedAccount();
        if (!a) { Log(L"Chưa chọn acc để lấy tọa NPC"); return; }
        std::wstring error;
        if (!ReadSnapshot(*a, error, 1200)) {
            LogAccount(*a, L"Không đọc được state để lấy tọa NPC: " + error);
            return;
        }
        int index = a->profile.sellNpcPreset;
        if (index < 0 || index >= static_cast<int>(kSellNpcs.size())) index = 0;
        const SellNpcPreset& npc = kSellNpcs[static_cast<std::size_t>(index)];
        const Snapshot& snap = a->snapshot;
        if ((snap.validMask & (ValidMap | ValidPosition)) != (ValidMap | ValidPosition)) {
            LogAccount(*a, L"State chưa có Map/X/Y để lấy tọa NPC");
            return;
        }
        if (snap.mapID != npc.mapID) {
            LogAccount(*a, L"Không lưu: đang ở MapID " + std::to_wstring(snap.mapID) +
                           L" nhưng NPC đã chọn thuộc MapID " + std::to_wstring(npc.mapID));
            return;
        }
        SellNpcPosition& pos = sellNpcPositions_[static_cast<std::size_t>(index)];
        pos.x = snap.x;
        pos.y = snap.y;
        pos.valid = true;
        SaveSharedSellNpcPositions(sellNpcPositions_);
        LoadSellNpcPositionToUi(*a);
        LogAccount(*a, L"ĐÃ LẤY TỌA NPC • " + std::wstring(npc.name) + L" • " +
                       std::to_wstring(pos.x) + L"," + std::to_wstring(pos.y));
    }

    bool BuildPortableMapConfig(std::wstring& text, std::wstring& error) const {
        text = L"TLMAPCFG\t1\r\n";
        int count = 0;
        for (const auto& spot : spots_) if (spot.valid && spot.mapID > 0 && !spot.name.empty()) ++count;
        text += L"MAP_COUNT\t" + std::to_wstring(count) + L"\r\n";
        for (const auto& spot : spots_) {
            if (!spot.valid || spot.mapID <= 0 || spot.name.empty()) continue;
            text += L"MAP\t" + EscapePortableField(SanitizeSpotName(spot.name)) + L"\t" +
                    std::to_wstring(spot.mapID) + L"\t" + std::to_wstring(spot.x) + L"\t" +
                    std::to_wstring(spot.y) + L"\r\n";
        }
        error.clear();
        return true;
    }

    bool ParsePortableMapConfig(const std::wstring& text, std::vector<TargetProfile>& maps,
                                std::wstring& error) const {
        maps.clear();
        bool header = false;
        int expected = -1;
        std::size_t start = 0;
        while (start <= text.size()) {
            std::size_t end = text.find(L'\n', start);
            std::wstring line = text.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
            if (!line.empty() && line.back() == L'\r') line.pop_back();
            if (!line.empty()) {
                const auto f = SplitPortableLine(line);
                if (!header) {
                    int version = 0;
                    if (f.size() != 2 || f[0] != L"TLMAPCFG" || !ParsePortableInt(f[1], version) || version != 1) {
                        error = L"Sai định dạng/version file MAP"; return false;
                    }
                    header = true;
                } else if (f[0] == L"MAP_COUNT") {
                    if (f.size() != 2 || !ParsePortableInt(f[1], expected) || expected < 0 || expected > 512) {
                        error = L"MAP_COUNT không hợp lệ"; return false;
                    }
                } else if (f[0] == L"MAP") {
                    if (f.size() != 5) { error = L"Dòng MAP sai cột"; return false; }
                    std::wstring name;
                    int mapID = 0, x = 0, y = 0;
                    if (!UnescapePortableField(f[1], name) || !ParsePortableInt(f[2], mapID) ||
                        !ParsePortableInt(f[3], x) || !ParsePortableInt(f[4], y)) {
                        error = L"Dòng MAP có dữ liệu không hợp lệ"; return false;
                    }
                    name = SanitizeSpotName(name);
                    if (name.empty() || mapID <= 0 || x < 0 || y < 0) {
                        error = L"MAP thiếu tên/MapID/tọa độ hợp lệ"; return false;
                    }
                    if (FindSpotIndex(maps, name) >= 0) { error = L"File MAP trùng tên bãi: " + name; return false; }
                    maps.push_back(TargetProfile{name, mapID, x, y, true});
                } else {
                    error = L"Dòng MAP không nhận dạng: " + f[0]; return false;
                }
            }
            if (end == std::wstring::npos) break;
            start = end + 1;
        }
        if (!header || expected < 0 || expected != static_cast<int>(maps.size())) {
            error = L"File MAP thiếu header/count hoặc số MAP không khớp"; return false;
        }
        return true;
    }

    void ExportPortableMapConfig() {
        std::wstring path;
        if (!PickMapConfigPath(hwnd_, true, path)) return;
        std::wstring text, error;
        if (!BuildPortableMapConfig(text, error) || !WriteUtf8File(path, text, error)) {
            MessageBoxW(hwnd_, error.c_str(), L"XUẤT MAP THẤT BẠI", MB_OK | MB_ICONERROR); return;
        }
        Log(L"XUẤT MAP PASS • " + std::to_wstring(spots_.size()) + L" bãi • " + path);
        MessageBoxW(hwnd_, L"Đã xuất danh sách MAP/bãi train ra file .tlmap.", L"XUẤT MAP PASS", MB_OK | MB_ICONINFORMATION);
    }

    void ImportPortableMapConfig() {
        std::wstring path;
        if (!PickMapConfigPath(hwnd_, false, path)) return;
        std::wstring text, error;
        if (!ReadUtf8File(path, text, error)) {
            MessageBoxW(hwnd_, error.c_str(), L"NHẬP MAP THẤT BẠI", MB_OK | MB_ICONERROR); return;
        }
        std::vector<TargetProfile> incoming;
        if (!ParsePortableMapConfig(text, incoming, error)) {
            MessageBoxW(hwnd_, error.c_str(), L"NHẬP MAP THẤT BẠI", MB_OK | MB_ICONERROR); return;
        }
        int added = 0, updated = 0;
        for (const auto& spot : incoming) {
            const int index = FindSpotIndex(spots_, spot.name);
            if (index < 0) { spots_.push_back(spot); ++added; }
            else {
                TargetProfile& old = spots_[static_cast<std::size_t>(index)];
                if (old.mapID != spot.mapID || old.x != spot.x || old.y != spot.y || !old.valid) {
                    old = spot; ++updated;
                }
            }
        }
        SaveSharedSpots(spots_);
        for (auto& account : accounts_) {
            if (!account) continue;
            NormalizeRotationProfile(account->profile);
            SaveProfile(account->profile);
        }
        RefreshSpotCombo();
        RefreshRotationList();
        LoadSelectedProfileToUi();
        Log(L"NHẬP MAP PASS • thêm " + std::to_wstring(added) + L" • cập nhật " + std::to_wstring(updated) +
            L" • giữ nguyên MAP khác đang có");
        MessageBoxW(hwnd_, (L"Nhập MAP xong.\nThêm: " + std::to_wstring(added) +
                            L"\nCập nhật cùng tên: " + std::to_wstring(updated) +
                            L"\nCác MAP khác đang có được giữ nguyên.").c_str(),
                    L"NHẬP MAP PASS", MB_OK | MB_ICONINFORMATION);
    }

    bool BuildPortableClickConfig(const Account& source, std::wstring& text, std::wstring& error) {
        text = L"TLCLICKCFG\t2\r\n";
        auto emitPoint = [&](const wchar_t* name, const ClickPoint& p) {
            text += L"POINT\t" + std::wstring(name) + L"\t" + std::to_wstring(p.valid ? 1 : 0) + L"\t" +
                    std::to_wstring(p.x) + L"\t" + std::to_wstring(p.y) + L"\t" +
                    std::to_wstring(p.baseW) + L"\t" + std::to_wstring(p.baseH) + L"\r\n";
        };
        emitPoint(L"AUTO", source.profile.points[static_cast<std::size_t>(ClickSlot::AutoMenu)]);
        emitPoint(L"ATTACK", source.profile.points[static_cast<std::size_t>(ClickSlot::Attack)]);
        emitPoint(L"STOP_AUTO", source.profile.points[static_cast<std::size_t>(ClickSlot::StopAuto2)]);

        text += L"MAIN_COUNT\t" + std::to_wstring(mainTradeSequence_.size()) + L"\r\n";
        for (std::size_t i = 0; i < mainTradeSequence_.size(); ++i) {
            const auto& st = mainTradeSequence_[i]; const auto& p = st.point;
            text += L"MAIN\t" + std::to_wstring(i) + L"\t" + std::to_wstring(p.valid ? 1 : 0) + L"\t" +
                    std::to_wstring(p.x) + L"\t" + std::to_wstring(p.y) + L"\t" + std::to_wstring(p.baseW) + L"\t" +
                    std::to_wstring(p.baseH) + L"\t" + std::to_wstring(st.delayMs) + L"\t" + std::to_wstring(st.repeat) + L"\t" +
                    EscapePortableField(st.description) + L"\r\n";
        }
        text += L"CHILD_COUNT\t" + std::to_wstring(childTradeSequence_.size()) + L"\r\n";
        for (std::size_t i = 0; i < childTradeSequence_.size(); ++i) {
            const auto& st = childTradeSequence_[i]; const auto& p = st.point;
            text += L"CHILD\t" + std::to_wstring(i) + L"\t" + std::to_wstring(st.target) + L"\t" + std::to_wstring(st.mainRef) + L"\t" +
                    std::to_wstring(p.valid ? 1 : 0) + L"\t" + std::to_wstring(p.x) + L"\t" + std::to_wstring(p.y) + L"\t" +
                    std::to_wstring(p.baseW) + L"\t" + std::to_wstring(p.baseH) + L"\t" + std::to_wstring(st.delayMs) + L"\t" +
                    std::to_wstring(st.repeat) + L"\t" + std::to_wstring(st.groupId) + L"\t" + std::to_wstring(st.groupRepeat) + L"\t" +
                    EscapePortableField(st.description) + L"\r\n";
        }
        text += L"SELL_COUNT\t" + std::to_wstring(source.profile.sellMacro.size()) + L"\r\n";
        for (std::size_t i = 0; i < source.profile.sellMacro.size(); ++i) {
            const auto& st = source.profile.sellMacro[i]; const auto& p = st.point;
            text += L"SELL\t" + std::to_wstring(i) + L"\t" + std::to_wstring(p.valid ? 1 : 0) + L"\t" +
                    std::to_wstring(p.x) + L"\t" + std::to_wstring(p.y) + L"\t" + std::to_wstring(p.baseW) + L"\t" +
                    std::to_wstring(p.baseH) + L"\t" + std::to_wstring(st.delayMs) + L"\t" + std::to_wstring(st.repeat) + L"\t" +
                    EscapePortableField(st.description) + L"\r\n";
        }
        error.clear(); return true;
    }

    bool ParsePortableClickConfig(const std::wstring& text,
                                  std::array<ClickPoint, 3>& points,
                                  std::vector<TradeSequenceStep>& mainSeq,
                                  std::vector<TradeSequenceStep>& childSeq,
                                  std::vector<SellMacroStep>& sellSeq,
                                  std::wstring& error) {
        points = {}; mainSeq.clear(); childSeq.clear(); sellSeq.clear();
        int expectedMain = -1, expectedChild = -1, expectedSell = -1;
        bool header = false; bool sawAuto = false, sawAttack = false, sawStop = false;
        std::size_t start = 0;
        while (start <= text.size()) {
            std::size_t end = text.find(L'\n', start);
            std::wstring line = text.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
            if (!line.empty() && line.back() == L'\r') line.pop_back();
            if (!line.empty()) {
                const auto f = SplitPortableLine(line);
                if (!header) {
                    int ver = 0;
                    if (f.size() != 2 || f[0] != L"TLCLICKCFG" || !ParsePortableInt(f[1], ver) || ver != 2) {
                        error = L"Sai định dạng/version file cấu hình"; return false;
                    }
                    header = true;
                } else if (f[0] == L"POINT") {
                    if (f.size() != 7) { error = L"Dòng POINT sai cột"; return false; }
                    int valid=0,x=0,y=0,w=0,h=0;
                    if (!ParsePortableInt(f[2],valid)||!ParsePortableInt(f[3],x)||!ParsePortableInt(f[4],y)||!ParsePortableInt(f[5],w)||!ParsePortableInt(f[6],h)) { error=L"POINT có số không hợp lệ"; return false; }
                    ClickPoint p{x,y,w,h,valid!=0}; if (p.valid && (x<0||y<0||w<=0||h<=0)) { error=L"POINT valid nhưng tọa/base size sai"; return false; }
                    if (f[1] == L"AUTO") { points[0]=p; sawAuto=true; }
                    else if (f[1] == L"ATTACK") { points[1]=p; sawAttack=true; }
                    else if (f[1] == L"STOP_AUTO") { points[2]=p; sawStop=true; }
                    else { error=L"POINT không nhận dạng"; return false; }
                } else if (f[0] == L"MAIN_COUNT" || f[0] == L"CHILD_COUNT" || f[0] == L"SELL_COUNT") {
                    if (f.size()!=2) { error=L"Dòng COUNT sai"; return false; }
                    int count=0; if(!ParsePortableInt(f[1],count)||count<0||count>64){error=L"COUNT ngoài 0..64";return false;}
                    if(f[0]==L"MAIN_COUNT") expectedMain=count; else if(f[0]==L"CHILD_COUNT") expectedChild=count; else expectedSell=count;
                } else if (f[0] == L"MAIN") {
                    if (f.size()!=10) { error=L"Dòng MAIN sai cột"; return false; }
                    int idx=0,valid=0,x=0,y=0,w=0,h=0,delay=0,repeat=0;
                    if(!ParsePortableInt(f[1],idx)||idx!=(int)mainSeq.size()||!ParsePortableInt(f[2],valid)||!ParsePortableInt(f[3],x)||!ParsePortableInt(f[4],y)||!ParsePortableInt(f[5],w)||!ParsePortableInt(f[6],h)||!ParsePortableInt(f[7],delay)||!ParsePortableInt(f[8],repeat)){error=L"MAIN có số không hợp lệ";return false;}
                    std::wstring desc; if(!UnescapePortableField(f[9],desc)){error=L"MAIN mô tả escape lỗi";return false;}
                    TradeSequenceStep st{}; st.target=1;st.mainRef=idx;st.description=desc;st.point={x,y,w,h,valid!=0};st.delayMs=std::clamp(delay,50,60000);st.repeat=std::clamp(repeat,1,999);
                    if(st.point.valid&&(x<0||y<0||w<=0||h<=0)){error=L"MAIN tọa độ lỗi";return false;} mainSeq.push_back(st);
                } else if (f[0] == L"CHILD") {
                    if (f.size()!=14) { error=L"Dòng CHILD sai cột"; return false; }
                    int v[12]{}; for(int i=1;i<=12;++i) if(!ParsePortableInt(f[(std::size_t)i],v[i-1])){error=L"CHILD có số không hợp lệ";return false;}
                    if(v[0]!=(int)childSeq.size()){error=L"CHILD index không liên tục";return false;}
                    std::wstring desc; if(!UnescapePortableField(f[13],desc)){error=L"CHILD mô tả escape lỗi";return false;}
                    TradeSequenceStep st{}; st.target=std::clamp(v[1],0,1);st.mainRef=v[2];st.point={v[4],v[5],v[6],v[7],v[3]!=0};st.delayMs=std::clamp(v[8],50,60000);st.repeat=std::clamp(v[9],1,999);st.groupId=std::max(0,v[10]);st.groupRepeat=std::clamp(v[11],1,999);st.description=desc;
                    if(st.point.valid&&(st.point.x<0||st.point.y<0||st.point.baseW<=0||st.point.baseH<=0)){error=L"CHILD tọa độ lỗi";return false;} childSeq.push_back(st);
                } else if (f[0] == L"SELL") {
                    if (f.size()!=10) { error=L"Dòng SELL sai cột"; return false; }
                    int idx=0,valid=0,x=0,y=0,w=0,h=0,delay=0,repeat=0;
                    if(!ParsePortableInt(f[1],idx)||idx!=(int)sellSeq.size()||!ParsePortableInt(f[2],valid)||!ParsePortableInt(f[3],x)||!ParsePortableInt(f[4],y)||!ParsePortableInt(f[5],w)||!ParsePortableInt(f[6],h)||!ParsePortableInt(f[7],delay)||!ParsePortableInt(f[8],repeat)){error=L"SELL có số không hợp lệ";return false;}
                    std::wstring desc; if(!UnescapePortableField(f[9],desc)){error=L"SELL mô tả escape lỗi";return false;}
                    SellMacroStep st{};st.point={x,y,w,h,valid!=0};st.delayMs=std::clamp(delay,50,60000);st.repeat=std::clamp(repeat,1,999);st.description=desc;
                    if(st.point.valid&&(x<0||y<0||w<=0||h<=0)){error=L"SELL tọa độ lỗi";return false;} sellSeq.push_back(st);
                } else { error = L"Dòng không nhận dạng: " + f[0]; return false; }
            }
            if (end == std::wstring::npos) break; start = end + 1;
        }
        if(!header||!sawAuto||!sawAttack||!sawStop||expectedMain<0||expectedChild<0||expectedSell<0){error=L"File cấu hình thiếu section bắt buộc";return false;}
        if(expectedMain!=(int)mainSeq.size()||expectedChild!=(int)childSeq.size()||expectedSell!=(int)sellSeq.size()){error=L"COUNT không khớp số dòng thực tế";return false;}
        for(const auto& st:childSeq) if(st.target==1&&(st.mainRef<0||st.mainRef>=(int)mainSeq.size())){error=L"CHILD tham chiếu MAIN ngoài phạm vi";return false;}
        return true;
    }

    void ExportPortableClickConfig() {
        Account* a = SelectedAccount();
        if (!a) { Log(L"XUẤT CFG: chọn một acc làm nguồn 3 điểm F8/macro bán trước."); return; }
        EnsureSharedChildTradeSequence();
        std::wstring path; if (!PickPortableConfigPath(hwnd_, true, path)) return;
        std::wstring text,error; if(!BuildPortableClickConfig(*a,text,error)||!WriteUtf8File(path,text,error)){MessageBoxW(hwnd_,error.c_str(),L"XUẤT CFG THẤT BẠI",MB_OK|MB_ICONERROR);return;}
        LogAccount(*a,L"XUẤT CFG PASS • MAIN="+std::to_wstring(mainTradeSequence_.size())+L" • CON="+std::to_wstring(childTradeSequence_.size())+L" • BÁN="+std::to_wstring(a->profile.sellMacro.size())+L" • 3 điểm F8 • "+path);
        MessageBoxW(hwnd_,L"Đã xuất cấu hình click portable.\nCó thể mang file .tlcfg sang tool/máy khác để NHẬP CFG.",L"XUẤT CFG PASS",MB_OK|MB_ICONINFORMATION);
    }

    void ImportPortableClickConfig() {
        Account* a = SelectedAccount();
        if (!a) { Log(L"NHẬP CFG: chọn acc đích trước để nhận 3 điểm F8/macro bán."); return; }
        std::wstring path; if (!PickPortableConfigPath(hwnd_, false, path)) return;
        std::wstring text,error; if(!ReadUtf8File(path,text,error)){MessageBoxW(hwnd_,error.c_str(),L"NHẬP CFG THẤT BẠI",MB_OK|MB_ICONERROR);return;}
        std::array<ClickPoint,3> points{}; std::vector<TradeSequenceStep> mainSeq,childSeq; std::vector<SellMacroStep> sellSeq;
        if(!ParsePortableClickConfig(text,points,mainSeq,childSeq,sellSeq,error)){MessageBoxW(hwnd_,error.c_str(),L"NHẬP CFG THẤT BẠI",MB_OK|MB_ICONERROR);return;}
        const std::wstring confirm=L"File sẽ thay thế:\n• CHUỖI GD MAIN dùng chung: "+std::to_wstring(mainSeq.size())+L" dòng\n• CHUỖI GD ACC CON dùng chung: "+std::to_wstring(childSeq.size())+L" dòng\n• Macro bán của acc đang chọn: "+std::to_wstring(sellSeq.size())+L" dòng\n• 3 điểm AUTO / ĐÁNH QUÁI / DỪNG AUTO\n\nKhông thay vai trò MAIN/CON, bãi train, NPC hay setting khác. Tiếp tục?";
        if(MessageBoxW(hwnd_,confirm.c_str(),L"NHẬP CLICK CFG",MB_YESNO|MB_ICONQUESTION)!=IDYES)return;
        mainTradeSequence_=std::move(mainSeq); childTradeSequence_=std::move(childSeq); NormalizeTradeGroups(childTradeSequence_); sharedChildTradeMigrationDone_=true;
        a->profile.sellMacro=std::move(sellSeq);
        a->profile.points[static_cast<std::size_t>(ClickSlot::AutoMenu)]=points[0]; a->profile.points[static_cast<std::size_t>(ClickSlot::Attack)]=points[1]; a->profile.points[static_cast<std::size_t>(ClickSlot::StopAuto2)]=points[2];
        SaveMainTradeSequence(); SaveSharedChildTradeSequence(); SaveProfile(a->profile);
        LoadSelectedProfileToUi(); RefreshTradeSequenceList(); RefreshSellMacroList(); PopulateTradeTargetCombo();
        LogAccount(*a,L"NHẬP CFG PASS • MAIN="+std::to_wstring(mainTradeSequence_.size())+L" • CON="+std::to_wstring(childTradeSequence_.size())+L" • BÁN="+std::to_wstring(a->profile.sellMacro.size())+L" • 3 điểm F8");
    }

    struct PortableMasterData {
        std::vector<TargetProfile> maps;
        std::array<ClickPoint, 3> points{};
        std::vector<TradeSequenceStep> mainSeq;
        std::vector<TradeSequenceStep> childSeq;
        std::vector<SellMacroStep> sellSeq;
        TargetProfile rendezvous{};
        std::array<SellNpcPosition, kSellNpcs.size()> sellerPositions{};
        std::array<std::pair<int, int>, 7> shortcutCoords{};
        std::array<std::pair<int, int>, 7> thdcCoords{};
        std::array<TimedClickPoint, 3> kunlunExitClicks{};
    };

    static std::array<std::pair<int, int>, 7> ShortcutCoordinatePairs(const ShortcutSettings& sc) {
        return {{{sc.kunlunNpcX, sc.kunlunNpcY}, {sc.xaTruyenX, sc.xaTruyenY},
                 {sc.ngaiX, sc.ngaiY}, {sc.tinhTucX, sc.tinhTucY},
                 {sc.thanhLienGateX, sc.thanhLienGateY}, {sc.phamLienGateX, sc.phamLienGateY},
                 {sc.khoVinhGateX, sc.khoVinhGateY}}};
    }

    static void ApplyShortcutCoordinatePairs(ShortcutSettings& sc,
                                              const std::array<std::pair<int, int>, 7>& v) {
        sc.kunlunNpcX=v[0].first; sc.kunlunNpcY=v[0].second;
        sc.xaTruyenX=v[1].first; sc.xaTruyenY=v[1].second;
        sc.ngaiX=v[2].first; sc.ngaiY=v[2].second;
        sc.tinhTucX=v[3].first; sc.tinhTucY=v[3].second;
        sc.thanhLienGateX=v[4].first; sc.thanhLienGateY=v[4].second;
        sc.phamLienGateX=v[5].first; sc.phamLienGateY=v[5].second;
        sc.khoVinhGateX=v[6].first; sc.khoVinhGateY=v[6].second;
    }

    static std::array<std::pair<int, int>, 7> ThdcCoordinatePairs(const ShortcutSettings& sc) {
        return {{{sc.thdcEntryX, sc.thdcEntryY}, {sc.thdcFloor1UpX, sc.thdcFloor1UpY},
                 {sc.thdcFloor2UpX, sc.thdcFloor2UpY}, {sc.thdcFloor2DownX, sc.thdcFloor2DownY},
                 {sc.thdcFloor3UpX, sc.thdcFloor3UpY}, {sc.thdcFloor3DownX, sc.thdcFloor3DownY},
                 {sc.thdcFloor4DownX, sc.thdcFloor4DownY}}};
    }

    static void ApplyThdcCoordinatePairs(ShortcutSettings& sc,
                                         const std::array<std::pair<int, int>, 7>& v) {
        sc.thdcEntryX=v[0].first; sc.thdcEntryY=v[0].second;
        sc.thdcFloor1UpX=v[1].first; sc.thdcFloor1UpY=v[1].second;
        sc.thdcFloor2UpX=v[2].first; sc.thdcFloor2UpY=v[2].second;
        sc.thdcFloor2DownX=v[3].first; sc.thdcFloor2DownY=v[3].second;
        sc.thdcFloor3UpX=v[4].first; sc.thdcFloor3UpY=v[4].second;
        sc.thdcFloor3DownX=v[5].first; sc.thdcFloor3DownY=v[5].second;
        sc.thdcFloor4DownX=v[6].first; sc.thdcFloor4DownY=v[6].second;
    }

    bool BuildPortableMasterConfig(Account& source, std::wstring& text, std::wstring& error) {
        std::wstring mapBlob, clickBlob;
        if (!BuildPortableMapConfig(mapBlob, error)) return false;
        EnsureSharedChildTradeSequence();
        if (!BuildPortableClickConfig(source, clickBlob, error)) return false;

        text = L"TLMASTERCFG\t2\r\n";
        text += L"MAP_BLOB\t" + EscapePortableField(mapBlob) + L"\r\n";
        text += L"CLICK_BLOB\t" + EscapePortableField(clickBlob) + L"\r\n";
        text += L"RENDEZVOUS\t" + std::to_wstring(tradeRendezvous_.valid ? 1 : 0) + L"\t" +
                std::to_wstring(tradeRendezvous_.mapID) + L"\t" + std::to_wstring(tradeRendezvous_.x) + L"\t" +
                std::to_wstring(tradeRendezvous_.y) + L"\r\n";
        text += L"SELLNPC_COUNT\t" + std::to_wstring(kSellNpcs.size()) + L"\r\n";
        for (std::size_t i=0;i<kSellNpcs.size();++i) {
            const auto& npc=kSellNpcs[i]; const auto& pos=sellNpcPositions_[i];
            text += L"SELLNPC\t"+std::to_wstring(i)+L"\t"+std::to_wstring(npc.npcID)+L"\t"+
                    std::to_wstring(npc.mapID)+L"\t"+std::to_wstring(pos.valid?1:0)+L"\t"+
                    std::to_wstring(pos.x)+L"\t"+std::to_wstring(pos.y)+L"\r\n";
        }
        static constexpr const wchar_t* keys[7] = {
            L"KUNLUN_NPC", L"XA_TRUYEN_BINH", L"NGAI_NI_NGOA_NHI", L"TINH_TUC_EXIT",
            L"THANH_LIEN_GATE", L"PHAM_LIEN_GATE", L"KHO_VINH_GATE"
        };
        const auto coords=ShortcutCoordinatePairs(shortcutSettings_);
        text += L"SHORTCUT_COUNT\t7\r\n";
        for (std::size_t i=0;i<coords.size();++i) {
            text += L"SHORTCUT\t"+std::wstring(keys[i])+L"\t"+std::to_wstring(coords[i].first)+L"\t"+
                    std::to_wstring(coords[i].second)+L"\r\n";
        }
        static constexpr const wchar_t* thdcKeys[7] = {
            L"M10000_TO_M10014", L"M10014_UP_M10015", L"M10015_UP_M10016",
            L"M10015_DOWN_M10014", L"M10016_UP_M10017", L"M10016_DOWN_M10015",
            L"M10017_DOWN_M10016"
        };
        static constexpr int thdcSourceMaps[7] = {10000, 10014, 10015, 10015, 10016, 10016, 10017};
        const auto thdc=ThdcCoordinatePairs(shortcutSettings_);
        text += L"THDC_COUNT\t7\r\n";
        for (std::size_t i=0;i<thdc.size();++i) {
            text += L"THDC\t"+std::wstring(thdcKeys[i])+L"\t"+std::to_wstring(thdcSourceMaps[i])+L"\t"+
                    std::to_wstring(thdc[i].first)+L"\t"+std::to_wstring(thdc[i].second)+L"\r\n";
        }
        for (std::size_t i=0;i<shortcutSettings_.kunlunExitClicks.size();++i) {
            const TimedClickPoint& click=shortcutSettings_.kunlunExitClicks[i];
            const ClickPoint& point=click.point;
            text += L"KUNLUN_EXIT_CLICK\t"+std::to_wstring(i)+L"\t"+std::to_wstring(point.valid?1:0)+L"\t"+
                    std::to_wstring(point.x)+L"\t"+std::to_wstring(point.y)+L"\t"+
                    std::to_wstring(point.baseW)+L"\t"+std::to_wstring(point.baseH)+L"\t"+
                    std::to_wstring(click.timeMs)+L"\t"+std::to_wstring(click.delayMs)+L"\r\n";
        }
        text += L"END\t1\r\n";
        error.clear(); return true;
    }

    bool ParsePortableMasterConfig(const std::wstring& text, PortableMasterData& out,
                                   std::wstring& error) {
        out = {};
        out.thdcCoords = ThdcCoordinatePairs(ShortcutSettings{});
        bool header=false, sawMap=false, sawClick=false, sawRendezvous=false, sawLegacyKunlunClick=false, sawEnd=false;
        int masterVersion=0;
        int expectedSeller=-1, expectedShortcut=-1, expectedThdc=-1;
        int sellerSeen=0, shortcutSeen=0, thdcSeen=0, kunlunClickSeen=0;
        std::wstring mapBlob, clickBlob;
        static constexpr const wchar_t* keys[7] = {
            L"KUNLUN_NPC", L"XA_TRUYEN_BINH", L"NGAI_NI_NGOA_NHI", L"TINH_TUC_EXIT",
            L"THANH_LIEN_GATE", L"PHAM_LIEN_GATE", L"KHO_VINH_GATE"
        };
        static constexpr const wchar_t* thdcKeys[7] = {
            L"M10000_TO_M10014", L"M10014_UP_M10015", L"M10015_UP_M10016",
            L"M10015_DOWN_M10014", L"M10016_UP_M10017", L"M10016_DOWN_M10015",
            L"M10017_DOWN_M10016"
        };
        static constexpr int thdcSourceMaps[7] = {10000, 10014, 10015, 10015, 10016, 10016, 10017};
        std::array<bool,7> shortcutSet{};
        std::array<bool,7> thdcSet{};
        std::array<bool,3> kunlunClickSet{};
        std::size_t start=0;
        while(start<=text.size()) {
            std::size_t end=text.find(L'\n',start);
            std::wstring line=text.substr(start,end==std::wstring::npos?std::wstring::npos:end-start);
            if(!line.empty()&&line.back()==L'\r') line.pop_back();
            if(!line.empty()) {
                const auto f=SplitPortableLine(line);
                if(!header) {
                    if(f.size()!=2||f[0]!=L"TLMASTERCFG"||!ParsePortableInt(f[1],masterVersion)||
                       (masterVersion!=1&&masterVersion!=2)) {
                        error=L"Sai định dạng/version file ALL (.tlmaster)"; return false;
                    }
                    header=true;
                } else if(f[0]==L"MAP_BLOB") {
                    if(sawMap||f.size()!=2||!UnescapePortableField(f[1],mapBlob)){error=L"MAP_BLOB lỗi/trùng";return false;} sawMap=true;
                } else if(f[0]==L"CLICK_BLOB") {
                    if(sawClick||f.size()!=2||!UnescapePortableField(f[1],clickBlob)){error=L"CLICK_BLOB lỗi/trùng";return false;} sawClick=true;
                } else if(f[0]==L"RENDEZVOUS") {
                    if(sawRendezvous||f.size()!=5){error=L"RENDEZVOUS lỗi/trùng";return false;}
                    int valid=0,map=0,x=0,y=0;
                    if(!ParsePortableInt(f[1],valid)||!ParsePortableInt(f[2],map)||!ParsePortableInt(f[3],x)||!ParsePortableInt(f[4],y)||
                       (valid!=0&&valid!=1)){error=L"RENDEZVOUS số không hợp lệ";return false;}
                    if(valid && (map<=0||x<0||y<0)){error=L"RENDEZVOUS valid nhưng Map/X/Y sai";return false;}
                    out.rendezvous={L"TỌA GD",map,x,y,valid!=0}; sawRendezvous=true;
                } else if(f[0]==L"SELLNPC_COUNT") {
                    if(f.size()!=2||expectedSeller>=0||!ParsePortableInt(f[1],expectedSeller)||expectedSeller!=(int)kSellNpcs.size()) {error=L"SELLNPC_COUNT phải đúng danh sách ResID hiện hành";return false;}
                } else if(f[0]==L"SELLNPC") {
                    if(f.size()!=7||sellerSeen>=(int)kSellNpcs.size()){error=L"Dòng SELLNPC sai";return false;}
                    int idx=0,id=0,map=0,valid=0,x=0,y=0;
                    if(!ParsePortableInt(f[1],idx)||!ParsePortableInt(f[2],id)||!ParsePortableInt(f[3],map)||!ParsePortableInt(f[4],valid)||!ParsePortableInt(f[5],x)||!ParsePortableInt(f[6],y)) {error=L"SELLNPC có số lỗi";return false;}
                    if(idx!=sellerSeen||idx<0||idx>=(int)kSellNpcs.size()||id!=kSellNpcs[(std::size_t)idx].npcID||map!=kSellNpcs[(std::size_t)idx].mapID) {error=L"SELLNPC ResID/MapID không khớp bản tool hiện tại";return false;}
                    if((valid!=0&&valid!=1)||(valid&&(x<0||y<0))){error=L"SELLNPC tọa độ/valid lỗi";return false;}
                    out.sellerPositions[(std::size_t)idx]={x,y,valid!=0}; ++sellerSeen;
                } else if(f[0]==L"SHORTCUT_COUNT") {
                    if(f.size()!=2||expectedShortcut>=0||!ParsePortableInt(f[1],expectedShortcut)||expectedShortcut!=7){error=L"SHORTCUT_COUNT phải = 7";return false;}
                } else if(f[0]==L"SHORTCUT") {
                    if(f.size()!=4){error=L"Dòng SHORTCUT sai cột";return false;}
                    int idx=-1,x=0,y=0;
                    for(int i=0;i<7;++i) if(f[1]==keys[i]) {idx=i;break;}
                    if(idx<0||shortcutSet[(std::size_t)idx]||!ParsePortableInt(f[2],x)||!ParsePortableInt(f[3],y)||x<0||y<0||((x==0)!=(y==0))) {error=L"SHORTCUT key/tọa độ lỗi hoặc trùng";return false;}
                    out.shortcutCoords[(std::size_t)idx]={x,y}; shortcutSet[(std::size_t)idx]=true; ++shortcutSeen;
                } else if(f[0]==L"THDC_COUNT") {
                    if(masterVersion!=2||f.size()!=2||expectedThdc>=0||!ParsePortableInt(f[1],expectedThdc)||expectedThdc!=7) {error=L"THDC_COUNT phải = 7 ở TLMASTERCFG v2";return false;}
                } else if(f[0]==L"THDC") {
                    if(masterVersion!=2||f.size()!=5){error=L"Dòng THDC sai cột/version";return false;}
                    int idx=-1,map=0,x=0,y=0;
                    for(int i=0;i<7;++i) if(f[1]==thdcKeys[i]) {idx=i;break;}
                    if(idx<0||thdcSet[(std::size_t)idx]||!ParsePortableInt(f[2],map)||!ParsePortableInt(f[3],x)||!ParsePortableInt(f[4],y)||
                       map!=thdcSourceMaps[idx]||x<=0||y<=0) {error=L"THDC key/MapID nguồn/tọa độ lỗi hoặc trùng";return false;}
                    out.thdcCoords[(std::size_t)idx]={x,y}; thdcSet[(std::size_t)idx]=true; ++thdcSeen;
                } else if(f[0]==L"KUNLUN_EXIT_CLICK") {
                    if(masterVersion!=2||f.size()!=9){error=L"KUNLUN_EXIT_CLICK sai cột/version";return false;}
                    int idx=0,valid=0,x=0,y=0,w=0,h=0,timeMs=0,delayMs=0;
                    if(!ParsePortableInt(f[1],idx)||!ParsePortableInt(f[2],valid)||!ParsePortableInt(f[3],x)||!ParsePortableInt(f[4],y)||
                       !ParsePortableInt(f[5],w)||!ParsePortableInt(f[6],h)||!ParsePortableInt(f[7],timeMs)||!ParsePortableInt(f[8],delayMs)||
                       idx<0||idx>=3||kunlunClickSet[(std::size_t)idx]||(valid!=0&&valid!=1)||timeMs<0||timeMs>60000||delayMs<0||delayMs>60000) {
                        error=L"KUNLUN_EXIT_CLICK có index/số/timing lỗi hoặc trùng";return false;
                    }
                    if(valid&&(x<0||y<0||w<=0||h<=0)){error=L"KUNLUN_EXIT_CLICK valid nhưng geometry sai";return false;}
                    out.kunlunExitClicks[(std::size_t)idx]={{x,y,w,h,valid!=0},timeMs,delayMs};
                    kunlunClickSet[(std::size_t)idx]=true; ++kunlunClickSeen;
                } else if(f[0]==L"KUNLUN_CLICK") {
                    if(masterVersion!=1||sawLegacyKunlunClick||f.size()!=6){error=L"KUNLUN_CLICK legacy lỗi/trùng/version";return false;}
                    int valid=0,x=0,y=0,w=0,h=0;
                    if(!ParsePortableInt(f[1],valid)||!ParsePortableInt(f[2],x)||!ParsePortableInt(f[3],y)||!ParsePortableInt(f[4],w)||!ParsePortableInt(f[5],h)||(valid!=0&&valid!=1)) {error=L"KUNLUN_CLICK có số lỗi";return false;}
                    if(valid&&(x<0||y<0||w<=0||h<=0)){error=L"KUNLUN_CLICK valid nhưng geometry sai";return false;}
                    out.kunlunExitClicks[0].point={x,y,w,h,valid!=0}; sawLegacyKunlunClick=true;
                } else if(f[0]==L"END") {
                    if(sawEnd||f.size()!=2||f[1]!=L"1"){error=L"END lỗi/trùng";return false;} sawEnd=true;
                } else { error=L"Dòng ALL không nhận dạng: "+f[0]; return false; }
            }
            if(end==std::wstring::npos) break; start=end+1;
        }
        const bool legacyClickReady = masterVersion==1 && sawLegacyKunlunClick;
        const bool v2SectionsReady = masterVersion==2 && expectedThdc==7 && thdcSeen==7 && kunlunClickSeen==3;
        if(!header||!sawMap||!sawClick||!sawRendezvous||!sawEnd||
           expectedSeller!=(int)kSellNpcs.size()||sellerSeen!=expectedSeller||
           expectedShortcut!=7||shortcutSeen!=7||(!legacyClickReady&&!v2SectionsReady)) {
            error=L"File ALL thiếu section/count bắt buộc"; return false;
        }
        if(!ParsePortableMapConfig(mapBlob,out.maps,error)) { error=L"MAP_BLOB: "+error; return false; }
        if(!ParsePortableClickConfig(clickBlob,out.points,out.mainSeq,out.childSeq,out.sellSeq,error)) { error=L"CLICK_BLOB: "+error; return false; }
        if(out.childSeq.empty()) {error=L"CHUỖI GD CON phải có ít nhất 1 bước autoclick";return false;}
        return true;
    }

    void ExportPortableMasterConfig() {
        Account* a=SelectedAccount();
        if(!a){Log(L"XUẤT TẤT CẢ: chọn 1 acc nguồn cho 3 điểm F8 + macro bán.");return;}
        PersistSellNpcPositionEditor(*a);
        if(shortcutWindow_&&IsWindow(shortcutWindow_)) PersistShortcutSettingsFromUi(false);
        std::wstring path; if(!PickMasterConfigPath(hwnd_,true,path))return;
        std::wstring text,error;
        if(!BuildPortableMasterConfig(*a,text,error)||!WriteUtf8File(path,text,error)){MessageBoxW(hwnd_,error.c_str(),L"XUẤT TẤT CẢ THẤT BẠI",MB_OK|MB_ICONERROR);return;}
        LogAccount(*a,L"XUẤT TẤT CẢ PASS • MAP="+std::to_wstring(spots_.size())+L" • NPC BÁN="+std::to_wstring(kSellNpcs.size())+
                     L" • SHORTCUT=7 • THDC=7 • CLS_CLICK=3 • MAIN="+std::to_wstring(mainTradeSequence_.size())+
                     L" • CON="+std::to_wstring(childTradeSequence_.size())+L" • "+path);
        MessageBoxW(hwnd_,L"Đã xuất 1 file .tlmaster duy nhất: MAP/bãi + NPC bán + tọa Đường tắt + 7 cổng THĐC đúng map nguồn + 3 click rời Côn Lôn (Time/Delay) + TỌA GD + 3 điểm AUTO/ĐÁNH/DỪNG + chuỗi GD + macro bán.",L"XUẤT TẤT CẢ PASS",MB_OK|MB_ICONINFORMATION);
    }

    void ImportPortableMasterConfig() {
        Account* a=SelectedAccount();
        if(!a){Log(L"NHẬP TẤT CẢ: chọn acc đích cho 3 điểm F8 + macro bán.");return;}
        std::wstring path; if(!PickMasterConfigPath(hwnd_,false,path))return;
        std::wstring text,error; if(!ReadUtf8File(path,text,error)){MessageBoxW(hwnd_,error.c_str(),L"NHẬP TẤT CẢ THẤT BẠI",MB_OK|MB_ICONERROR);return;}
        PortableMasterData incoming{};
        if(!ParsePortableMasterConfig(text,incoming,error)){MessageBoxW(hwnd_,error.c_str(),L"NHẬP TẤT CẢ THẤT BẠI",MB_OK|MB_ICONERROR);return;}
        const std::wstring confirm=L"File .tlmaster đã parse/validate hoàn chỉnh, chưa sửa dữ liệu hiện tại.\n\nSẽ THAY THẾ đồng bộ:\n• MAP/bãi: "+std::to_wstring(incoming.maps.size())+
            L"\n• 6 tọa NPC bán (ResID/MapID đã đối chiếu)\n• 7 tọa Đường tắt\n• 7 cổng THĐC kèm đúng MapID nguồn\n• 3 click rời Côn Lôn + Time/Delay riêng\n• TỌA GD\n• MAIN GD: "+std::to_wstring(incoming.mainSeq.size())+
            L" dòng\n• CON GD: "+std::to_wstring(incoming.childSeq.size())+L" dòng (thứ tự động, không có dòng đặc biệt)\n• Macro bán của acc đang chọn: "+std::to_wstring(incoming.sellSeq.size())+
            L" dòng\n• 3 điểm AUTO / ĐÁNH QUÁI / DỪNG AUTO\n\nTiếp tục?";
        if(MessageBoxW(hwnd_,confirm.c_str(),L"NHẬP TẤT CẢ",MB_YESNO|MB_ICONQUESTION)!=IDYES)return;

        const DWORD now=GetTickCount();
        if(tradeTxn_.phase!=TradePhase::Idle) AbortTrade(L"nhập cấu hình tổng .tlmaster",now);
        spots_=std::move(incoming.maps);
        mainTradeSequence_=std::move(incoming.mainSeq);
        childTradeSequence_=std::move(incoming.childSeq); NormalizeTradeGroups(childTradeSequence_); sharedChildTradeMigrationDone_=true;
        a->profile.sellMacro=std::move(incoming.sellSeq);
        a->profile.points[static_cast<std::size_t>(ClickSlot::AutoMenu)]=incoming.points[0];
        a->profile.points[static_cast<std::size_t>(ClickSlot::Attack)]=incoming.points[1];
        a->profile.points[static_cast<std::size_t>(ClickSlot::StopAuto2)]=incoming.points[2];
        tradeRendezvous_=incoming.rendezvous; tradeRendezvous_.name=L"TỌA GD";
        sellNpcPositions_=incoming.sellerPositions;
        ApplyShortcutCoordinatePairs(shortcutSettings_,incoming.shortcutCoords);
        ApplyThdcCoordinatePairs(shortcutSettings_,incoming.thdcCoords);
        shortcutSettings_.kunlunExitClicks=incoming.kunlunExitClicks;

        SaveSharedSpots(spots_); SaveMainTradeSequence(); SaveSharedChildTradeSequence(); SaveSharedSellNpcPositions(sellNpcPositions_); SaveShortcutSettings(shortcutSettings_);
        WriteIniInt(L"Global",L"TradeRendezvousMap",tradeRendezvous_.mapID); WriteIniInt(L"Global",L"TradeRendezvousX",tradeRendezvous_.x);
        WriteIniInt(L"Global",L"TradeRendezvousY",tradeRendezvous_.y); WriteIniInt(L"Global",L"TradeRendezvousValid",tradeRendezvous_.valid?1:0); FlushIni();
        SaveProfile(a->profile);
        for(auto& item:accounts_) if(item){NormalizeRotationProfile(item->profile);ResetShortcutRoute(item->runtime);SaveProfile(item->profile);}
        RefreshSpotCombo(); RefreshRotationList(); LoadSelectedProfileToUi(); RefreshTradeSequenceList(); RefreshSellMacroList(); PopulateTradeTargetCombo(); UpdateTradeRendezvousLabel();
        if(shortcutWindow_&&IsWindow(shortcutWindow_)){LoadShortcutSettingsToUi();RefreshShortcutSellerUi();}
        LogAccount(*a,L"NHẬP TẤT CẢ PASS • file đã validate trước khi apply • reset route/trade state để không dùng tọa cũ.");
        MessageBoxW(hwnd_,L"NHẬP TẤT CẢ PASS. Toàn bộ nhóm tọa/cấu hình đã được áp dụng đồng bộ từ 1 file .tlmaster.",L"NHẬP TẤT CẢ PASS",MB_OK|MB_ICONINFORMATION);
    }

    void LoadTradeSettings() {
        tradeEnabled_ = ReadIniInt(L"Global", L"TradeEnabled", 1) != 0;
        tradeRendezvous_.name = L"TỌA GD";
        tradeRendezvous_.mapID = ReadIniInt(L"Global", L"TradeRendezvousMap", 0);
        tradeRendezvous_.x = ReadIniInt(L"Global", L"TradeRendezvousX", 0);
        tradeRendezvous_.y = ReadIniInt(L"Global", L"TradeRendezvousY", 0);
        tradeRendezvous_.valid = tradeRendezvous_.mapID > 0 && ReadIniInt(L"Global", L"TradeRendezvousValid", 0) != 0;
        tradeRendezvousTolerance_ = kPreciseWorldTolerance; // v1.6 migration: do not reuse legacy 120 for GD.
    }

    void LoadTradeSequence() {
        // v0.2.7: exactly two reusable trade definitions:
        // 1) MAIN shared coordinate library; 2) one shared ordered ACC CON workflow.
        mainTradeSequence_.clear();
        childTradeSequence_.clear();
        legacyChildTradeTemplate_.clear();
        sharedChildTradeMigrationDone_ = false;

        // Prefer the MAIN-shared section.
        int mainCount = std::clamp(ReadIniInt(L"MainTradeSequence", L"Count", 0), 0, 64);
        for (int i = 0; i < mainCount; ++i) {
            TradeSequenceStep step{};
            const std::wstring prefix = L"Step_" + std::to_wstring(i) + L"_";
            step.target = 1;
            step.mainRef = i;
            step.description = ReadIniText(L"MainTradeSequence", prefix + L"Desc");
            step.point.x = ReadIniInt(L"MainTradeSequence", prefix + L"X", -1);
            step.point.y = ReadIniInt(L"MainTradeSequence", prefix + L"Y", -1);
            step.point.baseW = ReadIniInt(L"MainTradeSequence", prefix + L"W", 0);
            step.point.baseH = ReadIniInt(L"MainTradeSequence", prefix + L"H", 0);
            step.point.valid = step.point.x >= 0 && step.point.y >= 0 && step.point.baseW > 0 && step.point.baseH > 0;
            step.delayMs = std::clamp(ReadIniInt(L"MainTradeSequence", prefix + L"Delay", 500), 50, 60000);
            step.repeat = std::clamp(ReadIniInt(L"MainTradeSequence", prefix + L"Repeat", 1), 1, 999);
            mainTradeSequence_.push_back(step);
        }

        // v0.2.7 shared ACC CON workflow. Count=-1 means the section does not exist yet,
        // allowing one-time migration from the old per-CON profiles.
        const int sharedChildCountRaw = ReadIniInt(L"ChildTradeSequence", L"Count", -1);
        if (sharedChildCountRaw >= 0) {
            sharedChildTradeMigrationDone_ = true;
            const int sharedChildCount = std::clamp(sharedChildCountRaw, 0, 64);
            for (int i = 0; i < sharedChildCount; ++i) {
                TradeSequenceStep step{};
                const std::wstring prefix = L"Step_" + std::to_wstring(i) + L"_";
                step.target = std::clamp(ReadIniInt(L"ChildTradeSequence", prefix + L"Target", 0), 0, 1);
                step.mainRef = ReadIniInt(L"ChildTradeSequence", prefix + L"MainRef", -1);
                step.description = ReadIniText(L"ChildTradeSequence", prefix + L"Desc");
                step.point.x = ReadIniInt(L"ChildTradeSequence", prefix + L"X", -1);
                step.point.y = ReadIniInt(L"ChildTradeSequence", prefix + L"Y", -1);
                step.point.baseW = ReadIniInt(L"ChildTradeSequence", prefix + L"W", 0);
                step.point.baseH = ReadIniInt(L"ChildTradeSequence", prefix + L"H", 0);
                step.point.valid = step.point.x >= 0 && step.point.y >= 0 && step.point.baseW > 0 && step.point.baseH > 0;
                step.delayMs = std::clamp(ReadIniInt(L"ChildTradeSequence", prefix + L"Delay", 500), 50, 60000);
                step.repeat = std::clamp(ReadIniInt(L"ChildTradeSequence", prefix + L"Repeat", 1), 1, 999);
                step.groupId = std::max(0, ReadIniInt(L"ChildTradeSequence", prefix + L"GroupId", 0));
                step.groupRepeat = std::clamp(ReadIniInt(L"ChildTradeSequence", prefix + L"GroupRepeat", 1), 1, 999);
                        childTradeSequence_.push_back(step);
            }
        }

        // One-time migration template from v0.2.3 combined global TradeSequence.
        int legacyCount = std::clamp(ReadIniInt(L"TradeSequence", L"Count", 0), 0, 64);
        std::vector<int> oldMainToNew(static_cast<std::size_t>(legacyCount), -1);
        if (mainTradeSequence_.empty()) {
            for (int i = 0; i < legacyCount; ++i) {
                const std::wstring prefix = L"Step_" + std::to_wstring(i) + L"_";
                if (std::clamp(ReadIniInt(L"TradeSequence", prefix + L"Target", 0), 0, 1) != 0) continue;
                TradeSequenceStep mainStep{};
                mainStep.target = 1; mainStep.mainRef = static_cast<int>(mainTradeSequence_.size());
                mainStep.description = ReadIniText(L"TradeSequence", prefix + L"Desc");
                mainStep.point.x = ReadIniInt(L"TradeSequence", prefix + L"X", -1);
                mainStep.point.y = ReadIniInt(L"TradeSequence", prefix + L"Y", -1);
                mainStep.point.baseW = ReadIniInt(L"TradeSequence", prefix + L"W", 0);
                mainStep.point.baseH = ReadIniInt(L"TradeSequence", prefix + L"H", 0);
                mainStep.point.valid = mainStep.point.x >= 0 && mainStep.point.y >= 0 && mainStep.point.baseW > 0 && mainStep.point.baseH > 0;
                mainStep.delayMs = std::clamp(ReadIniInt(L"TradeSequence", prefix + L"Delay", 500), 50, 60000);
                mainStep.repeat = std::clamp(ReadIniInt(L"TradeSequence", prefix + L"Repeat", 1), 1, 999);
                oldMainToNew[static_cast<std::size_t>(i)] = mainStep.mainRef;
                mainTradeSequence_.push_back(mainStep);
            }
            if (!mainTradeSequence_.empty()) SaveMainTradeSequence();
        } else {
            // Map legacy MAIN rows to new MAIN rows in original MAIN-order for child migration.
            int ref = 0;
            for (int i = 0; i < legacyCount; ++i) {
                const std::wstring prefix = L"Step_" + std::to_wstring(i) + L"_";
                if (std::clamp(ReadIniInt(L"TradeSequence", prefix + L"Target", 0), 0, 1) == 0 && ref < static_cast<int>(mainTradeSequence_.size())) {
                    oldMainToNew[static_cast<std::size_t>(i)] = ref++;
                }
            }
        }
        for (int i = 0; i < legacyCount; ++i) {
            TradeSequenceStep step{};
            const std::wstring prefix = L"Step_" + std::to_wstring(i) + L"_";
            const int oldTarget = std::clamp(ReadIniInt(L"TradeSequence", prefix + L"Target", 0), 0, 1);
            step.description = ReadIniText(L"TradeSequence", prefix + L"Desc");
            step.delayMs = std::clamp(ReadIniInt(L"TradeSequence", prefix + L"Delay", 500), 50, 60000);
            step.repeat = std::clamp(ReadIniInt(L"TradeSequence", prefix + L"Repeat", 1), 1, 999);
            if (oldTarget == 0) {
                step.target = 1;
                step.mainRef = oldMainToNew[static_cast<std::size_t>(i)];
            } else {
                step.target = 0;
                step.mainRef = -1;
                step.point.x = ReadIniInt(L"TradeSequence", prefix + L"X", -1);
                step.point.y = ReadIniInt(L"TradeSequence", prefix + L"Y", -1);
                step.point.baseW = ReadIniInt(L"TradeSequence", prefix + L"W", 0);
                step.point.baseH = ReadIniInt(L"TradeSequence", prefix + L"H", 0);
                step.point.valid = step.point.x >= 0 && step.point.y >= 0 && step.point.baseW > 0 && step.point.baseH > 0;
            }
            if (step.target == 0 || step.mainRef >= 0) legacyChildTradeTemplate_.push_back(step);
        }
        NormalizeTradeGroups(childTradeSequence_);
        NormalizeTradeGroups(legacyChildTradeTemplate_);
    }

    void SaveMainTradeSequence() {
        EnsureUnicodeIni();
        WriteIniInt(L"MainTradeSequence", L"Count", static_cast<int>(mainTradeSequence_.size()));
        for (std::size_t i = 0; i < mainTradeSequence_.size(); ++i) {
            const TradeSequenceStep& step = mainTradeSequence_[i];
            const std::wstring prefix = L"Step_" + std::to_wstring(i) + L"_";
            WriteIniText(L"MainTradeSequence", prefix + L"Desc", step.description);
            WriteIniInt(L"MainTradeSequence", prefix + L"X", step.point.valid ? step.point.x : -1);
            WriteIniInt(L"MainTradeSequence", prefix + L"Y", step.point.valid ? step.point.y : -1);
            WriteIniInt(L"MainTradeSequence", prefix + L"W", step.point.valid ? step.point.baseW : 0);
            WriteIniInt(L"MainTradeSequence", prefix + L"H", step.point.valid ? step.point.baseH : 0);
            WriteIniInt(L"MainTradeSequence", prefix + L"Delay", step.delayMs);
            WriteIniInt(L"MainTradeSequence", prefix + L"Repeat", step.repeat);
        }
        FlushIni();
    }

    void SaveSharedChildTradeSequence() {
        EnsureUnicodeIni();
        NormalizeTradeGroups(childTradeSequence_);
        WriteIniInt(L"ChildTradeSequence", L"Count", static_cast<int>(childTradeSequence_.size()));
        for (std::size_t i = 0; i < childTradeSequence_.size(); ++i) {
            const TradeSequenceStep& step = childTradeSequence_[i];
            const std::wstring prefix = L"Step_" + std::to_wstring(i) + L"_";
            WriteIniInt(L"ChildTradeSequence", prefix + L"Target", step.target);
            WriteIniInt(L"ChildTradeSequence", prefix + L"MainRef", step.mainRef);
            WriteIniText(L"ChildTradeSequence", prefix + L"Desc", step.description);
            WriteIniInt(L"ChildTradeSequence", prefix + L"X", step.point.valid ? step.point.x : -1);
            WriteIniInt(L"ChildTradeSequence", prefix + L"Y", step.point.valid ? step.point.y : -1);
            WriteIniInt(L"ChildTradeSequence", prefix + L"W", step.point.valid ? step.point.baseW : 0);
            WriteIniInt(L"ChildTradeSequence", prefix + L"H", step.point.valid ? step.point.baseH : 0);
            WriteIniInt(L"ChildTradeSequence", prefix + L"Delay", step.delayMs);
            WriteIniInt(L"ChildTradeSequence", prefix + L"Repeat", step.repeat);
            WriteIniInt(L"ChildTradeSequence", prefix + L"GroupId", step.groupId);
            WriteIniInt(L"ChildTradeSequence", prefix + L"GroupRepeat", step.groupRepeat);
        }
        sharedChildTradeMigrationDone_ = true;
        FlushIni();
    }

    void EnsureSharedChildTradeSequence() {
        if (sharedChildTradeMigrationDone_) return;
        sharedChildTradeMigrationDone_ = true;

        // Prefer the old sequence of the lowest CON slot so existing setups migrate deterministically.
        for (int slot = 1; slot <= kChildTradeCount; ++slot) {
            Account* child = AccountByTradeRole(slot + 1);
            if (!child || child->profile.childTradeSequence.empty()) continue;
            childTradeSequence_ = child->profile.childTradeSequence;
            SaveSharedChildTradeSequence();
            LogAccount(*child, L"v0.2.7 MIGRATE: lấy chuỗi GD cũ của " + TradeRoleLabel(child->profile.tradeRole) +
                               L" làm CHUỖI GD ACC CON dùng chung cho CON1→CON12.");
            return;
        }

        if (!legacyChildTradeTemplate_.empty()) {
            childTradeSequence_ = legacyChildTradeTemplate_;
            SaveSharedChildTradeSequence();
            Log(L"v0.2.7 MIGRATE: lấy template GD legacy làm CHUỖI GD ACC CON dùng chung.");
            return;
        }

        // Persist an intentional empty shared section so we do not repeatedly scan legacy profiles.
        SaveSharedChildTradeSequence();
    }

    std::vector<TradeSequenceStep>* EditorSequence() {
        if (tradeEditorMode_ == 1) return &mainTradeSequence_;
        if (tradeEditorMode_ == 2) {
            Account* child = TradeEditorChild();
            if (!child || child->profile.tradeRole < 2) return nullptr; // selected CON is only the capture/test donor.
            EnsureSharedChildTradeSequence();
            return &childTradeSequence_;
        }
        return nullptr;
    }

    Account* TradeEditorChild() {
        Account* child = AccountByPid(tradeEditorChildPid_);
        return child && child->profile.tradeRole >= 2 ? child : nullptr;
    }

    const TradeSequenceStep* ResolveMainReference(const TradeSequenceStep& step) const {
        if (step.target != 1 || step.mainRef < 0 || step.mainRef >= static_cast<int>(mainTradeSequence_.size())) return nullptr;
        return &mainTradeSequence_[static_cast<std::size_t>(step.mainRef)];
    }

    void NormalizeTradeGroups(std::vector<TradeSequenceStep>& seq) {
        int nextId = 1;
        int previousOldId = 0;
        int currentNewId = 0;
        int currentRepeat = 1;
        for (std::size_t i = 0; i < seq.size(); ++i) {
            TradeSequenceStep& step = seq[i];
            const int oldId = step.groupId;
            if (oldId <= 0) {
                step.groupId = 0;
                step.groupRepeat = 1;
                previousOldId = 0;
                currentNewId = 0;
                continue;
            }
            if (i == 0 || oldId != previousOldId || currentNewId == 0) {
                currentNewId = nextId++;
                currentRepeat = std::clamp(step.groupRepeat, 1, 999);
            }
            step.groupId = currentNewId;
            step.groupRepeat = currentRepeat;
            previousOldId = oldId;
        }
    }

    int MaxTradeGroupId(const std::vector<TradeSequenceStep>& seq) const {
        int maxId = 0;
        for (const TradeSequenceStep& step : seq) maxId = std::max(maxId, step.groupId);
        return maxId;
    }

    std::size_t TradeGroupStart(const std::vector<TradeSequenceStep>& seq, std::size_t index) const {
        if (index >= seq.size() || seq[index].groupId <= 0) return index;
        const int id = seq[index].groupId;
        while (index > 0 && seq[index - 1].groupId == id) --index;
        return index;
    }

    std::size_t TradeGroupEnd(const std::vector<TradeSequenceStep>& seq, std::size_t index) const {
        if (index >= seq.size() || seq[index].groupId <= 0) return index;
        const int id = seq[index].groupId;
        while (index + 1 < seq.size() && seq[index + 1].groupId == id) ++index;
        return index;
    }

    std::wstring TradeGroupLabel(const TradeSequenceStep& step) const {
        if (step.groupId <= 0) return L"-";
        return L"G" + std::to_wstring(step.groupId) + L" ×" + std::to_wstring(step.groupRepeat);
    }

    bool TradeSequenceReady(std::wstring& reason) {
        EnsureSharedChildTradeSequence();
        if (childTradeSequence_.empty()) { reason = L"chưa có CHUỖI GD ACC CON dùng chung"; return false; }
        for (std::size_t i = 0; i < childTradeSequence_.size(); ++i) {
            const TradeSequenceStep& step = childTradeSequence_[i];
            if (step.target == 1) {
                const TradeSequenceStep* shared = ResolveMainReference(step);
                if (!shared) { reason = L"bước " + std::to_wstring(i + 1) + L" tham chiếu MAIN không tồn tại"; return false; }
                if (!shared->point.valid) { reason = L"MAIN bước " + std::to_wstring(step.mainRef + 1) + L" chưa lấy tọa độ"; return false; }
            } else if (!step.point.valid) {
                reason = L"CON bước " + std::to_wstring(i + 1) + L" chưa lấy tọa độ"; return false;
            }
        }
        return true;
    }

    std::wstring TradeStepTargetLabel(const TradeSequenceStep& step) {
        if (tradeEditorMode_ == 1) return L"MAIN";
        if (step.target == 1) return L"MAIN #" + std::to_wstring(step.mainRef + 1);
        return L"ACC CON ĐANG GD";
    }

    const TradeSequenceStep* EffectiveEditorStep(const TradeSequenceStep& step) const {
        return tradeEditorMode_ == 2 && step.target == 1 ? ResolveMainReference(step) : &step;
    }

    void RefreshTradeSequenceList() {
        if (!tradeSeqList_) return;
        ListView_DeleteAllItems(tradeSeqList_);
        std::vector<TradeSequenceStep>* seq = EditorSequence();
        if (!seq) return;
        for (std::size_t i = 0; i < seq->size(); ++i) {
            const TradeSequenceStep& step = (*seq)[i];
            const TradeSequenceStep* effective = EffectiveEditorStep(step);
            std::wstring idx = std::to_wstring(i + 1);
            LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = static_cast<int>(i); item.pszText = idx.data();
            ListView_InsertItem(tradeSeqList_, &item);
            const std::array<std::wstring, 6> cols = {{
                TradeStepTargetLabel(step),
                (effective && !effective->description.empty()) ? effective->description : L"(không mô tả)",
                effective ? PointDescription(effective->point) : L"MAIN REF LỖI",
                effective ? std::to_wstring(effective->delayMs) : L"-",
                effective ? std::to_wstring(effective->repeat) : L"-",
                tradeEditorMode_ == 2 ? TradeGroupLabel(step) : L"-"
            }};
            for (int col = 0; col < 6; ++col) ListView_SetItemText(tradeSeqList_, static_cast<int>(i), col + 1, const_cast<wchar_t*>(cols[static_cast<std::size_t>(col)].c_str()));
        }
    }

    void SelectTradeSequenceDragRange(int startRow, int endRow) {
        if (!tradeSeqList_) return;
        const int count = ListView_GetItemCount(tradeSeqList_);
        if (count <= 0) return;
        startRow = std::clamp(startRow, 0, count - 1);
        endRow = std::clamp(endRow, 0, count - 1);
        const int first = std::min(startRow, endRow);
        const int last = std::max(startRow, endRow);
        tradeSeqDragUpdating_ = true;
        for (int i = 0; i < count; ++i) {
            const UINT state = (i >= first && i <= last) ? LVIS_SELECTED : 0;
            ListView_SetItemState(tradeSeqList_, i, state, LVIS_SELECTED);
        }
        ListView_SetItemState(tradeSeqList_, endRow, LVIS_FOCUSED, LVIS_FOCUSED);
        ListView_EnsureVisible(tradeSeqList_, endRow, FALSE);
        tradeSeqDragUpdating_ = false;
        LoadTradeSequenceRowToEditor(endRow);
        UpdateRecorderUi(L"Đã kéo chọn " + std::to_wstring(last - first + 1) + L" dòng GD");
    }

    int SelectedTradeSequenceIndex() const {
        return FocusedSelectedRow(tradeSeqList_);
    }

    void PopulateTradeTargetCombo(const TradeSequenceStep* step = nullptr) {
        if (!tradeSeqTarget_) return;
        SendMessageW(tradeSeqTarget_, CB_RESETCONTENT, 0, 0);
        if (tradeEditorMode_ == 1) {
            SendMessageW(tradeSeqTarget_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"MAIN (DÙNG CHUNG)"));
            SendMessageW(tradeSeqTarget_, CB_SETCURSEL, 0, 0);
            EnableWindow(tradeSeqTarget_, FALSE);
            return;
        }
        EnableWindow(tradeSeqTarget_, TRUE);
        const std::wstring childName = L"ACC CON ĐANG GD (DÙNG CHUNG)";
        SendMessageW(tradeSeqTarget_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(childName.c_str()));
        for (std::size_t i = 0; i < mainTradeSequence_.size(); ++i) {
            std::wstring label = L"MAIN #" + std::to_wstring(i + 1) + L" • " + (mainTradeSequence_[i].description.empty() ? L"(không mô tả)" : mainTradeSequence_[i].description);
            SendMessageW(tradeSeqTarget_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        }
        int sel = 0;
        if (step && step->target == 1 && step->mainRef >= 0 && step->mainRef < static_cast<int>(mainTradeSequence_.size())) sel = step->mainRef + 1;
        SendMessageW(tradeSeqTarget_, CB_SETCURSEL, sel, 0);
    }

    void LoadTradeSequenceRowToEditor(int index) {
        std::vector<TradeSequenceStep>* seq = EditorSequence();
        if (!seq || index < 0 || index >= static_cast<int>(seq->size())) return;
        TradeSequenceStep& step = (*seq)[static_cast<std::size_t>(index)];
        const TradeSequenceStep* effective = EffectiveEditorStep(step);
        PopulateTradeTargetCombo(&step);
        if (effective) {
            SetText(tradeSeqDesc_, effective->description);
            SetText(tradeSeqDelay_, std::to_wstring(effective->delayMs));
            SetText(tradeSeqRepeat_, std::to_wstring(effective->repeat));
        }
        if (tradeSeqGroupRepeat_) SetText(tradeSeqGroupRepeat_, std::to_wstring(step.groupId > 0 ? step.groupRepeat : 1));
        const bool sharedRef = tradeEditorMode_ == 2 && step.target == 1;
        EnableWindow(tradeSeqDesc_, !sharedRef);
        EnableWindow(tradeSeqDelay_, !sharedRef);
        EnableWindow(tradeSeqRepeat_, !sharedRef);
        if (tradeSeqGroupRepeat_) EnableWindow(tradeSeqGroupRepeat_, tradeEditorMode_ == 2);
    }

    void SaveEditorSequence() {
        if (tradeEditorMode_ == 1) SaveMainTradeSequence();
        else if (tradeEditorMode_ == 2) SaveSharedChildTradeSequence();
    }

    void AddTradeSequenceRow() {
        std::vector<TradeSequenceStep>* seq = EditorSequence();
        if (!seq || seq->size() >= 64) return;
        const int selected = SelectedTradeSequenceIndex();
        const std::size_t insertAt = selected >= 0 && selected < static_cast<int>(seq->size())
            ? static_cast<std::size_t>(selected + 1) : seq->size();
        TradeSequenceStep step{};
        if (tradeEditorMode_ == 1) {
            // Inserting a MAIN row shifts every later MAIN reference used by the shared CON sequence.
            EnsureSharedChildTradeSequence();
            for (TradeSequenceStep& cs : childTradeSequence_) {
                if (cs.target == 1 && cs.mainRef >= static_cast<int>(insertAt)) ++cs.mainRef;
            }
            step.target = 1; step.mainRef = static_cast<int>(insertAt);
            step.description = L"MAIN bước " + std::to_wstring(insertAt + 1);
            seq->insert(seq->begin() + static_cast<std::ptrdiff_t>(insertAt), step);
            for (std::size_t i = 0; i < mainTradeSequence_.size(); ++i) mainTradeSequence_[i].mainRef = static_cast<int>(i);
            SaveSharedChildTradeSequence();
        } else {
            step.target = 0; step.mainRef = -1;
            step.description = L"CON bước " + std::to_wstring(insertAt + 1);
            seq->insert(seq->begin() + static_cast<std::ptrdiff_t>(insertAt), step);
            NormalizeTradeGroups(*seq);
        }
        SaveEditorSequence(); RefreshTradeSequenceList(); PopulateTradeTargetCombo();
        const int row = static_cast<int>(insertAt);
        ListView_SetItemState(tradeSeqList_, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(tradeSeqList_, row, FALSE); LoadTradeSequenceRowToEditor(row);
    }

    void DeleteTradeSequenceRow() {
        std::vector<TradeSequenceStep>* seq = EditorSequence();
        std::vector<int> rows = SelectedRows(tradeSeqList_);
        if (!seq || rows.empty()) return;
        rows.erase(std::remove_if(rows.begin(), rows.end(), [&](int row) {
            return row < 0 || row >= static_cast<int>(seq->size());
        }), rows.end());
        if (rows.empty()) return;
        std::sort(rows.begin(), rows.end());

        if (tradeEditorMode_ == 1) {
            // Repair the one shared ACC CON workflow against all deleted MAIN refs before erasing rows.
            EnsureSharedChildTradeSequence();
            for (TradeSequenceStep& cs : childTradeSequence_) if (cs.target == 1) {
                if (std::binary_search(rows.begin(), rows.end(), cs.mainRef)) {
                    cs.mainRef = -1;
                } else {
                    cs.mainRef -= static_cast<int>(std::lower_bound(rows.begin(), rows.end(), cs.mainRef) - rows.begin());
                }
            }
            SaveSharedChildTradeSequence();
        }

        for (auto it = rows.rbegin(); it != rows.rend(); ++it) seq->erase(seq->begin() + *it);
        if (tradeEditorMode_ == 1) {
            for (std::size_t i = 0; i < mainTradeSequence_.size(); ++i) mainTradeSequence_[i].mainRef = static_cast<int>(i);
        } else if (tradeEditorMode_ == 2) {
            NormalizeTradeGroups(*seq);
        }
        SaveEditorSequence();
        RefreshTradeSequenceList();
    }

    void MoveTradeSequenceRow(int delta) {
        std::vector<TradeSequenceStep>* seq = EditorSequence();
        const int row = SelectedTradeSequenceIndex();
        if (!seq || row < 0) return;
        const int next = row + delta;
        if (next < 0 || next >= static_cast<int>(seq->size())) return;
        if (tradeEditorMode_ == 1) {
            // Preserve shared ACC CON references by swapping ref IDs with MAIN content.
            std::swap((*seq)[static_cast<std::size_t>(row)], (*seq)[static_cast<std::size_t>(next)]);
            EnsureSharedChildTradeSequence();
            for (TradeSequenceStep& cs : childTradeSequence_) if (cs.target == 1) {
                if (cs.mainRef == row) cs.mainRef = next; else if (cs.mainRef == next) cs.mainRef = row;
            }
            for (std::size_t i = 0; i < seq->size(); ++i) (*seq)[i].mainRef = static_cast<int>(i);
            SaveSharedChildTradeSequence();
        } else {
            std::swap((*seq)[static_cast<std::size_t>(row)], (*seq)[static_cast<std::size_t>(next)]);
            NormalizeTradeGroups(*seq);
        }
        SaveEditorSequence(); RefreshTradeSequenceList();
        ListView_SetItemState(tradeSeqList_, next, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        LoadTradeSequenceRowToEditor(next);
    }

    void SaveTradeSequenceRowFromEditor(bool refreshUi = true, int forcedRow = -1) {
        std::vector<TradeSequenceStep>* seq = EditorSequence();
        const int row = forcedRow >= 0 ? forcedRow : SelectedTradeSequenceIndex();
        if (!seq || row < 0 || row >= static_cast<int>(seq->size())) return;
        TradeSequenceStep& step = (*seq)[static_cast<std::size_t>(row)];
        if (tradeEditorMode_ == 1) {
            step.target = 1; step.mainRef = row;
            step.description = GetText(tradeSeqDesc_);
            step.delayMs = std::clamp(_wtoi(GetText(tradeSeqDelay_).c_str()), 50, 60000);
            step.repeat = std::clamp(_wtoi(GetText(tradeSeqRepeat_).c_str()), 1, 999);
        } else {
            const LRESULT targetSel = SendMessageW(tradeSeqTarget_, CB_GETCURSEL, 0, 0);
            if (targetSel > 0) {
                step.target = 1; step.mainRef = static_cast<int>(targetSel - 1);
            } else {
                step.target = 0; step.mainRef = -1;
                step.description = GetText(tradeSeqDesc_);
                step.delayMs = std::clamp(_wtoi(GetText(tradeSeqDelay_).c_str()), 50, 60000);
                step.repeat = std::clamp(_wtoi(GetText(tradeSeqRepeat_).c_str()), 1, 999);
            }
        }
        SaveEditorSequence();
        if (!refreshUi) return;
        RefreshTradeSequenceList();
        ListView_SetItemState(tradeSeqList_, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        LoadTradeSequenceRowToEditor(row);
    }

    Account* TradeSequenceCaptureAccount(TradeSequenceStep& step, ClickPoint*& pointOut) {
        pointOut = nullptr;
        if (tradeEditorMode_ == 1) {
            Account* main = AccountByTradeRole(1); pointOut = &step.point; return main;
        }
        // A child-sequence row may reference MAIN. Do not require the donor CON to still
        // exist just to capture a MAIN coordinate. Resolve the actual target first.
        if (step.target == 1) {
            if (step.mainRef < 0 || step.mainRef >= static_cast<int>(mainTradeSequence_.size())) return nullptr;
            pointOut = &mainTradeSequence_[static_cast<std::size_t>(step.mainRef)].point;
            return AccountByTradeRole(1);
        }
        Account* child = TradeEditorChild();
        if (!child) return nullptr;
        pointOut = &step.point; return child;
    }

    void BeginTradeSequenceCapture() {
        // Freeze the exact row/target before arming F8. Saving the editor used to rebuild
        // the ListView first, so drag/multi-selection could change the row used by capture.
        const int row = SelectedTradeSequenceIndex();
        std::vector<TradeSequenceStep>* seq = EditorSequence();
        if (!seq || row < 0 || row >= static_cast<int>(seq->size())) {
            Log(L"BĐPT: hãy chọn đúng một dòng đang focus để lấy tọa độ chuỗi GD.");
            return;
        }
        SaveTradeSequenceRowFromEditor(false, row);
        seq = EditorSequence();
        if (!seq || row >= static_cast<int>(seq->size())) return;

        TradeSequenceStep& step = (*seq)[static_cast<std::size_t>(row)];
        ClickPoint* point = nullptr;
        Account* target = TradeSequenceCaptureAccount(step, point);
        if (!target || !point) { Log(L"BĐPT: không xác định được cửa sổ để lấy tọa độ chuỗi GD."); return; }

        captureSlot_ = ClickSlot::None;
        captureMacroIndex_ = -1;
        capturePkStepIndex_ = -1; capturePkClickIndex_ = -1;
        captureTradeSequenceIndex_ = row;
        captureTradeSequenceMode_ = tradeEditorMode_;
        captureTradeSequenceMainRef_ = (tradeEditorMode_ == 2 && step.target == 1) ? step.mainRef : -1;
        capturePid_ = target->game.pid;
        LogAccount(*target, L"BĐPT yêu cầu lấy tọa chuỗi GD dòng " + std::to_wstring(row + 1) + L" → đưa chuột vào đúng vị trí và F8.");
    }

    void TestTradeSequenceRow() {
        SaveTradeSequenceRowFromEditor();
        std::vector<TradeSequenceStep>* seq = EditorSequence();
        const int row = SelectedTradeSequenceIndex();
        if (!seq || row < 0 || row >= static_cast<int>(seq->size())) return;
        TradeSequenceStep& stored = (*seq)[static_cast<std::size_t>(row)];
        const TradeSequenceStep* effective = EffectiveEditorStep(stored);
        Account* target = nullptr;
        if (tradeEditorMode_ == 1 || stored.target == 1) target = AccountByTradeRole(1); else target = TradeEditorChild();
        if (!target || !effective) { Log(L"BĐPT: TEST dòng không xác định được acc/MAIN reference."); return; }
        std::wstring error;
        if (!CoordinatorInternalPointAction(
                *target, effective->point,
                L"TEST CHUỖI GD dòng " + std::to_wstring(row + 1), error)) {
            LogAccount(*target, L"TEST chuỗi GD FAIL: " + error); return;
        }
        LogAccount(*target, L"TEST chuỗi GD dòng " + std::to_wstring(row + 1) +
                           L" PASS qua HIDDEN ACTION BĐPT.");
    }

    void BuildTradeEditorUi(HWND parent) {
        auto addColumn = [&](int index, int width, const wchar_t* text) {
            LVCOLUMNW c{}; c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM; c.pszText = const_cast<wchar_t*>(text); c.cx = width; c.iSubItem = index;
            ListView_InsertColumn(tradeSeqList_, index, &c);
        };
        const wchar_t* heading = tradeEditorMode_ == 1
            ? L"CHUỖI GD MAIN — tọa MAIN dùng chung cho mọi giao dịch"
            : L"CHUỖI GD ACC CON (DÙNG CHUNG) — mọi CON1..CON12 dùng đúng một workflow này";
        MakeIn(parent, L"STATIC", heading, 0, 15, 10, 850, 23, 0);
        tradeSeqList_ = MakeIn(parent, WC_LISTVIEWW, L"", LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER, 15, 38, 850, 235, IDC_SEQ_LIST);
        ListView_SetExtendedListViewStyle(tradeSeqList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        SetWindowSubclass(tradeSeqList_, TradeSequenceListSubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
        addColumn(0, 35, L"#"); addColumn(1, 120, L"ACC THỰC HIỆN");
        addColumn(2, 205, L"Mô tả"); addColumn(3, 180, L"Tọa độ"); addColumn(4, 60, L"Delay"); addColumn(5, 55, L"Lặp"); addColumn(6, 110, L"Nhóm lặp");
        MakeIn(parent, L"STATIC", L"ACC:", 0, 15, 287, 38, 22, 0);
        tradeSeqTarget_ = MakeIn(parent, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL, 55, 282, 250, 220, IDC_SEQ_TARGET);
        MakeIn(parent, L"STATIC", L"Mô tả:", 0, 315, 287, 50, 22, 0);
        tradeSeqDesc_ = MakeIn(parent, L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, 367, 282, 220, 27, IDC_SEQ_DESC);
        MakeIn(parent, L"STATIC", L"Delay:", 0, 597, 287, 42, 22, 0);
        tradeSeqDelay_ = MakeIn(parent, L"EDIT", L"500", WS_BORDER | ES_NUMBER | ES_CENTER, 641, 282, 65, 27, IDC_SEQ_DELAY);
        MakeIn(parent, L"STATIC", L"Lặp:", 0, 716, 287, 32, 22, 0);
        tradeSeqRepeat_ = MakeIn(parent, L"EDIT", L"1", WS_BORDER | ES_NUMBER | ES_CENTER, 750, 282, 55, 27, IDC_SEQ_REPEAT);
        MakeIn(parent, L"BUTTON", L"+ THÊM", BS_PUSHBUTTON, 15, 323, 90, 30, IDC_SEQ_ADD);
        MakeIn(parent, L"BUTTON", L"- XÓA", BS_PUSHBUTTON, 112, 323, 80, 30, IDC_SEQ_DELETE);
        MakeIn(parent, L"BUTTON", L"LÊN", BS_PUSHBUTTON, 199, 323, 70, 30, IDC_SEQ_UP);
        MakeIn(parent, L"BUTTON", L"XUỐNG", BS_PUSHBUTTON, 276, 323, 75, 30, IDC_SEQ_DOWN);
        MakeIn(parent, L"BUTTON", L"LƯU DÒNG", BS_PUSHBUTTON, 360, 323, 105, 30, IDC_SEQ_SAVE);
        MakeIn(parent, L"BUTTON", L"LẤY TỌA (F8)", BS_PUSHBUTTON, 474, 323, 125, 30, IDC_SEQ_CAPTURE);
        MakeIn(parent, L"BUTTON", L"TEST DÒNG", BS_PUSHBUTTON, 608, 323, 105, 30, IDC_SEQ_TEST);
        MakeIn(parent, L"BUTTON", L"ĐÓNG", BS_PUSHBUTTON, 775, 323, 90, 30, IDC_SEQ_CLOSE);
        tradeRecordButton_ = MakeIn(parent, L"BUTTON", L"REC", BS_PUSHBUTTON, 15, 360, 92, 30, IDC_SEQ_REC);
        MakeIn(parent, L"BUTTON", L"SAO CHÉP", BS_PUSHBUTTON, 115, 360, 105, 30, IDC_SEQ_COPY);
        MakeIn(parent, L"BUTTON", L"DÁN", BS_PUSHBUTTON, 228, 360, 80, 30, IDC_SEQ_PASTE);
        if (tradeEditorMode_ == 2) {
            MakeIn(parent, L"STATIC", L"Lặp nhóm:", 0, 320, 365, 68, 22, 0);
            tradeSeqGroupRepeat_ = MakeIn(parent, L"EDIT", L"2", WS_BORDER | ES_NUMBER | ES_CENTER, 390, 360, 45, 30, IDC_SEQ_GROUP_REPEAT);
            MakeIn(parent, L"BUTTON", L"GOM DÒNG ĐÃ CHỌN", BS_PUSHBUTTON, 443, 360, 170, 30, IDC_SEQ_GROUP_SELECTED);
            MakeIn(parent, L"BUTTON", L"BỎ NHÓM", BS_PUSHBUTTON, 621, 360, 110, 30, IDC_SEQ_UNGROUP);
        }
        tradeRecordStatus_ = MakeIn(parent, L"STATIC", L"REC: sẵn sàng • chọn nhiều dòng liên tiếp để GOM và lặp mini-sequence", SS_LEFT | SS_CENTERIMAGE, 15, 398, 850, 30, 0);
        MakeIn(parent, L"STATIC", tradeEditorMode_ == 1
            ? L"MAIN sequence là thư viện tọa dùng chung. ACC CON workflow tham chiếu MAIN #n; sửa MAIN một lần áp dụng mọi giao dịch."
            : L"GOM 1/2/3/... dòng liên tiếp thành một nhóm; nhóm chạy đủ số lần rồi chuỗi lớn mới đi tiếp.", 0, 15, 435, 850, 23, 0);
        PopulateTradeTargetCombo(); RefreshTradeSequenceList();
    }

    void OpenTradeSequenceEditor(int mode) {
        Account* selected = SelectedAccount();
        if (mode == 1 && (!selected || selected->profile.tradeRole != 1)) { Log(L"Chỉ acc MAIN mới mở CHUỖI GD MAIN."); return; }
        if (mode == 2 && (!selected || selected->profile.tradeRole < 2)) { Log(L"Chọn một CON bất kỳ để mở CHUỖI GD ACC CON dùng chung."); return; }
        if (tradeEditor_ && IsWindow(tradeEditor_)) DestroyWindow(tradeEditor_);
        tradeEditorMode_ = mode;
        tradeEditorChildPid_ = mode == 2 ? selected->game.pid : 0;
        if (mode == 2) EnsureSharedChildTradeSequence();
        WNDCLASSEXW wc{}; wc.cbSize = sizeof(wc); wc.lpfnWndProc = TradeEditorWndProc; wc.hInstance = instance_; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); wc.lpszClassName = L"ThanLongTradeSequenceEditorV03";
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) { Log(L"Không đăng ký được cửa sổ chuỗi GD."); return; }
        const wchar_t* title = mode == 1 ? L"Thần Long • CHUỖI GD MAIN (DÙNG CHUNG) v0.3 • REC"
                                         : L"Thần Long • CHUỖI GD ACC CON (DÙNG CHUNG) v0.3 • REC + NHÓM LẶP";
        tradeEditor_ = CreateWindowExW(WS_EX_TOOLWINDOW, wc.lpszClassName, title, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                       CW_USEDEFAULT, CW_USEDEFAULT, 900, 530, hwnd_, nullptr, instance_, this);
        if (!tradeEditor_) { Log(L"Không mở được cửa sổ chuỗi GD."); return; }
        BuildTradeEditorUi(tradeEditor_); ShowWindow(tradeEditor_, SW_SHOW); UpdateWindow(tradeEditor_);
    }

    LRESULT HandleTradeEditor(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case WM_NOTIFY: {
                auto* hdr = reinterpret_cast<NMHDR*>(lp);
                if (hdr && hdr->idFrom == IDC_SEQ_LIST && hdr->code == LVN_ITEMCHANGED) {
                    const auto* n = reinterpret_cast<const NMLISTVIEW*>(hdr);
                    if (!tradeSeqDragUpdating_ && (n->uChanged & LVIF_STATE) != 0 && (n->uNewState & LVIS_SELECTED) != 0)
                        LoadTradeSequenceRowToEditor(n->iItem);
                }
                return 0;
            }
            case WM_COMMAND:
                switch (LOWORD(wp)) {
                    case IDC_SEQ_ADD: AddTradeSequenceRow(); return 0;
                    case IDC_SEQ_DELETE: DeleteTradeSequenceRow(); return 0;
                    case IDC_SEQ_UP: MoveTradeSequenceRow(-1); return 0;
                    case IDC_SEQ_DOWN: MoveTradeSequenceRow(1); return 0;
                    case IDC_SEQ_SAVE: SaveTradeSequenceRowFromEditor(); return 0;
                    case IDC_SEQ_CAPTURE: BeginTradeSequenceCapture(); return 0;
                    case IDC_SEQ_TEST: TestTradeSequenceRow(); return 0;
                    case IDC_SEQ_REC: ToggleTradeRecorder(); return 0;
                    case IDC_SEQ_COPY: CopySelectedTradeRows(); return 0;
                    case IDC_SEQ_PASTE: PasteTradeRows(); return 0;
                    case IDC_SEQ_GROUP_SELECTED: GroupSelectedTradeRows(); return 0;
                    case IDC_SEQ_UNGROUP: UngroupSelectedTradeRows(); return 0;
                    case IDC_SEQ_CLOSE: if (RecorderModeIsTrade(recorderMode_)) StopRecorder(true); DestroyWindow(hwnd); return 0;
                    case IDC_SEQ_TARGET: if (HIWORD(wp) == CBN_SELCHANGE) SaveTradeSequenceRowFromEditor(); return 0;
                }
                break;
            case WM_CLOSE: if (RecorderModeIsTrade(recorderMode_)) StopRecorder(true); DestroyWindow(hwnd); return 0;
            case WM_NCDESTROY:
                tradeEditor_ = nullptr; tradeSeqList_ = nullptr; tradeSeqTarget_ = nullptr; tradeSeqDesc_ = nullptr; tradeSeqDelay_ = nullptr; tradeSeqRepeat_ = nullptr; tradeSeqGroupRepeat_ = nullptr; tradeRecordButton_ = nullptr; tradeRecordStatus_ = nullptr;
                captureTradeSequenceIndex_ = -1; captureTradeSequenceMode_ = 0; captureTradeSequenceMainRef_ = -1; tradeEditorMode_ = 0; tradeEditorChildPid_ = 0;
                return DefWindowProcW(hwnd, msg, wp, lp);
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }


    static const wchar_t* FilterActionText(RuleAction action) {
        switch (action) {
            case RuleAction::Drop: return L"VỨT/HỦY";
            case RuleAction::Sell: return L"BÁN";
            default: return L"GIỮ";
        }
    }

    static void SplitInstanceID(std::int64_t id, std::int32_t& low, std::int32_t& high) {
        const std::uint64_t u = static_cast<std::uint64_t>(id);
        low = static_cast<std::int32_t>(static_cast<std::uint32_t>(u & 0xFFFFFFFFULL));
        high = static_cast<std::int32_t>(static_cast<std::uint32_t>((u >> 32) & 0xFFFFFFFFULL));
    }

    bool ScanBagSemantic(Account& a, std::vector<InventoryBagRow>& rows, std::wstring& error, int* freeSpaceOut = nullptr) {
        rows.clear();
        if (!EnsureAttach(a, error)) return false;
        int start = 0; int expectedTotal = -1; int lastFree = -1;
        for (int page = 0; page < 8; ++page) {
            Response r{};
            if (!a.bridge.Call(Command::ReadBagPage, start, 0, 0, r, error, 2200)) return false;
            const BagPageSnapshot& bag = r.bagPage;
            if (bag.totalCount < 0 || bag.totalCount > 1000 || bag.pageCount < 0 ||
                bag.pageCount > static_cast<int>(kBagPageCapacity) || bag.pageStart != start) {
                error = L"BagPage contract không hợp lệ"; return false;
            }
            if (expectedTotal < 0) expectedTotal = bag.totalCount;
            if (bag.totalCount != expectedTotal) {
                // Bag changed while paging: restart once from fresh state instead of mixing versions.
                if (page == 0) { error = L"Bag thay đổi liên tục khi scan"; return false; }
                rows.clear(); start = 0; expectedTotal = -1; page = -1; continue;
            }
            lastFree = bag.freeBagSpace;
            for (int i = 0; i < bag.pageCount; ++i) {
                const BagItemSnapshot& src = bag.items[i];
                InventoryBagRow row{};
                row.item.instanceID = src.instanceID; row.item.itemID = src.itemID; row.item.site = src.site;
                row.item.position = src.position; row.item.quantity = src.quantity; row.item.bound = src.bound != 0;
                row.item.throwable = src.throwable != 0; row.item.sellable = src.sellable != 0;
                row.item.isEquip = src.isEquip != 0; row.item.isWeapon = src.isWeapon != 0;
                row.item.itemType = src.itemType; row.name = src.name; row.equipType = src.equipType;
                rows.push_back(std::move(row));
            }
            start += bag.pageCount;
            if (start >= expectedTotal) break;
            if (bag.pageCount == 0) { error = L"BagPage dừng sớm"; return false; }
        }
        if (expectedTotal >= 0 && static_cast<int>(rows.size()) != expectedTotal) {
            error = L"Bag scan chưa đủ item"; return false;
        }
        if (freeSpaceOut) *freeSpaceOut = lastFree;
        return true;
    }

    bool TryAutoInventoryDrop(Account& a, DWORD now) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        if (!inventoryFilter_.enabled || (s.validMask & ValidBagSpace) == 0 || s.freeBagSpace > 0) {
            rt.bagFilterPendingInstance = 0;
            rt.bagFilterPendingTick = 0;
            return false;
        }

        // After one mutation, wait briefly for authoritative bag state before choosing another instance.
        if (rt.bagFilterPendingInstance != 0 && !Elapsed(now, rt.bagFilterPendingTick, 750)) {
            rt.status = L"LỌC ĐỒ • chờ tay nải cập nhật sau VỨT/HỦY";
            return true;
        }

        std::vector<InventoryBagRow> rows;
        std::wstring error;
        int freshFree = -1;
        if (!ScanBagSemantic(a, rows, error, &freshFree)) {
            rt.status = L"LỌC ĐỒ scan FAIL • giữ nguyên tay nải, không vứt/bán mù";
            if (BridgeLooksUnresponsive(error)) EnterClientFreeze(a, L"Bridge timeout lúc scan Lọc đồ tay nải", now);
            return true;
        }
        if (freshFree > 0) {
            rt.bagFilterPendingInstance = 0;
            rt.bagFilterPendingTick = 0;
            return false;
        }

        std::vector<inventory_filter_logic::ItemView> items;
        items.reserve(rows.size());
        for (const auto& row : rows) items.push_back(row.item);

        // If the last requested instance is still present, give the server a bounded window to remove/update it.
        if (rt.bagFilterPendingInstance != 0) {
            const bool stillThere = std::any_of(items.begin(), items.end(), [&](const auto& item){
                return item.instanceID == rt.bagFilterPendingInstance;
            });
            if (stillThere && !Elapsed(now, rt.bagFilterPendingTick, 2500)) {
                rt.status = L"LỌC ĐỒ • server chưa xác nhận item vừa VỨT/HỦY";
                return true;
            }
            rt.bagFilterPendingInstance = 0;
            rt.bagFilterPendingTick = 0;
        }

        const int candidate = inventory_filter_logic::FindCandidate(inventoryFilter_, items, RuleAction::Drop);
        if (candidate < 0) return false; // no more discardable matches: AUTO may now trade/sell.

        const auto& item = items[static_cast<std::size_t>(candidate)];
        std::int32_t low = 0, high = 0; SplitInstanceID(item.instanceID, low, high);
        Response response{};
        if (!a.bridge.Call(Command::DropBagItem, low, high, item.itemID, response, error, 2400)) {
            rt.status = L"LỌC ĐỒ VỨT FAIL • fail-closed";
            if (BridgeLooksUnresponsive(error)) EnterClientFreeze(a, L"Bridge timeout lúc vứt item tay nải", now);
            LogAccount(a, L"LỌC ĐỒ: không vứt được ItemID " + std::to_wstring(item.itemID) + L" • " + error);
            return true;
        }
        rt.bagFilterPendingInstance = item.instanceID;
        rt.bagFilterPendingTick = now;
        std::wstring name = L"ItemID " + std::to_wstring(item.itemID);
        for (const auto& row : rows) if (row.item.instanceID == item.instanceID && !row.name.empty()) { name = row.name; break; }
        rt.status = L"LỌC ĐỒ • đã gửi VỨT/HỦY: " + name + L" • chờ re-scan";
        LogAccount(a, L"LỌC ĐỒ VỨT/HỦY • " + name + L" • ItemID " + std::to_wstring(item.itemID) +
                      L" • instance " + std::to_wstring(item.instanceID));
        return true;
    }

    bool SemanticSellRulesActive() const {
        return inventoryFilter_.enabled && inventory_filter_logic::HasSellRules(inventoryFilter_);
    }

    void SetInventoryFilterStatus(const std::wstring& text) {
        if (inventoryFilterStatus_) SetText(inventoryFilterStatus_, text);
    }

    void LoadInventoryFilterChecksToUi() {
        auto set = [&](HWND h, bool v){ if (h) SendMessageW(h, BM_SETCHECK, v ? BST_CHECKED : BST_UNCHECKED, 0); };
        set(inventoryFilterEnabled_, inventoryFilter_.enabled);
        set(inventoryFilterProtectBound_, inventoryFilter_.protectBound);
        set(inventoryFilterKeepWeapon_, inventoryFilter_.keepWeapons);
        set(inventoryFilterDropEquip_, inventoryFilter_.dropEquipNonWeapon);
        set(inventoryFilterSellEquip_, inventoryFilter_.sellEquipNonWeapon);
        set(inventoryFilterDropCommon_, inventoryFilter_.dropCommon);
        set(inventoryFilterSellCommon_, inventoryFilter_.sellCommon);
        set(inventoryFilterDropGem_, inventoryFilter_.dropGem);
        set(inventoryFilterSellGem_, inventoryFilter_.sellGem);
        set(inventoryFilterDropMedicine_, inventoryFilter_.dropMedicine);
        set(inventoryFilterSellMedicine_, inventoryFilter_.sellMedicine);
        set(inventoryFilterDropPetEquip_, inventoryFilter_.dropPetEquip);
        set(inventoryFilterSellPetEquip_, inventoryFilter_.sellPetEquip);
    }

    void PersistInventoryFilterChecksFromUi() {
        auto get = [&](HWND h, bool fallback){ return h ? SendMessageW(h, BM_GETCHECK, 0, 0) == BST_CHECKED : fallback; };
        inventoryFilter_.enabled = get(inventoryFilterEnabled_, inventoryFilter_.enabled);
        inventoryFilter_.protectBound = get(inventoryFilterProtectBound_, inventoryFilter_.protectBound);
        inventoryFilter_.keepWeapons = get(inventoryFilterKeepWeapon_, inventoryFilter_.keepWeapons);
        inventoryFilter_.dropEquipNonWeapon = get(inventoryFilterDropEquip_, inventoryFilter_.dropEquipNonWeapon);
        inventoryFilter_.sellEquipNonWeapon = get(inventoryFilterSellEquip_, inventoryFilter_.sellEquipNonWeapon);
        inventoryFilter_.dropCommon = get(inventoryFilterDropCommon_, inventoryFilter_.dropCommon);
        inventoryFilter_.sellCommon = get(inventoryFilterSellCommon_, inventoryFilter_.sellCommon);
        inventoryFilter_.dropGem = get(inventoryFilterDropGem_, inventoryFilter_.dropGem);
        inventoryFilter_.sellGem = get(inventoryFilterSellGem_, inventoryFilter_.sellGem);
        inventoryFilter_.dropMedicine = get(inventoryFilterDropMedicine_, inventoryFilter_.dropMedicine);
        inventoryFilter_.sellMedicine = get(inventoryFilterSellMedicine_, inventoryFilter_.sellMedicine);
        inventoryFilter_.dropPetEquip = get(inventoryFilterDropPetEquip_, inventoryFilter_.dropPetEquip);
        inventoryFilter_.sellPetEquip = get(inventoryFilterSellPetEquip_, inventoryFilter_.sellPetEquip);
        SaveInventoryFilterSettings(inventoryFilter_);
    }

    void RefreshInventoryRuleList() {
        if (!inventoryFilterRuleList_) return;
        ListView_DeleteAllItems(inventoryFilterRuleList_);
        for (std::size_t i = 0; i < inventoryFilter_.rules.size(); ++i) {
            const auto& r = inventoryFilter_.rules[i];
            std::wstring no = std::to_wstring(i + 1);
            LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = static_cast<int>(i); item.pszText = no.data();
            ListView_InsertItem(inventoryFilterRuleList_, &item);
            std::wstring id = std::to_wstring(r.itemID);
            ListView_SetItemText(inventoryFilterRuleList_, static_cast<int>(i), 1, const_cast<wchar_t*>(FilterActionText(r.action)));
            ListView_SetItemText(inventoryFilterRuleList_, static_cast<int>(i), 2, const_cast<wchar_t*>(r.name.c_str()));
            ListView_SetItemText(inventoryFilterRuleList_, static_cast<int>(i), 3, id.data());
        }
    }

    void RefreshInventoryBagList() {
        if (!inventoryFilterBagList_) return;
        Account* a = AccountByPid(inventoryFilterPid_);
        if (!a) { SetInventoryFilterStatus(L"Acc đã mất khỏi danh sách • đóng/mở lại cửa sổ lọc"); return; }
        std::wstring error; int freeSpace = -1;
        std::vector<InventoryBagRow> fresh;
        if (!ScanBagSemantic(*a, fresh, error, &freeSpace)) {
            SetInventoryFilterStatus(L"SCAN FAIL • " + error); return;
        }
        inventoryFilterBagRows_ = std::move(fresh);
        ListView_DeleteAllItems(inventoryFilterBagList_);
        for (std::size_t i = 0; i < inventoryFilterBagRows_.size(); ++i) {
            const InventoryBagRow& row = inventoryFilterBagRows_[i];
            std::wstring no = std::to_wstring(i + 1), id = std::to_wstring(row.item.itemID), instance = std::to_wstring(row.item.instanceID), slot = std::to_wstring(row.item.position);
            std::wstring qty = std::to_wstring(row.item.quantity);
            LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = static_cast<int>(i); item.pszText = no.data();
            ListView_InsertItem(inventoryFilterBagList_, &item);
            ListView_SetItemText(inventoryFilterBagList_, static_cast<int>(i), 1, const_cast<wchar_t*>(row.name.c_str()));
            ListView_SetItemText(inventoryFilterBagList_, static_cast<int>(i), 2, id.data());
            ListView_SetItemText(inventoryFilterBagList_, static_cast<int>(i), 3, instance.data());
            ListView_SetItemText(inventoryFilterBagList_, static_cast<int>(i), 4, const_cast<wchar_t*>(row.item.itemType.c_str()));
            ListView_SetItemText(inventoryFilterBagList_, static_cast<int>(i), 5, const_cast<wchar_t*>(row.equipType.c_str()));
            ListView_SetItemText(inventoryFilterBagList_, static_cast<int>(i), 6, slot.data());
            ListView_SetItemText(inventoryFilterBagList_, static_cast<int>(i), 7, qty.data());
            ListView_SetItemText(inventoryFilterBagList_, static_cast<int>(i), 8, const_cast<wchar_t*>(row.item.bound ? L"Khóa" : L"-"));
            ListView_SetItemText(inventoryFilterBagList_, static_cast<int>(i), 9, const_cast<wchar_t*>(row.item.throwable ? L"Có" : L"Không"));
            ListView_SetItemText(inventoryFilterBagList_, static_cast<int>(i), 10, const_cast<wchar_t*>(row.item.sellable ? L"Có" : L"Không"));
        }
        SetInventoryFilterStatus(L"SCAN OK • " + std::to_wstring(inventoryFilterBagRows_.size()) + L" item • ô trống=" + std::to_wstring(freeSpace) +
                                 L" • đọc semantic, không mở túi/OCR");
    }

    void RefreshShortcutSellerUi() {
        if (!shortcutSellerCombo_) return;
        LRESULT sel = SendMessageW(shortcutSellerCombo_, CB_GETCURSEL, 0, 0);
        if (sel == CB_ERR || sel < 0 || sel >= static_cast<LRESULT>(kSellNpcs.size())) {
            sel = 0; SendMessageW(shortcutSellerCombo_, CB_SETCURSEL, 0, 0);
        }
        const std::size_t i = static_cast<std::size_t>(sel);
        const auto& pos = sellNpcPositions_[i];
        if (shortcutSellerCoordLabel_) {
            SetText(shortcutSellerCoordLabel_, pos.valid
                ? (L"ĐÃ GÁN: " + std::to_wstring(pos.x) + L"," + std::to_wstring(pos.y))
                : L"CHƯA GÁN • đứng sát NPC rồi bấm LẤY TỌA NPC BÁN");
        }
    }

    void LoadShortcutSettingsToUi() {
        if (!shortcutWindow_) return;
        if (shortcutTheme_) SendMessageW(shortcutTheme_, CB_SETCURSEL, shortcutSettings_.theme, 0);
        const int values[28] = {
            shortcutSettings_.kunlunNpcX, shortcutSettings_.kunlunNpcY,
            shortcutSettings_.xaTruyenX, shortcutSettings_.xaTruyenY,
            shortcutSettings_.ngaiX, shortcutSettings_.ngaiY,
            shortcutSettings_.tinhTucX, shortcutSettings_.tinhTucY,
            shortcutSettings_.thanhLienGateX, shortcutSettings_.thanhLienGateY,
            shortcutSettings_.phamLienGateX, shortcutSettings_.phamLienGateY,
            shortcutSettings_.khoVinhGateX, shortcutSettings_.khoVinhGateY,
            shortcutSettings_.thdcEntryX, shortcutSettings_.thdcEntryY,
            shortcutSettings_.thdcFloor1UpX, shortcutSettings_.thdcFloor1UpY,
            shortcutSettings_.thdcFloor2UpX, shortcutSettings_.thdcFloor2UpY,
            shortcutSettings_.thdcFloor2DownX, shortcutSettings_.thdcFloor2DownY,
            shortcutSettings_.thdcFloor3UpX, shortcutSettings_.thdcFloor3UpY,
            shortcutSettings_.thdcFloor3DownX, shortcutSettings_.thdcFloor3DownY,
            shortcutSettings_.thdcFloor4DownX, shortcutSettings_.thdcFloor4DownY,
        };
        for (std::size_t i = 0; i < shortcutCoordEdits_.size(); ++i) {
            if (shortcutCoordEdits_[i]) SetWindowTextW(shortcutCoordEdits_[i], std::to_wstring(values[i]).c_str());
        }
        for (std::size_t i = 0; i < shortcutSettings_.kunlunExitClicks.size(); ++i) {
            const TimedClickPoint& click = shortcutSettings_.kunlunExitClicks[i];
            if (shortcutClickLabels_[i]) {
                SetText(shortcutClickLabels_[i], click.point.valid
                    ? PointDescription(click.point)
                    : L"CHƯA GÁN • đưa chuột vào game rồi F8");
            }
            if (shortcutClickTimeEdits_[i]) SetText(shortcutClickTimeEdits_[i], std::to_wstring(click.timeMs));
            if (shortcutClickDelayEdits_[i]) SetText(shortcutClickDelayEdits_[i], std::to_wstring(click.delayMs));
        }
        RefreshShortcutSellerUi();
    }

    void PersistShortcutSettingsFromUi(bool logSaved = true) {
        if (shortcutTheme_) {
            const LRESULT sel = SendMessageW(shortcutTheme_, CB_GETCURSEL, 0, 0);
            if (sel != CB_ERR) shortcutSettings_.theme = std::clamp(static_cast<int>(sel), 0, 1);
        }
        if (shortcutCoordEdits_[0]) {
            auto read = [&](std::size_t i, int fallback){ return shortcutCoordEdits_[i] ? ParseEditInt(shortcutCoordEdits_[i], fallback, 0, 1000000) : fallback; };
            shortcutSettings_.kunlunNpcX = read(0, shortcutSettings_.kunlunNpcX); shortcutSettings_.kunlunNpcY = read(1, shortcutSettings_.kunlunNpcY);
            shortcutSettings_.xaTruyenX = read(2, shortcutSettings_.xaTruyenX); shortcutSettings_.xaTruyenY = read(3, shortcutSettings_.xaTruyenY);
            shortcutSettings_.ngaiX = read(4, shortcutSettings_.ngaiX); shortcutSettings_.ngaiY = read(5, shortcutSettings_.ngaiY);
            shortcutSettings_.tinhTucX = read(6, shortcutSettings_.tinhTucX); shortcutSettings_.tinhTucY = read(7, shortcutSettings_.tinhTucY);
            shortcutSettings_.thanhLienGateX = read(8, shortcutSettings_.thanhLienGateX); shortcutSettings_.thanhLienGateY = read(9, shortcutSettings_.thanhLienGateY);
            shortcutSettings_.phamLienGateX = read(10, shortcutSettings_.phamLienGateX); shortcutSettings_.phamLienGateY = read(11, shortcutSettings_.phamLienGateY);
            shortcutSettings_.khoVinhGateX = read(12, shortcutSettings_.khoVinhGateX); shortcutSettings_.khoVinhGateY = read(13, shortcutSettings_.khoVinhGateY);
            shortcutSettings_.thdcEntryX = read(14, shortcutSettings_.thdcEntryX); shortcutSettings_.thdcEntryY = read(15, shortcutSettings_.thdcEntryY);
            shortcutSettings_.thdcFloor1UpX = read(16, shortcutSettings_.thdcFloor1UpX); shortcutSettings_.thdcFloor1UpY = read(17, shortcutSettings_.thdcFloor1UpY);
            shortcutSettings_.thdcFloor2UpX = read(18, shortcutSettings_.thdcFloor2UpX); shortcutSettings_.thdcFloor2UpY = read(19, shortcutSettings_.thdcFloor2UpY);
            shortcutSettings_.thdcFloor2DownX = read(20, shortcutSettings_.thdcFloor2DownX); shortcutSettings_.thdcFloor2DownY = read(21, shortcutSettings_.thdcFloor2DownY);
            shortcutSettings_.thdcFloor3UpX = read(22, shortcutSettings_.thdcFloor3UpX); shortcutSettings_.thdcFloor3UpY = read(23, shortcutSettings_.thdcFloor3UpY);
            shortcutSettings_.thdcFloor3DownX = read(24, shortcutSettings_.thdcFloor3DownX); shortcutSettings_.thdcFloor3DownY = read(25, shortcutSettings_.thdcFloor3DownY);
            shortcutSettings_.thdcFloor4DownX = read(26, shortcutSettings_.thdcFloor4DownX); shortcutSettings_.thdcFloor4DownY = read(27, shortcutSettings_.thdcFloor4DownY);
        }
        for (std::size_t i = 0; i < shortcutSettings_.kunlunExitClicks.size(); ++i) {
            TimedClickPoint& click = shortcutSettings_.kunlunExitClicks[i];
            if (shortcutClickTimeEdits_[i])
                click.timeMs = ParseEditInt(shortcutClickTimeEdits_[i], click.timeMs, 0, 60000);
            if (shortcutClickDelayEdits_[i])
                click.delayMs = ParseEditInt(shortcutClickDelayEdits_[i], click.delayMs, 0, 60000);
        }
        SaveShortcutSettings(shortcutSettings_);
        if (logSaved) Log(L"TÙY CHỈNH v3.3: đã lưu tọa + 3 click CLS Time/Delay + 7 cổng THĐC đúng map nguồn.");
    }

    void ApplyShortcutPanelTheme(HWND hwnd) {
        if (!hwnd) return;
        if (shortcutSettings_.theme == 1 && !shortcutDarkBrush_) shortcutDarkBrush_ = CreateSolidBrush(RGB(35, 35, 35));
        InvalidateRect(hwnd, nullptr, TRUE);
        UpdateWindow(hwnd);
    }

    void CaptureShortcutCoordinate(int index) {
        if (index < 0 || index >= 14) return;
        Account* a = SelectedAccount();
        if (!a) { Log(L"TÙY CHỈNH TỌA: chọn 1 acc đang đứng đúng điểm trước."); return; }
        std::wstring error;
        if (!ReadSnapshot(*a, error, 1200)) { LogAccount(*a, L"Không đọc được state để LẤY TỌA: " + error); return; }
        const Snapshot& snap = a->snapshot;
        if ((snap.validMask & (ValidMap | ValidPosition)) != (ValidMap | ValidPosition)) { LogAccount(*a, L"State chưa có Map/X/Y để LẤY TỌA"); return; }
        static constexpr int expectedMaps[14] = {
            75, 5, 5, 12, 10000, 10000, 10000,
            10000, 10014, 10015, 10015, 10016, 10016, 10017
        };
        static constexpr const wchar_t* labels[14] = {
            L"NPC RỜI Côn Lôn Sơn", L"Xa Truyền Bình • ResID 387 • ĐI VÀO Côn Lôn", L"Ngải Ni Ngoã Nhĩ • ResID 913",
            L"Tinh Túc Hải điểm ra", L"Cổng Thanh Liên Trại", L"Cổng Phàm Liên Trại", L"Cổng Khô Vinh Đạo",
            L"THĐC: M10000 → tầng 1", L"THĐC: tầng 1 M10014 → tầng 2", L"THĐC: tầng 2 M10015 → tầng 3",
            L"THĐC: tầng 2 M10015 → tầng 1", L"THĐC: tầng 3 M10016 → tầng 4",
            L"THĐC: tầng 3 M10016 → tầng 2", L"THĐC: tầng 4 M10017 → tầng 3"
        };
        if (snap.mapID != expectedMaps[index]) {
            LogAccount(*a, std::wstring(L"KHÔNG LƯU ") + labels[index] + L": đang M" + std::to_wstring(snap.mapID) +
                           L" nhưng điểm này phải gán ở M" + std::to_wstring(expectedMaps[index]));
            return;
        }
        PersistShortcutSettingsFromUi(false);
        int* xs[14] = {&shortcutSettings_.kunlunNpcX,&shortcutSettings_.xaTruyenX,&shortcutSettings_.ngaiX,&shortcutSettings_.tinhTucX,
                       &shortcutSettings_.thanhLienGateX,&shortcutSettings_.phamLienGateX,&shortcutSettings_.khoVinhGateX,
                       &shortcutSettings_.thdcEntryX,&shortcutSettings_.thdcFloor1UpX,&shortcutSettings_.thdcFloor2UpX,
                       &shortcutSettings_.thdcFloor2DownX,&shortcutSettings_.thdcFloor3UpX,&shortcutSettings_.thdcFloor3DownX,
                       &shortcutSettings_.thdcFloor4DownX};
        int* ys[14] = {&shortcutSettings_.kunlunNpcY,&shortcutSettings_.xaTruyenY,&shortcutSettings_.ngaiY,&shortcutSettings_.tinhTucY,
                       &shortcutSettings_.thanhLienGateY,&shortcutSettings_.phamLienGateY,&shortcutSettings_.khoVinhGateY,
                       &shortcutSettings_.thdcEntryY,&shortcutSettings_.thdcFloor1UpY,&shortcutSettings_.thdcFloor2UpY,
                       &shortcutSettings_.thdcFloor2DownY,&shortcutSettings_.thdcFloor3UpY,&shortcutSettings_.thdcFloor3DownY,
                       &shortcutSettings_.thdcFloor4DownY};
        *xs[index]=snap.x; *ys[index]=snap.y;
        SaveShortcutSettings(shortcutSettings_); LoadShortcutSettingsToUi();
        for (auto& item : accounts_) if (item) ResetShortcutRoute(item->runtime);
        LogAccount(*a, std::wstring(L"COORD CAPTURE RAW SHORTCUT • ") + labels[index] + L" • M" + std::to_wstring(snap.mapID) + L" • " +
                       std::to_wstring(snap.x) + L"," + std::to_wstring(snap.y));
    }

    void CaptureShortcutSellerPosition() {
        Account* a=SelectedAccount();
        if(!a){Log(L"TÙY CHỈNH NPC BÁN: chọn 1 acc đứng sát NPC trước.");return;}
        if(!shortcutSellerCombo_) return;
        const LRESULT sel=SendMessageW(shortcutSellerCombo_,CB_GETCURSEL,0,0);
        if(sel==CB_ERR||sel<0||sel>=static_cast<LRESULT>(kSellNpcs.size())) return;
        std::wstring error;
        if(!ReadSnapshot(*a,error,1200)){LogAccount(*a,L"Không đọc state để lấy tọa NPC bán: "+error);return;}
        const auto& npc=kSellNpcs[static_cast<std::size_t>(sel)]; const auto& snap=a->snapshot;
        if((snap.validMask&(ValidMap|ValidPosition))!=(ValidMap|ValidPosition)){LogAccount(*a,L"State thiếu Map/X/Y");return;}
        if(snap.mapID!=npc.mapID){LogAccount(*a,L"KHÔNG LƯU: "+std::wstring(npc.name)+L" thuộc M"+std::to_wstring(npc.mapID)+L", hiện đang M"+std::to_wstring(snap.mapID));return;}
        auto& pos=sellNpcPositions_[static_cast<std::size_t>(sel)]; pos={snap.x,snap.y,true}; SaveSharedSellNpcPositions(sellNpcPositions_);
        RefreshShortcutSellerUi();
        Account* selected=SelectedAccount(); if(selected) LoadSellNpcPositionToUi(*selected);
        LogAccount(*a,L"ĐÃ GÁN NPC BÁN • "+std::wstring(npc.name)+L" • ResID "+std::to_wstring(npc.npcID)+L" • "+std::to_wstring(pos.x)+L","+std::to_wstring(pos.y));
    }

    void BuildShortcutSettingsUi(HWND parent) {
        MakeIn(parent, L"STATIC", L"TÙY CHỈNH v3.3 • THĐC đã điền sẵn đúng map chứa cổng; mọi tọa đều sửa tay hoặc LẤY TỌA. 0,0 = chưa gán / fail-closed.",
               SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 15, 12, 930, 32, 0);
        MakeIn(parent, L"STATIC", L"Theme:", SS_LEFT | SS_CENTERIMAGE, 15, 54, 60, 25, 0);
        shortcutTheme_ = MakeIn(parent, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST, 78, 52, 170, 160, IDC_SC_THEME);
        SendMessageW(shortcutTheme_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Sáng / hệ thống"));
        SendMessageW(shortcutTheme_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Tối"));

        struct Row { const wchar_t* label; int idX; int idY; int captureId; };
        const Row rows[7] = {
            {L"NPC RỜI Côn Lôn • M75", IDC_SC_KUNLUN_X, IDC_SC_KUNLUN_Y, IDC_SC_CAPTURE_COORD_0},
            {L"Xa Truyền Bình • ID387 • ĐI VÀO Côn Lôn • M5", IDC_SC_XA_X, IDC_SC_XA_Y, IDC_SC_CAPTURE_COORD_1},
            {L"Ngải Ni Ngoã Nhĩ • ResID 913 • M5", IDC_SC_NGAI_X, IDC_SC_NGAI_Y, IDC_SC_CAPTURE_COORD_2},
            {L"Tinh Túc Hải điểm ra • M12", IDC_SC_TINHTUC_X, IDC_SC_TINHTUC_Y, IDC_SC_CAPTURE_COORD_3},
            {L"Cổng Thanh Liên • từ M10000", IDC_SC_THANHLIEN_X, IDC_SC_THANHLIEN_Y, IDC_SC_CAPTURE_COORD_4},
            {L"Cổng Phàm Liên • từ M10000", IDC_SC_PHAMLIEN_X, IDC_SC_PHAMLIEN_Y, IDC_SC_CAPTURE_COORD_5},
            {L"Cổng Khô Vinh • từ M10000", IDC_SC_KHOVINH_X, IDC_SC_KHOVINH_Y, IDC_SC_CAPTURE_COORD_6},
        };
        const Row thdcRows[7] = {
            {L"M10000 → THĐC tầng 1", IDC_SC_THDC_X_0, IDC_SC_THDC_Y_0, IDC_SC_CAPTURE_THDC_0},
            {L"M10014 tầng 1 → tầng 2", IDC_SC_THDC_X_1, IDC_SC_THDC_Y_1, IDC_SC_CAPTURE_THDC_1},
            {L"M10015 tầng 2 → tầng 3", IDC_SC_THDC_X_2, IDC_SC_THDC_Y_2, IDC_SC_CAPTURE_THDC_2},
            {L"M10015 tầng 2 → tầng 1", IDC_SC_THDC_X_3, IDC_SC_THDC_Y_3, IDC_SC_CAPTURE_THDC_3},
            {L"M10016 tầng 3 → tầng 4", IDC_SC_THDC_X_4, IDC_SC_THDC_Y_4, IDC_SC_CAPTURE_THDC_4},
            {L"M10016 tầng 3 → tầng 2", IDC_SC_THDC_X_5, IDC_SC_THDC_Y_5, IDC_SC_CAPTURE_THDC_5},
            {L"M10017 tầng 4 → tầng 3", IDC_SC_THDC_X_6, IDC_SC_THDC_Y_6, IDC_SC_CAPTURE_THDC_6},
        };
        MakeIn(parent,L"STATIC",L"ĐƯỜNG TẮT HIỆN CÓ",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,15,86,450,25,0);
        MakeIn(parent,L"STATIC",L"TẦN HOÀNG ĐỊA CUNG • TỌA NẰM TRÊN MAP NGUỒN GHI Ở TỪNG DÒNG",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,490,86,455,25,0);
        auto drawCoordinateRow = [&](int x, int y, const Row& row, std::size_t editOffset) {
            MakeIn(parent,L"STATIC",row.label,SS_LEFT|SS_CENTERIMAGE,x,y,220,27,0);
            MakeIn(parent,L"STATIC",L"X",SS_CENTERIMAGE,x+220,y,14,27,0);
            shortcutCoordEdits_[editOffset]=MakeIn(parent,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,x+234,y,58,27,row.idX);
            MakeIn(parent,L"STATIC",L"Y",SS_CENTERIMAGE,x+294,y,14,27,0);
            shortcutCoordEdits_[editOffset+1]=MakeIn(parent,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,x+308,y,58,27,row.idY);
            MakeIn(parent,L"BUTTON",L"LẤY TỌA",BS_PUSHBUTTON,x+371,y,79,27,row.captureId);
        };
        for (int i=0;i<7;++i) {
            // Xa Truyền Bình ID387 uses ONLY the main Auto-Sell NPC coordinate source.
            if (i == 1) {
                shortcutCoordEdits_[2] = nullptr;
                shortcutCoordEdits_[3] = nullptr;
                continue;
            }
            const int visualIndex = i > 1 ? i - 1 : i;
            drawCoordinateRow(15, 116+visualIndex*36, rows[i], static_cast<std::size_t>(i*2));
        }
        for (int i=0;i<7;++i)
            drawCoordinateRow(490, 116+i*36, thdcRows[i], static_cast<std::size_t>(14+i*2));

        MakeIn(parent,L"STATIC",L"RỜI CÔN LÔN • ĐÚNG 3 TRYCLICKUI • Time (ms)=chờ trước click, Delay (ms)=chờ sau click; không dùng callback Đại Lý/Xác nhận.",
               SS_LEFT|SS_CENTERIMAGE|WS_BORDER,15,380,930,30,0);
        static constexpr const wchar_t* clickNames[3] = {L"1. Mở NPC rời CLS", L"2. Chọn Đại Lý", L"3. Xác nhận"};
        static constexpr int captureIds[3] = {IDC_SC_CAPTURE_KUNLUN_CLICK_0,IDC_SC_CAPTURE_KUNLUN_CLICK_1,IDC_SC_CAPTURE_KUNLUN_CLICK_2};
        static constexpr int timeIds[3] = {IDC_SC_KUNLUN_TIME_0,IDC_SC_KUNLUN_TIME_1,IDC_SC_KUNLUN_TIME_2};
        static constexpr int delayIds[3] = {IDC_SC_KUNLUN_DELAY_0,IDC_SC_KUNLUN_DELAY_1,IDC_SC_KUNLUN_DELAY_2};
        for (int i=0;i<3;++i) {
            const int y=418+i*38;
            MakeIn(parent,L"STATIC",clickNames[i],SS_LEFT|SS_CENTERIMAGE,15,y,150,28,0);
            shortcutClickLabels_[static_cast<std::size_t>(i)]=MakeIn(parent,L"STATIC",L"",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,165,y,350,28,0);
            MakeIn(parent,L"STATIC",L"Time",SS_CENTERIMAGE,520,y,38,28,0);
            shortcutClickTimeEdits_[static_cast<std::size_t>(i)]=MakeIn(parent,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,558,y,65,28,timeIds[i]);
            MakeIn(parent,L"STATIC",L"Delay",SS_CENTERIMAGE,628,y,45,28,0);
            shortcutClickDelayEdits_[static_cast<std::size_t>(i)]=MakeIn(parent,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,673,y,65,28,delayIds[i]);
            MakeIn(parent,L"BUTTON",L"LẤY CLICK (F8)",BS_PUSHBUTTON,748,y,130,28,captureIds[i]);
        }
        shortcutSellerCombo_ = nullptr;
        shortcutSellerCoordLabel_ = nullptr;
        MakeIn(parent,L"STATIC",L"NPC BÁN / XA TRUYỀN BÌNH: chỉ dùng tọa ở màn hình chính → chọn NPC bán → LẤY VỊ TRÍ. Không còn nguồn tọa thứ hai trong TÙY CHỈNH.",
               SS_LEFT|SS_CENTERIMAGE|WS_BORDER,15,542,930,38,0);
        MakeIn(parent,L"STATIC",
               L"THĐC: từ M10000 đi cổng vào M10014 rồi mới Xác nhận popup. Sau đó mỗi tầng chỉ đi đúng cổng trên map hiện tại và phải check MapID tầng kế tiếp; không nhảy tầng.",
               SS_LEFT|WS_BORDER,15,588,930,52,0);
        MakeIn(parent,L"BUTTON",L"LƯU",BS_DEFPUSHBUTTON,705,653,105,32,IDC_SC_SAVE);
        MakeIn(parent,L"BUTTON",L"ĐÓNG",BS_PUSHBUTTON,820,653,125,32,IDC_SC_CLOSE);
        LoadShortcutSettingsToUi(); ApplyShortcutPanelTheme(parent);
    }

    void OpenShortcutSettingsWindow() {
        if (shortcutWindow_ && IsWindow(shortcutWindow_)) { ShowWindow(shortcutWindow_,SW_SHOW); SetForegroundWindow(shortcutWindow_); return; }
        WNDCLASSEXW wc{}; wc.cbSize=sizeof(wc); wc.lpfnWndProc=ShortcutWndProc; wc.hInstance=instance_;
        wc.hCursor=LoadCursor(nullptr,IDC_ARROW); wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1); wc.lpszClassName=L"ThanLongShortcutSettingsV30";
        if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS){Log(L"Không đăng ký được cửa sổ Đường tắt.");return;}
        shortcutWindow_=CreateWindowExW(WS_EX_TOOLWINDOW,wc.lpszClassName,L"AUTO Thần Long đa tính năng Pro v4.0 • TÙY CHỈNH",
            WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,CW_USEDEFAULT,CW_USEDEFAULT,980,735,hwnd_,nullptr,instance_,this);
        if(!shortcutWindow_){Log(L"Không mở được cửa sổ Đường tắt.");return;}
        BuildShortcutSettingsUi(shortcutWindow_); ShowWindow(shortcutWindow_,SW_SHOW); UpdateWindow(shortcutWindow_);
    }

    void BeginKunlunExitClickCapture(int index) {
        if (index < 0 || index >= 3) return;
        Account* a=SelectedAccount();
        if(!a){Log(L"ĐƯỜNG TẮT: chọn 1 acc mẫu trước khi lấy 3 click RỜI Côn Lôn.");return;}
        shortcutKunlunCaptureIndex_=index; captureSlot_=ClickSlot::None; captureMacroIndex_=-1; captureTradeSequenceIndex_=-1;
        captureTradeSequenceMode_=0; captureTradeSequenceMainRef_=-1; capturePkStepIndex_=-1; capturePkClickIndex_=-1; capturePid_=a->game.pid;
        static constexpr const wchar_t* labels[3] = {L"mở NPC rời Côn Lôn",L"chọn dòng Đại Lý",L"bấm Xác nhận"};
        LogAccount(*a,L"ĐƯỜNG TẮT CLS: đưa chuột đúng điểm \""+std::wstring(labels[index])+L"\" rồi nhấn F8 • click "+
                      std::to_wstring(index+1)+L"/3 • dùng chung ALL ACC.");
    }

    LRESULT HandleShortcutWindow(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        switch(msg){
            case WM_COMMAND:
                switch(LOWORD(wp)){
                    case IDC_SC_CAPTURE_KUNLUN_CLICK_0: case IDC_SC_CAPTURE_KUNLUN_CLICK_1: case IDC_SC_CAPTURE_KUNLUN_CLICK_2:
                        if(HIWORD(wp)==BN_CLICKED) BeginKunlunExitClickCapture(LOWORD(wp)-IDC_SC_CAPTURE_KUNLUN_CLICK_0); return 0;
                    case IDC_SC_CAPTURE_COORD_0: case IDC_SC_CAPTURE_COORD_2: case IDC_SC_CAPTURE_COORD_3:
                    case IDC_SC_CAPTURE_COORD_4: case IDC_SC_CAPTURE_COORD_5: case IDC_SC_CAPTURE_COORD_6:
                        if(HIWORD(wp)==BN_CLICKED) CaptureShortcutCoordinate(LOWORD(wp)-IDC_SC_CAPTURE_COORD_0); return 0;
                    case IDC_SC_CAPTURE_THDC_0: case IDC_SC_CAPTURE_THDC_1: case IDC_SC_CAPTURE_THDC_2:
                    case IDC_SC_CAPTURE_THDC_3: case IDC_SC_CAPTURE_THDC_4: case IDC_SC_CAPTURE_THDC_5: case IDC_SC_CAPTURE_THDC_6:
                        if(HIWORD(wp)==BN_CLICKED) CaptureShortcutCoordinate(7+LOWORD(wp)-IDC_SC_CAPTURE_THDC_0); return 0;
                    case IDC_SC_SELLER_CAPTURE: if(HIWORD(wp)==BN_CLICKED) CaptureShortcutSellerPosition(); return 0;
                    case IDC_SC_SELLER_COMBO: if(HIWORD(wp)==CBN_SELCHANGE) RefreshShortcutSellerUi(); return 0;
                    case IDC_SC_SAVE: if(HIWORD(wp)==BN_CLICKED){PersistShortcutSettingsFromUi();ApplyShortcutPanelTheme(hwnd);LoadShortcutSettingsToUi();} return 0;
                    case IDC_SC_CLOSE: if(HIWORD(wp)==BN_CLICKED){PersistShortcutSettingsFromUi(false);ShowWindow(hwnd,SW_HIDE);} return 0;
                    case IDC_SC_THEME: if(HIWORD(wp)==CBN_SELCHANGE){PersistShortcutSettingsFromUi(false);ApplyShortcutPanelTheme(hwnd);} return 0;
                } break;
            case WM_CTLCOLORSTATIC: case WM_CTLCOLOREDIT:
                if(shortcutSettings_.theme==1){HDC dc=reinterpret_cast<HDC>(wp);SetTextColor(dc,RGB(235,235,235));SetBkColor(dc,RGB(35,35,35));if(!shortcutDarkBrush_)shortcutDarkBrush_=CreateSolidBrush(RGB(35,35,35));return reinterpret_cast<LRESULT>(shortcutDarkBrush_);} break;
            case WM_ERASEBKGND:
                if(shortcutSettings_.theme==1){RECT rc{};GetClientRect(hwnd,&rc);if(!shortcutDarkBrush_)shortcutDarkBrush_=CreateSolidBrush(RGB(35,35,35));FillRect(reinterpret_cast<HDC>(wp),&rc,shortcutDarkBrush_);return 1;} break;
            case WM_CLOSE: PersistShortcutSettingsFromUi(false); ShowWindow(hwnd,SW_HIDE); return 0;
            case WM_NCDESTROY:
                shortcutWindow_=nullptr;shortcutTheme_=nullptr;shortcutSellerCombo_=nullptr;shortcutSellerCoordLabel_=nullptr;
                shortcutCoordEdits_.fill(nullptr);shortcutClickLabels_.fill(nullptr);shortcutClickTimeEdits_.fill(nullptr);
                shortcutClickDelayEdits_.fill(nullptr);return 0;
        }
        return DefWindowProcW(hwnd,msg,wp,lp);
    }

    void AddInventoryRuleFromSelection(RuleAction action) {
        if (!inventoryFilterBagList_) return;
        const int selected = ListView_GetNextItem(inventoryFilterBagList_, -1, LVNI_SELECTED);
        if (selected < 0 || selected >= static_cast<int>(inventoryFilterBagRows_.size())) {
            SetInventoryFilterStatus(L"Chọn 1 item ở bảng tay nải trước"); return;
        }
        const InventoryBagRow& row = inventoryFilterBagRows_[static_cast<std::size_t>(selected)];
        auto it = std::find_if(inventoryFilter_.rules.begin(), inventoryFilter_.rules.end(), [&](const auto& r){ return r.itemID == row.item.itemID; });
        if (it == inventoryFilter_.rules.end()) inventoryFilter_.rules.push_back({row.item.itemID, action, row.name});
        else { it->action = action; it->name = row.name; }
        SaveInventoryFilterSettings(inventoryFilter_); RefreshInventoryRuleList();
        SetInventoryFilterStatus(std::wstring(L"Đã lưu rule ") + FilterActionText(action) + L" • " + row.name + L" • ItemID " + std::to_wstring(row.item.itemID));
    }

    void DeleteSelectedInventoryRule() {
        if (!inventoryFilterRuleList_) return;
        const int selected = ListView_GetNextItem(inventoryFilterRuleList_, -1, LVNI_SELECTED);
        if (selected < 0 || selected >= static_cast<int>(inventoryFilter_.rules.size())) return;
        inventoryFilter_.rules.erase(inventoryFilter_.rules.begin() + selected);
        SaveInventoryFilterSettings(inventoryFilter_); RefreshInventoryRuleList();
        SetInventoryFilterStatus(L"Đã xóa rule");
    }

    void BuildInventoryFilterUi(HWND parent) {
        auto addColumn = [](HWND list, int i, int width, const wchar_t* text){
            LVCOLUMNW col{}; col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM; col.cx = width; col.iSubItem = i; col.pszText = const_cast<wchar_t*>(text);
            ListView_InsertColumn(list, i, &col);
        };
        Account* a = AccountByPid(inventoryFilterPid_);
        const std::wstring label = a ? (L"ACC: " + AccountTag(*a)) : L"ACC: ?";
        MakeIn(parent, L"STATIC", label.c_str(), SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 14, 12, 720, 28, 0);
        MakeIn(parent, L"BUTTON", L"QUÉT LẠI TAY NẢI", BS_PUSHBUTTON, 748, 12, 180, 28, IDC_IF_REFRESH);
        inventoryFilterStatus_ = MakeIn(parent, L"STATIC", L"Chưa scan", SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 14, 46, 1000, 28, IDC_IF_STATUS);

        inventoryFilterBagList_ = MakeIn(parent, WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER, 14, 82, 1000, 230, IDC_IF_BAG_LIST);
        ListView_SetExtendedListViewStyle(inventoryFilterBagList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        const int widths[] = {35,180,85,125,80,100,40,40,50,50,50};
        const wchar_t* names[] = {L"#",L"Tên",L"ItemID",L"InstanceID",L"Loại",L"Equip",L"Ô",L"SL",L"Khóa",L"Vứt?",L"Bán?"};
        for (int i=0;i<11;++i) addColumn(inventoryFilterBagList_, i, widths[i], names[i]);

        MakeIn(parent, L"BUTTON", L"+ GIỮ", BS_PUSHBUTTON, 14, 320, 105, 30, IDC_IF_ADD_KEEP);
        MakeIn(parent, L"BUTTON", L"+ VỨT/HỦY", BS_PUSHBUTTON, 126, 320, 120, 30, IDC_IF_ADD_DROP);
        MakeIn(parent, L"BUTTON", L"+ BÁN", BS_PUSHBUTTON, 253, 320, 105, 30, IDC_IF_ADD_SELL);
        MakeIn(parent, L"BUTTON", L"XÓA RULE", BS_PUSHBUTTON, 365, 320, 110, 30, IDC_IF_DELETE_RULE);

        inventoryFilterRuleList_ = MakeIn(parent, WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER, 14, 356, 500, 255, IDC_IF_RULE_LIST);
        ListView_SetExtendedListViewStyle(inventoryFilterRuleList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        addColumn(inventoryFilterRuleList_,0,35,L"#"); addColumn(inventoryFilterRuleList_,1,90,L"Hành động");
        addColumn(inventoryFilterRuleList_,2,255,L"Tên cố định"); addColumn(inventoryFilterRuleList_,3,105,L"ItemID");

        inventoryFilterEnabled_ = MakeIn(parent,L"BUTTON",L"BẬT LỌC ĐỒ TỰ ĐỘNG TRONG AUTO",BS_AUTOCHECKBOX,535,326,310,25,IDC_IF_ENABLED);
        inventoryFilterProtectBound_ = MakeIn(parent,L"BUTTON",L"Bảo vệ đồ khóa",BS_AUTOCHECKBOX,535,355,180,24,IDC_IF_PROTECT_BOUND);
        inventoryFilterKeepWeapon_ = MakeIn(parent,L"BUTTON",L"Luôn giữ vũ khí",BS_AUTOCHECKBOX,730,355,180,24,IDC_IF_KEEP_WEAPON);
        MakeIn(parent,L"STATIC",L"PHÂN LOẠI TỰ ĐỘNG — VỨT ưu tiên trước BÁN; mọi VỨT vẫn bắt buộc IsItemThrowable=true",SS_LEFT|SS_CENTERIMAGE,535,385,475,25,0);
        inventoryFilterDropEquip_ = MakeIn(parent,L"BUTTON",L"Vứt Equip không phải Weapon",BS_AUTOCHECKBOX,535,414,225,23,IDC_IF_DROP_EQUIP_NONWEAPON);
        inventoryFilterSellEquip_ = MakeIn(parent,L"BUTTON",L"Bán Equip không phải Weapon",BS_AUTOCHECKBOX,770,414,225,23,IDC_IF_SELL_EQUIP_NONWEAPON);
        inventoryFilterDropCommon_ = MakeIn(parent,L"BUTTON",L"Vứt CommonItem",BS_AUTOCHECKBOX,535,443,180,23,IDC_IF_DROP_COMMON);
        inventoryFilterSellCommon_ = MakeIn(parent,L"BUTTON",L"Bán CommonItem",BS_AUTOCHECKBOX,770,443,180,23,IDC_IF_SELL_COMMON);
        inventoryFilterDropGem_ = MakeIn(parent,L"BUTTON",L"Vứt Gem",BS_AUTOCHECKBOX,535,472,180,23,IDC_IF_DROP_GEM);
        inventoryFilterSellGem_ = MakeIn(parent,L"BUTTON",L"Bán Gem",BS_AUTOCHECKBOX,770,472,180,23,IDC_IF_SELL_GEM);
        inventoryFilterDropMedicine_ = MakeIn(parent,L"BUTTON",L"Vứt Medicine",BS_AUTOCHECKBOX,535,501,180,23,IDC_IF_DROP_MEDICINE);
        inventoryFilterSellMedicine_ = MakeIn(parent,L"BUTTON",L"Bán Medicine",BS_AUTOCHECKBOX,770,501,180,23,IDC_IF_SELL_MEDICINE);
        inventoryFilterDropPetEquip_ = MakeIn(parent,L"BUTTON",L"Vứt PetEquip",BS_AUTOCHECKBOX,535,530,180,23,IDC_IF_DROP_PETEQUIP);
        inventoryFilterSellPetEquip_ = MakeIn(parent,L"BUTTON",L"Bán PetEquip",BS_AUTOCHECKBOX,770,530,180,23,IDC_IF_SELL_PETEQUIP);
        MakeIn(parent,L"STATIC",L"Rule GIỮ + quest-family + bảo vệ đồ khóa/vũ khí có ưu tiên cao nhất. Khi túi FULL: thử VỨT 1 instance → re-scan; hết đồ VỨT mà vẫn FULL mới vào luồng bán.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,535,565,475,46,0);
        LoadInventoryFilterChecksToUi(); RefreshInventoryRuleList(); RefreshInventoryBagList();
    }

    void OpenInventoryFilterWindow() {
        Account* selected = SelectedAccount();
        if (!selected) { Log(L"LỌC ĐỒ TAY NẢI: chọn acc trước."); return; }
        if (inventoryFilterWindow_ && IsWindow(inventoryFilterWindow_)) DestroyWindow(inventoryFilterWindow_);
        inventoryFilterPid_ = selected->game.pid;
        WNDCLASSEXW wc{}; wc.cbSize=sizeof(wc); wc.lpfnWndProc=InventoryFilterWndProc; wc.hInstance=instance_;
        wc.hCursor=LoadCursor(nullptr,IDC_ARROW); wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1); wc.lpszClassName=L"ThanLongInventoryFilterV13";
        if (!RegisterClassExW(&wc) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) { Log(L"Không đăng ký được cửa sổ Lọc đồ."); return; }
        inventoryFilterWindow_ = CreateWindowExW(WS_EX_TOOLWINDOW, wc.lpszClassName, L"AUTO Thần Long đa tính năng Pro v4.0 • LỌC ĐỒ TAY NẢI",
            WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX, CW_USEDEFAULT,CW_USEDEFAULT,1045,665,hwnd_,nullptr,instance_,this);
        if (!inventoryFilterWindow_) { Log(L"Không mở được cửa sổ Lọc đồ."); return; }
        BuildInventoryFilterUi(inventoryFilterWindow_); ShowWindow(inventoryFilterWindow_,SW_SHOW); UpdateWindow(inventoryFilterWindow_);
    }

    LRESULT HandleInventoryFilterWindow(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        (void)lp;
        switch(msg) {
            case WM_COMMAND:
                switch(LOWORD(wp)) {
                    case IDC_IF_REFRESH: RefreshInventoryBagList(); return 0;
                    case IDC_IF_ADD_KEEP: AddInventoryRuleFromSelection(RuleAction::Keep); return 0;
                    case IDC_IF_ADD_DROP: AddInventoryRuleFromSelection(RuleAction::Drop); return 0;
                    case IDC_IF_ADD_SELL: AddInventoryRuleFromSelection(RuleAction::Sell); return 0;
                    case IDC_IF_DELETE_RULE: DeleteSelectedInventoryRule(); return 0;
                    case IDC_IF_ENABLED: case IDC_IF_PROTECT_BOUND: case IDC_IF_KEEP_WEAPON:
                    case IDC_IF_DROP_EQUIP_NONWEAPON: case IDC_IF_SELL_EQUIP_NONWEAPON:
                    case IDC_IF_DROP_COMMON: case IDC_IF_SELL_COMMON: case IDC_IF_DROP_GEM: case IDC_IF_SELL_GEM:
                    case IDC_IF_DROP_MEDICINE: case IDC_IF_SELL_MEDICINE: case IDC_IF_DROP_PETEQUIP: case IDC_IF_SELL_PETEQUIP:
                        if (HIWORD(wp)==BN_CLICKED) { PersistInventoryFilterChecksFromUi(); SetInventoryFilterStatus(L"Đã lưu cấu hình lọc • áp dụng ngay cho tab AUTO"); }
                        return 0;
                }
                break;
            case WM_CLOSE: PersistInventoryFilterChecksFromUi(); DestroyWindow(hwnd); return 0;
            case WM_NCDESTROY:
                inventoryFilterWindow_=nullptr; inventoryFilterBagList_=nullptr; inventoryFilterRuleList_=nullptr; inventoryFilterStatus_=nullptr;
                inventoryFilterEnabled_=nullptr; inventoryFilterProtectBound_=nullptr; inventoryFilterKeepWeapon_=nullptr;
                inventoryFilterDropEquip_=nullptr; inventoryFilterSellEquip_=nullptr; inventoryFilterDropCommon_=nullptr; inventoryFilterSellCommon_=nullptr;
                inventoryFilterDropGem_=nullptr; inventoryFilterSellGem_=nullptr; inventoryFilterDropMedicine_=nullptr; inventoryFilterSellMedicine_=nullptr;
                inventoryFilterDropPetEquip_=nullptr; inventoryFilterSellPetEquip_=nullptr; inventoryFilterBagRows_.clear(); inventoryFilterPid_=0;
                return DefWindowProcW(hwnd,msg,wp,lp);
        }
        return DefWindowProcW(hwnd,msg,wp,lp);
    }

    void ShowSellMacroEditor(bool show) {
        sellMacroEditorVisible_ = show;
        for (HWND h : sellMacroControls_) if (h) ShowWindow(h, show ? SW_SHOW : SW_HIDE);
        if (logCaption_) SetWindowPos(logCaption_, nullptr, 18, show ? 882 : 742, 190, 20, SWP_NOZORDER);
        if (log_) SetWindowPos(log_, nullptr, 18, show ? 902 : 764, 1005, show ? 60 : 159, SWP_NOZORDER);
        if (show) { RefreshSellMacroList(); ClearSellMacroEditor(); }
    }

    void ToggleSellMacroEditor() {
        if (sellMacroEditorVisible_ && recorderMode_ == RecorderMode::Sell) StopRecorder(true);
        Account* a = SelectedAccount();
        if (!a) { Log(L"CHUỖI CLICK BÁN ĐỒ: chưa chọn acc."); return; }
        ShowSellMacroEditor(!sellMacroEditorVisible_);
    }

    void UpdateRoleActionButtons() {
        Account* a = SelectedAccount();
        const int role = a ? a->profile.tradeRole : 0;
        if (sellSequenceButton_) ShowWindow(sellSequenceButton_, a ? SW_SHOW : SW_HIDE);
        if (mainTradeSequenceButton_) ShowWindow(mainTradeSequenceButton_, role == 1 ? SW_SHOW : SW_HIDE);
        if (childTradeSequenceButton_) {
            ShowWindow(childTradeSequenceButton_, role >= 2 ? SW_SHOW : SW_HIDE);
            if (role >= 2) SetWindowTextW(childTradeSequenceButton_, L"CHUỖI GD ACC CON");
        }
        if (tradeRendezvousCaptureButton_) ShowWindow(tradeRendezvousCaptureButton_, SW_SHOW);
        if (tradeRendezvousLabel_) ShowWindow(tradeRendezvousLabel_, SW_SHOW);
        if (!a && sellMacroEditorVisible_) ShowSellMacroEditor(false);
    }

    void PersistGlobalTradeSettings() {
        WriteIniInt(L"Global", L"TradeEnabled", tradeEnabled_ ? 1 : 0);
        WriteIniInt(L"Global", L"ChildTriggerFreeSlots", 0);
        WriteIniInt(L"Global", L"TradeRendezvousMap", tradeRendezvous_.mapID);
        WriteIniInt(L"Global", L"TradeRendezvousX", tradeRendezvous_.x);
        WriteIniInt(L"Global", L"TradeRendezvousY", tradeRendezvous_.y);
        WriteIniInt(L"Global", L"TradeRendezvousValid", tradeRendezvous_.valid ? 1 : 0);
        WriteIniInt(L"Global", L"TradeRendezvousTolerance", tradeRendezvousTolerance_);
        FlushIni();
    }

    void UpdateConsolidationButton() {
        if (!tradeEnable_) return;
        SetWindowTextW(tradeEnable_, tradeEnabled_ ? L"DỒN ĐỒ: BẬT" : L"DỒN ĐỒ: TẮT");
    }

    void ToggleConsolidationMode() {
        tradeEnabled_ = !tradeEnabled_;
        if (!tradeEnabled_) {
            if (tradeTxn_.phase != TradePhase::Idle) {
                AbortTrade(L"người dùng TẮT DỒN ĐỒ", GetTickCount());
            }
            ReleaseTradeHolds(); // hard cleanup: no stale rendezvous HOLD may survive OFF.
            SetTradeStatus(L"DỒN ĐỒ TẮT • scheduler GD bị chặn thật • các acc auto-train/bán đồ độc lập");
            Log(L"DỒN ĐỒ: TẮT → vô hiệu hóa giao dịch MAIN↔CON; chỉ acc đã tick AUTO BÁN ĐỒ mới tự bán khi túi FULL.");
        } else {
            SetTradeStatus(L"DỒN ĐỒ BẬT • scheduler MAIN↔CON hoạt động");
            Log(L"DỒN ĐỒ: BẬT → MAIN chỉ mở pass khi FreeBag >=9; dưới 9 ô thì nhả MAIN cho Auto Sell, CON FULL ưu tiên FIFO.");
        }
        WriteIniInt(L"Global", L"TradeEnabled", tradeEnabled_ ? 1 : 0);
        FlushIni();
        UpdateConsolidationButton();
    }

    void CopySellSequenceFromAnotherAccount() {
        Account* target = SelectedAccount();
        if (!target) { Log(L"LẤY CHUỖI BÁN: chưa chọn acc đích."); return; }

        HMENU menu = CreatePopupMenu();
        if (!menu) return;
        constexpr UINT kBase = 7300;
        std::vector<Account*> sources;
        for (auto& item : accounts_) {
            Account* source = item.get();
            if (!source || source == target || source->profile.sellMacro.empty()) continue;
            sources.push_back(source);
            const std::wstring label = AccountTag(*source) + L" • " + std::to_wstring(source->profile.sellMacro.size()) + L" bước";
            AppendMenuW(menu, MF_STRING, kBase + static_cast<UINT>(sources.size() - 1), label.c_str());
        }
        if (sources.empty()) {
            AppendMenuW(menu, MF_STRING | MF_GRAYED, 1, L"Chưa có acc khác có chuỗi bán");
        }

        RECT rc{};
        if (sellMacroList_) GetWindowRect(sellMacroList_, &rc);
        else GetWindowRect(hwnd_, &rc);
        const UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN,
                                        rc.left + 12, rc.top + 12, 0, hwnd_, nullptr);
        DestroyMenu(menu);
        if (cmd < kBase || cmd >= kBase + sources.size()) return;

        Account* source = sources[cmd - kBase];
        if (!source) return;
        if (!target->profile.sellMacro.empty()) {
            const std::wstring q = L"Acc đích đang có " + std::to_wstring(target->profile.sellMacro.size()) +
                                   L" bước bán.\n\nThay toàn bộ bằng chuỗi của " + AccountTag(*source) + L"?";
            if (MessageBoxW(hwnd_, q.c_str(), L"LẤY CHUỖI CLICK BÁN ĐỒ", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
        }
        target->profile.sellMacro = source->profile.sellMacro;
        SaveProfile(target->profile);
        RefreshSellMacroList();
        ClearSellMacroEditor();
        LogAccount(*target, L"Đã lấy CHUỖI CLICK BÁN ĐỒ từ " + AccountTag(*source) +
                            L" • " + std::to_wstring(target->profile.sellMacro.size()) + L" bước.");
    }

    void SetTradeStatus(const std::wstring& text) {
        if (!tradeStatus_) return;
        const std::wstring line = L"ĐIỀU PHỐI: " + text;
        SetWindowTextW(tradeStatus_, line.c_str());
    }

    void ApplySelectedTradeRole() {
        Account* selected = SelectedAccount();
        if (!selected || !tradeRoleCombo_) return;
        const LRESULT sel = SendMessageW(tradeRoleCombo_, CB_GETCURSEL, 0, 0);
        if (sel == CB_ERR || sel < 0 || sel > kLastChildTradeRole) return;
        const int newRole = static_cast<int>(sel);
        if (newRole != 0) {
            for (auto& item : accounts_) {
                Account& other = *item;
                if (&other == selected) continue;
                if (other.profile.tradeRole == newRole) {
                    other.profile.tradeRole = 0;
                    other.tradeHeld = false;
                    SaveProfile(other.profile);
                    LogAccount(other, L"Vai trò " + TradeRoleLabel(newRole) + L" được chuyển sang acc khác → trả về KHÔNG.");
                }
            }
        }
        selected->profile.tradeRole = newRole;
        if (newRole == 1) {
            selected->profile.enableSell = true; // MAIN must sell immediately at/below threshold.
            if (enableSell_) SendMessageW(enableSell_, BM_SETCHECK, BST_CHECKED, 0);
        }
        if (newRole >= 2) {
            selected->profile.enableSell = false; // hard rule: child never sells.
            selected->runtime.sellPhase = 0;
            if (enableSell_) SendMessageW(enableSell_, BM_SETCHECK, BST_UNCHECKED, 0);
        }
        SaveProfile(selected->profile);
        for (std::size_t i = 0; i < accounts_.size(); ++i) UpdateAccountRow(static_cast<int>(i), *accounts_[i]);
        LoadSelectedProfileToUi();
        LogAccount(*selected, L"Đặt vai trò giao dịch = " + TradeRoleLabel(newRole));
    }

    Account* AccountByTradeRole(int role) {
        for (auto& a : accounts_) if (a->profile.tradeRole == role) return a.get();
        return nullptr;
    }

    bool TradeStateReady(const Account& a) const {
        if (!a.runtime.running || !a.snapshotValid || !IsWindow(a.game.window)) return false;
        const Snapshot& s = a.snapshot;
        const std::uint32_t need = ValidLifeState | ValidBagSpace | ValidMap | ValidPosition;
        if ((s.validMask & need) != need) return false;
        if (s.dead || !s.mapReady || s.waitingChangeMap) return false;
        if (a.runtime.clientFreezeActive || a.runtime.revivePhase != 0 || a.runtime.sellPhase != 0 ||
            a.runtime.trainRecoveryPhase != 0 || a.runtime.routeOwnershipResetPending ||
            a.runtime.bagFilterPendingInstance != 0) return false;
        return true;
    }

    bool TradePairReadyForPreparation(const Account& main, const Account& child) const {
        if (!main.runtime.running || !child.runtime.running) return false;
        if (!main.snapshotValid || !child.snapshotValid) return false;
        const Snapshot& ms = main.snapshot;
        const Snapshot& cs = child.snapshot;
        const std::uint32_t need = ValidLifeState | ValidBagSpace | ValidMap | ValidPosition | ValidAutoFight | ValidAutoPath | ValidRiding;
        if ((ms.validMask & need) != need || (cs.validMask & need) != need) return false;
        if (ms.dead || cs.dead || !ms.mapReady || ms.waitingChangeMap || !cs.mapReady || cs.waitingChangeMap) return false;
        return IsWindow(main.game.window) && IsWindow(child.game.window);
    }

    void ResetTradeRendezvousTravel(Account& a) {
        RuntimeState& rt = a.runtime;
        rt.tradeTravelPhase = 0;
        rt.tradeTravelTick = 0;
        rt.tradeTravelReady = false;
        ResetRobustTravel(rt);
    }

    bool TradeAccountAtRendezvous(const Account& a) const {
        if (!tradeRendezvous_.valid || !a.snapshotValid) return false;
        const Snapshot& s = a.snapshot;
        const std::uint32_t need = ValidLifeState | ValidMap | ValidPosition | ValidAutoPath | ValidRiding;
        if ((s.validMask & need) != need || s.dead || !s.mapReady || s.waitingChangeMap) return false;
        State state{};
        state.valid = true; state.mapReady = true; state.waitingMap = false;
        state.mapID = s.mapID; state.x = s.x; state.y = s.y;
        state.autoPathing = s.autoPathing != 0; state.riding = s.riding != 0;
        Target target{tradeRendezvous_.mapID, tradeRendezvous_.x, tradeRendezvous_.y, tradeRendezvousTolerance_};
        return AtTarget(state, target) && !s.autoPathing && !s.riding;
    }

    void BeginTradeRendezvousTravel(Account& a, DWORD now, const wchar_t* who) {
        RuntimeState& rt = a.runtime;
        ResetTradeRendezvousTravel(a);
        rt.tradeTravelPhase = 4;
        rt.tradeTravelTick = now;
        rt.trainPositionMonitorArmed = false;
        rt.lastTrainPositionCheckTick = 0;
        rt.fightPhase = 0;
        rt.fightAttempts = 0;
        rt.fightRetryWaitTick = 0;

        // The old train AutoPath belongs to the normal core and must not survive into
        // a consolidation rendezvous. This StopPath is internal and does not touch F4 state.
        if (a.bridge.Attached() && a.snapshotValid && (a.snapshot.validMask & ValidAutoPath) && a.snapshot.autoPathing) {
            Response r{}; std::wstring ignored;
            (void)a.bridge.Call(Command::StopPath, 0, 0, 0, r, ignored, 700);
        }
        LogAccount(a, L"GD TỌA: HOLD " + std::wstring(who ? who : L"ACC") +
                      L" • hủy AutoPath bãi cũ → v0.3 Travel Guard bắt buộc AutoFight OFF → cùng đi TỌA GD.");
    }

    bool HandleTradeRendezvousTravel(Account& a, DWORD now, const wchar_t* who) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        const std::wstring tag = who ? who : L"ACC";

        if (!tradeRendezvous_.valid || !a.runtime.running || !a.snapshotValid || !IsWindow(a.game.window)) return false;
        if (rt.autoPathFightConflictLatched) {
            rt.status = L"GD TỌA • chờ ROUTE/FIGHT INVARIANT recovery";
            return true;
        }
        const std::uint32_t need = ValidLifeState | ValidMap | ValidPosition | ValidAutoPath | ValidRiding;
        if ((s.validMask & need) != need || s.dead || !s.mapReady || s.waitingChangeMap) return false;

        // Once ready, keep the first-arriving account parked at TỌA GD. Any stale/automatic
        // path that reappears is stopped before the coordinator can advance the transaction.
        if (rt.tradeTravelReady) {
            if (s.autoPathing) {
                Response r{}; std::wstring ignored;
                if (a.bridge.Attached()) (void)a.bridge.Call(Command::StopPath, 0, 0, 0, r, ignored, 700);
                rt.status = L"GD HOLD • " + tag + L" đã tới TỌA GD • chặn AutoPath bãi cũ";
                return true;
            }
            if (s.riding) {
                (void)SendDecision(a, Action::Dismount, tradeRendezvous_, L"TỌA GD");
                rt.status = L"GD HOLD • " + tag + L" xuống ngựa tại TỌA GD";
                return true;
            }
            if (!TradeAccountAtRendezvous(a)) {
                rt.tradeTravelReady = false;
                rt.tradeTravelPhase = 4;
                rt.tradeTravelTick = now;
                ResetRobustTravel(rt);
                rt.status = L"GD RELOCK • " + tag + L" lệch TỌA GD → quay lại";
                return true;
            }
            rt.status = L"GD HOLD • " + tag + L" đã tới TỌA GD • chờ acc còn lại";
            return true;
        }

        if (rt.tradeTravelPhase == 0) {
            BeginTradeRendezvousTravel(a, now, who);
            return true;
        }

        if (rt.tradeTravelPhase == 4) {
            bool arrived = false;
            (void)HandleRobustTravel(a, now, tradeRendezvous_, L"TỌA GD", arrived, tradeRendezvousTolerance_);
            if (!arrived) {
                rt.status = L"GD TỌA • " + tag + L" đang đi M" + std::to_wstring(tradeRendezvous_.mapID) +
                            L" " + std::to_wstring(tradeRendezvous_.x) + L"," + std::to_wstring(tradeRendezvous_.y);
                return true;
            }
            rt.tradeTravelPhase = 5;
            rt.tradeTravelTick = now;
            rt.status = L"GD TỌA • " + tag + L" đã tới • khóa path và verify";
            return true;
        }

        if (rt.tradeTravelPhase == 5) {
            if (s.autoPathing) {
                Response r{}; std::wstring ignored;
                if (a.bridge.Attached()) (void)a.bridge.Call(Command::StopPath, 0, 0, 0, r, ignored, 700);
                rt.tradeTravelTick = now;
                rt.status = L"GD HOLD • " + tag + L" StopPath tại TỌA GD";
                return true;
            }
            if (s.riding) {
                (void)SendDecision(a, Action::Dismount, tradeRendezvous_, L"TỌA GD");
                rt.tradeTravelTick = now;
                return true;
            }
            if (!Elapsed(now, rt.tradeTravelTick, 450)) return true;

            // v0.3: reuse the same fail-closed guard at the rendezvous itself. No separate
            // trade stop-Auto state machine remains.
            if ((s.validMask & ValidAutoFight) == 0 || s.autoFight) {
                if (!EnsureAutoFightOffForTravel(a, now, L"TỌA GD")) {
                    rt.status = L"GD HOLD • " + tag + L" chờ Travel Guard xác nhận AutoFight OFF";
                    return true;
                }
            }
            if (!TradeAccountAtRendezvous(a)) {
                rt.tradeTravelPhase = 4; rt.tradeTravelTick = now; ResetRobustTravel(rt);
                return true;
            }
            rt.tradeTravelReady = true;
            rt.tradeTravelPhase = 0;
            rt.status = L"GD HOLD • " + tag + L" ĐÃ TỚI TỌA GD";
            LogAccount(a, L"GD TỌA PASS: " + tag + L" đã đứng tại TỌA GD; giữ HOLD chờ acc còn lại.");
            return true;
        }
        return true;
    }

    bool TradeQueueContains(DWORD pid) const {
        return std::find(tradeQueuePids_.begin(), tradeQueuePids_.end(), pid) != tradeQueuePids_.end();
    }

    Account* EarliestQueuedChild() {
        Account* best = nullptr;
        for (DWORD pid : tradeQueuePids_) {
            Account* child = AccountByPid(pid);
            if (!child || child->profile.tradeRole < kFirstChildTradeRole ||
                child->profile.tradeRole > kLastChildTradeRole) continue;
            if (!best || itemtrade_coordinator::EarlierWorkflowEntry(
                    child->runtime.tradeWorkflowEntrySeq, child->profile.tradeRole - 1,
                    best->runtime.tradeWorkflowEntrySeq, best->profile.tradeRole - 1)) {
                best = child;
            }
        }
        return best;
    }

    void RelockRendezvousToEarliestQueued(DWORD now) {
        (void)now;
        if (tradeTxn_.phase != TradePhase::Rendezvous) return;
        Account* earliest = EarliestQueuedChild();
        if (!earliest || earliest->game.pid == tradeTxn_.childPid) return;
        Account* old = AccountByPid(tradeTxn_.childPid);
        const int oldSlot = tradeTxn_.childSlot;
        tradeTxn_.childPid = earliest->game.pid;
        tradeTxn_.childSlot = earliest->profile.tradeRole - 1;
        tradeTxn_.sequenceIndex = 0;
        tradeTxn_.sequenceRepeatDone = 0;
        tradeTxn_.sequenceGroupRepeatDone = 0;
        tradeTxn_.sequencePass = 1;
        tradeTxn_.sequenceDueTick = 0;
        if (old) LogAccount(*old, L"FIFO RELOCK: nhường active rendezvous cho CON" +
                                  std::to_wstring(tradeTxn_.childSlot) + L" vì CON này vào workflow trước.");
        LogAccount(*earliest, L"FIFO RELOCK PASS: giữ đúng vé workflow #" +
                              std::to_wstring(earliest->runtime.tradeWorkflowEntrySeq) +
                              L" • active trước CON" + std::to_wstring(oldSlot) + L".");
    }

    void ReleaseTradeHold(Account& a) {
        a.tradeHeld = false;
        a.runtime.tradeWorkflowEntrySeq = 0;
        ResetTradeRendezvousTravel(a);
    }

    void RemoveTradeQueuePid(DWORD pid) {
        tradeQueuePids_.erase(std::remove(tradeQueuePids_.begin(), tradeQueuePids_.end(), pid), tradeQueuePids_.end());
    }

    void ReleaseTradeHolds() {
        for (auto& item : accounts_) {
            Account& a = *item;
            if (!a.tradeHeld) continue;
            ReleaseTradeHold(a);
        }
        tradeQueuePids_.clear();
    }

    void AbortTrade(const std::wstring& reason, DWORD now) {
        Account* main = AccountByPid(tradeTxn_.mainPid);
        Account* child = AccountByPid(tradeTxn_.childPid);
        if (main) LogAccount(*main, L"GD ABORT: " + reason);
        if (child) LogAccount(*child, L"GD ABORT: " + reason);
        ReleaseTradeHolds();
        tradeTxn_.phase = TradePhase::Idle;
        tradeTxn_.mainPid = 0;
        tradeTxn_.childPid = 0;
        tradeTxn_.childSlot = 0;
        tradeTxn_.sequenceIndex = 0; tradeTxn_.sequenceRepeatDone = 0; tradeTxn_.sequenceGroupRepeatDone = 0; tradeTxn_.sequencePass = 1; tradeTxn_.sequenceDueTick = 0; tradeTxn_.sequenceMainFreeBeforePass = -1; tradeTxn_.sequenceBagVerifyStartedTick = 0; tradeTxn_.sequenceBagStableSinceTick = 0; tradeTxn_.sequenceBagLastFree = -1;
        tradeTxn_.targetStartedTick = 0; tradeTxn_.targetRetryTick = 0; tradeTxn_.targetLastSelectTick = 0; tradeTxn_.targetAttempts = 0;
        tradeTxn_.cooldownUntil = now + 2500;
        SetTradeStatus(L"HỦY • " + reason + L" • nhả hàng đợi + tradeTxn/HOLD");
    }

    void FinishTrade(DWORD now, int receivedSlots) {
        Account* main = AccountByPid(tradeTxn_.mainPid);
        Account* child = AccountByPid(tradeTxn_.childPid);
        const DWORD finishedChildPid = tradeTxn_.childPid;
        const int finishedSlot = tradeTxn_.childSlot;

        if (main) LogAccount(*main, L"GD xong với CON " + std::to_wstring(finishedSlot) +
                                 L" • pass cuối MAIN nhận ≤8 slot; CON đang xếp hàng vẫn giữ TỌA GD.");
        if (child) LogAccount(*child, L"GD ĐẠT ĐIỀU KIỆN MỚI • pass cuối làm MAIN nhận ≤8 slot"
                                  L" • nhả HOLD để core quay bãi; slot queue giải phóng ngay.");
        if (main && child) TelegramRecordTradeCompleted(*main, *child, receivedSlots, tradeTxn_.sequencePass);

        if (child) ReleaseTradeHold(*child);
        RemoveTradeQueuePid(finishedChildPid);

        // MAIN stays parked only while another queued CON still needs it. When the queue
        // becomes empty, release MAIN exactly like old behavior so normal core resumes.
        if (main && tradeQueuePids_.empty()) ReleaseTradeHold(*main);

        tradeTxn_.phase = TradePhase::Idle;
        tradeTxn_.mainPid = main ? main->game.pid : 0;
        tradeTxn_.childPid = 0;
        tradeTxn_.childSlot = 0;
        tradeTxn_.sequenceIndex = 0; tradeTxn_.sequenceRepeatDone = 0; tradeTxn_.sequenceGroupRepeatDone = 0; tradeTxn_.sequencePass = 1; tradeTxn_.sequenceDueTick = 0; tradeTxn_.sequenceMainFreeBeforePass = -1; tradeTxn_.sequenceBagVerifyStartedTick = 0; tradeTxn_.sequenceBagStableSinceTick = 0; tradeTxn_.sequenceBagLastFree = -1;
        tradeTxn_.cooldownUntil = now + 1500;
        SetTradeStatus(L"HOÀN TẤT CON" + std::to_wstring(finishedSlot) + L" • còn xếp hàng " +
                       std::to_wstring(tradeQueuePids_.size()) + L"/3");
    }


    void YieldActiveTradeForMainSell(DWORD now, const std::wstring& reason, int receivedSlots = -1) {
        Account* main = AccountByPid(tradeTxn_.mainPid);
        Account* child = AccountByPid(tradeTxn_.childPid);
        const DWORD releasedChildPid = tradeTxn_.childPid;
        const int releasedSlot = tradeTxn_.childSlot;

        if (main) {
            LogAccount(*main, L"GD NHƯỜNG MAIN ĐI BÁN • " + reason +
                              L" • giữ nguyên các CON khác đang chờ FIFO.");
        }
        if (child) {
            LogAccount(*child, L"GD DỪNG DO MAIN <9 Ô • nhả CON" + std::to_wstring(releasedSlot) +
                               L" về bãi train; các CON khác trong FIFO vẫn đứng chờ.");
            if (receivedSlots >= 0 && main)
                TelegramRecordTradeCompleted(*main, *child, receivedSlots, tradeTxn_.sequencePass);
            ReleaseTradeHold(*child);
        }
        if (releasedChildPid != 0) RemoveTradeQueuePid(releasedChildPid);
        if (main && main->tradeHeld) ReleaseTradeHold(*main);

        tradeTxn_.phase = TradePhase::Idle;
        tradeTxn_.mainPid = main ? main->game.pid : 0;
        tradeTxn_.childPid = 0;
        tradeTxn_.childSlot = 0;
        tradeTxn_.sequenceIndex = 0; tradeTxn_.sequenceRepeatDone = 0; tradeTxn_.sequenceGroupRepeatDone = 0; tradeTxn_.sequencePass = 1; tradeTxn_.sequenceDueTick = 0; tradeTxn_.sequenceMainFreeBeforePass = -1; tradeTxn_.sequenceBagVerifyStartedTick = 0; tradeTxn_.sequenceBagStableSinceTick = 0; tradeTxn_.sequenceBagLastFree = -1;
        tradeTxn_.targetStartedTick = 0; tradeTxn_.targetRetryTick = 0; tradeTxn_.targetLastSelectTick = 0; tradeTxn_.targetAttempts = 0;
        tradeTxn_.cooldownUntil = now + 1500;
        SetTradeStatus(L"MAIN <9 ô → NHƯỜNG AUTO SELL • CON hiện tại về train • còn " +
                       std::to_wstring(tradeQueuePids_.size()) + L" CON FIFO đứng chờ");
    }

    bool ExecuteTradeSequenceTick(Account& main, Account& child, DWORD now) {
        EnsureSharedChildTradeSequence();
        std::vector<TradeSequenceStep>& seq = childTradeSequence_;
        if (tradeTxn_.sequenceIndex >= seq.size()) {
            // v3.3: after the final macro delay, watch MAIN FreeBagSpace only.
            // Stable 1500 ms or max 3200 ms, then keep the proven v1.4 before/after delta rule.
            if (tradeTxn_.sequenceDueTick != 0 && static_cast<LONG>(now - tradeTxn_.sequenceDueTick) < 0) return true;
            if (!main.snapshotValid || (main.snapshot.validMask & ValidBagSpace) == 0) {
                SetTradeStatus(L"TRADE WORKFLOW • chờ MAIN FreeBagSpace hợp lệ sau pass GD");
                return true;
            }
            const int observedFree = main.snapshot.freeBagSpace;
            if (tradeTxn_.sequenceBagVerifyStartedTick == 0) {
                tradeTxn_.sequenceBagVerifyStartedTick = now;
                tradeTxn_.sequenceBagStableSinceTick = now;
                tradeTxn_.sequenceBagLastFree = observedFree;
                SetTradeStatus(L"TRADE WORKFLOW • xác minh túi MAIN sau pass • cần ổn định 1500ms");
                return true;
            }
            if (observedFree != tradeTxn_.sequenceBagLastFree) {
                tradeTxn_.sequenceBagLastFree = observedFree;
                tradeTxn_.sequenceBagStableSinceTick = now;
                SetTradeStatus(L"TRADE WORKFLOW • MAIN FreeBagSpace vừa đổi → reset cửa sổ ổn định 1500ms");
                return true;
            }
            const bool stableEnough = Elapsed(now, tradeTxn_.sequenceBagStableSinceTick, kTradeBagStableMs);
            const bool verifyTimedOut = Elapsed(now, tradeTxn_.sequenceBagVerifyStartedTick, kTradeBagVerifyMaxMs);
            if (!stableEnough && !verifyTimedOut) return true;

            const int beforeFree = tradeTxn_.sequenceMainFreeBeforePass;
            const int afterFree = tradeTxn_.sequenceBagLastFree;
            const int receivedSlots = ReceivedSlots(beforeFree, afterFree);
            if (DecidePass(beforeFree, afterFree) == PassDecision::RepeatSameChild &&
                !CanStartTradePass(afterFree)) {
                LogAccount(main, L"GD PASS ĐỦ • MAIN " + std::to_wstring(beforeFree) + L"→" +
                                 std::to_wstring(afterFree) + L" • delta=" + std::to_wstring(receivedSlots) +
                                 L" (>=9) nhưng FreeBag còn <9 → CON hiện tại về train, MAIN đi bán.");
                YieldActiveTradeForMainSell(now, L"pass vừa nhận đủ >=9 item nhưng FreeBag MAIN còn <9", receivedSlots);
                return true;
            }
            if (DecidePass(beforeFree, afterFree) == PassDecision::RepeatSameChild) {
                ++tradeTxn_.sequencePass;
                tradeTxn_.sequenceIndex = 0;
                tradeTxn_.sequenceRepeatDone = 0;
                tradeTxn_.sequenceGroupRepeatDone = 0;
                tradeTxn_.sequenceDueTick = 0;
                tradeTxn_.sequenceMainFreeBeforePass = -1;
                tradeTxn_.sequenceBagVerifyStartedTick = 0;
                tradeTxn_.sequenceBagStableSinceTick = 0;
                tradeTxn_.sequenceBagLastFree = -1;

                // Keep the SAME active CON/FIFO exactly like v1.4. The only modern adaptation
                // is target MAIN RoleID again before re-running the plain saved macro.
                tradeTxn_.phase = TradePhase::TargetMain;
                tradeTxn_.targetStartedTick = now;
                tradeTxn_.targetRetryTick = now + 250;
                tradeTxn_.targetLastSelectTick = 0;
                tradeTxn_.targetAttempts = 0;
                SetTradeStatus(L"TRADE WORKFLOW • v1.4 LOGIC • CON" + std::to_wstring(tradeTxn_.childSlot) +
                               L" • MAIN nhận " + std::to_wstring(receivedSlots) +
                               L" slot (>8) → GIỮ nguyên CON/FIFO • TARGET LẠI MAIN ID trước pass " +
                               std::to_wstring(tradeTxn_.sequencePass));
                LogAccount(child, L"GD CÒN ĐỒ v1.4 • MAIN " + std::to_wstring(beforeFree) + L"→" +
                                  std::to_wstring(afterFree) + L" • delta=" + std::to_wstring(receivedSlots) +
                                  L" (>8) → KHÔNG quét FULL CON lại; target MAIN lại trước macro pass " +
                                  std::to_wstring(tradeTxn_.sequencePass) + L".");
                return true;
            }
            LogAccount(main, L"GD PASS CUỐI v1.4 • MAIN " + std::to_wstring(beforeFree) + L"→" +
                             std::to_wstring(afterFree) + L" • delta=" + std::to_wstring(receivedSlots) +
                             L" (≤8) → suy luận CON hiện tại hết đồ và kết thúc workflow CON.");
            FinishTrade(now, receivedSlots);
            return true;
        }
        TradeSequenceStep& stored = seq[tradeTxn_.sequenceIndex];
        const TradeSequenceStep* effective = &stored;
        Account* target = &child;
        if (stored.target == 1) {
            effective = ResolveMainReference(stored);
            target = &main;
            if (!effective) { AbortTrade(L"MAIN reference hỏng tại bước " + std::to_wstring(tradeTxn_.sequenceIndex + 1), now); return false; }
        }
        if (tradeTxn_.sequenceDueTick != 0 && static_cast<LONG>(now - tradeTxn_.sequenceDueTick) < 0) return true;
        std::wstring error;
        const std::wstring who = stored.target == 1 ? L"MAIN" : (L"CON" + std::to_wstring(tradeTxn_.childSlot));
        SetTradeStatus(L"HIDDEN REQUEST → " + who + L" • bước " +
                       std::to_wstring(tradeTxn_.sequenceIndex + 1) + L"/" +
                       std::to_wstring(seq.size()) + L" • InputSync");
        if (!CoordinatorInternalPointAction(
                *target, effective->point,
                L"GD ACC CON dùng chung → " + TradeRoleLabel(child.profile.tradeRole) +
                    L" • bước " + std::to_wstring(tradeTxn_.sequenceIndex + 1),
                error)) {
            AbortTrade(L"BĐPT hidden action bước " +
                       std::to_wstring(tradeTxn_.sequenceIndex + 1) +
                       L" FAIL: " + error, now);
            return false;
        }
        ++tradeTxn_.sequenceRepeatDone;
        tradeTxn_.sequenceDueTick = GetTickCount() + static_cast<DWORD>(effective->delayMs);
        const int repeatLimit = effective->repeat;
        if (tradeTxn_.sequenceRepeatDone >= repeatLimit) {
            tradeTxn_.sequenceRepeatDone = 0;
            if (stored.groupId > 0) {
                const std::size_t groupStart = TradeGroupStart(seq, tradeTxn_.sequenceIndex);
                const std::size_t groupEnd = TradeGroupEnd(seq, tradeTxn_.sequenceIndex);
                if (tradeTxn_.sequenceIndex < groupEnd) {
                    ++tradeTxn_.sequenceIndex;
                } else {
                    ++tradeTxn_.sequenceGroupRepeatDone;
                    const int groupLimit = std::max(1, seq[groupStart].groupRepeat);
                    if (tradeTxn_.sequenceGroupRepeatDone < groupLimit) {
                        tradeTxn_.sequenceIndex = groupStart;
                        SetTradeStatus(L"NHÓM G" + std::to_wstring(stored.groupId) + L" • lặp " +
                                       std::to_wstring(tradeTxn_.sequenceGroupRepeatDone + 1) + L"/" + std::to_wstring(groupLimit));
                    } else {
                        tradeTxn_.sequenceIndex = groupEnd + 1;
                        tradeTxn_.sequenceGroupRepeatDone = 0;
                    }
                }
            } else {
                ++tradeTxn_.sequenceIndex;
                tradeTxn_.sequenceGroupRepeatDone = 0;
            }
        }
        return true;
    }

    void TickTradeCoordinator(DWORD now) {
        // v0.6.1.6: SELL on an unrelated PID never stalls the trade coordinator.
        // The active MAIN/CON pair is protected by tradeTxn_/tradeHeld only; SELL keeps
        // its own per-account macro ordering and has no cross-window input ownership.

        if (!tradeEnabled_) {
            if (tradeTxn_.phase != TradePhase::Idle || !tradeQueuePids_.empty())
                AbortTrade(L"DỒN ĐỒ đang TẮT", now);
            SetTradeStatus(L"DỒN ĐỒ TẮT • AUTO TRAIN/BÁN ĐỒ ĐỘC LẬP");
            return;
        }
        if (!tradeRendezvous_.valid) {
            if (!tradeQueuePids_.empty()) AbortTrade(L"mất TỌA GD đã lưu", now);
            SetTradeStatus(L"chưa GET TỌA GD • chọn acc đang đứng điểm GD rồi bấm TỌA GD • LẤY");
            return;
        }

        Account* main = AccountByTradeRole(1);
        if (!main) {
            if (!tradeQueuePids_.empty() || tradeTxn_.phase != TradePhase::Idle) AbortTrade(L"mất MAIN", now);
            SetTradeStatus(L"chưa chọn MAIN");
            return;
        }

        // If an active transaction exists, its MAIN identity must remain stable.
        if (tradeTxn_.phase != TradePhase::Idle && tradeTxn_.mainPid != main->game.pid) {
            AbortTrade(L"MAIN bị đổi giữa workflow", now);
            return;
        }

        // v0.5 World Flow recovery barrier: while traveling/rendezvous (not inside the
        // atomic trade click Sequence), a held account that dies keeps its FIFO/hold slot.
        // P2 revive and the life observer run outside this coordinator; when ALIVE returns,
        // RuntimeState travel phases restart cleanly and World Flow resumes toward TỌA GD.
        auto worldFlowLifeRecovery = [&](const Account* item) {
            if (!item || !item->tradeHeld || tradeTxn_.phase == TradePhase::Sequence) return false;
            const bool deadNow = item->snapshotValid && (item->snapshot.validMask & ValidLifeState) && item->snapshot.dead;
            return deadNow || item->deathSessionLatched || item->runtime.revivePhase != 0;
        };
        if (worldFlowLifeRecovery(main)) {
            SetTradeStatus(L"WORLD FLOW PAUSE • MAIN chết/đang Đầu thai • giữ FIFO/HOLD • LIFE P2 ưu tiên");
            return;
        }
        for (DWORD pid : tradeQueuePids_) {
            Account* queued = AccountByPid(pid);
            if (!worldFlowLifeRecovery(queued)) continue;
            const int slot = queued ? std::max(1, queued->profile.tradeRole - 1) : 0;
            SetTradeStatus(L"WORLD FLOW PAUSE • CON" + std::to_wstring(slot) + L" chết/đang Đầu thai • giữ FIFO/HOLD");
            return;
        }

        const bool mainBaseHealthy = main->runtime.running && main->snapshotValid && IsWindow(main->game.window) &&
            (main->snapshot.validMask & (ValidLifeState | ValidBagSpace | ValidMap | ValidPosition)) ==
                (ValidLifeState | ValidBagSpace | ValidMap | ValidPosition) &&
            !main->snapshot.dead;
        if (!mainBaseHealthy) {
            if (!tradeQueuePids_.empty() || tradeTxn_.phase != TradePhase::Idle)
                AbortTrade(L"MAIN dừng/mất state/chết trong workflow", now);
            else SetTradeStatus(L"MAIN chưa ở state rảnh/an toàn");
            return;
        }
        // MAIN capacity is checked only between full macro passes. Once Sequence starts,
        // never interrupt it: every pass must run the complete saved MAIN/CON macro.
        if (tradeTxn_.phase != TradePhase::Sequence && !CanStartTradePass(main->snapshot.freeBagSpace)) {
            if (tradeTxn_.phase != TradePhase::Idle) {
                YieldActiveTradeForMainSell(now, L"FreeBag MAIN <9 trước pass mới");
            } else {
                if (main->tradeHeld) ReleaseTradeHold(*main);
                SetTradeStatus(L"MAIN <9 ô → nhả MAIN cho AUTO SELL • giữ " +
                               std::to_wstring(tradeQueuePids_.size()) + L" CON FIFO đứng chờ");
            }
            return;
        }
        // When no click transaction is active, MAIN must finish its own sell/revive/recovery
        // work before World Flow can reclaim it. Queued CONs keep their FIFO/HOLD tickets
        // and remain parked while MAIN sells; this prevents a rising FreeBag snapshot during
        // an in-progress sell from interrupting Auto Sell and pulling MAIN back too early.
        if (tradeTxn_.phase == TradePhase::Idle && !main->tradeHeld && !TradeStateReady(*main)) {
            SetTradeStatus(main->runtime.sellPhase != 0
                               ? L"MAIN đang AUTO SELL • giữ CON FIFO đứng chờ"
                               : L"MAIN chưa ở state rảnh/an toàn • giữ FIFO hiện có");
            return;
        }

        // Remove waiting children that can no longer participate. The active child remains
        // fail-closed and is handled below so a broken in-flight transaction aborts cleanly.
        for (std::size_t i = 0; i < tradeQueuePids_.size();) {
            const DWORD pid = tradeQueuePids_[i];
            if (pid == tradeTxn_.childPid && tradeTxn_.phase != TradePhase::Idle) { ++i; continue; }
            Account* child = AccountByPid(pid);
            const bool validRole = child && child->profile.tradeRole >= kFirstChildTradeRole &&
                                   child->profile.tradeRole <= kLastChildTradeRole;
            const bool heldPriorityRecovery = child && child->tradeHeld &&
                (child->deathSessionLatched || child->runtime.revivePhase != 0 ||
                 (child->snapshotValid && (child->snapshot.validMask & ValidLifeState) && child->snapshot.dead));
            // A non-active queued traveler may die while another pair owns the atomic trade
            // Sequence. Keep its FIFO/HOLD slot; P2/life recovery runs globally and that CON
            // resumes rendezvous travel after ALIVE instead of being silently dropped.
            if (validRole && heldPriorityRecovery) { ++i; continue; }
            const bool alive = child && child->runtime.running && IsWindow(child->game.window) && child->snapshotValid &&
                ((child->snapshot.validMask & ValidLifeState) == 0 || !child->snapshot.dead);
            if (validRole && alive) { ++i; continue; }
            if (child) {
                LogAccount(*child, L"GD QUEUE: rời hàng đợi vì dừng/chết/đổi role • nhả HOLD cho core xử lý.");
                ReleaseTradeHold(*child);
            }
            tradeQueuePids_.erase(tradeQueuePids_.begin() + static_cast<std::ptrdiff_t>(i));
        }

        std::wstring sequenceReason;
        const bool sequenceReady = TradeSequenceReady(sequenceReason);

        // Fill up to three travel/wait slots. Admission scan is deliberately CON1→CON12, so
        // children that become FULL in the same scheduler tick enter by the smallest slot first.
        // Once staged, vector order is strict FIFO by workflow-entry time and is NEVER re-sorted:
        // e.g. an already-staged CON3 remains ahead of a CON1 that becomes FULL later. FULL is
        // only the entry gate; later bag deltas do not kick a staged child out.
        if (sequenceReady && tradeTxn_.phase != TradePhase::Sequence) {
            for (int slot = 1; slot <= kChildTradeCount && tradeQueuePids_.size() < kMaxQueuedChildren; ++slot) {
                Account* child = AccountByTradeRole(slot + 1);
                if (!child || TradeQueueContains(child->game.pid)) continue;
                if (!ShouldAdmitFullChild(TradeStateReady(*child), child->snapshot.freeBagSpace,
                                          tradeQueuePids_.size())) continue;

                const bool firstQueued = tradeQueuePids_.empty();
                tradeQueuePids_.push_back(child->game.pid);
                child->runtime.tradeWorkflowEntrySeq = ++tradeWorkflowEntryCounter_;
                child->tradeHeld = true;
                const std::wstring childTag = L"CON" + std::to_wstring(slot);
                BeginTradeRendezvousTravel(*child, now, childTag.c_str());

                if (firstQueued) {
                    // Only the first queued child needs to pull MAIN away from train. Adding
                    // CON2/CON3 must never restart MAIN's rendezvous state.
                    main->tradeHeld = true;
                    BeginTradeRendezvousTravel(*main, now, L"MAIN");
                }

                LogAccount(*child, childTag + L" FULL → vào FIFO workflow vị trí " +
                           std::to_wstring(tradeQueuePids_.size()) + L"/3 • vé #" +
                           std::to_wstring(child->runtime.tradeWorkflowEntrySeq) +
                           L" • đồng thời FULL thì ưu tiên CON số nhỏ; đã có vé rồi thì CON vào sau không được chen.");
            }
        }

        if (!sequenceReady && tradeQueuePids_.empty() && tradeTxn_.phase == TradePhase::Idle) {
            // Only surface the missing-sequence error when a FULL child actually exists.
            for (int slot = 1; slot <= kChildTradeCount; ++slot) {
                Account* child = AccountByTradeRole(slot + 1);
                if (child && TradeStateReady(*child) && child->snapshot.freeBagSpace <= 0) {
                    SetTradeStatus(L"CON" + std::to_wstring(slot) + L" FULL nhưng " + sequenceReason + L" • mở CHUỖI GD ACC CON");
                    return;
                }
            }
        }

        // TRADE WORKFLOW LOCK protects the business ordering of the point-based
        // trade macro. Every dispatch is now a per-client Bridge action, so queued
        // travelers keep progressing without borrowing the Windows mouse.
        if (!tradeQueuePids_.empty()) {
            if (tradeTxn_.phase == TradePhase::Idle || tradeTxn_.phase == TradePhase::Rendezvous) {
                main->tradeHeld = true;
                (void)HandleTradeRendezvousTravel(*main, now, L"MAIN");
                for (DWORD pid : tradeQueuePids_) {
                    Account* child = AccountByPid(pid);
                    if (!child) continue;
                    child->tradeHeld = true;
                    const int slot = std::max(1, child->profile.tradeRole - 1);
                    const std::wstring tag = L"CON" + std::to_wstring(slot);
                    (void)HandleTradeRendezvousTravel(*child, now, tag.c_str());
                }
            } else {
                for (DWORD pid : tradeQueuePids_) {
                    // The active trade pair must stay parked at TỌA GD for click safety.
                    // Only the other queued CON accounts are allowed to keep AutoPath moving.
                    if (pid == tradeTxn_.childPid) continue;
                    Account* child = AccountByPid(pid);
                    if (!child) continue;
                    child->tradeHeld = true;
                    const int slot = std::max(1, child->profile.tradeRole - 1);
                    const std::wstring tag = L"CON" + std::to_wstring(slot);
                    (void)HandleTradeRendezvousTravel(*child, now, tag.c_str());
                }
            }
        }

        // R7 FIFO invariant: while still in Rendezvous (before any trade click), the
        // active CON must be the account with the oldest immutable workflow-entry ticket.
        // This fixes cases where a later traveler could remain selected while an earlier CON
        // was already parked and waiting at the rendezvous.
        RelockRendezvousToEarliestQueued(now);

        Account* activeMain = tradeTxn_.phase == TradePhase::Idle ? nullptr : AccountByPid(tradeTxn_.mainPid);
        Account* activeChild = tradeTxn_.phase == TradePhase::Idle ? nullptr : AccountByPid(tradeTxn_.childPid);

        if (tradeTxn_.phase == TradePhase::Rendezvous) {
            if (!activeMain || !activeChild || !TradeQueueContains(activeChild->game.pid) ||
                !IsWindow(activeMain->game.window) || !IsWindow(activeChild->game.window)) {
                AbortTrade(L"mất cửa sổ/acc/hàng đợi khi đi TỌA GD", now); return;
            }
            if (!activeMain->runtime.running || !activeChild->runtime.running ||
                !activeMain->snapshotValid || !activeChild->snapshotValid) {
                AbortTrade(L"MAIN/CON dừng hoặc mất snapshot khi đi TỌA GD", now); return;
            }
            const Snapshot& ms = activeMain->snapshot;
            const Snapshot& cs = activeChild->snapshot;
            if (((ms.validMask & ValidLifeState) && ms.dead) || ((cs.validMask & ValidLifeState) && cs.dead)) {
                AbortTrade(L"MAIN/CON chết khi đi TỌA GD • nhả HOLD để core xử lý đầu thai", now); return;
            }

            const bool mainReady = activeMain->runtime.tradeTravelReady && TradeAccountAtRendezvous(*activeMain);
            const bool childReady = activeChild->runtime.tradeTravelReady && TradeAccountAtRendezvous(*activeChild);
            if (activeMain->runtime.autoPathFightConflictLatched ||
                activeChild->runtime.autoPathFightConflictLatched) {
                SetTradeStatus(L"GD TỌA • chờ MAIN/CON hoàn tất ROUTE/FIGHT INVARIANT recovery");
                return;
            }
            if (!mainReady || !childReady) {
                SetTradeStatus(L"QUEUE " + std::to_wstring(tradeQueuePids_.size()) + L"/3 • MAIN " +
                               std::wstring(mainReady ? L"ĐÃ TỚI" : L"ĐANG ĐI") + L" • CON" +
                               std::to_wstring(tradeTxn_.childSlot) + L" " +
                               std::wstring(childReady ? L"ĐÃ TỚI" : L"ĐANG ĐI") +
                               L" • XN Lâu Lan watchdog chỉ chạy khi M5 bị kẹt cổng");
                return;
            }

            if (!sequenceReady) { AbortTrade(L"chuỗi click GD chưa sẵn sàng: " + sequenceReason, now); return; }

            // MAIN + active CON are parked at TỌA GD. Target/verify MAIN by RoleID first.
            // No sequence row is dispatched here and no row index has special meaning.
            tradeTxn_.phase = TradePhase::TargetMain;
            tradeTxn_.targetStartedTick = now;
            tradeTxn_.targetRetryTick = now;
            tradeTxn_.targetLastSelectTick = 0;
            tradeTxn_.targetAttempts = 0;
            SetTradeStatus(L"TRADE WORKFLOW • CON" + std::to_wstring(tradeTxn_.childSlot) +
                           L" đã tới TỌA GD • auto-target MAIN theo RoleID trước macro");
            return;
        }

        if (tradeTxn_.phase == TradePhase::TargetMain) {
            if (!activeMain || !activeChild || !TradeQueueContains(activeChild->game.pid) ||
                !TradePairReadyForPreparation(*activeMain, *activeChild)) {
                AbortTrade(L"mất acc/state khi target MAIN", now); return;
            }
            if (activeMain->snapshot.roleID <= 0) {
                AbortTrade(L"RoleID MAIN không hợp lệ khi target", now); return;
            }
            if (activeMain->runtime.autoPathFightConflictLatched || activeChild->runtime.autoPathFightConflictLatched) {
                SetTradeStatus(L"TARGET MAIN • pause khi ROUTE/FIGHT INVARIANT đang recovery");
                return;
            }
            if (!TradeAccountAtRendezvous(*activeMain) || !TradeAccountAtRendezvous(*activeChild)) {
                AbortTrade(L"MAIN/CON rời TỌA GD trước khi target MAIN", now); return;
            }
            if (Elapsed(now, tradeTxn_.targetStartedTick, kTradeTargetTimeoutMs)) {
                AbortTrade(L"timeout target MAIN theo RoleID", now); return;
            }
            if (tradeTxn_.targetRetryTick != 0 && static_cast<LONG>(now - tradeTxn_.targetRetryTick) < 0) return;

            Response response{};
            std::wstring error;
            ++tradeTxn_.targetAttempts;
            // v1.5.3: verification polls are read-only. Permit SelectTarget only on the
            // first successful request, then at most one re-select every 2 seconds.
            // This prevents the v1.5.2 pattern of mutating the client's target every timer tick.
            const bool allowSelect = tradeTxn_.targetLastSelectTick == 0 ||
                                     Elapsed(now, tradeTxn_.targetLastSelectTick, 2000);
            const bool ok = activeChild->bridge.Call(Command::SelectTargetByRoleID,
                                                     activeMain->snapshot.roleID, allowSelect ? 1 : 0, 0,
                                                     response, error, 1100);
            if (!ok) {
                LogAccount(*activeChild, L"TARGET MAIN lần " + std::to_wstring(tradeTxn_.targetAttempts) +
                                          L" chưa chạy được • " + error);
                tradeTxn_.targetRetryTick = GetTickCount() + kTradeTargetRetryMs;
                return;
            }
            if (response.value1 != 0) tradeTxn_.targetLastSelectTick = GetTickCount();
            if (response.resultCode != static_cast<std::int32_t>(ActionResult::ActionInvoked)) {
                tradeTxn_.targetRetryTick = GetTickCount() + kTradeTargetRetryMs;
                SetTradeStatus(response.value1 != 0
                                   ? L"TARGET MAIN • đã SelectTarget 1 lần • chờ verify RoleID"
                                   : L"TARGET MAIN • verify read-only • chờ SelectedTarget.RoleID khớp MAIN");
                return;
            }

            // Target verified: start the saved macro from its current first row. Deleting,
            // inserting or moving rows only changes order; no row is reserved by the FSM.
            tradeTxn_.phase = TradePhase::Sequence;
            tradeTxn_.sequenceIndex = 0;
            tradeTxn_.sequenceRepeatDone = 0;
            tradeTxn_.sequenceGroupRepeatDone = 0;
            tradeTxn_.sequenceDueTick = GetTickCount() + 100;
            tradeTxn_.sequenceMainFreeBeforePass = activeMain->snapshot.freeBagSpace;
            tradeTxn_.sequenceBagVerifyStartedTick = 0;
            tradeTxn_.sequenceBagStableSinceTick = 0;
            tradeTxn_.sequenceBagLastFree = -1;
            LogAccount(*activeChild, L"TARGET MAIN PASS " + std::to_wstring(tradeTxn_.sequencePass) + L" • " + std::wstring(response.detail) +
                                      L" • FreeBagSpace MAIN trước pass=" +
                                      std::to_wstring(tradeTxn_.sequenceMainFreeBeforePass) +
                                      L" • bắt đầu macro động từ bước đầu.");
            SetTradeStatus(L"TRADE WORKFLOW • target MAIN PASS • bắt đầu chuỗi GD CON" +
                           std::to_wstring(tradeTxn_.childSlot) + L" từ bước đầu • queued " +
                           std::to_wstring(tradeQueuePids_.size()) + L"/3");
            return;
        }

        if (tradeTxn_.phase == TradePhase::Sequence) {
            if (!activeMain || !activeChild || !TradePairReadyForPreparation(*activeMain, *activeChild)) {
                AbortTrade(L"mất acc/state trong chuỗi giao dịch", now); return;
            }
            if (activeMain->runtime.autoPathFightConflictLatched ||
                activeChild->runtime.autoPathFightConflictLatched) {
                SetTradeStatus(L"TRADE WORKFLOW • pause action khi ROUTE/FIGHT INVARIANT đang recovery");
                return;
            }
            if (activeMain->snapshot.autoPathing || activeChild->snapshot.autoPathing) {
                for (Account* a : {activeMain, activeChild}) {
                    if (!a || !a->snapshot.autoPathing || !a->bridge.Attached()) continue;
                    Response r{}; std::wstring ignored;
                    (void)a->bridge.Call(Command::StopPath, 0, 0, 0, r, ignored, 700);
                }
                SetTradeStatus(L"TRADE WORKFLOW • phát hiện AutoPath bật lại tại TỌA GD → StopPath trước action tiếp");
                return;
            }
            if (!TradeAccountAtRendezvous(*activeMain) || !TradeAccountAtRendezvous(*activeChild)) {
                AbortTrade(L"MAIN/CON rời TỌA GD giữa chuỗi • fail-closed để tránh click nhầm", now); return;
            }
            (void)ExecuteTradeSequenceTick(*activeMain, *activeChild, now);
            return;
        }

        if (tradeTxn_.phase != TradePhase::Idle) {
            AbortTrade(L"state giao dịch không hợp lệ", now); return;
        }

        // No click transaction is active. A queue slot may be refilled immediately, even
        // during the existing post-trade cooldown, so the next FULL child can start travel
        // as soon as the previous child has been released to run back to train.
        if (tradeQueuePids_.empty()) {
            if (main->tradeHeld) ReleaseTradeHold(*main);
            SetTradeStatus(L"IDLE • chưa có CON FULL");
            return;
        }

        if (tradeTxn_.cooldownUntil != 0 && static_cast<LONG>(now - tradeTxn_.cooldownUntil) < 0) {
            SetTradeStatus(L"QUEUE " + std::to_wstring(tradeQueuePids_.size()) + L"/3 • chờ cooldown giữa 2 giao dịch");
            return;
        }

        Account* nextChild = EarliestQueuedChild();
        if (!nextChild || nextChild->profile.tradeRole < kFirstChildTradeRole ||
            nextChild->profile.tradeRole > kLastChildTradeRole) {
            if (nextChild) {
                const DWORD badPid = nextChild->game.pid;
                ReleaseTradeHold(*nextChild);
                RemoveTradeQueuePid(badPid);
            } else if (!tradeQueuePids_.empty()) {
                tradeQueuePids_.erase(tradeQueuePids_.begin());
            }
            return;
        }

        tradeTxn_.mainPid = main->game.pid;
        tradeTxn_.childPid = nextChild->game.pid;
        tradeTxn_.childSlot = nextChild->profile.tradeRole - 1;
        tradeTxn_.cooldownUntil = 0;
        tradeTxn_.sequenceIndex = 0;
        tradeTxn_.sequenceRepeatDone = 0;
        tradeTxn_.sequenceGroupRepeatDone = 0;
        tradeTxn_.sequencePass = 1;
        tradeTxn_.sequenceDueTick = 0;
        tradeTxn_.targetStartedTick = 0;
        tradeTxn_.targetRetryTick = 0;
        tradeTxn_.targetLastSelectTick = 0;
        tradeTxn_.targetAttempts = 0;
        tradeTxn_.phase = TradePhase::Rendezvous;

        SetTradeStatus(L"QUEUE " + std::to_wstring(tradeQueuePids_.size()) + L"/3 • tới lượt CON" +
                       std::to_wstring(tradeTxn_.childSlot) + L" • chờ MAIN+CON cùng tới TỌA GD");
    }

    void ClearEditor() {
        SetText(selected_, L"ACC ĐANG CHỈNH: chưa chọn");
        SetText(live_, L"STATE: chưa có");
        if (tradeRoleCombo_) SendMessageW(tradeRoleCombo_, CB_SETCURSEL, 0, 0);
        SetText(targetName_, L"");
        if (spotCombo_) SendMessageW(spotCombo_, CB_SETCURSEL, -1, 0);
        SetText(targetText_, L"CHƯA CHỌN");
        SetText(tolerance_, L"120");
        SendMessageW(enableRevive_, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(enableConfirm_, BM_SETCHECK, BST_UNCHECKED, 0);
        SetText(rotateDeathLimit_, std::to_wstring(kRotateDeathLimitDefault));
        SetText(rotateDeathWindow_, std::to_wstring(kRotateDeathWindowMinDefault));
        SetText(rotateNoFullBag_, std::to_wstring(kRotateNoFullBagMinDefault));
        if (rotationList_) ListView_DeleteAllItems(rotationList_);
        SendMessageW(enableFight_, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(enableSell_, BM_SETCHECK, BST_UNCHECKED, 0);
        if (sellNpcCombo_) SendMessageW(sellNpcCombo_, CB_SETCURSEL, 0, 0);
        SetText(sellNpcX_, L"");
        SetText(sellNpcY_, L"");
        SetText(sellNpcPosText_, L"CHƯA LẤY");
        for (HWND h : pointLabels_) if (h) SetText(h, L"CHƯA LẤY");
        RefreshSellMacroList();
        ClearSellMacroEditor();
        UpdateRoleActionButtons();
    }

    void LoadSelectedProfileToUi() {
        Account* a = SelectedAccount();
        if (!a) { ClearEditor(); return; }
        ResolveProfileTarget(a->profile);
        SetText(selected_, L"ACC ĐANG CHỈNH: " + AccountTag(*a));
        if (tradeRoleCombo_) SendMessageW(tradeRoleCombo_, CB_SETCURSEL, a->profile.tradeRole, 0);
        RefreshSpotCombo();
        RefreshRotationList();
        SetText(targetName_, a->profile.selectedSpot);
        SetText(tolerance_, std::to_wstring(a->profile.tolerance));
        SetText(rotateDeathLimit_, std::to_wstring(a->profile.rotateDeathLimit));
        SetText(rotateDeathWindow_, std::to_wstring(a->profile.rotateDeathWindowMin));
        SetText(rotateNoFullBag_, std::to_wstring(a->profile.rotateNoFullBagMin));
        SendMessageW(enableRevive_, BM_SETCHECK, a->profile.enableRevive ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(enableConfirm_, BM_SETCHECK, a->profile.enableConfirm ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(enableFight_, BM_SETCHECK, a->profile.enableFight ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(enableSell_, BM_SETCHECK, a->profile.enableSell ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(sellNpcCombo_, CB_SETCURSEL, a->profile.sellNpcPreset, 0);
        LoadSellNpcPositionToUi(*a);
        if (a->profile.target.valid) {
            SetText(targetText_, L"M" + std::to_wstring(a->profile.target.mapID) + L" • " +
                                std::to_wstring(a->profile.target.x) + L"," + std::to_wstring(a->profile.target.y));
        } else {
            SetText(targetText_, L"CHƯA CHỌN");
        }
        for (int i : {static_cast<int>(ClickSlot::AutoMenu), static_cast<int>(ClickSlot::Attack), static_cast<int>(ClickSlot::StopAuto2)}) {
            if (pointLabels_[static_cast<std::size_t>(i)])
                SetText(pointLabels_[static_cast<std::size_t>(i)], PointDescription(a->profile.points[static_cast<std::size_t>(i)]));
        }
        RefreshSellMacroList();
        ClearSellMacroEditor();
        UpdateRoleActionButtons();
        UpdateSelectedLive();
    }

    void PersistSelectedEditor() {
        Account* a = SelectedAccount();
        if (!a) return;
        int tol = _wtoi(GetText(tolerance_).c_str());
        if (tol < 20) tol = 20;
        if (tol > 2000) tol = 2000;
        a->profile.tolerance = tol;
        a->profile.enableRevive = SendMessageW(enableRevive_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        a->profile.enableConfirm = SendMessageW(enableConfirm_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        int deathLimit = _wtoi(GetText(rotateDeathLimit_).c_str());
        if (deathLimit < kRotateDeathLimitMin) deathLimit = kRotateDeathLimitMin;
        if (deathLimit > kRotateDeathLimitMax) deathLimit = kRotateDeathLimitMax;
        int deathWindow = _wtoi(GetText(rotateDeathWindow_).c_str());
        if (deathWindow < kRotateWindowMin) deathWindow = kRotateWindowMin;
        if (deathWindow > kRotateWindowMax) deathWindow = kRotateWindowMax;
        int noBagWindow = _wtoi(GetText(rotateNoFullBag_).c_str());
        if (noBagWindow < kRotateWindowMin) noBagWindow = kRotateWindowMin;
        if (noBagWindow > kRotateWindowMax) noBagWindow = kRotateWindowMax;
        a->profile.rotateDeathLimit = deathLimit;
        a->profile.rotateDeathWindowMin = deathWindow;
        a->profile.rotateNoFullBagMin = noBagWindow;
        SetText(rotateDeathLimit_, std::to_wstring(deathLimit));
        SetText(rotateDeathWindow_, std::to_wstring(deathWindow));
        SetText(rotateNoFullBag_, std::to_wstring(noBagWindow));
        PersistRotationListFromUi(*a);
        a->profile.enableFight = SendMessageW(enableFight_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        a->profile.enableSell = SendMessageW(enableSell_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        PersistSellNpcPositionEditor(*a);
        const LRESULT sellSel = SendMessageW(sellNpcCombo_, CB_GETCURSEL, 0, 0);
        if (sellSel != CB_ERR && sellSel >= 0 && sellSel < static_cast<LRESULT>(kSellNpcs.size())) a->profile.sellNpcPreset = static_cast<int>(sellSel);
        SaveProfile(a->profile);
        const int row = SelectedIndex();
        if (row >= 0) UpdateAccountRow(row, *a);
    }

    void SaveTargetForSelected() {
        Account* a = SelectedAccount();
        if (!a) { Log(L"Chưa chọn acc"); return; }
        PersistSelectedEditor();
        std::wstring error;
        if (!ReadSnapshot(*a, error, 1200)) { LogAccount(*a, L"Không đọc được state để lưu bãi: " + error); return; }
        const Snapshot& s = a->snapshot;
        if (!s.mapReady || s.waitingChangeMap ||
            (s.validMask & (ValidMap | ValidPosition)) != (ValidMap | ValidPosition)) {
            LogAccount(*a, L"State chưa ổn định, không lưu bãi.");
            return;
        }
        std::wstring name = GetText(targetName_);
        if (name.empty()) name = L"Bãi M" + std::to_wstring(s.mapID) + L" " + std::to_wstring(s.x) + L"," + std::to_wstring(s.y);
        TargetProfile spot{name, s.mapID, s.x, s.y, true};
        const int existing = FindSpotIndex(spots_, name);
        if (existing >= 0) spots_[static_cast<std::size_t>(existing)] = spot;
        else spots_.push_back(spot);
        SaveSharedSpots(spots_);
        a->profile.selectedSpot = name;
        a->profile.target = spot;
        ApplyAutoSellerForTrainingTarget(*a);
        a->profile.rotationSpots.clear();
        a->profile.rotationSpots.push_back(name);
        NormalizeRotationProfile(a->profile);
        SaveProfile(a->profile);
        RefreshSpotCombo();
        LoadSelectedProfileToUi();
        for (std::size_t i = 0; i < accounts_.size(); ++i) {
            if (_wcsicmp(accounts_[i]->profile.selectedSpot.c_str(), name.c_str()) == 0) {
                accounts_[i]->profile.target = spot;
                ApplyAutoSellerForTrainingTarget(*accounts_[i], false);
                SaveProfile(accounts_[i]->profile);
                UpdateAccountRow(static_cast<int>(i), *accounts_[i]);
            }
        }
        LogAccount(*a, L"COORD CAPTURE RAW TRAIN • đã lưu/cập nhật bãi CHUNG: " + name + L" • M" + std::to_wstring(s.mapID) + L" • " +
                       std::to_wstring(s.x) + L"," + std::to_wstring(s.y));
    }

    void UpdateTradeRendezvousLabel() {
        if (!tradeRendezvousLabel_) return;
        if (!tradeRendezvous_.valid) {
            SetText(tradeRendezvousLabel_, L"CHƯA LẤY TỌA GD");
            return;
        }
        SetText(tradeRendezvousLabel_, L"M" + std::to_wstring(tradeRendezvous_.mapID) + L" • " +
                                      std::to_wstring(tradeRendezvous_.x) + L"," + std::to_wstring(tradeRendezvous_.y));
    }

    void CaptureTradeRendezvous() {
        Account* source = SelectedAccount();
        if (!source) source = AccountByTradeRole(1);
        if (!source) { Log(L"TỌA GD: hãy chọn một acc hoặc gán MAIN trước."); return; }
        std::wstring error;
        if (!ReadSnapshot(*source, error, 1200)) { LogAccount(*source, L"TỌA GD: không đọc được state: " + error); return; }
        const Snapshot& state = source->snapshot;
        if (!state.mapReady || state.waitingChangeMap ||
            (state.validMask & (ValidMap | ValidPosition)) != (ValidMap | ValidPosition)) {
            LogAccount(*source, L"TỌA GD: state Map/X/Y chưa ổn định, không lưu.");
            return;
        }
        tradeRendezvous_.name = L"TỌA GD";
        tradeRendezvous_.mapID = state.mapID;
        tradeRendezvous_.x = state.x;
        tradeRendezvous_.y = state.y;
        tradeRendezvous_.valid = true;
        WriteIniInt(L"Global", L"TradeRendezvousMap", tradeRendezvous_.mapID);
        WriteIniInt(L"Global", L"TradeRendezvousX", tradeRendezvous_.x);
        WriteIniInt(L"Global", L"TradeRendezvousY", tradeRendezvous_.y);
        WriteIniInt(L"Global", L"TradeRendezvousValid", 1);
        WriteIniInt(L"Global", L"TradeRendezvousTolerance", tradeRendezvousTolerance_);
        FlushIni();
        UpdateTradeRendezvousLabel();
        LogAccount(*source, L"COORD CAPTURE RAW GD • M" + std::to_wstring(tradeRendezvous_.mapID) + L" • " +
                            std::to_wstring(tradeRendezvous_.x) + L"," + std::to_wstring(tradeRendezvous_.y) +
                            L" • TOL=" + std::to_wstring(tradeRendezvousTolerance_));
    }

    void BeginCapture(ClickSlot slot) {
        Account* a = SelectedAccount();
        if (!a) { Log(L"Chưa chọn acc để lấy tọa độ"); return; }
        captureSlot_ = slot;
        captureMacroIndex_ = -1;
        captureTradeSequenceIndex_ = -1;
        captureTradeSequenceMode_ = 0;
        captureTradeSequenceMainRef_ = -1;
        capturePid_ = a->game.pid;
        const int index = static_cast<int>(slot);
        LogAccount(*a, L"Đang chờ F8 để lấy điểm " + std::wstring(kClickLabels[static_cast<std::size_t>(index)]) + L".");
        SetText(selected_, L"LẤY TỌA ĐỘ CHO " + AccountTag(*a) + L" • đưa chuột vào nút rồi F8");
    }

    void CaptureHotkeyPoint() {
        const bool hasMode = shortcutKunlunCaptureIndex_ >= 0 || captureSlot_ != ClickSlot::None || captureMacroIndex_ >= 0 ||
                             captureTradeSequenceIndex_ >= 0 || capturePkClickIndex_ >= 0;
        if (!hasMode || capturePid_ == 0) return;
        Account* captureAccount = AccountByPid(capturePid_);
        if (!captureAccount || !IsWindow(captureAccount->game.window)) {
            Log(L"Lấy tọa độ thất bại: acc/cửa sổ đã mất.");
            captureSlot_ = ClickSlot::None; captureMacroIndex_ = -1; capturePid_ = 0;
            captureTradeSequenceIndex_ = -1; captureTradeSequenceMode_ = 0; captureTradeSequenceMainRef_ = -1;
            capturePkStepIndex_ = -1; capturePkClickIndex_ = -1; shortcutKunlunCaptureIndex_ = -1;
            return;
        }
        POINT screen{};
        if (!GetCursorPos(&screen)) return;
        POINT client = screen;
        if (!ScreenToClient(captureAccount->game.window, &client)) return;
        RECT rc{};
        if (!GetClientRect(captureAccount->game.window, &rc)) return;
        const int width = rc.right - rc.left;
        const int height = rc.bottom - rc.top;
        if (client.x < 0 || client.y < 0 || client.x >= width || client.y >= height) {
            LogAccount(*captureAccount, L"F8 bỏ qua: con trỏ không nằm trong client game của acc đích.");
            return;
        }
        const ClickPoint captured{client.x, client.y, width, height, true};
        if (shortcutKunlunCaptureIndex_ >= 0 && shortcutKunlunCaptureIndex_ < 3) {
            const int index = shortcutKunlunCaptureIndex_;
            shortcutSettings_.kunlunExitClicks[static_cast<std::size_t>(index)].point = captured;
            SaveShortcutSettings(shortcutSettings_);
            LoadShortcutSettingsToUi();
            LogAccount(*captureAccount, L"ĐƯỜNG TẮT CLS: đã lưu TryClickUI " + std::to_wstring(index + 1) +
                                         L"/3 dùng chung ALL ACC = " + PointDescription(captured));
        } else if (capturePkClickIndex_ >= 0) {
            if (capturePkStepIndex_ >= 0 && capturePkStepIndex_ < static_cast<int>(autoPkSteps_.size()) &&
                capturePkClickIndex_ < static_cast<int>(autoPkSteps_[static_cast<std::size_t>(capturePkStepIndex_)].clicks.size())) {
                autoPkSteps_[static_cast<std::size_t>(capturePkStepIndex_)].clicks[static_cast<std::size_t>(capturePkClickIndex_)].point = captured;
                SaveAutoPkSettings();
                RefreshAutoPkStepList(capturePkStepIndex_);
                RefreshAutoPkClickList(capturePkClickIndex_);
                LogAccount(*captureAccount, L"AUTO PK F8 • đã lưu click = " + PointDescription(captured));
                SetAutoPkStatus(L"STOP • đã lưu tọa độ click ẩn");
            } else {
                LogAccount(*captureAccount, L"AUTO PK F8 thất bại: bước/click đã thay đổi.");
            }
        } else if (captureTradeSequenceIndex_ >= 0) {
            bool saved = false;
            if (captureTradeSequenceMode_ == 1) {
                if (captureTradeSequenceIndex_ < static_cast<int>(mainTradeSequence_.size())) {
                    mainTradeSequence_[static_cast<std::size_t>(captureTradeSequenceIndex_)].point = captured;
                    SaveMainTradeSequence();
                    saved = true;
                }
            } else if (captureTradeSequenceMode_ == 2) {
                if (captureTradeSequenceMainRef_ >= 0) {
                    if (captureTradeSequenceMainRef_ < static_cast<int>(mainTradeSequence_.size())) {
                        mainTradeSequence_[static_cast<std::size_t>(captureTradeSequenceMainRef_)].point = captured;
                        SaveMainTradeSequence();
                        saved = true;
                    }
                } else {
                    EnsureSharedChildTradeSequence();
                    if (captureTradeSequenceIndex_ < static_cast<int>(childTradeSequence_.size())) {
                        childTradeSequence_[static_cast<std::size_t>(captureTradeSequenceIndex_)].point = captured;
                        SaveSharedChildTradeSequence();
                        saved = true;
                    }
                }
            }

            if (!saved) {
                LogAccount(*captureAccount, L"F8 chuỗi GD thất bại: dòng/đích capture đã đổi hoặc không còn tồn tại.");
            } else {
                if (tradeEditor_ && IsWindow(tradeEditor_) && tradeEditorMode_ == captureTradeSequenceMode_) {
                    RefreshTradeSequenceList();
                    if (tradeSeqList_ && captureTradeSequenceIndex_ < ListView_GetItemCount(tradeSeqList_)) {
                        ListView_SetItemState(tradeSeqList_, captureTradeSequenceIndex_,
                                              LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                        ListView_EnsureVisible(tradeSeqList_, captureTradeSequenceIndex_, FALSE);
                        LoadTradeSequenceRowToEditor(captureTradeSequenceIndex_);
                    }
                }
                LogAccount(*captureAccount, L"Đã lưu chuỗi GD dòng " + std::to_wstring(captureTradeSequenceIndex_ + 1) + L" qua F8 = " + PointDescription(captured));
            }
        } else if (captureMacroIndex_ >= 0) {
            if (captureMacroIndex_ >= static_cast<int>(captureAccount->profile.sellMacro.size())) {
                LogAccount(*captureAccount, L"F8 macro thất bại: dòng đã bị xóa.");
            } else {
                captureAccount->profile.sellMacro[static_cast<std::size_t>(captureMacroIndex_)].point = captured;
                SaveProfile(captureAccount->profile);
                LogAccount(*captureAccount, L"Đã lưu macro dòng " + std::to_wstring(captureMacroIndex_ + 1) + L" = " + PointDescription(captured));
            }
        } else {
            const int index = static_cast<int>(captureSlot_);
            if (index >= 0 && index < 5) {
                captureAccount->profile.points[static_cast<std::size_t>(index)] = captured;
                SaveProfile(captureAccount->profile);
                LogAccount(*captureAccount, L"Đã lưu " + std::wstring(kClickLabels[static_cast<std::size_t>(index)]) + L" = " + PointDescription(captured));
            }
        }
        LoadSelectedProfileToUi();
        captureSlot_ = ClickSlot::None; captureMacroIndex_ = -1; capturePid_ = 0;
        captureTradeSequenceIndex_ = -1; captureTradeSequenceMode_ = 0; captureTradeSequenceMainRef_ = -1;
        capturePkStepIndex_ = -1; capturePkClickIndex_ = -1; shortcutKunlunCaptureIndex_ = -1;
    }

    bool DispatchInternalPointActionDirect(Account& a, const ClickPoint& savedPoint,
                                           const std::wstring& request,
                                           std::wstring& error) {
        if (a.runtime.clientFreezeActive) {
            error = L"client/map đang FREEZE; hidden action bị chặn";
            return false;
        }
        if (!IsWindow(a.game.window)) {
            error = L"Cửa sổ game không còn tồn tại";
            return false;
        }

        int normalizedX = -1;
        int normalizedY = -1;
        if (!NormalizeClickPointForBridge(a.game, savedPoint,
                                          normalizedX, normalizedY, error)) {
            return false;
        }

        std::wstring attachError;
        if (!EnsureAttach(a, attachError)) {
            error = L"Không attach được Bridge cho hidden action: " + attachError;
            return false;
        }

        Response response{};
        const bool ok = a.bridge.Call(Command::ClickInternalPoint,
                                      normalizedX, normalizedY, 0,
                                      response, error, 2200);
        const DWORD completedAt = GetTickCount();
        if (!ok) {
            if (BridgeLooksUnresponsive(error)) {
                EnterClientFreeze(a, L"Bridge timeout khi chạy hidden point action", completedAt);
            }
            return false;
        }

        LogAccount(a, L"HIDDEN ACTION DISPATCH PASS • " + request +
                      L" • normalized=" + std::to_wstring(normalizedX) + L"," +
                      std::to_wstring(normalizedY) + L" • " + response.detail);
        return true;
    }

    // v0.6.1.9: hidden InputSync actions do not share a Windows-input resource.
    // Business workflows serialize themselves (SELL per account, TRADE per active pair),
    // so there is no global input/owner/sequence lease between unrelated clients.
    bool CoordinatorInternalPointAction(Account& target, const ClickPoint& savedPoint,
                                        const std::wstring& request,
                                        std::wstring& error) {
        if (RecorderBlocksAccount(target)) {
            error = L"acc đang REC cấu hình; hidden action của chính acc này tạm giữ";
            return false;
        }
        return DispatchInternalPointActionDirect(target, savedPoint, request, error);
    }

    bool QueuePriorityAutoClick(Account& a, ClickSlot slot, PriorityAutoOwner owner,
                                const std::wstring& reason) {
        RuntimeState& rt = a.runtime;
        if (slot != ClickSlot::AutoMenu && slot != ClickSlot::Attack && slot != ClickSlot::StopAuto2) return false;
        if (owner == PriorityAutoOwner::None) return false;
        if ((!rt.running && !a.pk.active && !a.dungeonOwned) || rt.clientFreezeActive || !a.snapshotValid || !IsWindow(a.game.window)) return false;
        if (slot == ClickSlot::Attack &&
            !travel_fight_guard_logic::CanDispatchFightStart(
                (a.snapshot.validMask & ValidAutoPath) != 0,
                a.snapshot.autoPathing != 0)) {
            rt.status = L"PRIORITY #3 AUTO • chặn bật AutoFight vì AutoPath chưa authoritative OFF";
            return false;
        }
        if (rt.priorityAutoCompletedSlot != ClickSlot::None &&
            (rt.priorityAutoCompletedSlot != slot ||
             rt.priorityAutoCompletedOwner != owner)) {
            LogAccount(a, L"PRIORITY #3 AUTO: bỏ result cũ " + std::wstring(kClickLabels[static_cast<std::size_t>(rt.priorityAutoCompletedSlot)]) + L" do workflow đã đổi pha.");
            rt.priorityAutoCompletedSlot = ClickSlot::None;
            rt.priorityAutoCompletedOwner = PriorityAutoOwner::None;
            rt.priorityAutoCompletedOk = false;
            rt.priorityAutoCompletedTick = 0;
        }
        if (rt.priorityAutoRequestSlot == slot &&
            rt.priorityAutoRequestOwner == owner) return true;
        if (rt.priorityAutoRequestSlot != ClickSlot::None || rt.priorityAutoCompletedSlot != ClickSlot::None) return false;
        rt.priorityAutoRequestSlot = slot;
        rt.priorityAutoRequestOwner = owner;
        rt.priorityAutoPointPhase = 0;
        rt.priorityAutoPointTick = 0;
        rt.status = L"PRIORITY #3 AUTO INPUTSYNC • đã xếp hàng " +
                    std::wstring(kClickLabels[static_cast<std::size_t>(slot)]);
        if (!reason.empty()) LogAccount(a, L"PRIORITY #3 AUTO QUEUE: " + reason);
        return true;
    }

    bool ConsumePriorityAutoResult(Account& a, ClickSlot slot, PriorityAutoOwner owner,
                                   bool& ok, DWORD& clickedAt) {
        RuntimeState& rt = a.runtime;
        if (rt.priorityAutoCompletedSlot != slot ||
            rt.priorityAutoCompletedOwner != owner) return false;
        ok = rt.priorityAutoCompletedOk;
        clickedAt = rt.priorityAutoCompletedTick;
        rt.priorityAutoCompletedSlot = ClickSlot::None;
        rt.priorityAutoCompletedOwner = PriorityAutoOwner::None;
        rt.priorityAutoCompletedOk = false;
        rt.priorityAutoCompletedTick = 0;
        return true;
    }

    bool PriorityAutoClick(Account& a) {
        RuntimeState& rt = a.runtime;
        const ClickSlot requestedSlot = rt.priorityAutoRequestSlot;
        const PriorityAutoOwner requestedOwner = rt.priorityAutoRequestOwner;
        if (requestedSlot == ClickSlot::None) return false;
        if (requestedOwner == PriorityAutoOwner::None) {
            rt.priorityAutoRequestSlot = ClickSlot::None;
            return false;
        }
        if ((!rt.running && !a.pk.active && !a.dungeonOwned) || rt.clientFreezeActive || !a.snapshotValid || !IsWindow(a.game.window)) return false;
        const Snapshot& s = a.snapshot;

        auto complete = [&](bool ok, DWORD completedAt) {
            rt.priorityAutoRequestSlot = ClickSlot::None;
            rt.priorityAutoRequestOwner = PriorityAutoOwner::None;
            rt.priorityAutoCompletedSlot = requestedSlot;
            rt.priorityAutoCompletedOwner = requestedOwner;
            rt.priorityAutoCompletedOk = ok;
            rt.priorityAutoCompletedTick = completedAt;
            rt.priorityAutoPointPhase = 0;
            rt.priorityAutoPointTick = 0;
        };
        const bool unsafeFightStart = requestedSlot == ClickSlot::Attack &&
            (!s.mapReady || s.waitingChangeMap ||
             ((s.validMask & ValidLifeState) && s.dead) ||
             !travel_fight_guard_logic::CanDispatchFightStart(
                 (s.validMask & ValidAutoPath) != 0, s.autoPathing != 0));
        const bool staleTrainStart = requestedSlot == ClickSlot::Attack &&
            requestedOwner == PriorityAutoOwner::Train &&
            AutoFightCheckBusy(a, GetTickCount());
        if (unsafeFightStart || staleTrainStart) {
            complete(false, GetTickCount());
            rt.status = L"PRIORITY #3 AUTO • hủy request cũ vì state không còn cho phép bật Fight";
            LogAccount(a, L"PRIORITY #3 AUTO SAFETY: hủy AUTO→ĐÁNH QUÁI trước dispatch vì "
                          L"AutoPath/map/workflow không còn ở state đã xếp hàng; không dùng request cũ.");
            return false;
        }

        if (!s.mapReady || s.waitingChangeMap ||
            ((s.validMask & ValidLifeState) && s.dead)) return false;

        // v0.6.1.7: both AUTO->Attack and AUTO->Stop are menu-choice sequences.
        // StopAuto2 is not a standalone visible control while the AUTO menu is closed.
        // The v0.6.1.4 direct-Stop shortcut could therefore raycast empty UI exactly when
        // Trade/Travel Guard tried to leave a training spot. Restore the proven v0.5
        // lifecycle, but keep every phase on the hidden InputSync dispatcher.
        constexpr auto autoChoicePlan = internal_ui_click_logic::AutoMenuChoicePlan();
        const bool autoMenuChoiceSequence =
            requestedSlot == ClickSlot::Attack || requestedSlot == ClickSlot::StopAuto2;
        ClickSlot pointSlot = requestedSlot;
        if (autoMenuChoiceSequence) {
            if (rt.priorityAutoPointPhase < 0 ||
                rt.priorityAutoPointPhase >= static_cast<int>(autoChoicePlan.size())) {
                rt.priorityAutoPointPhase = 0;
                rt.priorityAutoPointTick = 0;
            }
            const auto& step = autoChoicePlan[static_cast<std::size_t>(rt.priorityAutoPointPhase)];
            if (step.waitBeforeMs > 0 &&
                !Elapsed(GetTickCount(), rt.priorityAutoPointTick,
                         static_cast<DWORD>(step.waitBeforeMs))) {
                return false;
            }
            pointSlot = step.point == internal_ui_click_logic::AutoMenuChoicePoint::AutoMenu
                ? ClickSlot::AutoMenu : requestedSlot;
        }

        const int pointIndex = static_cast<int>(pointSlot);
        std::wstring error;
        if (pointIndex < 0 || pointIndex >= static_cast<int>(a.profile.points.size())) {
            complete(false, GetTickCount());
            LogAccount(a, L"PRIORITY #3 AUTO INPUTSYNC FAIL: slot điểm không hợp lệ");
            return false;
        }

        int normalizedX = -1;
        int normalizedY = -1;
        if (!NormalizeClickPointForBridge(
                a.game, a.profile.points[static_cast<std::size_t>(pointIndex)],
                normalizedX, normalizedY, error)) {
            complete(false, GetTickCount());
            LogAccount(a, L"PRIORITY #3 AUTO INPUTSYNC FAIL tọa độ " +
                          std::wstring(kClickLabels[static_cast<std::size_t>(pointIndex)]) +
                          L": " + error);
            return false;
        }

        Response response{};
        const bool ok = a.bridge.Call(Command::ClickInternalPoint,
                                      normalizedX, normalizedY, 0,
                                      response, error, 2200);
        const DWORD clickedAt = GetTickCount();

        if (autoMenuChoiceSequence && rt.priorityAutoPointPhase == 0 && ok) {
            rt.priorityAutoPointPhase = 1;
            rt.priorityAutoPointTick = clickedAt;
            const std::wstring next = requestedSlot == ClickSlot::Attack
                ? L"ĐÁNH QUÁI" : L"DỪNG AUTO 2";
            rt.status = L"P3 AUTO INPUTSYNC • click 1/2 AUTO xong • chờ mở menu để " + next;
            LogAccount(a, L"PRIORITY #3 AUTO INPUTSYNC PASS click 1/2: AUTO → chờ " + next +
                          L" • " + std::wstring(response.detail));
            return true;
        }

        complete(ok, clickedAt);
        if (ok) {
            const std::wstring phase = requestedSlot == ClickSlot::Attack
                ? L"click 2/2: ĐÁNH QUÁI" :
                requestedSlot == ClickSlot::StopAuto2
                    ? L"click 2/2: DỪNG AUTO 2"
                    : std::wstring(kClickLabels[static_cast<std::size_t>(pointIndex)]);
            LogAccount(a, L"PRIORITY #3 AUTO INPUTSYNC PASS " + phase +
                          L" • TryClickUI→EndUIDrag • không chiếm chuột Windows.");
        } else {
            if (BridgeLooksUnresponsive(error)) EnterClientFreeze(a, L"Bridge timeout khi click AUTO InputSync", clickedAt);
            LogAccount(a, L"PRIORITY #3 AUTO INPUTSYNC FAIL tại " +
                          std::wstring(kClickLabels[static_cast<std::size_t>(pointIndex)]) +
                          L": " + error);
        }
        return ok;
    }

    void ResetTravelFightGuard(RuntimeState& rt) {
        rt.travelFightGuardPhase = 0;
        rt.travelFightGuardTick = 0;
        rt.travelFightStopAttempts = 0;
    }

    bool EnsureAutoFightOffForTravel(Account& a, DWORD now, const wchar_t* context) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        const std::wstring where = context ? context : L"di chuyển";

        if ((s.validMask & ValidAutoFight) == 0) {
            rt.status = L"TRAVEL GUARD • chờ AutoFight authoritative trước StartPath tới " + where;
            return false;
        }
        if (travel_fight_guard_logic::CanDispatchMovement(true, s.autoFight != 0)) {
            if (rt.travelFightGuardPhase != 0 || rt.travelFightStopAttempts != 0) {
                LogAccount(a, L"TRAVEL GUARD PASS: AutoFight OFF authoritative → nhả StartPath tới " + where);
            }
            ResetTravelFightGuard(rt);
            return true;
        }

        bool ok = false;
        DWORD clickedAt = 0;
        switch (rt.travelFightGuardPhase) {
            case 0:
                if (QueuePriorityAutoClick(a, ClickSlot::StopAuto2,
                                           PriorityAutoOwner::TravelGuardStop,
                                           L"TRAVEL GUARD: click điểm DỪNG AUTO nội bộ trước " + where)) {
                    rt.travelFightGuardPhase = 1;
                    rt.status = L"TRAVEL GUARD • AutoFight ON → chờ click DỪNG InputSync";
                }
                return false;
            case 1:
                if (!ConsumePriorityAutoResult(a, ClickSlot::StopAuto2,
                                               PriorityAutoOwner::TravelGuardStop,
                                               ok, clickedAt)) return false;
                if (!ok) { rt.travelFightGuardPhase = 0; return false; }
                rt.travelFightGuardPhase = 2;
                rt.travelFightGuardTick = clickedAt;
                ++rt.travelFightStopAttempts;
                rt.status = L"TRAVEL GUARD • đã click DỪNG InputSync lần " +
                            std::to_wstring(rt.travelFightStopAttempts) + L" • verify OFF";
                return false;
            case 2:
                if (!Elapsed(now, rt.travelFightGuardTick, kPriorityAutoVerifyMs)) return false;
                if (travel_fight_guard_logic::NeedsAnotherStopBeforeReset(
                        rt.travelFightStopAttempts)) {
                    rt.travelFightGuardPhase = 0;
                    return false;
                }
                rt.travelFightGuardPhase = 3;
                rt.travelFightStopAttempts = 0;
                LogAccount(a, L"TRAVEL GUARD: click DỪNG 2 lần vẫn ON → chạy AUTO→ĐÁNH QUÁI InputSync reset rồi lặp DỪNG.");
                return false;
            case 3:
                if (QueuePriorityAutoClick(a, ClickSlot::Attack,
                                           PriorityAutoOwner::TravelGuardReset,
                                           L"TRAVEL GUARD RESET: chạy AUTO→ĐÁNH QUÁI InputSync")) {
                    rt.travelFightGuardPhase = 4;
                }
                return false;
            case 4:
                if (!ConsumePriorityAutoResult(a, ClickSlot::Attack,
                                               PriorityAutoOwner::TravelGuardReset,
                                               ok, clickedAt)) return false;
                if (!ok) { rt.travelFightGuardPhase = 3; return false; }
                rt.travelFightGuardPhase = 5;
                rt.travelFightGuardTick = clickedAt;
                return false;
            case 5:
                if (!Elapsed(now, rt.travelFightGuardTick, kPriorityAutoVerifyMs)) return false;
                rt.travelFightGuardPhase = 0;
                rt.travelFightStopAttempts = 0;
                rt.status = L"TRAVEL GUARD • reset AUTO→ĐÁNH QUÁI xong • lặp DỪNG nội bộ";
                return false;
            default:
                ResetTravelFightGuard(rt);
                return false;
        }
    }

    void ResetAutoPathFightConflict(RuntimeState& rt) {
        rt.autoPathFightConflictLatched = false;
        rt.autoPathFightConflictTick = 0;
        rt.autoPathFightConflictStopAttempts = 0;
    }

    bool HandleAutoPathFightInvariant(Account& a, DWORD now) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        const std::uint32_t need = ValidAutoPath | ValidAutoFight;

        if ((s.validMask & need) != need) {
            if (rt.autoPathFightConflictLatched) {
                rt.status = L"ROUTE/FIGHT INVARIANT • chờ AutoPath + AutoFight authoritative";
                return true;
            }
            return false;
        }
        if ((s.validMask & ValidLifeState) && s.dead) return false;

        const bool conflict = travel_fight_guard_logic::HasAutoPathFightConflict(
            s.autoPathing != 0, s.autoFight != 0);
        if (conflict && !rt.autoPathFightConflictLatched) {
            rt.autoPathFightConflictLatched = true;
            rt.autoPathFightConflictTick = 0;
            rt.autoPathFightConflictStopAttempts = 0;
            ResetTravelFightGuard(rt);
            LogAccount(a, L"ROUTE/FIGHT INVARIANT VIOLATION: phát hiện AutoPath ON + AutoFight ON"
                          L" → StopPath fail-closed, sau đó DỪNG x2/reset cho tới khi cả hai OFF.");
        }

        if (!rt.autoPathFightConflictLatched) return false;

        if (travel_fight_guard_logic::ConflictRecoveryComplete(
                rt.autoPathFightConflictLatched,
                s.autoPathing != 0, s.autoFight != 0)) {
            ResetAutoPathFightConflict(rt);
            ResetTravelFightGuard(rt);
            rt.status = L"ROUTE/FIGHT INVARIANT PASS • AutoPath OFF + AutoFight OFF";
            LogAccount(a, L"ROUTE/FIGHT INVARIANT RECOVERED: cả AutoPath và AutoFight đều OFF"
                          L" → route kế tiếp phải đi lại qua Travel Guard.");
            return true; // one-cycle barrier before any new route decision
        }

        if (s.autoPathing) {
            if (rt.autoPathFightConflictTick == 0 ||
                Elapsed(now, rt.autoPathFightConflictTick,
                        kAutoPathFightConflictRetryMs)) {
                std::wstring attachError;
                Response response{};
                std::wstring error;
                bool ok = EnsureAttach(a, attachError);
                if (!ok) {
                    error = L"không attach được Bridge: " + attachError;
                } else {
                    ok = a.bridge.Call(Command::StopPath, 0, 0, 0,
                                       response, error, 900);
                }
                rt.autoPathFightConflictTick = now;
                ++rt.autoPathFightConflictStopAttempts;
                if (ok) {
                    LogAccount(a, L"ROUTE/FIGHT INVARIANT: đã gửi StopPath nội bộ lần " +
                                  std::to_wstring(rt.autoPathFightConflictStopAttempts) +
                                  L" • chờ AutoPath OFF authoritative.");
                } else {
                    if (BridgeLooksUnresponsive(error)) {
                        EnterClientFreeze(a, L"Bridge timeout khi dập AutoPath/Fight conflict", now);
                    }
                    LogAccount(a, L"ROUTE/FIGHT INVARIANT: StopPath fail-closed lần " +
                                  std::to_wstring(rt.autoPathFightConflictStopAttempts) +
                                  L" • " + error);
                }
            }
            rt.status = L"ROUTE/FIGHT INVARIANT • đang dập AutoPath trước khi tắt AutoFight";
            return true;
        }

        // AutoPath is now OFF. Reuse the proven stop-stop-reset loop and do not
        // release any route until AutoFight is authoritatively OFF as well.
        if (!EnsureAutoFightOffForTravel(a, now, L"khôi phục invariant AutoPath/AutoFight")) {
            rt.status = L"ROUTE/FIGHT INVARIANT • AutoPath OFF • đang DỪNG AutoFight x2/reset";
            return true;
        }
        return true;
    }

    void TestClick(ClickSlot slot) {
        Account* a = SelectedAccount();
        if (!a) { Log(L"TEST: chưa chọn acc"); return; }
        std::wstring attachError;
        if (!EnsureAttach(*a, attachError)) {
            LogAccount(*a, L"TEST nội bộ không attach được Bridge: " + attachError);
            return;
        }
        Command command = Command::None;
        int arg0 = 0;
        int arg1 = 0;
        std::wstring error;
        switch (slot) {
            case ClickSlot::AutoMenu:
            case ClickSlot::Attack:
            case ClickSlot::StopAuto2: {
                const int index = static_cast<int>(slot);
                if (index < 0 || index >= static_cast<int>(a->profile.points.size()) ||
                    !NormalizeClickPointForBridge(
                        a->game, a->profile.points[static_cast<std::size_t>(index)],
                        arg0, arg1, error)) {
                    LogAccount(*a, L"TEST INPUTSYNC " +
                                   std::wstring(kClickLabels[static_cast<std::size_t>(index)]) +
                                   L" FAIL tọa độ: " + error);
                    return;
                }
                command = Command::ClickInternalPoint;
                break;
            }
            default: break;
        }
        if (command == Command::None) return;
        Response response{};
        const bool ok = a->bridge.Call(command, arg0, arg1, 0, response, error, 2200);
        const int index = static_cast<int>(slot);
        LogAccount(*a, L"TEST NỘI BỘ " + std::wstring(kClickLabels[static_cast<std::size_t>(index)]) +
                       (ok ? L" PASS • không chiếm chuột • " + std::wstring(response.detail)
                           : L" FAIL • " + error));
    }

    void StartChecked() {
        PersistSelectedEditor();
        const bool hadRunningBeforeStart = AnyRunningAccount();
        int started = 0;
        const int count = ListView_GetItemCount(clientList_);
        for (int i = 0; i < count && i < static_cast<int>(accounts_.size()); ++i) {
            if (!ListView_GetCheckState(clientList_, i)) continue;
            Account& a = *accounts_[static_cast<std::size_t>(i)];
            if (a.dungeonOwned) {
                LogAccount(a, L"Không start AUTO: acc đang thuộc tổ đội AUTO PHÓ BẢN.");
                continue;
            }
            if (!a.profile.target.valid) {
                LogAccount(a, L"Không start: acc chưa chọn bãi chung.");
                continue;
            }
            std::wstring error;
            if (!EnsureAttach(a, error)) {
                LogAccount(a, L"Không start: " + error);
                continue;
            }
            a.deathSessionLatched = false;
            a.rotationDeathTicks.clear();
            a.rotationMetricTick = GetTickCount();
            a.rotationActiveTrainMs = 0;
            a.rotationBagWasFull = false;
            a.runtime.running = true;
            ResetRuntime(a.runtime);
            a.runtime.running = true;
            a.runtime.routeOwnershipResetPending = true;
            a.runtime.status = L"Đang giám sát • chuẩn hóa ownership AutoPath";
            ++started;
            LogAccount(a, L"BẮT ĐẦU • bãi " + a.profile.target.name + L" • M" +
                           std::to_wstring(a.profile.target.mapID) + L" • " +
                           std::to_wstring(a.profile.target.x) + L"," + std::to_wstring(a.profile.target.y) +
                           L" • vòng " + std::to_wstring(a.profile.rotationSpots.size()) + L" bãi • chết quá " +
                           std::to_wstring(a.profile.rotateDeathLimit) + L"/" + std::to_wstring(a.profile.rotateDeathWindowMin) +
                           L" phút • chưa FULL túi " + std::to_wstring(a.profile.rotateNoFullBagMin) + L" phút");
            UpdateAccountRow(i, a);
        }
        if (started == 0) Log(L"Không có acc hợp lệ được start. Hãy tick checkbox và chọn bãi chung cho acc.");
        else if (!hadRunningBeforeStart) BeginTelegramSession();
    }

    void StopAccount(Account& a) {
        const bool wasFrozen = a.runtime.clientFreezeActive;
        a.deathSessionLatched = false;
        a.rotationDeathTicks.clear();
        a.rotationMetricTick = 0;
        a.rotationActiveTrainMs = 0;
        a.rotationBagWasFull = false;
        a.runtime.running = false;
        ResetRuntime(a.runtime);
        a.runtime.running = false;
        a.runtime.status = L"Đã dừng";
        if (a.bridge.Attached() && !wasFrozen) {
            Response r{};
            std::wstring ignored;
            (void)a.bridge.Call(Command::StopPath, 0, 0, 0, r, ignored, 700);
        }
        LogAccount(a, L"Đã dừng. Không tự đổi trạng thái ngựa.");
    }

    void StopChecked() {
        const bool hadRunningBeforeStop = AnyRunningAccount();
        int stopped = 0;
        const int count = ListView_GetItemCount(clientList_);
        for (int i = 0; i < count && i < static_cast<int>(accounts_.size()); ++i) {
            if (!ListView_GetCheckState(clientList_, i)) continue;
            StopAccount(*accounts_[static_cast<std::size_t>(i)]);
            UpdateAccountRow(i, *accounts_[static_cast<std::size_t>(i)]);
            ++stopped;
        }
        if (tradeTxn_.phase != TradePhase::Idle) {
            Account* main = AccountByPid(tradeTxn_.mainPid);
            Account* child = AccountByPid(tradeTxn_.childPid);
            if ((main && !main->runtime.running) || (child && !child->runtime.running)) {
                AbortTrade(L"người dùng DỪNG AUTO acc thuộc workflow giao dịch", GetTickCount());
            }
        }
        if (stopped == 0) Log(L"Không có acc nào được tick để dừng.");
        if (hadRunningBeforeStop && !AnyRunningAccount() && telegramStats_.active) {
            (void)SendTelegramSummary(L"DỪNG TOÀN BỘ ACC", true);
            if (telegramSettings_.enabled && telegramSettings_.notifyToolState) {
                (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage,
                    L"⏹ AUTO SESSION STOP\nThời gian: " + LocalDateTimeText(), L"SESSION STOP", L"-");
            }
            telegramStats_.active = false;
        }
    }

    static bool BridgeLooksUnresponsive(const std::wstring& error) {
        return error.find(L"timeout") != std::wstring::npos ||
               error.find(L"Bridge còn bận") != std::wstring::npos ||
               error.find(L"Bridge busy") != std::wstring::npos;
    }

    bool WindowResponsive(const GameClient& game) const {
        if (!game.window || !IsWindow(game.window)) return false;
        DWORD_PTR ignored = 0;
        const LRESULT ok = SendMessageTimeoutW(game.window, WM_NULL, 0, 0,
                                               SMTO_ABORTIFHUNG | SMTO_BLOCK,
                                               kWindowResponsiveProbeMs, &ignored);
        return ok != 0;
    }

    void EnterClientFreeze(Account& a, const wchar_t* reason, DWORD now) {
        RuntimeState& rt = a.runtime;
        const bool first = !rt.clientFreezeActive;
        rt.clientFreezeActive = true;
        if (rt.clientFreezeSinceTick == 0) rt.clientFreezeSinceTick = now;
        rt.clientStableSinceTick = 0;
        rt.candidateCount = 0;
        rt.qualifiedMap = 0;
        rt.stallSinceTick = 0;
        rt.fightPhase = 0;
        if (first) {
            LogAccount(a, L"FREEZE ACTION: " + std::wstring(reason ? reason : L"client/map chưa ổn định"));
            TelegramRecordCriticalFreeze(a, reason);
        }
    }

    void MarkReadStateFailure(Account& a, const std::wstring& error, DWORD now) {
        RuntimeState& rt = a.runtime;
        EnterClientFreeze(a, L"ReadState/Bridge không phản hồi", now);
        TelegramRecordCriticalFreeze(a, L"ReadState/Bridge không phản hồi");
        ++rt.readStateFailStreak;
        rt.clientStableSinceTick = 0;
        rt.status = L"CLIENT KHÔNG PHẢN HỒI • FREEZE ACTION";
        if (rt.lastReadFailureLogTick == 0 || now - rt.lastReadFailureLogTick >= kReadFailLogIntervalMs) {
            LogAccount(a, L"ReadState fail x" + std::to_wstring(rt.readStateFailStreak) + L": " + error +
                          L" • FREEZE, không gửi action mới");
            rt.lastReadFailureLogTick = now;
        }
    }

    bool HoldUntilClientStable(Account& a, DWORD now) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;

        if (!s.mapReady || s.waitingChangeMap) {
            EnterClientFreeze(a, L"game đang chuyển map", now);
            rt.clientStableSinceTick = 0;
            rt.status = L"ĐANG CHUYỂN MAP • FREEZE ACTION";
            return true;
        }

        if (!rt.clientFreezeActive) {
            rt.readStateFailStreak = 0;
            rt.lastReadFailureLogTick = 0;
            return false;
        }

        if (!WindowResponsive(a.game)) {
            TelegramRecordCriticalFreeze(a, L"Cửa sổ game không phản hồi");
            rt.clientStableSinceTick = 0;
            rt.status = L"CỬA SỔ GAME CHƯA PHẢN HỒI • FREEZE ACTION";
            return true;
        }

        if (rt.clientStableSinceTick == 0) {
            rt.clientStableSinceTick = now;
            rt.status = L"MAP/CLIENT ĐÃ PHẢN HỒI • chờ ổn định 2.0s";
            return true;
        }
        if (!Elapsed(now, rt.clientStableSinceTick, kClientStableResumeMs)) {
            const DWORD elapsed = now - rt.clientStableSinceTick;
            const DWORD remainMs = elapsed >= kClientStableResumeMs ? 0 : kClientStableResumeMs - elapsed;
            rt.status = L"CLIENT ĐANG ỔN ĐỊNH • chờ " + std::to_wstring((remainMs + 99) / 100) + L"00ms";
            return true;
        }

        rt.clientFreezeActive = false;
        rt.clientFreezeSinceTick = 0;
        rt.clientStableSinceTick = 0;
        TelegramRecordFreezeRecovered(a);
        rt.readStateFailStreak = 0;
        rt.lastReadFailureLogTick = 0;
        rt.lastActionTick = 0;
        rt.lastAction = Action::Wait;
        LogAccount(a, L"CLIENT ỔN ĐỊNH LIÊN TỤC 2s → mở khóa action, tiếp tục auto.");
        rt.status = L"Client ổn định 2s • tiếp tục auto";
        return false;
    }

    bool CooldownReady(RuntimeState& rt, Action a, DWORD now) {
        DWORD delay = 1500;
        if (a == Action::Mount || a == Action::Dismount) delay = 4000;
        if (a == Action::StartPath) delay = 5000;
        if (a != rt.lastAction) {
            rt.lastAction = a;
            rt.lastActionTick = 0;
        }
        return rt.lastActionTick == 0 || now - rt.lastActionTick >= delay;
    }

    bool SendDecision(Account& a, Action action, const TargetProfile& t, const wchar_t* context, int diagnosticTolerance = 0) {
        RuntimeState& rt = a.runtime;
        if (rt.clientFreezeActive) {
            rt.status = L"FREEZE ACTION • bỏ qua route/mount command";
            return false;
        }
        const DWORD now = GetTickCount();
        // Single authoritative movement gate: neither Mount nor StartPath may be
        // emitted while AutoFight is ON/unreadable. StartPath additionally waits for
        // the hard AutoPath+Fight conflict recovery to observe both states OFF.
        if (action == Action::Mount || action == Action::StartPath) {
            if (action == Action::StartPath && rt.autoPathFightConflictLatched) {
                rt.status = L"ROUTE/FIGHT INVARIANT • cấm StartPath khi recovery chưa hoàn tất";
                return false;
            }
            if (action == Action::StartPath &&
                (!a.snapshotValid || (a.snapshot.validMask & ValidRiding) == 0 ||
                 !CanStartPath(a.snapshot.riding != 0))) {
                rt.status = L"MOUNT REQUIRED • chặn StartPath vì chưa xác nhận đang trên ngựa";
                LogAccount(a, L"MOUNT REQUIRED: StartPath bị chặn fail-closed • phải thấy IsRiding=1 trước mọi AutoPath.");
                return false;
            }
            const wchar_t* movementContext = context ? context :
                (action == Action::Mount ? L"lên ngựa" : L"AutoPath");
            if (!EnsureAutoFightOffForTravel(a, now, movementContext)) return false;
        }
        if (!CooldownReady(rt, action, now)) return false;
        Response r{};
        std::wstring error;
        bool ok = false;
        const std::wstring where = context ? context : L"đích";
        switch (action) {
            case Action::Mount:
                ok = a.bridge.Call(Command::ToggleRide, 1, 0, 0, r, error, 1000);
                if (ok) rt.status = L"Đang lên ngựa • " + where;
                break;
            case Action::Dismount:
                ok = a.bridge.Call(Command::ToggleRide, 0, 0, 0, r, error, 1000);
                if (ok) rt.status = L"Tới " + where + L" • xuống ngựa";
                break;
            case Action::StartPath:
                ok = a.bridge.Call(Command::StartPath, t.mapID, t.x, t.y, r, error, 1300);
                if (ok) rt.status = L"Đang AutoPath tới " + where;
                break;
            case Action::StopPath:
                ok = a.bridge.Call(Command::StopPath, 0, 0, 0, r, error, 900);
                if (ok) rt.status = L"Tới " + where + L" • StopPath";
                break;
            default:
                return false;
        }
        if (ok) {
            rt.lastActionTick = now;
            if (action == Action::StartPath) rt.lastStartPathPassTick = now;
        } else if (action == Action::StartPath) {
            // v1.6: a transient Bridge rejection must not create a fake 5-second AutoPath success window.
            rt.lastActionTick = now > 3500 ? now - 3500 : 0;
        } else {
            rt.lastActionTick = now;
        }
        if (ok && action == Action::StartPath) {
            const long long dx = static_cast<long long>(a.snapshot.x) - t.x;
            const long long dy = static_cast<long long>(a.snapshot.y) - t.y;
            const long long d2 = dx * dx + dy * dy;
            const long long distance = static_cast<long long>(std::llround(std::sqrt(static_cast<long double>(d2))));
            const std::wstring tolText = diagnosticTolerance < 0
                ? L"PORTAL_SPECIAL"
                : std::to_wstring(diagnosticTolerance > 0 ? diagnosticTolerance : a.profile.tolerance);
            LogAccount(a, L"COORD STARTPATH PASS • CURRENT M" + std::to_wstring(a.snapshot.mapID) +
                          L"/" + std::to_wstring(a.snapshot.x) + L"," + std::to_wstring(a.snapshot.y) +
                          L" • TARGET M" + std::to_wstring(t.mapID) + L"/" + std::to_wstring(t.x) + L"," + std::to_wstring(t.y) +
                          L" • DX=" + std::to_wstring(dx) + L" DY=" + std::to_wstring(dy) +
                          L" DISTANCE=" + std::to_wstring(distance) + L" TOL=" + tolText +
                          L" • BRIDGE=" + std::wstring(r.detail));
        }
        if (!ok && BridgeLooksUnresponsive(error)) {
            EnterClientFreeze(a, L"Bridge action timeout/busy", now);
        }
        if (ok && action == Action::StartPath && t.mapID != a.snapshot.mapID) {
            // Arm cross-map confirmation from the command itself. Movement/autoPath
            // evidence is still required before any Confirm click is allowed.
            if (!rt.crossMapRouteArmed) rt.crossMapRouteMoved = false;
            rt.crossMapRouteArmed = true;
        }
        if (!ok) {
            rt.status = L"ROUTE ACTION FAIL • " + where + L" • " + error;
            LogAccount(a, L"Route action fail-closed: " + error);
        }
        return ok;
    }

    bool CompleteToolOwnedRoute(RuntimeState& rt, bool atTarget, bool autoPathing, bool riding) {
        // Intermediate map changes MUST keep ownership armed so the Lâu Lan P1 watchdog
        // can still prove that this is the tool-owned cross-map route. Release ownership
        // only at the physical final destination: in tolerance, AutoPath OFF, on foot.
        if (!travel_fight_guard_logic::IsPhysicalRouteCompletion(atTarget, autoPathing, riding)) return false;
        rt.crossMapRouteArmed = false;
        rt.crossMapRouteMoved = false;
        rt.crossMapSeenAutoPath = false;
        rt.stallSinceTick = 0;
        rt.confirmAttempts = 0;
        rt.lastLauLanConfirmTick = 0;
        return true;
    }

    void ObserveMovement(Account& a, DWORD now) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        if (rt.lastObservedMap != s.mapID) {
            const bool keepToolOwnedCrossMapRoute = rt.crossMapRouteArmed;
            rt.lastObservedMap = s.mapID;
            rt.lastObservedX = s.x;
            rt.lastObservedY = s.y;
            rt.lastMovementTick = now;
            // v0.5: a World Flow / robust travel route is still the SAME tool-owned route
            // after crossing an intermediate map. Do not lose the Lâu Lan gate watchdog
            // merely because MapID changed. Crossing a map is itself proof the route moved.
            if (keepToolOwnedCrossMapRoute) {
                rt.crossMapRouteMoved = true;
                if (s.autoPathing) rt.crossMapSeenAutoPath = true;
            } else {
                rt.crossMapSeenAutoPath = false;
                rt.crossMapRouteMoved = false;
            }
            rt.stallSinceTick = 0;
            rt.confirmAttempts = 0;
            rt.lastLauLanConfirmTick = 0;
            rt.fightPhase = 0;
            rt.fightAttempts = 0;
            rt.wasAtTarget = false;
            return;
        }
        if (rt.crossMapRouteArmed && s.autoPathing) rt.crossMapSeenAutoPath = true;
        const long long dx = static_cast<long long>(s.x) - rt.lastObservedX;
        const long long dy = static_cast<long long>(s.y) - rt.lastObservedY;
        if (dx * dx + dy * dy >= 25) {
            if (rt.crossMapRouteArmed) rt.crossMapRouteMoved = true;
            rt.lastMovementTick = now;
            rt.lastObservedX = s.x;
            rt.lastObservedY = s.y;
            rt.stallSinceTick = 0;
        }
    }

    void ResetRotationWindow(Account& a, DWORD now) {
        a.rotationDeathTicks.clear();
        a.rotationMetricTick = now;
        a.rotationActiveTrainMs = 0;
        a.rotationBagWasFull = false;
    }

    bool SwitchToNextRotationSpot(Account& a, DWORD now, const std::wstring& reason) {
        NormalizeRotationProfile(a.profile);
        const std::size_t count = a.profile.rotationSpots.size();
        if (count <= 1) {
            ResetRotationWindow(a, now);
            LogAccount(a, L"XOAY BÃI bỏ qua: chỉ tick 1 bãi • " + reason);
            SaveProfile(a.profile);
            return false;
        }
        std::size_t current = 0;
        for (std::size_t i = 0; i < count; ++i) {
            if (_wcsicmp(a.profile.rotationSpots[i].c_str(), a.profile.selectedSpot.c_str()) == 0) {
                current = i;
                break;
            }
        }
        const std::size_t next = NextRotationIndex(current, count);
        const std::wstring oldName = a.profile.selectedSpot;
        const std::wstring nextName = a.profile.rotationSpots[next];
        const int spotIndex = FindSpotIndex(spots_, nextName);
        if (spotIndex < 0) {
            a.profile.rotationSpots.erase(a.profile.rotationSpots.begin() + static_cast<std::ptrdiff_t>(next));
            NormalizeRotationProfile(a.profile);
            ResetRotationWindow(a, now);
            SaveProfile(a.profile);
            LogAccount(a, L"XOAY BÃI: bãi kế tiếp không còn trong data, đã loại khỏi vòng: " + nextName);
            return false;
        }
        a.profile.selectedSpot = nextName;
        a.profile.target = spots_[static_cast<std::size_t>(spotIndex)];
        a.profile.target.valid = true;
        ApplyAutoSellerForTrainingTarget(a);
        ResetRotationWindow(a, now);
        SaveProfile(a.profile);
        if (SelectedAccount() == &a) LoadSelectedProfileToUi();
        LogAccount(a, L"XOAY BÃI: " + oldName + L" → " + nextName + L" • " + reason +
                      L" • M" + std::to_wstring(a.profile.target.mapID) + L" " +
                      std::to_wstring(a.profile.target.x) + L"," + std::to_wstring(a.profile.target.y));
        return true;
    }

    void RecordDeathForRotation(Account& a, DWORD now) {
        NormalizeRotationProfile(a.profile);
        if (a.profile.rotationSpots.size() <= 1) {
            ResetRotationWindow(a, now);
            return;
        }
        const DWORD windowMs = static_cast<DWORD>(a.profile.rotateDeathWindowMin) * 60u * 1000u;
        a.rotationDeathTicks.push_back(now);
        a.rotationDeathTicks.erase(std::remove_if(a.rotationDeathTicks.begin(), a.rotationDeathTicks.end(), [&](DWORD t){
            return static_cast<DWORD>(now - t) > windowMs;
        }), a.rotationDeathTicks.end());
        const std::size_t count = a.rotationDeathTicks.size();
        LogAccount(a, L"XOAY BÃI death-window: " + std::to_wstring(count) + L" chết / " +
                      std::to_wstring(a.profile.rotateDeathWindowMin) + L" phút");
        if (DeathLimitExceeded(count, a.profile.rotateDeathLimit)) {
            const std::wstring reason = L"chết quá " + std::to_wstring(a.profile.rotateDeathLimit) + L" lần / " +
                                        std::to_wstring(a.profile.rotateDeathWindowMin) + L" phút";
            (void)SwitchToNextRotationSpot(a, now, reason);
        }
    }

    bool UpdateRotationEfficiency(Account& a, DWORD now) {
        NormalizeRotationProfile(a.profile);
        if (a.profile.rotationSpots.size() <= 1) {
            if (a.rotationActiveTrainMs != 0 || !a.rotationDeathTicks.empty()) ResetRotationWindow(a, now);
            return false;
        }
        const Snapshot& s = a.snapshot;
        if (a.rotationMetricTick == 0) a.rotationMetricTick = now;
        DWORD delta = now - a.rotationMetricTick;
        a.rotationMetricTick = now;
        if (delta > 2000) delta = 2000;

        if (s.validMask & ValidBagSpace) {
            const bool full = s.freeBagSpace <= 0;
            if (full && !a.rotationBagWasFull) {
                a.rotationBagWasFull = true;
                a.rotationActiveTrainMs = 0;
                LogAccount(a, L"XOAY BÃI: ghi nhận 1 lần FULL túi → reset đồng hồ hiệu quả bãi.");
            } else if (!full) {
                a.rotationBagWasFull = false;
            }
        }

        bool activelyTraining = false;
        if (a.profile.target.valid &&
            (s.validMask & (ValidMap | ValidPosition | ValidAutoFight | ValidLifeState | ValidBagSpace)) ==
                (ValidMap | ValidPosition | ValidAutoFight | ValidLifeState | ValidBagSpace) &&
            !s.dead && s.autoFight) {
            State state{};
            state.valid = true; state.mapReady = true; state.waitingMap = false;
            state.mapID = s.mapID; state.x = s.x; state.y = s.y;
            Target target{a.profile.target.mapID, a.profile.target.x, a.profile.target.y, a.profile.tolerance};
            activelyTraining = AtTarget(state, target);
        }
        if (activelyTraining) a.rotationActiveTrainMs += delta;

        if (!NoFullBagWindowReached(a.rotationActiveTrainMs, a.profile.rotateNoFullBagMin)) return false;
        const std::wstring reason = L"train thực " + std::to_wstring(a.profile.rotateNoFullBagMin) + L" phút chưa FULL túi";
        if (!SwitchToNextRotationSpot(a, now, reason)) return false;
        BeginTrainRecovery(a, now);
        return true;
    }

    void ResetRuntimeForLifeBoundary(Account& a) {
        // World Flow/FIFO ownership lives partly outside RuntimeState (tradeHeld + queue).
        // Preserve only the immutable FIFO ticket across death/alive hard resets; all travel
        // phases restart cleanly so the same held account can AutoPath to TỌA GD again.
        const std::uint64_t workflowTicket = a.runtime.tradeWorkflowEntrySeq;
        const bool preserveWorkflowTicket = a.tradeHeld && workflowTicket != 0;
        ResetRuntime(a.runtime);
        if (preserveWorkflowTicket) a.runtime.tradeWorkflowEntrySeq = workflowTicket;
    }

    bool HandleDeath(Account& a, DWORD now) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;

        // Life state is authoritative for the death-session boundary. If it becomes
        // temporarily unavailable while a death session is latched, fail closed and
        // preserve the latch/timers instead of silently returning to normal automation.
        if ((s.validMask & ValidLifeState) == 0) {
            if (a.deathSessionLatched) {
                rt.status = L"DEATH SESSION • chờ life-state authoritative";
                return true;
            }
            return false;
        }

        if (!s.dead) {
            if (!a.deathSessionLatched) return false;
            a.rotationMetricTick = now;

            // SECOND boundary reset: the character is alive again on a stable client
            // snapshot. Wipe every revive/travel/fight/sell/confirm/watchdog phase and
            // resume exactly like a fresh BẮT ĐẦU, while AccountProfile/settings and
            // the existing Bridge attachment remain intact.
            ResetRuntimeForLifeBoundary(a);
            a.deathSessionLatched = false;
            rt.routeOwnershipResetPending = true;
            rt.status = L"ALIVE • cold restart + chuẩn hóa ownership AutoPath";
            LogAccount(a, L"POST-REVIVE COLD START: ResetRuntime toàn bộ • giữ nguyên setting/bãi/click • phiên auto mới.");
            return true;
        }

        if (!a.deathSessionLatched) {
            a.rotationMetricTick = now;
            RecordDeathForRotation(a, now);
            // FIRST boundary reset: a new authoritative death is a hard session
            // boundary. Never carry ANY runtime state from the previous life. The
            // lifecycle latch is outside RuntimeState so this full reset cannot cause
            // a repeated-reset loop while the same dead snapshot remains true.
            ResetRuntimeForLifeBoundary(a);
            a.deathSessionLatched = true;
            rt.deadSinceTick = now;
            rt.status = L"DEAD • hard reset runtime đời trước";
            LogAccount(a, L"NEW DEATH SESSION: HARD ResetRuntime toàn bộ • coi như AUTO vừa được bật lại từ đầu.");
        }

        rt.status = L"Nhân vật đang chết";
        if (!a.profile.enableRevive) {
            rt.status = L"CHẾT • chờ Đầu thai thủ công";
            return true;
        }
        if (rt.revivePhase == 0 && Elapsed(now, rt.deadSinceTick, 500) &&
            (rt.lastReviveClickTick == 0 || Elapsed(now, rt.lastReviveClickTick, 5000))) {
            // The Revive callback is emitted by the per-account P2 priority pass. Keep this
            // path fail-closed for the same client without blocking unrelated windows.
            rt.status = L"ĐẦU THAI đến hạn • chờ P2 cục bộ của chính acc";
            return true;
        }
        if (rt.revivePhase == 1 && Elapsed(now, rt.revivePhaseTick, 900)) {
            // Map Confirm is not injected as a special revive action; global P1 Lâu Lan
            // watchdog owns its own internal MessageBox callback.
            rt.revivePhase = 2;
            rt.revivePhaseTick = now;
            rt.status = L"Đầu thai đã gửi • chờ sống lại; World Flow vẫn HOLD và sẽ resume";
            return true;
        }
        if (rt.revivePhase == 2 && Elapsed(now, rt.revivePhaseTick, 4500)) {
            rt.revivePhase = 0;
            rt.revivePhaseTick = now;
        }
        return true;
    }

    bool HandleRouteOwnershipReset(Account& a, DWORD now) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        if (!rt.routeOwnershipResetPending) return false;

        // This is the missing game-side half of a true cold start. ResetRuntime()
        // clears controller ownership flags, but the client may preserve AutoPath=ON
        // across death/revive. If we accepted that stale path as our route, then
        // crossMapRouteArmed would stay false forever and Confirm would fail closed.
        if ((s.validMask & ValidAutoPath) == 0) {
            rt.status = L"SESSION ROUTE RESET • chờ AutoPath authoritative";
            return true;
        }

        if (!s.autoPathing) {
            rt.routeOwnershipResetPending = false;
            rt.routeOwnershipStopTick = 0;
            rt.routeOwnershipStopAttempts = 0;
            rt.crossMapRouteArmed = false;
            rt.crossMapRouteMoved = false;
            rt.crossMapSeenAutoPath = false;
            rt.confirmAttempts = 0;
            rt.status = L"SESSION ROUTE RESET • AutoPath OFF • ownership sạch";
            if (!rt.routeOwnershipResetLogged) {
                LogAccount(a, L"SESSION ROUTE RESET PASS: AutoPath OFF → route kế tiếp phải do tool StartPath mới để arm Confirm.");
                rt.routeOwnershipResetLogged = true;
            }
            return true; // one-cycle barrier before normal route logic
        }

        rt.routeOwnershipResetLogged = false;
        if (rt.routeOwnershipStopAttempts >= kRouteOwnershipStopMaxAttempts &&
            rt.routeOwnershipStopTick != 0 &&
            Elapsed(now, rt.routeOwnershipStopTick, kRouteOwnershipStopRetryMs)) {
            rt.status = L"SESSION ROUTE RESET • AutoPath cũ vẫn ON sau 3 StopPath • fail-closed";
            return true;
        }

        if (rt.routeOwnershipStopTick == 0 || Elapsed(now, rt.routeOwnershipStopTick, kRouteOwnershipStopRetryMs)) {
            if (SendDecision(a, Action::StopPath, a.profile.target, L"session route ownership reset")) {
                ++rt.routeOwnershipStopAttempts;
                rt.routeOwnershipStopTick = now;
                rt.status = L"SESSION ROUTE RESET • phát hiện AutoPath cũ ON → StopPath, chờ verify OFF";
                LogAccount(a, L"SESSION ROUTE RESET: AutoPath=ON nhưng controller vừa cold-reset → StopPath để xóa path đời trước trước khi route mới.");
            } else {
                rt.status = L"SESSION ROUTE RESET • chờ gửi StopPath fail-closed";
            }
        } else {
            rt.status = L"SESSION ROUTE RESET • đã StopPath → chờ snapshot AutoPath OFF";
        }
        return true;
    }

    bool CurrentTravelDestinationMap(const Account& a, int& destinationMap) const {
        const RuntimeState& rt = a.runtime;

        // SELL phase 4 is the only phase currently travelling to the NPC. Phase 8 travels
        // back to the training target. Other SELL phases are parked/working in place.
        if (rt.sellPhase == 4) {
            const TargetProfile npc = SellNpcTarget(a);
            if (!npc.valid) return false;
            destinationMap = npc.mapID;
            return destinationMap > 0;
        }
        if (rt.sellPhase == 8 || rt.trainRecoveryPhase != 0) {
            destinationMap = a.profile.target.mapID;
            return destinationMap > 0;
        }

        // Normal training/rotation route uses the current profile target. Trade-held
        // accounts are advanced outside TickAccount and are already protected directly
        // by the shared Mount/StartPath Travel Guard.
        if (!a.tradeHeld && rt.sellPhase == 0) {
            destinationMap = a.profile.target.mapID;
            return destinationMap > 0;
        }
        return false;
    }

    bool HandleUnderworldAutoFightGuard(Account& a, DWORD now) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        int destinationMap = 0;
        const bool hasTravelDestination = CurrentTravelDestinationMap(a, destinationMap);
        if (!travel_fight_guard_logic::ShouldGuardUnderworldExit(
                s.mapID, destinationMap, hasTravelDestination, kUnderworldMapId)) {
            rt.underworldGuardLogged = false;
            return false;
        }
        if ((s.validMask & ValidAutoFight) == 0) {
            rt.status = L"ĐỊA PHỦ M87 • chờ AutoFight authoritative • Travel Guard fail-closed";
            return true;
        }
        if (!s.autoFight) {
            if (!rt.underworldGuardLogged) {
                LogAccount(a, L"ĐỊA PHỦ M87: dùng Travel Guard chung • AutoFight OFF → route được phép tiếp tục.");
                rt.underworldGuardLogged = true;
            }
            ResetTravelFightGuard(rt);
            return false;
        }
        rt.underworldGuardLogged = false;
        if (!EnsureAutoFightOffForTravel(a, now, L"rời Địa Phủ M87")) {
            rt.status = L"ĐỊA PHỦ M87 • Travel Guard đang tắt AutoFight • CẤM route khi còn ON";
            return true;
        }
        return false;
    }

    bool HandleFightClicks(Account& a, DWORD now) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        if (!a.profile.enableFight) {
            rt.fightPhase = 0;
            rt.fightAttempts = 0;
            rt.fightRetryWaitTick = 0;
            return false;
        }
        if ((s.validMask & ValidAutoFight) == 0) {
            rt.status = L"Đúng bãi • chờ đọc trạng thái AutoFight";
            return true;
        }
        if (s.autoFight) {
            rt.fightPhase = 3;
            rt.fightAttempts = 0;
            rt.fightRetryWaitTick = 0;
            if (!rt.trainPositionMonitorArmed) {
                rt.trainPositionMonitorArmed = true;
                rt.lastTrainPositionCheckTick = now;
                LogAccount(a, L"AutoFight ON • bắt đầu check tọa độ train 1 phút/lần.");
            }
            rt.lastAutoFightCheckTick = now;
            rt.status = L"Đúng bãi • AutoFight ON • check Auto mỗi 1 phút";
            return true;
        }
        if (rt.fightAttempts >= auto_fight_retry_logic::kImmediateAttemptLimit) {
            const auto retryDecision = auto_fight_retry_logic::DecideExhaustedRetry(
                now, rt.fightRetryWaitTick, kAutoFightRecheckMs);
            if (retryDecision == auto_fight_retry_logic::ExhaustedRetryDecision::StartWait) {
                rt.fightRetryWaitTick = now;
                rt.fightPhase = 3;
                rt.status = L"P3 AUTO→Đánh quái thử 2 lần • bắt đầu chờ retry 60s";
                LogAccount(a, L"P3 AUTO RETRY: 2 lần chưa bật được AutoFight • neo timer 60s một lần, không reset mỗi tick.");
                return true;
            }
            if (retryDecision == auto_fight_retry_logic::ExhaustedRetryDecision::KeepWaiting) {
                const DWORD elapsedMs = now - rt.fightRetryWaitTick;
                const DWORD remainSec = elapsedMs >= kAutoFightRecheckMs
                    ? 0 : (kAutoFightRecheckMs - elapsedMs + 999) / 1000;
                rt.status = L"P3 AUTO→Đánh quái thử 2 lần • retry sau " +
                            std::to_wstring(remainSec) + L"s";
                return true;
            }
            rt.fightAttempts = 0;
            rt.fightPhase = 0;
            rt.fightRetryWaitTick = 0;
            LogAccount(a, L"P3 AUTO RETRY 60s: AutoFight vẫn OFF → cấp lại 2 lần AUTO→Đánh quái.");
        }
        if (rt.fightPhase == 3) rt.fightPhase = 0;

        bool ok = false;
        DWORD clickedAt = 0;
        if (rt.fightPhase == 0) {
            if (ConsumePriorityAutoResult(a, ClickSlot::Attack,
                                          PriorityAutoOwner::Train,
                                          ok, clickedAt)) {
                ++rt.fightAttempts;
                if (ok) {
                    rt.fightPhase = 2;
                    rt.fightPhaseTick = clickedAt;
                    rt.status = L"P3 AUTO INPUTSYNC • đủ 2 click • verify AutoFight";
                } else {
                    rt.status = L"P3 AUTO INPUTSYNC • sequence fail lần " +
                                std::to_wstring(rt.fightAttempts) + L"/2";
                }
                return true;
            }
            (void)QueuePriorityAutoClick(a, ClickSlot::Attack,
                                         PriorityAutoOwner::Train,
                                         L"TRAIN: InputSync AUTO→ĐÁNH QUÁI");
            rt.status = L"P3 AUTO INPUTSYNC • chờ Priority #3 chạy click 1→2";
            return true;
        }
        if (rt.fightPhase == 2 && Elapsed(now, rt.fightPhaseTick, 1500)) {
            if (s.autoFight) {
                rt.fightPhase = 3;
                rt.fightAttempts = 0;
                rt.fightRetryWaitTick = 0;
                rt.lastAutoFightCheckTick = now;
                if (!rt.trainPositionMonitorArmed) {
                    rt.trainPositionMonitorArmed = true;
                    rt.lastTrainPositionCheckTick = now;
                }
                rt.status = L"AutoFight ON • P3 InputSync bật thành công";
                LogAccount(a, L"PRIORITY #3 AUTO→ĐÁNH QUÁI InputSync verify AutoFight ON.");
                return true;
            }
            if (rt.fightAttempts < 2) {
                rt.fightPhase = 0;
                rt.fightPhaseTick = now;
                return true;
            }
        }
        return true;
    }

    bool LauLanGateConfirmDue(Account& a, DWORD now) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        if (!a.profile.enableConfirm) return false;
        if (!a.snapshotValid || rt.clientFreezeActive || globalPaused_ || RecorderBlocksAccount(a)) return false;
        if ((s.validMask & (ValidMap | ValidPosition | ValidAutoPath | ValidLifeState)) !=
            (ValidMap | ValidPosition | ValidAutoPath | ValidLifeState)) return false;
        if (!s.mapReady || s.waitingChangeMap || s.dead) return false;

        // Lâu Lan watchdog is completely dormant outside Map 5.
        if (s.mapID != kLauLanMapId) {
            rt.stallSinceTick = 0;
            rt.confirmAttempts = 0;
            rt.lastLauLanConfirmTick = 0;
            return false;
        }

        // Only a tool-owned cross-map route that actually moved can arm the gate watchdog.
        // Standing still in Lâu Lan without a live route can never cause a blind XN click.
        if (!rt.crossMapRouteArmed || !rt.crossMapRouteMoved || !rt.crossMapSeenAutoPath) return false;
        // The gate condition is specifically: AutoPath is still ON but position has stalled.
        // World Flow / SELL / GD UI sub-state must not suppress this P1 observer.
        if (!s.autoPathing) return false;
        if (rt.lastMovementTick == 0 || !Elapsed(now, rt.lastMovementTick, kLauLanGateStallMs)) {
            rt.stallSinceTick = 0;
            return false;
        }
        if (rt.stallSinceTick == 0) rt.stallSinceTick = rt.lastMovementTick;
        if (rt.lastLauLanConfirmTick != 0 && !Elapsed(now, rt.lastLauLanConfirmTick, kLauLanConfirmRetryMs)) return false;
        return true;
    }

    bool PriorityLauLanGateConfirmClick(Account& a, DWORD now) {
        if (!LauLanGateConfirmDue(a, now)) return false;
        std::wstring error;
        Response response{};
        const bool ok = a.bridge.Call(Command::ConfirmMap, 0, 0, 0, response, error, 2200);
        const DWORD clickedAt = GetTickCount();

        a.runtime.lastLauLanConfirmTick = clickedAt;
        if (ok) {
            ++a.runtime.confirmAttempts;
            a.runtime.lastMovementTick = clickedAt; // require a fresh full 3s stall before retry
            a.runtime.stallSinceTick = clickedAt;
            a.runtime.status = L"LÂU LAN M5 • AutoPath đứng ~3s → CALLBACK XN NỘI BỘ • lần " +
                               std::to_wstring(a.runtime.confirmAttempts);
            LogAccount(a, L"LÂU LAN GATE WATCHDOG P1: tìm MessageBox + gọi callback nút đồng ý nội bộ • KHÔNG foreground/chuột • lần " +
                          std::to_wstring(a.runtime.confirmAttempts));
            TelegramRecordLauLanConfirm(a, a.runtime.confirmAttempts);
            return true;
        }
        if (BridgeLooksUnresponsive(error)) EnterClientFreeze(a, L"Bridge timeout khi XN map nội bộ", clickedAt);
        LogAccount(a, L"LÂU LAN GATE XN NỘI BỘ FAIL: " + error);
        return false;
    }

    bool PrimeDeathSessionForPriorityRevive(Account& a, DWORD now) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        if (!a.runtime.running || !a.snapshotValid || !IsWindow(a.game.window) || rt.clientFreezeActive) return false;
        if (!s.mapReady || s.waitingChangeMap) return false;
        if ((s.validMask & ValidLifeState) == 0 || !s.dead) return false;
        if (a.deathSessionLatched) return true;

        // Keep the existing FIRST-death boundary in the priority pre-pass so a click
        // sequence in another window cannot postpone detecting this account's death.
        // HandleDeath() sees deathSessionLatched and therefore does not reset twice.
        a.rotationMetricTick = now;
        RecordDeathForRotation(a, now);
        ResetRuntimeForLifeBoundary(a);
        a.deathSessionLatched = true;
        rt.deadSinceTick = now;
        rt.status = L"DEAD • hard reset runtime đời trước • chờ ĐẦU THAI P2";
        LogAccount(a, L"NEW DEATH SESSION: HARD ResetRuntime toàn bộ • P2 ĐẦU THAI đã nhận death trước auto click thường.");
        return true;
    }

    bool PriorityReviveDue(Account& a, DWORD now) {
        if (!PrimeDeathSessionForPriorityRevive(a, now)) return false;
        const RuntimeState& rt = a.runtime;
        if (!a.profile.enableRevive) return false;
        if (rt.revivePhase != 0 || rt.deadSinceTick == 0) return false;
        if (!Elapsed(now, rt.deadSinceTick, 500)) return false;
        return rt.lastReviveClickTick == 0 || Elapsed(now, rt.lastReviveClickTick, 5000);
    }

    bool PriorityReviveClick(Account& a, DWORD now) {
        if (!PriorityReviveDue(a, now)) return false;
        std::wstring error;
        Response response{};
        const bool ok = a.bridge.Call(Command::Revive, 0, 0, 0, response, error, 2200);
        const DWORD clickedAt = GetTickCount();

        if (ok) {
            a.runtime.lastReviveClickTick = clickedAt;
            a.runtime.revivePhase = 1;
            a.runtime.revivePhaseTick = clickedAt;
            a.runtime.status = L"ĐẦU THAI NỘI BỘ PASS • callback đúng acc chết • không chiếm chuột";
            LogAccount(a, L"ĐẦU THAI P2 PASS: Bridge xác minh IsDeath rồi gọi UIButton.HandleClickEvent nội bộ; chuỗi acc khác không mất index/repeat.");
            return true;
        }
        if (BridgeLooksUnresponsive(error)) EnterClientFreeze(a, L"Bridge timeout khi Đầu thai nội bộ", clickedAt);
        LogAccount(a, L"ĐẦU THAI NỘI BỘ FAIL: " + error);
        return false;
    }

    bool AutoFightCheckBusy(const Account& a, DWORD now) const {
        const RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        // Hard exclusion gate: an AutoFight check/click sequence may run only when the
        // account is completely idle at the train spot. Do not interleave with any
        // route, mount, death, sell, recovery or another click operation.
        if (rt.sellPhase != 0 || rt.trainRecoveryPhase != 0 || rt.revivePhase != 0) return true;
        if (a.tradeHeld || rt.tradeTravelPhase != 0 || rt.tradeTravelReady) return true;
        if (rt.travelFightGuardPhase != 0 || rt.travelFightStopAttempts != 0) return true;
        if (rt.autoPathFightConflictLatched) return true;
        if (rt.travelMountAttempts != 0 || rt.travelFightBoostPhase != 0 ||
            rt.travelFootFallback) return true;
        if (rt.crossMapRouteArmed || rt.crossMapRouteMoved) return true;
        if (s.riding || s.autoPathing || s.waitingChangeMap || !s.mapReady) return true;
        if ((s.validMask & ValidLifeState) && s.dead) return true;
        return false;
    }

    void ResetRobustTravel(RuntimeState& rt) {
        rt.travelMountAttempts = 0;
        rt.travelMountTick = 0;
        rt.travelMountCycle = 0;
        rt.travelFightBoostPhase = 0;
        rt.travelFightBoostTick = 0;
        rt.travelFootFallback = false;
        rt.travelFootTick = 0;
    }

    bool HandleMountFightBoost(Account& a, DWORD now, const wchar_t* context) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        const std::wstring where = context ? context : L"đích";
        if ((s.validMask & ValidAutoFight) == 0) {
            rt.status = L"MOUNT RECOVERY • chờ AutoFight authoritative trước boost 10s";
            return true;
        }

        bool ok = false;
        DWORD clickedAt = 0;
        if (rt.travelFightBoostPhase == 0) {
            if (s.autoFight) {
                rt.travelFightBoostPhase = 5;
                rt.travelFightBoostTick = now;
                rt.status = L"MOUNT RECOVERY • AutoFight đã ON → đánh thêm 10s";
                LogAccount(a, L"MOUNT RECOVERY: 2 lần lên ngựa fail; AutoFight đang ON → tính 10s đánh quái.");
                return true;
            }
            if (QueuePriorityAutoClick(a, ClickSlot::Attack,
                                       PriorityAutoOwner::MountRecovery,
                                       L"MOUNT RECOVERY: chạy AUTO→ĐÁNH QUÁI InputSync trước boost 10s")) {
                rt.travelFightBoostPhase = 1;
            }
            return true;
        }
        if (rt.travelFightBoostPhase == 1) {
            if (!ConsumePriorityAutoResult(a, ClickSlot::Attack,
                                           PriorityAutoOwner::MountRecovery,
                                           ok, clickedAt)) return true;
            if (!ok) { rt.travelFightBoostPhase = 0; return true; }
            rt.travelFightBoostPhase = 4;
            rt.travelFightBoostTick = clickedAt;
            rt.status = L"MOUNT RECOVERY • đã chạy 2 click AUTO nội bộ • verify ON";
            return true;
        }
        if (rt.travelFightBoostPhase == 4) {
            if (s.autoFight) {
                rt.travelFightBoostPhase = 5;
                rt.travelFightBoostTick = now;
                rt.status = L"MOUNT RECOVERY • AutoFight ON → đánh thêm 10s";
                LogAccount(a, L"MOUNT RECOVERY: AUTO→ĐÁNH QUÁI InputSync verify ON → đánh thêm 10s trước lần lên ngựa kế.");
                return true;
            }
            if (Elapsed(now, rt.travelFightBoostTick, 1500)) {
                rt.travelFightBoostPhase = 0;
                rt.status = L"MOUNT RECOVERY • chưa bật được Đánh quái → P3 retry";
            }
            return true;
        }
        if (rt.travelFightBoostPhase == 5) {
            if (!Elapsed(now, rt.travelFightBoostTick, kMountFightBoostMs)) {
                const DWORD sec = (now - rt.travelFightBoostTick) / 1000;
                rt.status = L"MOUNT RECOVERY • đánh quái " + std::to_wstring(sec) + L"/10s • " + where;
                return true;
            }
            // After the 10-second fight boost, use exactly the same fail-closed Travel Guard
            // to stop AutoFight before the second mount x2 cycle begins.
            if (!EnsureAutoFightOffForTravel(a, now, L"sau boost 10s trước lên ngựa lại")) {
                rt.status = L"MOUNT RECOVERY • đủ 10s → P3 DỪNG AUTO • chờ OFF";
                return true;
            }
            rt.travelFightBoostPhase = 0;
            rt.travelFightBoostTick = 0;
            rt.travelMountCycle = 1;
            rt.travelMountAttempts = 0;
            rt.travelMountTick = 0;
            rt.status = L"MOUNT RECOVERY • AutoFight OFF → lặp lại lên ngựa x2";
            LogAccount(a, L"MOUNT RECOVERY: đánh 10s xong + AutoFight OFF → bắt đầu chu kỳ lên ngựa x2 lần thứ hai.");
            return true;
        }
        rt.travelFightBoostPhase = 0;
        return true;
    }

    bool HandleRobustTravelDirect(Account& a, DWORD now, const TargetProfile& targetProfile,
                                  const wchar_t* context, bool& arrived, int toleranceOverride = 0) {
        arrived = false;
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        State logic{};
        logic.valid = true; logic.mapReady = true; logic.waitingMap = false;
        logic.mapID = s.mapID; logic.x = s.x; logic.y = s.y;
        logic.riding = s.riding != 0; logic.autoPathing = s.autoPathing != 0;
        const int travelTolerance = toleranceOverride > 0 ? toleranceOverride : a.profile.tolerance;
        Target target{targetProfile.mapID, targetProfile.x, targetProfile.y, travelTolerance};
        const std::wstring where = context ? context : L"đích";

        if (AtTarget(logic, target)) {
            if (s.autoPathing) {
                (void)SendDecision(a, Action::StopPath, targetProfile, context);
                return true;
            }
            if (s.riding) {
                (void)SendDecision(a, Action::Dismount, targetProfile, context);
                return true;
            }
            ResetRobustTravel(rt);
            ResetTravelFightGuard(rt);
            (void)CompleteToolOwnedRoute(rt, true, s.autoPathing != 0, s.riding != 0);
            arrived = true;
            return true;
        }

        // Once mount #1/#2 have both timed out in the first cycle, do not immediately
        // walk. v0.3 performs P3 AUTO→Đánh quái for 10s, stops it via Travel Guard,
        // then grants a fresh second mount x2 cycle.
        if (rt.travelFightBoostPhase != 0) {
            return HandleMountFightBoost(a, now, context);
        }

        const DWORD phaseElapsed = rt.travelFootFallback
            ? (rt.travelFootTick == 0 ? 0 : now - rt.travelFootTick)
            : (rt.travelMountTick == 0 ? 0 : now - rt.travelMountTick);
        const MountAssistAction assist = DecideMountAssist(s.riding != 0, s.autoPathing != 0,
                                                           rt.travelMountAttempts, rt.travelFootFallback,
                                                           phaseElapsed, kMountRetryWaitMs);
        if (s.riding) {
            const int completedCycle = rt.travelMountCycle;
            ResetRobustTravel(rt);
            if (assist == MountAssistAction::StartPath) {
                (void)SendDecision(a, Action::StartPath, targetProfile, context, travelTolerance);
            } else {
                rt.status = L"Đang cưỡi ngựa AutoPath tới " + where;
            }
            if (completedCycle == 1) LogAccount(a, L"MOUNT RECOVERY PASS: lên ngựa thành công sau boost 10s.");
            return true;
        }
        if (rt.travelFootFallback) {
            // Compatibility cleanup for a stale runtime created by an older build.
            // Walking AutoPath is no longer permitted under any route.
            if (s.autoPathing) (void)SendDecision(a, Action::StopPath, targetProfile, context);
            ResetRobustTravel(rt);
            ResetTravelFightGuard(rt);
            rt.status = L"MOUNT REQUIRED • đã hủy trạng thái chạy bộ cũ, quay lại chu kỳ lên ngựa";
            return true;
        }
        if (assist == MountAssistAction::Wait) {
            rt.status = rt.travelMountAttempts <= 1 ? L"Chờ lên ngựa lần 1 • tối đa 5s" : L"Chờ lên ngựa lần 2 • tối đa 5s";
            return true;
        }
        if (assist == MountAssistAction::Mount) {
            if (SendDecision(a, Action::Mount, targetProfile, context)) {
                ++rt.travelMountAttempts;
                if (rt.travelMountAttempts > 2) rt.travelMountAttempts = 2;
                rt.travelMountTick = now;
                rt.status = rt.travelMountAttempts == 1 ? L"Lên ngựa lần 1 • chờ 5s" : L"Lên ngựa lần 2 • chờ 5s";
            } else {
                rt.status = L"Chờ gửi lệnh lên ngựa • chưa tính lần thử";
            }
            return true;
        }

        // DecideMountAssist reaches MountCycleFailed here only after two mount attempts timed out.
        if (rt.travelMountCycle == 0) {
            rt.travelFightBoostPhase = 0;
            return HandleMountFightBoost(a, now, context);
        }

        // Second mount x2 cycle also failed. StartPath remains forbidden on foot;
        // reset and retry the mount workflow instead of silently walking.
        ResetRobustTravel(rt);
        ResetTravelFightGuard(rt);
        rt.status = L"MOUNT REQUIRED • 2 chu kỳ chưa lên được ngựa • lặp lại, tuyệt đối không StartPath chạy bộ";
        LogAccount(a, L"MOUNT REQUIRED: Mount x2 → Fight10s → Mount x2 vẫn fail • reset chu kỳ; KHÔNG chạy bộ AutoPath.");
        return true;
    }

    void ResetShortcutRoute(RuntimeState& rt) {
        rt.shortcutKind = ShortcutKind::None;
        rt.shortcutPhase = 0;
        rt.shortcutFinalMap = 0;
        rt.shortcutExpectedMap = 0;
        rt.shortcutSourceMap = 0;
        rt.shortcutClickIndex = 0;
        rt.shortcutTick = 0;
        rt.shortcutAttempts = 0;
        ResetRobustTravel(rt);
        ResetTravelFightGuard(rt);
    }

    bool ShortcutBridgeCall(Account& a, Command command, int arg0, const std::wstring& label,
                            DWORD now, int timeoutMs = 2200) {
        if (a.runtime.clientFreezeActive) return false;
        std::wstring error;
        if (!EnsureAttach(a, error)) {
            a.runtime.status = L"ĐƯỜNG TẮT • không attach được Bridge: " + error;
            return false;
        }
        Response r{};
        if (!a.bridge.Call(command, arg0, 0, 0, r, error, timeoutMs)) {
            if (BridgeLooksUnresponsive(error)) EnterClientFreeze(a, L"Bridge timeout trong đường tắt", now);
            a.runtime.status = L"ĐƯỜNG TẮT • " + label + L" chưa pass: " + error;
            return false;
        }
        LogAccount(a, L"ĐƯỜNG TẮT PASS • " + label + L" • " + r.detail);
        return true;
    }

    void FailShortcutRoute(Account& a, const std::wstring& reason) {
        a.runtime.shortcutPhase = 99;
        a.runtime.status = L"ĐƯỜNG TẮT FAIL-CLOSED • " + reason;
        ResetRobustTravel(a.runtime);
        ResetTravelFightGuard(a.runtime);
        LogAccount(a, L"ĐƯỜNG TẮT FAIL-CLOSED: " + reason + L" • không click/chuyển tiếp mù.");
    }

    TargetProfile ShortcutWorldTarget(const wchar_t* name, int mapID, int worldX, int worldY) const {
        TargetProfile t{}; t.name = name; t.mapID = mapID; t.x = worldX; t.y = worldY;
        t.valid = mapID > 0 && worldX > 0 && worldY > 0;
        return t;
    }

    bool ShortcutTravelLeg(Account& a, DWORD now, const TargetProfile& leg, const wchar_t* label, bool& arrived) {
        arrived = false;
        if (!leg.valid) {
            FailShortcutRoute(a, std::wstring(L"chưa gán tọa: ") + label + L" • lấy vị trí đúng nguồn cấu hình trước");
            return true;
        }
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        if (!a.snapshotValid || !s.mapReady || s.waitingChangeMap ||
            (s.validMask & (ValidMap | ValidPosition | ValidAutoPath | ValidRiding)) !=
            (ValidMap | ValidPosition | ValidAutoPath | ValidRiding)) {
            rt.status = std::wstring(L"ĐƯỜNG TẮT • chờ state ổn định trước FSM tới ") + label;
            return true;
        }

        // v3.2: use exactly the same travel machine as normal sell/train travel.
        // Never bypass mount assist with a direct StartPath for a shortcut waypoint.
        return HandleRobustTravelDirect(a, now, leg, label, arrived, kPreciseWorldTolerance);
    }

    bool HandleKunLunExitTryClickRoute(Account& a, DWORD now, const TargetProfile& finalTarget,
                                       const TargetProfile& npcPoint) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        if (rt.shortcutPhase == 99) return true;

        if (rt.shortcutPhase <= 1) {
            bool reached = false;
            (void)ShortcutTravelLeg(a, now, npcPoint, L"NPC RỜI Côn Lôn Sơn", reached);
            if (!reached) { rt.shortcutPhase = 1; return true; }
            for (std::size_t i = 0; i < shortcutSettings_.kunlunExitClicks.size(); ++i) {
                const TimedClickPoint& click = shortcutSettings_.kunlunExitClicks[i];
                if (!click.point.valid) {
                    FailShortcutRoute(a, L"thiếu tọa TryClickUI " + std::to_wstring(i + 1) +
                                         L"/3 của NPC RỜI Côn Lôn; phải F8 đủ Mở NPC → Đại Lý → Xác nhận");
                    return true;
                }
                if (click.timeMs < 0 || click.timeMs > 60000 || click.delayMs < 0 || click.delayMs > 60000) {
                    FailShortcutRoute(a, L"Time/Delay click " + std::to_wstring(i + 1) + L"/3 ngoài khoảng 0..60000 ms");
                    return true;
                }
            }
            rt.shortcutSourceMap = s.mapID;
            rt.shortcutClickIndex = 0;
            rt.shortcutPhase = 2;
            rt.shortcutTick = now;
            rt.shortcutAttempts = 0;
            rt.status = L"ĐƯỜNG TẮT CLS • đã tới đúng NPC rời Côn Lôn • chuẩn bị chuỗi 3 TryClickUI";
            LogAccount(a, L"ĐƯỜNG TẮT CLS ARM 3-CLICK • NPC RỜI Côn Lôn (khác Xa Truyền Bình ID387 đi vào Côn Lôn) • không còn callback Đại Lý/Xác nhận.");
            return true;
        }

        if (rt.shortcutPhase == 2) {
            if (rt.shortcutClickIndex < 0 || rt.shortcutClickIndex >= 3) {
                FailShortcutRoute(a, L"index chuỗi 3 TryClickUI không hợp lệ");
                return true;
            }
            const int index = rt.shortcutClickIndex;
            const TimedClickPoint& click = shortcutSettings_.kunlunExitClicks[static_cast<std::size_t>(index)];
            const DWORD previousDelay = index == 0 ? 0u :
                static_cast<DWORD>(shortcutSettings_.kunlunExitClicks[static_cast<std::size_t>(index - 1)].delayMs);
            const DWORD waitBefore = previousDelay + static_cast<DWORD>(click.timeMs);
            if (!Elapsed(now, rt.shortcutTick, waitBefore)) {
                rt.status = L"ĐƯỜNG TẮT CLS • chờ timing click " + std::to_wstring(index + 1) + L"/3";
                return true;
            }

            std::wstring error;
            static constexpr const wchar_t* labels[3] = {L"mở NPC", L"chọn Đại Lý", L"Xác nhận"};
            if (!DispatchInternalPointActionDirect(a, click.point,
                    L"CLS TryClickUI " + std::to_wstring(index + 1) + L"/3 • " + labels[index], error)) {
                FailShortcutRoute(a, L"TryClickUI " + std::to_wstring(index + 1) + L"/3 thất bại: " + error);
                return true;
            }
            ++rt.shortcutClickIndex;
            rt.shortcutTick = now;
            if (rt.shortcutClickIndex == 3) {
                rt.shortcutPhase = 3;
                rt.status = L"ĐƯỜNG TẮT CLS • đã click đủ 3/3 • chờ Delay click 3 rồi mới check MapID đổi";
                LogAccount(a, L"ĐƯỜNG TẮT CLS 3-CLICK PASS • đã chạy đủ Mở NPC → Đại Lý → Xác nhận; bây giờ mới bắt đầu check chuyển map.");
            } else {
                rt.status = L"ĐƯỜNG TẮT CLS • click " + std::to_wstring(rt.shortcutClickIndex) +
                            L"/3 PASS • chờ Delay + Time của click kế tiếp";
            }
            return true;
        }

        if (rt.shortcutPhase == 3) {
            const DWORD finalDelay = static_cast<DWORD>(shortcutSettings_.kunlunExitClicks[2].delayMs);
            if (!Elapsed(now, rt.shortcutTick, finalDelay)) return true;
            if (a.snapshotValid && (s.validMask & ValidMap) == ValidMap &&
                s.mapID != rt.shortcutSourceMap && s.mapReady && !s.waitingChangeMap) {
                LogAccount(a, L"ĐƯỜNG TẮT CLS CHUYỂN MAP PASS • M" + std::to_wstring(rt.shortcutSourceMap) +
                              L" → M" + std::to_wstring(s.mapID) + L" • trả về AutoPath đích M" +
                              std::to_wstring(finalTarget.mapID));
                ResetShortcutRoute(rt);
                return false;
            }
            if (Elapsed(now, rt.shortcutTick, finalDelay + 15000u)) {
                FailShortcutRoute(a, L"đã chạy đủ 3 TryClickUI nhưng MapID vẫn chưa đổi sau thời gian chờ");
            } else {
                rt.status = L"ĐƯỜNG TẮT CLS • đã đủ 3 click • chờ MapID đổi rồi mới AutoPath đích";
            }
            return true;
        }
        return true;
    }

    bool HandleShortcutNpcRoute(Account& a, DWORD now, const TargetProfile& finalTarget,
                                const TargetProfile& npcPoint, int npcID,
                                TravelSemantic semantic, int expectedMap, const wchar_t* label,
                                TravelSemantic intermediateSemantic = TravelSemantic::None,
                                bool destinationChangesMapImmediately = false) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        if (rt.shortcutPhase == 99) return true;

        if (rt.shortcutPhase <= 1) {
            bool reached = false;
            (void)ShortcutTravelLeg(a, now, npcPoint, label, reached);
            if (!reached) { rt.shortcutPhase = 1; return true; }
            rt.shortcutPhase = 2; rt.shortcutTick = now; rt.shortcutAttempts = 0;
            rt.status = std::wstring(L"ĐƯỜNG TẮT • đã tới ") + label + L" • chuẩn bị mở NPC";
            return true;
        }

        if (rt.shortcutPhase == 2) {
            const bool opened = ShortcutBridgeCall(a, Command::ClickNpc, npcID,
                                                   L"ClickNPC(" + std::to_wstring(npcID) + L")", now);
            if (!opened) {
                ++rt.shortcutAttempts;
                if (rt.shortcutAttempts >= 3)
                    FailShortcutRoute(a, L"không mở được NPC ID " + std::to_wstring(npcID) + L" sau 3 lần có kiểm soát");
                return true;
            }
            rt.shortcutPhase = 3; rt.shortcutTick = now; rt.shortcutAttempts = 0;
            return true;
        }

        if (rt.shortcutPhase == 3) {
            // Do not read the menu in the same/next fast tick as ClickNPC. The
            // v0.1.8 probe had the dialog already open; the production route
            // must first allow the NPC to instantiate its GameDialog tree.
            const DWORD semanticWait = rt.shortcutAttempts == 0
                ? kShortcutNpcUiReadyMs : kShortcutSemanticRetryMs;
            if (!Elapsed(now, rt.shortcutTick, semanticWait)) return true;
            const TravelSemantic firstSemantic = intermediateSemantic == TravelSemantic::None
                ? semantic : intermediateSemantic;
            const std::wstring firstLabel = intermediateSemantic == TravelSemantic::None
                ? L"callback dòng điểm đến semantic"
                : L"callback dòng trung gian Đến các môn phái";
            if (ShortcutBridgeCall(a, Command::ClickTravelSemantic, static_cast<int>(firstSemantic),
                                   firstLabel, now, 7000)) {
                if (intermediateSemantic != TravelSemantic::None) {
                    rt.shortcutPhase = 6;
                } else if (destinationChangesMapImmediately) {
                    rt.shortcutPhase = 5;
                    rt.shortcutExpectedMap = expectedMap;
                    rt.status = L"ĐƯỜNG TẮT • điểm đến tự chuyển map • bỏ Xác nhận • chờ M" +
                                std::to_wstring(expectedMap);
                } else {
                    rt.shortcutPhase = 4;
                }
                rt.shortcutTick = now; rt.shortcutAttempts = 0;
                return true;
            }
            ++rt.shortcutAttempts; rt.shortcutTick = now;
            // Reopening the NPC while its first GameDialog is still being
            // constructed resets the menu and races the resolver. v0.1.8 does
            // one open followed by read-only polling, so production follows the
            // same read-only polling contract for every remaining semantic ClickNPC route.
            if (rt.shortcutAttempts >= kShortcutSemanticMaxAttempts) {
                FailShortcutRoute(a, L"không tìm được đúng 1 dòng điểm đến semantic sau " +
                                  std::to_wstring(kShortcutSemanticMaxAttempts) +
                                  L" lần; xem log SEMANTIC_SCAN để biết NPC/dialog nào đang mở");
            } else {
                rt.status = L"ĐƯỜNG TẮT • đã mở NPC • chờ GameDialog/dòng semantic " +
                            std::to_wstring(rt.shortcutAttempts) + L"/" +
                            std::to_wstring(kShortcutSemanticMaxAttempts);
            }
            return true;
        }

        if (rt.shortcutPhase == 6) {
            // Ngải Ni Ngoã Nhĩ is a two-level GameDialog: first "Đến các môn
            // phái", then the exact "Tinh Túc" destination. Never reopen the NPC
            // between these two callbacks because that would reset menu level 1.
            const DWORD semanticWait = rt.shortcutAttempts == 0
                ? kShortcutNpcUiReadyMs : kShortcutSemanticRetryMs;
            if (!Elapsed(now, rt.shortcutTick, semanticWait)) return true;
            if (ShortcutBridgeCall(a, Command::ClickTravelSemantic, static_cast<int>(semantic),
                                   L"callback dòng Tinh Túc ở menu cấp 2", now, 7000)) {
                if (destinationChangesMapImmediately) {
                    // HỎA DIỆM NO-CONFIRM: callback "Tinh Túc" tự chuyển sang
                    // Tinh Túc Hải M12. Không quét/callback ConfirmTravelSemantic.
                    rt.shortcutPhase = 5;
                    rt.shortcutExpectedMap = expectedMap;
                    rt.status = L"ĐƯỜNG TẮT HỎA • đã callback Tinh Túc • KHÔNG XÁC NHẬN • chờ M" +
                                std::to_wstring(expectedMap);
                    LogAccount(a, L"ĐƯỜNG TẮT HỎA: callback Tinh Túc PASS • game tự dịch sang Tinh Túc Hải • bỏ hoàn toàn bước Xác nhận.");
                } else {
                    rt.shortcutPhase = 4;
                }
                rt.shortcutTick = now; rt.shortcutAttempts = 0;
                return true;
            }
            ++rt.shortcutAttempts; rt.shortcutTick = now;
            if (rt.shortcutAttempts >= kShortcutSemanticMaxAttempts) {
                FailShortcutRoute(a, L"đã chọn Đến các môn phái nhưng " +
                                  std::to_wstring(kShortcutSemanticMaxAttempts) +
                                  L"s vẫn không thấy đúng dòng Tinh Túc ở menu cấp 2");
            } else {
                rt.status = L"ĐƯỜNG TẮT • đã chọn Đến các môn phái • chờ dòng Tinh Túc " +
                            std::to_wstring(rt.shortcutAttempts) + L"/" +
                            std::to_wstring(kShortcutSemanticMaxAttempts);
            }
            return true;
        }

        if (rt.shortcutPhase == 4) {
            // Use the complete v0.1.8 donor resolver: all ACTIVE UIObject instances,
            // HandleClickEvent capability, UI delta and positive confirmation semantic.
            const DWORD confirmWait = rt.shortcutAttempts == 0
                ? kShortcutConfirmUiReadyMs : kShortcutConfirmRetryMs;
            if (!Elapsed(now, rt.shortcutTick, confirmWait)) return true;
            if (ShortcutBridgeCall(a, Command::ConfirmTravelSemantic, 0,
                                   L"callback Xác nhận theo UI-delta v0.1.8", now, 5000)) {
                rt.shortcutPhase = 5; rt.shortcutTick = now; rt.shortcutAttempts = 0;
                rt.shortcutExpectedMap = expectedMap;
                return true;
            }
            ++rt.shortcutAttempts; rt.shortcutTick = now;
            if (rt.shortcutAttempts >= kShortcutConfirmMaxAttempts) {
                FailShortcutRoute(a, L"đã callback dòng điểm đến nhưng " +
                                  std::to_wstring(kShortcutConfirmMaxAttempts) +
                                  L"s vẫn chưa callback được popup ConfirmMap chuẩn");
            } else {
                rt.status = L"ĐƯỜNG TẮT • đã callback điểm đến • chờ popup ConfirmMap " +
                            std::to_wstring(rt.shortcutAttempts) + L"/" +
                            std::to_wstring(kShortcutConfirmMaxAttempts);
            }
            return true;
        }

        if (rt.shortcutPhase == 5) {
            if (s.mapID == expectedMap && s.mapReady && !s.waitingChangeMap) {
                LogAccount(a, std::wstring(L"ĐƯỜNG TẮT CHUYỂN MAP PASS • check authoritative M") + std::to_wstring(expectedMap) +
                              L" → tiếp tục AutoPath đích M" + std::to_wstring(finalTarget.mapID));
                ResetShortcutRoute(rt);
                return false;
            }
            if (Elapsed(now, rt.shortcutTick, 15000)) {
                FailShortcutRoute(a, destinationChangesMapImmediately
                    ? L"đã callback Tinh Túc (không có bước xác nhận) nhưng sau 15s chưa check được MapID đích M" + std::to_wstring(expectedMap)
                    : L"đã callback xác nhận nhưng sau 15s chưa check được MapID đích M" + std::to_wstring(expectedMap));
            } else {
                rt.status = destinationChangesMapImmediately
                    ? L"ĐƯỜNG TẮT HỎA • chờ check map M" + std::to_wstring(expectedMap) + L" sau callback Tinh Túc • không xác nhận"
                    : L"ĐƯỜNG TẮT • chờ check map M" + std::to_wstring(expectedMap) + L" sau xác nhận";
            }
            return true;
        }
        return true;
    }

    bool HandleShortcutInterserverGate(Account& a, DWORD now, const TargetProfile& finalTarget,
                                       const TargetProfile& gate) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        if (rt.shortcutPhase == 99) return true;
        if (!gate.valid) {
            FailShortcutRoute(a, L"chưa gán tọa cổng liên-server • bấm LẤY TỌA ở M10000");
            return true;
        }

        if (rt.shortcutPhase <= 1) {
            if (!a.snapshotValid || !s.mapReady || s.waitingChangeMap ||
                (s.validMask & (ValidMap | ValidPosition | ValidAutoPath | ValidRiding)) !=
                    (ValidMap | ValidPosition | ValidAutoPath | ValidRiding)) {
                rt.status = L"LIÊN-SERVER • chờ state M/X/Y/AutoPath/IsRiding ổn định";
                return true;
            }
            if (s.mapID != gate.mapID) {
                FailShortcutRoute(a, L"PORTAL SPECIAL chỉ được chạy waypoint cổng khi đang đúng M10000");
                return true;
            }

            const long long dx = static_cast<long long>(s.x) - gate.x;
            const long long dy = static_cast<long long>(s.y) - gate.y;
            const long long d2 = dx * dx + dy * dy;
            const bool preciseAtGate = d2 <= static_cast<long long>(kPreciseWorldTolerance) * kPreciseWorldTolerance;
            const bool movementObservedAfterDispatch = rt.shortcutAttempts > 0 && rt.shortcutTick != 0 &&
                rt.lastMovementTick != 0 && static_cast<LONG>(rt.lastMovementTick - rt.shortcutTick) > 0;
            const bool stalledThreeSeconds = rt.shortcutAttempts > 0 &&
                (s.autoPathing || movementObservedAfterDispatch) &&
                rt.lastMovementTick != 0 && Elapsed(now, rt.lastMovementTick, kLauLanGateStallMs);

            if (preciseAtGate || stalledThreeSeconds) {
                rt.shortcutPhase = 2;
                rt.shortcutTick = now;
                rt.shortcutAttempts = 0;
                rt.status = preciseAtGate
                    ? L"LIÊN-SERVER • PORTAL SPECIAL tới sát tọa cổng • KHÔNG StopPath theo radius 120 • chờ popup"
                    : L"LIÊN-SERVER • PORTAL SPECIAL đã có movement proof rồi đứng ~3s • chờ popup";
                LogAccount(a, preciseAtGate
                    ? L"PORTAL SPECIAL: vị trí đã vào tolerance 20; giữ nguyên path, không StopPath sớm, chuyển sang confirm semantic v0.1.8."
                    : L"PORTAL SPECIAL: đã chứng minh route thực sự di chuyển rồi stall ~3s; chuyển sang confirm semantic dù IsAutoPathing có thể đã tắt." );
                return true;
            }

            if (s.autoPathing || movementObservedAfterDispatch) {
                rt.status = L"LIÊN-SERVER • PORTAL SPECIAL đang chạy thật tới cổng " +
                            std::to_wstring(gate.x) + L"," + std::to_wstring(gate.y);
                return true;
            }

            if (rt.shortcutAttempts >= kShortcutPathMaxDispatch && rt.shortcutTick != 0 &&
                Elapsed(now, rt.shortcutTick, kShortcutPathAcceptMs)) {
                FailShortcutRoute(a, L"PORTAL SPECIAL: đã lên ngựa và gửi StartPath 5 lần nhưng không thấy AutoPath hoặc movement proof");
                return true;
            }
            if (rt.shortcutTick != 0 && !Elapsed(now, rt.shortcutTick, kShortcutPathAcceptMs)) {
                rt.status = L"LIÊN-SERVER • STARTPATH PASS • chờ tối đa 5s để thấy AutoPath/movement proof";
                return true;
            }

            // Reuse the normal robust FSM so M10000 follows the same hard contract:
            // AutoFight OFF -> IsRiding=1 -> StartPath. There is no foot fallback.
            const DWORD startPathPassBefore = rt.lastStartPathPassTick;
            bool arrived = false;
            (void)HandleRobustTravelDirect(a, now, gate, L"cổng liên-server M10000", arrived,
                                           kPreciseWorldTolerance);
            if (rt.lastStartPathPassTick != 0 && rt.lastStartPathPassTick != startPathPassBefore) {
                ++rt.shortcutAttempts;
                rt.shortcutTick = rt.lastStartPathPassTick;
                rt.lastObservedX = s.x;
                rt.lastObservedY = s.y;
                rt.lastMovementTick = rt.shortcutTick;
                rt.status = L"LIÊN-SERVER • PORTAL SPECIAL MOUNTED STARTPATH PASS • lần " +
                            std::to_wstring(rt.shortcutAttempts) + L"/" + std::to_wstring(kShortcutPathMaxDispatch) +
                            L" • chờ AutoPath/movement proof";
            }
            return true;
        }

        if (rt.shortcutPhase == 2) {
            // Portal confirmation uses the same UI-frame pacing as the NPC
            // destination flow. It is intentionally not a fast one-shot click.
            const DWORD confirmWait = rt.shortcutAttempts == 0
                ? kShortcutConfirmUiReadyMs : kShortcutConfirmRetryMs;
            if (!Elapsed(now, rt.shortcutTick, confirmWait)) return true;
            if (ShortcutBridgeCall(a, Command::ConfirmTravelSemantic, 0,
                                   L"Xác nhận popup cổng liên-server theo v0.1.8", now, 5000)) {
                rt.shortcutPhase = 3;
                rt.shortcutTick = now;
                rt.shortcutAttempts = 0;
                rt.shortcutExpectedMap = finalTarget.mapID;
                return true;
            }
            ++rt.shortcutAttempts;
            rt.shortcutTick = now;
            if (rt.shortcutAttempts >= kShortcutConfirmMaxAttempts) {
                FailShortcutRoute(a, L"PORTAL SPECIAL đã tới/stall nhưng " +
                                  std::to_wstring(kShortcutConfirmMaxAttempts) +
                                  L"s vẫn không thấy đúng popup Xác nhận");
            } else {
                rt.status = L"LIÊN-SERVER • PORTAL SPECIAL đang chờ popup Xác nhận " +
                            std::to_wstring(rt.shortcutAttempts) + L"/" +
                            std::to_wstring(kShortcutConfirmMaxAttempts);
            }
            return true;
        }

        if (rt.shortcutPhase == 3) {
            if (s.mapID == rt.shortcutExpectedMap && s.mapReady && !s.waitingChangeMap) {
                LogAccount(a, L"LIÊN-SERVER PASS • M10000 → M" + std::to_wstring(rt.shortcutExpectedMap) +
                              L" • MapReady authoritative • tiếp tục AutoPath tới tọa train.");
                ResetShortcutRoute(rt);
                return false;
            }
            if (Elapsed(now, rt.shortcutTick, 15000)) {
                FailShortcutRoute(a, L"popup cổng đã xác nhận nhưng chưa check được MapID+MapReady đích sau 15s");
            } else {
                rt.status = L"LIÊN-SERVER • chờ MapID+MapReady M" + std::to_wstring(rt.shortcutExpectedMap);
            }
            return true;
        }
        return true;
    }

    bool HandleThdcRoute(Account& a, DWORD now, const TargetProfile& finalTarget) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        if (rt.shortcutPhase == 99) return true;

        auto continueAfterTransition = [&](bool entryNeedsConfirm) {
            ResetRobustTravel(rt);
            ResetTravelFightGuard(rt);
            rt.shortcutAttempts = 0;
            rt.shortcutTick = now;
            if (entryNeedsConfirm) {
                rt.shortcutPhase = 3;
                rt.status = L"THĐC • đã vào M10014 • bây giờ mới chờ popup Xác nhận";
                LogAccount(a, L"THĐC ENTRY MAP PASS • M10000 → M10014 • không xác nhận ở M10000; bắt đầu chờ popup trong tầng 1.");
                return true;
            }
            LogAccount(a, L"THĐC GATE PASS • M" + std::to_wstring(rt.shortcutSourceMap) + L" → M" +
                          std::to_wstring(rt.shortcutExpectedMap) + L" • đã check MapID tầng kế tiếp.");
            if (rt.shortcutExpectedMap == finalTarget.mapID) {
                ResetShortcutRoute(rt);
                return false;
            }
            rt.shortcutPhase = 1;
            rt.shortcutSourceMap = 0;
            rt.shortcutExpectedMap = 0;
            return true;
        };

        if (rt.shortcutPhase <= 1) {
            if (!a.snapshotValid || (s.validMask & ValidMap) != ValidMap || !s.mapReady || s.waitingChangeMap) {
                rt.status = L"THĐC • chờ MapID/MapReady ổn định trước khi chọn cổng tầng kế";
                return true;
            }
            if (s.mapID == finalTarget.mapID) {
                LogAccount(a, L"THĐC ĐÃ ĐÚNG TẦNG M" + std::to_wstring(finalTarget.mapID) + L" • trả về AutoPath tọa đích.");
                ResetShortcutRoute(rt);
                return false;
            }
            const thdc_route_logic::GatePlan plan = thdc_route_logic::NextGate(s.mapID, finalTarget.mapID);
            if (!plan.valid) {
                FailShortcutRoute(a, L"không có cổng tuần tự hợp lệ từ M" + std::to_wstring(s.mapID) +
                                     L" tới tầng đích M" + std::to_wstring(finalTarget.mapID));
                return true;
            }
            rt.shortcutSourceMap = plan.sourceMap;
            rt.shortcutExpectedMap = plan.expectedMap;
            rt.shortcutPhase = 2;
            rt.shortcutTick = 0;
            rt.shortcutAttempts = 0;
            ResetRobustTravel(rt);
            ResetTravelFightGuard(rt);
            LogAccount(a, L"THĐC LEG ARM • cổng nằm trên M" + std::to_wstring(plan.sourceMap) +
                          L" • chỉ chấp nhận tầng kế M" + std::to_wstring(plan.expectedMap) +
                          L" • index tọa=" + std::to_wstring(plan.coordinateIndex));
        }

        if (rt.shortcutPhase == 2) {
            const thdc_route_logic::GatePlan plan =
                thdc_route_logic::NextGate(rt.shortcutSourceMap, finalTarget.mapID);
            if (!plan.valid || plan.expectedMap != rt.shortcutExpectedMap) {
                FailShortcutRoute(a, L"state cổng THĐC không còn khớp map nguồn/tầng kế");
                return true;
            }
            if (s.mapID == rt.shortcutExpectedMap) {
                if (!s.mapReady || s.waitingChangeMap) {
                    rt.status = L"THĐC • đã thấy M" + std::to_wstring(rt.shortcutExpectedMap) + L" • chờ MapReady";
                    return true;
                }
                return continueAfterTransition(plan.confirmAfterTransition);
            }
            if (s.mapID != rt.shortcutSourceMap) {
                FailShortcutRoute(a, L"đi cổng từ M" + std::to_wstring(rt.shortcutSourceMap) +
                                     L" nhưng sang M" + std::to_wstring(s.mapID) +
                                     L", khác tầng kế bắt buộc M" + std::to_wstring(rt.shortcutExpectedMap));
                return true;
            }
            if (!a.snapshotValid || !s.mapReady || s.waitingChangeMap ||
                (s.validMask & (ValidMap | ValidPosition | ValidAutoPath | ValidRiding)) !=
                    (ValidMap | ValidPosition | ValidAutoPath | ValidRiding)) {
                rt.status = L"THĐC • chờ state M/X/Y/AutoPath/IsRiding ổn định";
                return true;
            }

            const auto coordinates = ThdcCoordinatePairs(shortcutSettings_);
            const auto gateCoordinate = coordinates[static_cast<std::size_t>(plan.coordinateIndex)];
            const TargetProfile gate = ShortcutWorldTarget(
                L"cổng THĐC", plan.sourceMap, gateCoordinate.first, gateCoordinate.second);
            if (!gate.valid) {
                FailShortcutRoute(a, L"tọa cổng THĐC index " + std::to_wstring(plan.coordinateIndex) +
                                     L" trên M" + std::to_wstring(plan.sourceMap) + L" bị thiếu/0");
                return true;
            }

            const long long dx = static_cast<long long>(s.x) - gate.x;
            const long long dy = static_cast<long long>(s.y) - gate.y;
            const long long d2 = dx * dx + dy * dy;
            const bool preciseAtGate = d2 <= static_cast<long long>(kPreciseWorldTolerance) * kPreciseWorldTolerance;
            const bool movementObservedAfterDispatch = rt.shortcutAttempts > 0 && rt.shortcutTick != 0 &&
                rt.lastMovementTick != 0 && static_cast<LONG>(rt.lastMovementTick - rt.shortcutTick) > 0;
            const bool stalledThreeSeconds = rt.shortcutAttempts > 0 &&
                (s.autoPathing || movementObservedAfterDispatch) && rt.lastMovementTick != 0 &&
                Elapsed(now, rt.lastMovementTick, kLauLanGateStallMs);
            if (preciseAtGate || stalledThreeSeconds) {
                rt.shortcutPhase = 4;
                rt.shortcutTick = now;
                rt.shortcutAttempts = 0;
                rt.status = L"THĐC • đã vào cổng trên M" + std::to_wstring(plan.sourceMap) +
                            L" • không StopPath sớm • chờ đúng M" + std::to_wstring(plan.expectedMap);
                return true;
            }
            if (s.autoPathing || movementObservedAfterDispatch) {
                rt.status = L"THĐC • đang chạy tới cổng trên M" + std::to_wstring(plan.sourceMap) +
                            L" tại " + std::to_wstring(gate.x) + L"," + std::to_wstring(gate.y);
                return true;
            }
            if (rt.shortcutAttempts >= kShortcutPathMaxDispatch && rt.shortcutTick != 0 &&
                Elapsed(now, rt.shortcutTick, kShortcutPathAcceptMs)) {
                FailShortcutRoute(a, L"đã gửi StartPath cổng THĐC 5 lần nhưng không thấy AutoPath/movement proof");
                return true;
            }
            if (rt.shortcutTick != 0 && !Elapsed(now, rt.shortcutTick, kShortcutPathAcceptMs)) {
                rt.status = L"THĐC • STARTPATH PASS • chờ tối đa 5s thấy AutoPath/movement proof";
                return true;
            }

            const DWORD startPathPassBefore = rt.lastStartPathPassTick;
            bool arrived = false;
            (void)HandleRobustTravelDirect(a, now, gate, L"cổng THĐC trên đúng map nguồn", arrived,
                                           kPreciseWorldTolerance);
            if (rt.lastStartPathPassTick != 0 && rt.lastStartPathPassTick != startPathPassBefore) {
                ++rt.shortcutAttempts;
                rt.shortcutTick = rt.lastStartPathPassTick;
                rt.lastObservedX = s.x;
                rt.lastObservedY = s.y;
                rt.lastMovementTick = rt.shortcutTick;
            }
            return true;
        }

        if (rt.shortcutPhase == 4) {
            if (s.mapID == rt.shortcutExpectedMap) {
                if (!s.mapReady || s.waitingChangeMap) {
                    rt.status = L"THĐC • đã thấy tầng kế M" + std::to_wstring(rt.shortcutExpectedMap) + L" • chờ MapReady";
                    return true;
                }
                return continueAfterTransition(rt.shortcutSourceMap == 10000);
            }
            if (s.mapID != rt.shortcutSourceMap) {
                FailShortcutRoute(a, L"cổng THĐC chuyển sang map ngoài tầng kế bắt buộc: M" + std::to_wstring(s.mapID));
                return true;
            }
            if (Elapsed(now, rt.shortcutTick, 15000)) {
                FailShortcutRoute(a, L"đã tới cổng nhưng sau 15s MapID chưa đổi sang tầng kế M" +
                                     std::to_wstring(rt.shortcutExpectedMap));
            } else {
                rt.status = L"THĐC • chờ cổng tự dịch M" + std::to_wstring(rt.shortcutSourceMap) +
                            L" → M" + std::to_wstring(rt.shortcutExpectedMap);
            }
            return true;
        }

        if (rt.shortcutPhase == 3) {
            if (s.mapID != 10014 || !s.mapReady || s.waitingChangeMap) {
                rt.status = L"THĐC • chỉ xác nhận sau khi M10014 đã ổn định";
                return true;
            }
            const DWORD confirmWait = rt.shortcutAttempts == 0
                ? kShortcutConfirmUiReadyMs : kShortcutConfirmRetryMs;
            if (!Elapsed(now, rt.shortcutTick, confirmWait)) return true;
            if (ShortcutBridgeCall(a, Command::ConfirmTravelSemantic, 0,
                                   L"THĐC M10014 • callback Xác nhận popup Chú ý", now, 5000)) {
                LogAccount(a, L"THĐC ENTRY CONFIRM PASS • Xác nhận đúng một lần sau khi đã vào M10014.");
                if (finalTarget.mapID == 10014) {
                    ResetShortcutRoute(rt);
                    return false;
                }
                rt.shortcutPhase = 1;
                rt.shortcutSourceMap = 0;
                rt.shortcutExpectedMap = 0;
                rt.shortcutAttempts = 0;
                rt.shortcutTick = now;
                ResetRobustTravel(rt);
                ResetTravelFightGuard(rt);
                return true;
            }
            ++rt.shortcutAttempts;
            rt.shortcutTick = now;
            if (rt.shortcutAttempts >= kShortcutConfirmMaxAttempts) {
                FailShortcutRoute(a, L"đã vào M10014 nhưng không tìm được popup Xác nhận THĐC sau số lần retry giới hạn");
            } else {
                rt.status = L"THĐC • M10014 chờ popup Xác nhận " + std::to_wstring(rt.shortcutAttempts) +
                            L"/" + std::to_wstring(kShortcutConfirmMaxAttempts);
            }
            return true;
        }
        return true;
    }

    bool HandleShortcutTravel(Account& a, DWORD now, const TargetProfile& finalTarget) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        const bool interserverTarget = finalTarget.mapID == 10005 || finalTarget.mapID == 10004 ||
                                       finalTarget.mapID == 10007;
        const bool thdcTarget = thdc_route_logic::IsThdcFloor(finalTarget.mapID);
        const bool mandatoryInterserver = (s.mapID == 10000 && interserverTarget) ||
                                          rt.shortcutKind == ShortcutKind::InterserverGate;
        const bool mandatoryThdc = (thdcTarget && (s.mapID == 10000 ||
                                    (thdc_route_logic::IsThdcFloor(s.mapID) && s.mapID != finalTarget.mapID))) ||
                                   rt.shortcutKind == ShortcutKind::ThdcRoute;
        if (!shortcutSettings_.enabled && !mandatoryInterserver && !mandatoryThdc) {
            if (rt.shortcutKind != ShortcutKind::None) ResetShortcutRoute(rt);
            return false;
        }
        if (rt.shortcutKind != ShortcutKind::None && rt.shortcutFinalMap != finalTarget.mapID) {
            LogAccount(a, L"ĐƯỜNG TẮT: đích đổi giữa chừng → reset waypoint cũ, tính lại theo MapID mới.");
            ResetShortcutRoute(rt);
        }

        if (rt.shortcutKind == ShortcutKind::None) {
            const bool currentKunlun = s.mapID == 75 || s.mapID == 76;
            const bool finalKunlun = finalTarget.mapID == 75 || finalTarget.mapID == 76;
            const bool currentFire = s.mapID == 55 || s.mapID == 70;
            // M10000/THĐC portals are mandatory world routing, not optional shortcuts.
            // They must work even when the ĐƯỜNG TẮT checkbox is off.
            if (thdcTarget && s.mapID != finalTarget.mapID &&
                (s.mapID == 10000 || thdc_route_logic::IsThdcFloor(s.mapID))) {
                rt.shortcutKind = ShortcutKind::ThdcRoute;
            } else if (s.mapID == 10000 && interserverTarget) {
                rt.shortcutKind = ShortcutKind::InterserverGate;
            } else if (!shortcutSettings_.enabled) {
                return false;
            } else if (currentKunlun && !finalKunlun) {
                rt.shortcutKind = ShortcutKind::KunLunExit;
            } else if (!currentKunlun && finalKunlun) {
                rt.shortcutKind = ShortcutKind::KunLunEnter;
            } else if (s.mapID == 5 && (finalTarget.mapID == 55 || finalTarget.mapID == 70)) {
                rt.shortcutKind = ShortcutKind::FireEnter;
            } else if (currentFire && finalTarget.mapID != 55 && finalTarget.mapID != 70) {
                rt.shortcutKind = ShortcutKind::FireExit;
            } else if (travel_network_logic::SelectNpcTeleport(s.mapID, finalTarget.mapID).valid) {
                rt.shortcutKind = ShortcutKind::TravelNetwork;
                rt.shortcutSourceMap = s.mapID;
            } else {
                return false;
            }
            rt.shortcutFinalMap = finalTarget.mapID;
            rt.shortcutPhase = 1; rt.shortcutTick = now; rt.shortcutAttempts = 0;
            ResetRobustTravel(rt); ResetTravelFightGuard(rt);
            LogAccount(a, L"ĐƯỜNG TẮT ARM • current M" + std::to_wstring(s.mapID) + L" → final M" + std::to_wstring(finalTarget.mapID) +
                          L" • kind=" + std::to_wstring(static_cast<int>(rt.shortcutKind)));
        }

        switch (rt.shortcutKind) {
            case ShortcutKind::KunLunExit: {
                const TargetProfile npc = ShortcutWorldTarget(L"NPC RỜI Côn Lôn Sơn", 75, shortcutSettings_.kunlunNpcX, shortcutSettings_.kunlunNpcY);
                return HandleKunLunExitTryClickRoute(a, now, finalTarget, npc);
            }
            case ShortcutKind::KunLunEnter: {
                const SellNpcPosition* xaPos = nullptr;
                for (std::size_t i = 0; i < kSellNpcs.size(); ++i) {
                    if (kSellNpcs[i].npcID == kXaTruyenBinhNpcId) {
                        xaPos = &sellNpcPositions_[i];
                        break;
                    }
                }
                if (!xaPos || !xaPos->valid) {
                    FailShortcutRoute(a, L"Xa Truyền Bình ID 387 chưa có tọa ngoài màn hình chính • chọn NPC bán Xa Truyền Bình rồi LẤY VỊ TRÍ");
                    return true;
                }
                const TargetProfile npc = ShortcutWorldTarget(L"Xa Truyền Bình", 5, xaPos->x, xaPos->y);
                return HandleShortcutNpcRoute(a, now, finalTarget, npc, kXaTruyenBinhNpcId, TravelSemantic::KunLunSon, 75, L"Xa Truyền Bình");
            }
            case ShortcutKind::FireEnter: {
                const TargetProfile npc = ShortcutWorldTarget(L"Ngải Ni Ngoã Nhĩ", 5, shortcutSettings_.ngaiX, shortcutSettings_.ngaiY);
                return HandleShortcutNpcRoute(a, now, finalTarget, npc, kNgaiNiNgoaNhiNpcId,
                                              TravelSemantic::TinhTucHai, 12, L"Ngải Ni Ngoã Nhĩ",
                                              TravelSemantic::DenCacMonPhai, true);
            }
            case ShortcutKind::FireExit: {
                const TargetProfile tt = ShortcutWorldTarget(L"Tinh Túc Hải điểm ra", 12, shortcutSettings_.tinhTucX, shortcutSettings_.tinhTucY);
                bool reached = false;
                (void)ShortcutTravelLeg(a, now, tt, L"Tinh Túc Hải điểm ra", reached);
                if (!reached) return true;
                LogAccount(a, L"ĐƯỜNG TẮT HỎA PASS • đã tới điểm ra M12 do người dùng gán → tiếp tục AutoPath đích bình thường.");
                ResetShortcutRoute(rt); return false;
            }
            case ShortcutKind::InterserverGate: {
                int x = 0, y = 0;
                if (finalTarget.mapID == 10005) { x = shortcutSettings_.thanhLienGateX; y = shortcutSettings_.thanhLienGateY; }
                else if (finalTarget.mapID == 10004) { x = shortcutSettings_.phamLienGateX; y = shortcutSettings_.phamLienGateY; }
                else { x = shortcutSettings_.khoVinhGateX; y = shortcutSettings_.khoVinhGateY; }
                const TargetProfile gate = ShortcutWorldTarget(L"cổng liên-server", 10000, x, y);
                return HandleShortcutInterserverGate(a, now, finalTarget, gate);
            }
            case ShortcutKind::ThdcRoute:
                return HandleThdcRoute(a, now, finalTarget);
            case ShortcutKind::TravelNetwork: {
                const auto plan = travel_network_logic::SelectNpcTeleport(rt.shortcutSourceMap, rt.shortcutFinalMap);
                if (!plan.valid) {
                    FailShortcutRoute(a, L"travel network mất descriptor hợp lệ cho source/destination đã arm");
                    return true;
                }

                TargetProfile npc{};
                if (plan.useSharedXaTruyenBinhPosition) {
                    const SellNpcPosition* xaPos = nullptr;
                    for (std::size_t i = 0; i < kSellNpcs.size(); ++i) {
                        if (kSellNpcs[i].npcID == travel_network_logic::kXaTruyenBinhNpcId) {
                            xaPos = &sellNpcPositions_[i];
                            break;
                        }
                    }
                    if (!xaPos || !xaPos->valid) {
                        FailShortcutRoute(a, L"Xa Truyền Bình ID 387 chưa có tọa dùng chung trong sellNpcPositions_ • không tạo tọa NPC thứ hai");
                        return true;
                    }
                    npc = ShortcutWorldTarget(L"Xa Truyền Bình", plan.fromMap, xaPos->x, xaPos->y);
                } else {
                    npc = ShortcutWorldTarget(plan.label, plan.fromMap, plan.npcX, plan.npcY);
                }

                const TravelSemantic semantic = ToProtocolTravelSemantic(plan.semantic);
                if (semantic == TravelSemantic::None) {
                    FailShortcutRoute(a, L"travel network semantic chưa được ánh xạ • fail-closed");
                    return true;
                }
                return HandleShortcutNpcRoute(a, now, finalTarget, npc, plan.npcID, semantic,
                                              plan.expectedMap, plan.label, TravelSemantic::None,
                                              !plan.needConfirm);
            }
            case ShortcutKind::None: return false;
        }
        return false;
    }

    bool HandleRobustTravel(Account& a, DWORD now, const TargetProfile& targetProfile,
                            const wchar_t* context, bool& arrived, int toleranceOverride = 0) {
        arrived = false;
        if (HandleShortcutTravel(a, now, targetProfile)) return true;
        return HandleRobustTravelDirect(a, now, targetProfile, context, arrived, toleranceOverride);
    }

    void BeginTrainRecovery(Account& a, DWORD now) {
        RuntimeState& rt = a.runtime;
        rt.trainPositionMonitorArmed = false;
        rt.lastTrainPositionCheckTick = 0;
        rt.trainRecoveryPhase = 4;
        rt.fightPhase = 0;
        rt.fightAttempts = 0;
        ResetRobustTravel(rt);
        ResetTravelFightGuard(rt);
        LogAccount(a, L"CHECK 1 PHÚT: lệch bãi → v0.3 Travel Guard bắt buộc AutoFight OFF trước mọi StartPath → quay lại tọa train.");
    }

    bool HandleTrainRecovery(Account& a, DWORD now) {
        RuntimeState& rt = a.runtime;
        if (rt.trainRecoveryPhase == 0) return false;

        bool arrived = false;
        (void)HandleRobustTravel(a, now, a.profile.target, L"bãi train", arrived);
        if (arrived) {
            rt.trainRecoveryPhase = 0;
            rt.wasAtTarget = false;
            rt.fightPhase = 0;
            rt.fightAttempts = 0;
            rt.status = L"Đã về bãi • chuẩn bị bật lại Đánh quái";
            LogAccount(a, L"Đã quay lại bãi sau check lệch • Travel Guard đã bảo đảm path không chạy cùng AutoFight • chuẩn bị P3 AUTO→Đánh quái.");
        }
        return true;
    }

    TargetProfile SellNpcTarget(const Account& a) const {
        const int presetIndex = (a.profile.sellNpcPreset >= 0 && a.profile.sellNpcPreset < static_cast<int>(kSellNpcs.size()))
            ? a.profile.sellNpcPreset : 0;
        const SellNpcPreset& npc = kSellNpcs[static_cast<std::size_t>(presetIndex)];
        TargetProfile t{};
        t.name = npc.name;
        t.mapID = npc.mapID;
        const SellNpcPosition& pos = sellNpcPositions_[static_cast<std::size_t>(presetIndex)];
        t.x = pos.x;
        t.y = pos.y;
        t.valid = pos.valid;
        return t;
    }

    bool SellMacroConfigured(const Account& a, std::wstring& reason) const {
        const int row = fixed_slot_sell_logic::ConfigRowIndex(a.profile.sellMacro.size());
        if (row < 0) {
            reason = L"chưa có dòng tọa độ ô trang bị thứ 2";
            return false;
        }
        const SellMacroStep& step = a.profile.sellMacro[static_cast<std::size_t>(row)];
        if (!step.point.valid) {
            reason = L"dòng item " + std::to_wstring(row + 1) + L" chưa lấy tọa độ (F8)";
            return false;
        }
        reason.clear();
        return true;
    }

    void BeginAutoSell(Account& a, DWORD now) {
        RuntimeState& rt = a.runtime;
        rt.sellPhase = 4;
        rt.sellPhaseTick = now;
        rt.sellOpenAttempts = 0;
        rt.sellMacroIndex = 0;
        rt.sellMacroRepeatDone = 0;
        rt.sellMacroNextTick = 0;
        rt.sellMacroCompletionDueTick = 0;
        rt.sellMacroPass = 0;
        rt.sellLastFreeBag = a.snapshot.freeBagSpace;
        rt.sellBagStableSince = 0;
        rt.trainPositionMonitorArmed = false;
        rt.lastTrainPositionCheckTick = 0;
        rt.trainRecoveryPhase = 0;
        rt.fightPhase = 0;
        rt.fightAttempts = 0;
        rt.wasAtTarget = false;
        rt.crossMapSeenAutoPath = false;
        rt.stallSinceTick = 0;
        rt.confirmAttempts = 0;
        rt.crossMapRouteArmed = false;
        rt.crossMapRouteMoved = false;
        ResetRobustTravel(rt);
        if (a.bridge.Attached()) {
            Response r{}; std::wstring error;
            if (!a.bridge.Call(Command::StopPath, 0, 0, 0, r, error, 700) && BridgeLooksUnresponsive(error)) {
                EnterClientFreeze(a, L"Bridge timeout lúc bắt đầu Auto Sell", now);
            }
        }
        const SellNpcPreset& npc = kSellNpcs[static_cast<std::size_t>(a.profile.sellNpcPreset)];
        LogAccount(a, L"TÚI CHẠM NGƯỠNG → bắt đầu bán • " + std::wstring(npc.name));
    }

    bool RunSellMacroClick(Account& a, DWORD now) {
        RuntimeState& rt = a.runtime;
        const int fixedRow = fixed_slot_sell_logic::ConfigRowIndex(a.profile.sellMacro.size());
        const SellMacroStep* fixedStep = fixedRow >= 0
            ? &a.profile.sellMacro[static_cast<std::size_t>(fixedRow)] : nullptr;
        const DWORD fixedDelay = fixedStep
            ? static_cast<DWORD>(std::clamp(fixedStep->delayMs, 50, 60000)) : 260u;
        const DWORD delay = rt.sellMacroIndex < 4 ? 350u : (rt.sellMacroIndex == 4 ? fixedDelay : 180u);
        if (rt.sellMacroNextTick != 0 && !Elapsed(now, rt.sellMacroNextTick, delay)) return true;

        Response response{};
        std::wstring error;
        if (rt.sellMacroIndex < 4) {
            if (!a.bridge.Call(Command::AdvanceBackgroundSell, 0, 0, 0, response, error, 2400)) {
                if (BridgeLooksUnresponsive(error)) EnterClientFreeze(a, L"Bridge timeout lúc mở UI bán nền", now);
                ++rt.sellOpenAttempts;
                rt.sellMacroNextTick = now;
                rt.status = L"BÁN NỀN • chờ đúng control UI • thử " +
                            std::to_wstring(rt.sellOpenAttempts) + L"/12";
                if (rt.sellOpenAttempts >= 12) {
                    rt.sellPhase = 10;
                    LogAccount(a, L"BÁN NỀN FAIL khi mở chuỗi shop: " + error + L" • dừng fail-closed");
                }
                return true;
            }
            rt.sellOpenAttempts = 0;
            rt.sellMacroIndex = std::clamp(response.value0, 0, 4);
            rt.sellMacroNextTick = now;
            rt.status = L"BÁN NỀN • UI semantic stage " + std::to_wstring(rt.sellMacroIndex) +
                        L"/4 • " + std::wstring(response.detail);
            return true;
        }

        if (rt.sellMacroIndex == 4) {
            if (SemanticSellRulesActive()) {
                std::vector<InventoryBagRow> rows;
                int freshFree = -1;
                if (!ScanBagSemantic(a, rows, error, &freshFree)) {
                    if (BridgeLooksUnresponsive(error)) EnterClientFreeze(a, L"Bridge timeout lúc scan item BÁN semantic", now);
                    ++rt.sellOpenAttempts;
                    rt.sellMacroNextTick = now;
                    rt.status = L"BÁN SEMANTIC • scan lỗi " + std::to_wstring(rt.sellOpenAttempts) + L"/6";
                    if (rt.sellOpenAttempts >= 6) {
                        rt.sellPhase = 10;
                        LogAccount(a, L"BÁN SEMANTIC FAIL scan tay nải: " + error + L" • fail-closed");
                    }
                    return true;
                }
                rt.sellOpenAttempts = 0;
                std::vector<inventory_filter_logic::ItemView> items;
                items.reserve(rows.size());
                for (const auto& row : rows) items.push_back(row.item);

                if (rt.bagFilterPendingInstance != 0) {
                    const bool stillThere = std::any_of(items.begin(), items.end(), [&](const auto& item){
                        return item.instanceID == rt.bagFilterPendingInstance;
                    });
                    if (stillThere && !Elapsed(now, rt.bagFilterPendingTick, 2500)) {
                        rt.status = L"BÁN SEMANTIC • chờ server xác nhận item trước";
                        rt.sellMacroNextTick = now;
                        return true;
                    }
                    rt.bagFilterPendingInstance = 0;
                    rt.bagFilterPendingTick = 0;
                }

                const int candidate = inventory_filter_logic::FindCandidate(inventoryFilter_, items, RuleAction::Sell);
                if (candidate < 0) {
                    rt.sellMacroIndex = 5;
                    rt.sellMacroRepeatDone = 0;
                    rt.sellMacroCompletionDueTick = now + 180;
                    rt.status = L"BÁN SEMANTIC • hết item khớp rule → đóng shop";
                    return true;
                }

                const auto& item = items[static_cast<std::size_t>(candidate)];
                std::int32_t low = 0, high = 0; SplitInstanceID(item.instanceID, low, high);
                if (!a.bridge.Call(Command::SellBagItem, low, high, item.itemID, response, error, 2600)) {
                    if (BridgeLooksUnresponsive(error)) EnterClientFreeze(a, L"Bridge timeout lúc bán item semantic", now);
                    ++rt.sellOpenAttempts;
                    rt.sellMacroNextTick = now;
                    rt.status = L"BÁN SEMANTIC • request lỗi " + std::to_wstring(rt.sellOpenAttempts) + L"/6";
                    if (rt.sellOpenAttempts >= 6) {
                        rt.sellPhase = 10;
                        LogAccount(a, L"BÁN SEMANTIC FAIL request item: " + error + L" • fail-closed");
                    }
                    return true;
                }
                rt.sellOpenAttempts = 0;
                rt.bagFilterPendingInstance = item.instanceID;
                rt.bagFilterPendingTick = now;
                rt.sellMacroNextTick = now;
                std::wstring name = L"ItemID " + std::to_wstring(item.itemID);
                for (const auto& row : rows) if (row.item.instanceID == item.instanceID && !row.name.empty()) { name = row.name; break; }
                rt.status = L"BÁN SEMANTIC • " + name + L" • chờ re-scan";
                LogAccount(a, L"BÁN SEMANTIC • " + name + L" • ItemID " + std::to_wstring(item.itemID) +
                              L" • instance " + std::to_wstring(item.instanceID));
                return true;
            }

            if (!fixedStep || !fixedStep->point.valid) {
                rt.sellPhase = 10;
                rt.status = L"BÁN NỀN FAIL • mất tọa độ ô trang bị cố định";
                LogAccount(a, L"BÁN NỀN FAIL: cấu hình tọa độ item không còn hợp lệ • dừng fail-closed");
                return true;
            }
            const int clickTarget = fixed_slot_sell_logic::EffectiveClickCount(a.sellStep5LearnedRepeat);
            int normalizedX = -1, normalizedY = -1;
            if (!NormalizeClickPointForBridge(a.game, fixedStep->point,
                                              normalizedX, normalizedY, error)) {
                rt.sellPhase = 10;
                rt.status = L"BÁN NỀN FAIL • tọa độ ô trang bị không hợp lệ";
                LogAccount(a, L"BÁN NỀN FAIL tọa độ item: " + error + L" • dừng fail-closed");
                return true;
            }
            if (!a.bridge.Call(Command::SellNextBagItem, normalizedX, normalizedY, 0,
                               response, error, 2600)) {
                if (BridgeLooksUnresponsive(error)) EnterClientFreeze(a, L"Bridge timeout lúc bán ô trang bị", now);
                ++rt.sellOpenAttempts;
                rt.sellMacroNextTick = now;
                rt.status = L"BÁN NỀN • callback ô lỗi " + std::to_wstring(rt.sellOpenAttempts) + L"/6";
                if (rt.sellOpenAttempts >= 6) {
                    rt.sellPhase = 10;
                    LogAccount(a, L"BÁN NỀN FAIL callback item: " + error + L" • dừng fail-closed");
                }
                return true;
            }
            rt.sellOpenAttempts = 0;
            rt.sellMacroNextTick = now;
            rt.sellLastFreeBag = response.value0;
            ++rt.sellMacroRepeatDone;
            if (rt.sellMacroRepeatDone >= clickTarget) {
                rt.sellMacroIndex = 5;
                rt.sellMacroRepeatDone = 0;
                rt.sellMacroCompletionDueTick = now + fixedDelay;
                rt.status = L"BÁN NỀN • đủ " + std::to_wstring(clickTarget) +
                            L" callback ô cố định • bắt đầu đóng shop/tay nải";
            } else {
                rt.status = L"BÁN NỀN • ô cố định " + std::to_wstring(rt.sellMacroRepeatDone) +
                            L"/" + std::to_wstring(clickTarget) +
                            L" • FreeBag=" + std::to_wstring(response.value0);
            }
            return true;
        }

        if (rt.sellMacroIndex == 5) {
            if (rt.sellMacroCompletionDueTick != 0 &&
                static_cast<LONG>(now - rt.sellMacroCompletionDueTick) < 0) {
                rt.status = L"BÁN NỀN • callback cuối xong • chờ hết delay dòng item";
                return true;
            }
            rt.sellMacroCompletionDueTick = 0;
            if (rt.sellMacroRepeatDone >= 4) {
                rt.sellPhase = 7;
                rt.sellPhaseTick = now;
                rt.sellBagStableSince = 0;
                rt.status = L"BÁN NỀN xong • chờ FreeBagSpace xác nhận";
                return true;
            }
            if (!a.bridge.Call(Command::CloseBackgroundSell, 0, 0, 0, response, error, 2200)) {
                if (BridgeLooksUnresponsive(error)) EnterClientFreeze(a, L"Bridge timeout lúc đóng UI bán", now);
                ++rt.sellOpenAttempts;
                rt.sellMacroNextTick = now;
                if (rt.sellOpenAttempts >= 4) {
                    rt.sellPhase = 7;
                    rt.sellPhaseTick = now;
                    rt.sellBagStableSince = 0;
                    LogAccount(a, L"BÁN NỀN: không đóng hết UI sau 4 lần • vẫn chuyển sang verify túi");
                }
                return true;
            }
            rt.sellOpenAttempts = 0;
            rt.sellMacroNextTick = now;
            if (response.resultCode == static_cast<std::int32_t>(ActionResult::NothingToClose)) {
                rt.sellPhase = 7;
                rt.sellPhaseTick = now;
                rt.sellBagStableSince = 0;
                rt.status = L"BÁN NỀN xong • UI đã đóng • verify túi";
            } else {
                ++rt.sellMacroRepeatDone;
                rt.status = L"BÁN NỀN • đã đóng " + std::to_wstring(rt.sellMacroRepeatDone) + L" lớp UI";
            }
        }
        return true;
    }

    bool HandleAutoSell(Account& a, DWORD now) {
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        if (rt.sellPhase == 0) return false;

        if (rt.sellPhase == 4) {
            const TargetProfile npcTarget = SellNpcTarget(a);
            if (!npcTarget.valid) {
                rt.status = L"NPC bán chưa có tọa độ • nhập X/Y hoặc LẤY VỊ TRÍ";
                return true;
            }

            bool arrived = false;
            (void)HandleRobustTravel(a, now, npcTarget, L"NPC bán", arrived, kPreciseWorldTolerance);
            if (arrived) {
                rt.lastAction = Action::Hold;
                rt.sellPhase = 5; rt.sellPhaseTick = now;
                rt.status = L"Đã tới NPC • chuẩn bị ClickNPC";
            }
            return true;
        }

        if (rt.sellPhase == 5) {
            if (!Elapsed(now, rt.sellPhaseTick, 500)) return true;
            const SellNpcPreset& npc = kSellNpcs[static_cast<std::size_t>(a.profile.sellNpcPreset)];
            Response r{}; std::wstring error;
            if (!a.bridge.Call(Command::BeginBackgroundSell, npc.npcID, 0, 0, r, error, 2200)) {
                if (BridgeLooksUnresponsive(error)) EnterClientFreeze(a, L"Bridge timeout/busy khi mở phiên bán nền", now);
                ++rt.sellOpenAttempts;
                LogAccount(a, L"BEGIN BÁN NỀN NPC " + std::to_wstring(npc.npcID) + L" FAIL: " + error);
                if (rt.sellOpenAttempts >= 2) { rt.sellPhase = 10; rt.status = L"Không mở được NPC bằng callback nội bộ • chờ thủ công"; }
                else rt.sellPhaseTick = now;
                return true;
            }
            ++rt.sellOpenAttempts;
            ++rt.sellMacroPass;
            rt.sellPhase = 6; rt.sellPhaseTick = now;
            rt.sellMacroIndex = 0; rt.sellMacroRepeatDone = 0; rt.sellMacroNextTick = 0; rt.sellMacroCompletionDueTick = 0;
            // SELL owns only this account's sellPhase/macro cursor. It never stalls
            // an unrelated trade workflow or another client's hidden action.
            rt.status = L"Đã ClickNPC nội bộ ID " + std::to_wstring(npc.npcID) +
                        L" • SELL riêng acc • cửa sổ khác tiếp tục độc lập";
            return true;
        }

        if (rt.sellPhase == 6) {
            if (!Elapsed(now, rt.sellPhaseTick, 1200)) return true;
            return RunSellMacroClick(a, now);
        }

        if (rt.sellPhase == 7) {
            if ((s.validMask & ValidBagSpace) == 0) {
                rt.status = L"Không đọc được FreeBagSpace • không tự kết luận bán xong";
                return true;
            }
            if (s.freeBagSpace > 0) {
                if (rt.sellLastFreeBag != s.freeBagSpace) {
                    rt.sellLastFreeBag = s.freeBagSpace;
                    rt.sellBagStableSince = now;
                } else if (rt.sellBagStableSince == 0) {
                    rt.sellBagStableSince = now;
                } else if (Elapsed(now, rt.sellBagStableSince, 1500)) {
                    a.sellStep5LearnedRepeat = s.freeBagSpace;
                    rt.sellPhase = 8; rt.sellPhaseTick = now;
                    rt.crossMapSeenAutoPath = false; rt.crossMapRouteArmed = false; rt.crossMapRouteMoved = false; rt.stallSinceTick = 0; rt.confirmAttempts = 0;
                    ResetRobustTravel(rt);
                    rt.status = L"Đã nhận diện bán xong • quay về bãi train";
                    LogAccount(a, L"BÁN NỀN XONG • FreeBagSpace=" + std::to_wstring(s.freeBagSpace) +
                                  L" ổn định 1.5s • lần bán tới callback ô cố định=" +
                                  std::to_wstring(fixed_slot_sell_logic::EffectiveClickCount(a.sellStep5LearnedRepeat)) +
                                  L" • quay bãi train");
                    TelegramRecordSellCompleted(a, s.freeBagSpace);
                }
                return true;
            }
            if (Elapsed(now, rt.sellPhaseTick, 3500)) {
                if (rt.sellMacroPass < 2) {
                    rt.sellPhase = 5; rt.sellPhaseTick = now; rt.sellOpenAttempts = 0;
                    rt.status = L"Túi vẫn full • mở NPC + chạy macro lại lần 2";
                } else {
                    rt.sellPhase = 10;
                    rt.status = L"Macro bán 2 lần nhưng túi vẫn full • chờ thủ công";
                }
            }
            return true;
        }

        if (rt.sellPhase == 8) {
            const TargetProfile& trainTarget = a.profile.target;
            bool arrived = false;
            (void)HandleRobustTravel(a, now, trainTarget, L"bãi train", arrived);
            if (arrived) {
                rt.sellPhase = 0;
                rt.fightPhase = 0; rt.fightAttempts = 0; rt.wasAtTarget = false;
                rt.trainPositionMonitorArmed = false; rt.lastTrainPositionCheckTick = 0;
                rt.lastAction = Action::Hold;
                rt.status = L"Đã về bãi • tiếp tục AUTO train";
                LogAccount(a, L"Đã về bãi train sau bán đồ • tiếp tục chu trình.");
                return false;
            }
            return true;
        }

        if (rt.sellPhase == 10) {
            if ((s.validMask & ValidBagSpace) && s.freeBagSpace > 0) {
                rt.sellPhase = 8; rt.sellPhaseTick = now; ResetRobustTravel(rt);
                rt.status = L"Túi đã có ô trống • quay về bãi train";
            }
            return true;
        }
        return true;
    }

    void TickAccount(Account& a) {
        if (!a.runtime.running) return;
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        const DWORD now = GetTickCount();

        if (!s.mapReady || s.waitingChangeMap) {
            rt.candidateCount = 0;
            rt.qualifiedMap = 0;
            rt.stallSinceTick = 0;
            rt.fightPhase = 0;
            rt.status = L"Đang chuyển map • chặn action/click";
            return;
        }
        const std::uint32_t need = ValidMap | ValidPosition | ValidRiding | ValidAutoPath;
        if ((s.validMask & need) != need) {
            rt.status = L"State chưa đủ";
            return;
        }

        // Movement observation is serviced globally before P1/P2/P3 so World Flow-held
        // accounts receive the exact same Lâu Lan stall detection as normal accounts.
        if (HandleDeath(a, now)) return;
        if (UpdateRotationEfficiency(a, now)) return;
        if (HandleRouteOwnershipReset(a, now)) return;
        if (HandleUnderworldAutoFightGuard(a, now)) return;
        // P1 XN is PER-ACCOUNT PRIORITY ONLY and now uses a per-client internal callback, so it
        // remains eligible during World Flow HOLD without touching the Windows cursor.

        if (rt.qualifiedMap != s.mapID) {
            if (rt.candidateMap == s.mapID) ++rt.candidateCount;
            else { rt.candidateMap = s.mapID; rt.candidateCount = 1; }
            if (rt.candidateCount < 2) {
                rt.status = L"Ổn định Map 1/2";
                return;
            }
            rt.qualifiedMap = s.mapID;
            rt.candidateCount = 0;
        }

        // v1.3 inventory filter owns FULL-bag discard first. It mutates one live instance,
        // waits for a fresh semantic bag state, and only when no DROP candidate remains
        // allows the existing Trade/Auto-Sell policy to take over.
        if (rt.sellPhase == 0 && inventoryFilter_.enabled && (s.validMask & ValidBagSpace) && s.freeBagSpace <= 0) {
            if (TryAutoInventoryDrop(a, now)) return;
        }

        if (rt.sellPhase != 0) {
            if (HandleAutoSell(a, now)) return;
        } else if ((s.validMask & ValidBagSpace) &&
                   ShouldAutoSell(tradeEnabled_, a.profile.tradeRole, a.profile.enableSell,
                                  s.freeBagSpace)) {
            const TargetProfile npcTarget = SellNpcTarget(a);
            if (!npcTarget.valid) {
                rt.status = L"TÚI CẦN BÁN nhưng NPC bán chưa có tọa độ • nhập X/Y hoặc LẤY VỊ TRÍ";
                return;
            }
            std::wstring sellReason;
            if (!SemanticSellRulesActive() && !SellMacroConfigured(a, sellReason)) {
                rt.status = L"TÚI CẦN BÁN nhưng " + sellReason;
                return;
            }
            BeginAutoSell(a, now);
            if (HandleAutoSell(a, now)) return;
        }

        if (rt.trainRecoveryPhase != 0) {
            if (HandleTrainRecovery(a, now)) return;
        }

        // Semantic MessageBox Confirm remains disabled. Lâu Lan P1 XN is scheduled per account
        // globally before P2/P3 and is not part of this train FSM.
        // Steady training mode: AutoFight is checked once per minute, but ONLY when
        // no death/sell/recovery/confirm/path action is active. A busy state does not
        // advance the timer; the check is deferred until the account becomes idle.
        if (rt.trainPositionMonitorArmed) {
            // The exclusion gate also applies while an AUTO→Đánh quái sequence is
            // already in progress. If another operation starts between the two clicks,
            // freeze the sequence and resume only after the account is idle again.
            if (AutoFightCheckBusy(a, now)) {
                rt.status = L"Train • đang có thao tác khác → hoãn check/bật AutoFight";
                return;
            }
            if (rt.fightPhase != 3) {
                if (HandleFightClicks(a, now)) return;
            }

            const bool autoCheckDue = rt.lastAutoFightCheckTick == 0 ||
                                      Elapsed(now, rt.lastAutoFightCheckTick, kAutoFightRecheckMs);
            if (autoCheckDue && !AutoFightCheckBusy(a, now)) {
                if ((s.validMask & ValidAutoFight) == 0) {
                    rt.status = L"CHECK AUTO 1 PHÚT: getter chưa sẵn sàng • không click";
                    return;
                }
                rt.lastAutoFightCheckTick = now;
                if (!s.autoFight) {
                    rt.fightPhase = 0;
                    rt.fightAttempts = 0;
                    rt.fightRetryWaitTick = 0;
                    LogAccount(a, L"CHECK AUTO 1 PHÚT: AutoFight OFF → chạy AUTO→Đánh quái.");
                    if (HandleFightClicks(a, now)) return;
                } else {
                    LogAccount(a, L"CHECK AUTO 1 PHÚT: AutoFight vẫn ON • không click.");
                }
            }

            if (!Elapsed(now, rt.lastTrainPositionCheckTick, kTrainPositionCheckMs)) {
                const DWORD elapsedMs = rt.lastTrainPositionCheckTick == 0 ? 0 : now - rt.lastTrainPositionCheckTick;
                const DWORD remainSec = elapsedMs >= kTrainPositionCheckMs ? 0 : (kTrainPositionCheckMs - elapsedMs + 999) / 1000;
                rt.status = L"Train ổn định • Auto check 1 phút • tọa check sau " + std::to_wstring(remainSec) + L"s";
                return;
            }

            rt.lastTrainPositionCheckTick = now;
            State monitor{};
            monitor.valid = true; monitor.mapReady = true; monitor.waitingMap = false;
            monitor.mapID = s.mapID; monitor.x = s.x; monitor.y = s.y;
            monitor.riding = s.riding != 0; monitor.autoPathing = s.autoPathing != 0;
            Target monitorTarget{a.profile.target.mapID, a.profile.target.x, a.profile.target.y, a.profile.tolerance};
            if (AtTarget(monitor, monitorTarget)) {
                rt.status = L"CHECK 1 PHÚT: đúng tọa • tiếp tục đánh";
                LogAccount(a, L"CHECK 1 PHÚT: tọa train vẫn đúng.");
                return;
            }
            BeginTrainRecovery(a, now);
            if (HandleTrainRecovery(a, now)) return;
        }

        // V3.0 SHORTCUT FIRST: the initial AUTO TRAIN route must use the same shortcut selector
        // already used by sell-return/recovery. Only when no shortcut applies may normal Decide()
        // issue a direct route to the final training target.
        if (HandleShortcutTravel(a, now, a.profile.target)) return;

        State logic{};
        logic.valid = true;
        logic.mapReady = true;
        logic.waitingMap = false;
        logic.mapID = s.mapID;
        logic.x = s.x;
        logic.y = s.y;
        logic.riding = s.riding != 0;
        logic.autoPathing = s.autoPathing != 0;
        Target target{a.profile.target.mapID, a.profile.target.x, a.profile.target.y, a.profile.tolerance};
        const bool atTarget = AtTarget(logic, target);
        if (!atTarget) {
            rt.trainPositionMonitorArmed = false;
            rt.lastTrainPositionCheckTick = 0;
            rt.lastAutoFightCheckTick = 0;
            if (rt.wasAtTarget) {
                rt.fightPhase = 0;
                rt.fightAttempts = 0;
                rt.fightRetryWaitTick = 0;
            }
            rt.wasAtTarget = false;
        }

        const Action action = Decide(logic, target);
        if (action == Action::Hold) {
            rt.lastAction = Action::Hold;
            (void)CompleteToolOwnedRoute(rt, true, s.autoPathing != 0, s.riding != 0);
            if (!rt.wasAtTarget) {
                rt.fightPhase = 0;
                rt.fightAttempts = 0;
                rt.fightRetryWaitTick = 0;
                LogAccount(a, L"Đã tới bãi và ổn định.");
            }
            rt.wasAtTarget = true;
            if (HandleFightClicks(a, now)) return;
            rt.status = L"Đúng bãi • giám sát tọa độ";
            return;
        }
        if (action == Action::Wait) {
            if (s.autoPathing) rt.status = L"Đang AutoPath tới bãi";
            return;
        }
        SendDecision(a, action, a.profile.target, L"bãi train");
    }

    void RefreshAccountIdentityIfNeeded(Account& a) {
        if (!a.snapshotValid) return;
        const std::wstring newSection = ProfileSection(a.snapshot, a.game.pid);
        if (a.profile.section == newSection) return;
        // PID fallback is only temporary. Once RoleID is proven, switch to the persistent role profile.
        AccountProfile persistent = LoadProfile(newSection);
        const bool persistentHasData = persistent.tradeRole != 0 ||
            !persistent.selectedSpot.empty() || !persistent.rotationSpots.empty() || persistent.target.valid || persistent.enableSell ||
            !persistent.sellMacro.empty() ||
            std::any_of(persistent.points.begin(), persistent.points.end(), [](const ClickPoint& p){ return p.valid; });
        if (!persistentHasData) {
            persistent = a.profile;
            persistent.section = newSection;
        } else {
            // Merge data captured while identity was temporarily PID-based. The old
            // all-or-nothing switch could make newly captured clicks/macro appear lost.
            if (persistent.tradeRole == 0 && a.profile.tradeRole != 0) persistent.tradeRole = a.profile.tradeRole;
            if (persistent.selectedSpot.empty() && !a.profile.selectedSpot.empty()) persistent.selectedSpot = a.profile.selectedSpot;
            if (persistent.rotationSpots.empty() && !a.profile.rotationSpots.empty()) persistent.rotationSpots = a.profile.rotationSpots;
            if (!persistent.target.valid && a.profile.target.valid) persistent.target = a.profile.target;
            for (std::size_t i = 0; i < persistent.points.size(); ++i) {
                if (!persistent.points[i].valid && a.profile.points[i].valid) persistent.points[i] = a.profile.points[i];
            }
            if (persistent.sellMacro.empty() && !a.profile.sellMacro.empty()) persistent.sellMacro = a.profile.sellMacro;
        }
        persistent.section = newSection;
        SaveProfile(persistent);
        a.profile = persistent;
        if (a.profile.tradeRole >= 2) a.profile.enableSell = false;
        MigrateLegacySpot(a.profile);
        a.displayName = DisplayName(a.snapshot, a.game.pid);
    }

    void UpdateSelectedLive() {
        Account* a = SelectedAccount();
        if (!a) return;
        if (!a->snapshotValid) {
            SetText(live_, L"STATE: chưa đọc được snapshot");
            return;
        }
        const Snapshot& s = a->snapshot;
        std::wstring text = L"STATE " + AccountTag(*a) + L" • " + TradeRoleLabel(a->profile.tradeRole) + L" • M" + std::to_wstring(s.mapID) + L" • " +
                            std::to_wstring(s.x) + L"," + std::to_wstring(s.y) +
                            L" • Ngựa " + (s.riding ? L"ON" : L"OFF") +
                            L" • Path " + (s.autoPathing ? L"ON" : L"OFF");
        if (s.validMask & ValidLifeState) text += L" • " + std::wstring(s.dead ? L"CHẾT" : L"SỐNG");
        if (s.validMask & ValidAutoFight) text += L" • Đánh quái " + std::wstring(s.autoFight ? L"ON" : L"OFF");
        if (s.validMask & ValidBagSpace) text += L" • Túi trống " + std::to_wstring(s.freeBagSpace);
        if (a->profile.enableConfirm) text += (s.mapID == kLauLanMapId ? L" • XN LL watchdog ON" : L" • XN LL idle");
        if (globalPaused_) text += L" • F4 PAUSE";
        if (a->runtime.clientFreezeActive) text += L" • FREEZE ACTION";
        if (!s.mapReady || s.waitingChangeMap) text = L"STATE " + AccountTag(*a) + L" • ĐANG CHUYỂN MAP • FREEZE ACTION";
        SetText(live_, text);
    }

    void Tick() {

        // Snapshots + movement-observation run first, then v0.6.1 services semantic
        // background priorities before coordinate-based trade clicks.
        std::vector<bool> snapshotReady(accounts_.size(), false);
        for (std::size_t i = 0; i < accounts_.size(); ++i) {
            Account& a = *accounts_[i];
            const bool selected = static_cast<int>(i) == SelectedIndex();
            if (!a.runtime.running && !a.pk.active && !a.dungeonOwned && !selected) continue;
            std::wstring error;
            const DWORD now = GetTickCount();
            if (!ReadSnapshot(a, error, (a.runtime.running || a.pk.active || a.dungeonOwned) ? 700 : 900)) {
                if (a.runtime.running || a.pk.active) MarkReadStateFailure(a, error, now);
                else if (a.dungeonOwned) { a.snapshotValid = false; a.runtime.status = L"AUTO PHÓ BẢN • mất state/bridge"; }
                else a.runtime.status = L"Mất state/bridge";
                continue;
            }
            snapshotReady[i] = true;
            RefreshAccountIdentityIfNeeded(a);
            // Read-only movement observation MUST run even when BĐPT World Flow holds
            // this account. This feeds the Lâu Lan 3s stall watchdog before any priority click.
            const std::uint32_t observeNeed = ValidMap | ValidPosition | ValidAutoPath;
            if ((a.snapshot.validMask & observeNeed) == observeNeed && a.snapshot.mapReady && !a.snapshot.waitingChangeMap) {
                ObserveMovement(a, GetTickCount());
            }
            // Telegram v0.6 observer: same authoritative snapshot; network stays on worker thread.
            if (a.runtime.running) ObserveTelegramAccountState(a, GetTickCount());
        }

        // v0.6.1.9 priorities remain scoped per account, not global input barriers.
        // For each PID preserve local safety order P1 XN -> P2 revive -> P3 AUTO, while
        // unrelated windows never wait merely because another PID has a higher-priority action.
        if (!globalPaused_) {
            for (std::size_t i = 0; i < accounts_.size(); ++i) {
                if (i >= snapshotReady.size() || !snapshotReady[i]) continue;
                Account& a = *accounts_[i];
                if ((!a.runtime.running && !a.pk.active && !a.dungeonOwned) || RecorderBlocksAccount(a)) continue;
                const DWORD priorityNow = GetTickCount();
                if (PriorityLauLanGateConfirmClick(a, priorityNow)) continue;
                if (PriorityReviveClick(a, priorityNow)) continue;
                if (a.runtime.priorityAutoRequestSlot != ClickSlot::None) {
                    (void)PriorityAutoClick(a);
                }
            }
        }

        TickAutoPk(GetTickCount());
        if (!globalPaused_) TickDungeon(GetTickCount());

        for (std::size_t i = 0; i < accounts_.size(); ++i) {
            Account& a = *accounts_[i];
            const bool selected = static_cast<int>(i) == SelectedIndex();
            if (!a.runtime.running && !a.pk.active && !selected) {
                UpdateAccountRow(static_cast<int>(i), a);
                continue;
            }
            if (!snapshotReady[i]) {
                UpdateAccountRow(static_cast<int>(i), a);
                continue;
            }
            const DWORD now = GetTickCount();
            if (a.runtime.running) {
                // Hidden actions are per-client. REC pauses only the window(s) being
                // captured; unrelated accounts keep their normal FSM ticks. F4 remains global.
                if (RecorderBlocksAccount(a)) {
                    a.runtime.status = L"BĐPT RECORDING CỤC BỘ • chỉ acc này tạm giữ để ghi thao tác tay";
                } else if (HoldUntilClientStable(a, now)) {
                    UpdateAccountRow(static_cast<int>(i), a);
                    continue;
                } else if (!globalPaused_) {
                    if (HandleAutoPathFightInvariant(a, now)) {
                        // Hard invariant owns this tick for both normal and held accounts.
                    } else if (a.tradeHeld) {
                        // World Flow HOLD never owns the life observer. P2 may have invoked
                        // Đầu thai above; keep advancing DEAD -> revive phases -> ALIVE cold restart
                        // here, without releasing FIFO/World Flow ownership.
                        if (!HandleDeath(a, now)) {
                            a.runtime.status = a.runtime.tradeTravelReady
                                ? L"BĐPT HOLD • đã tới TỌA GD • chờ đúng FIFO • LIFE/XN vẫn check"
                                : L"BĐPT WORLD FLOW • đang đi TỌA GD • LIFE/XN vẫn check ưu tiên";
                        }
                    } else TickAccount(a);
                } else a.runtime.status = L"TẠM DỪNG F4 • BĐPT không cấp tick cho acc";
            }
            UpdateAccountRow(static_cast<int>(i), a);
        }
        if (!globalPaused_) {
            Account* activeMain = AccountByPid(tradeTxn_.mainPid);
            Account* activeChild = AccountByPid(tradeTxn_.childPid);
            const bool tradeRecorderBlocked = (activeMain && RecorderBlocksAccount(*activeMain)) ||
                                              (activeChild && RecorderBlocksAccount(*activeChild));
            if (!tradeRecorderBlocked) TickTradeCoordinator(GetTickCount());
            else SetTradeStatus(L"RECORDING CỤC BỘ • giữ workflow GD liên quan; acc khác vẫn chạy");
        }
        // Coordinator can change FIFO/WorldFlow state after the first observation.
        for (std::size_t i = 0; i < accounts_.size(); ++i) {
            if (i < snapshotReady.size() && snapshotReady[i] && accounts_[i]->runtime.running)
                ObserveTelegramAccountState(*accounts_[i], GetTickCount());
        }
        TickTelegramSchedules(GetTickCount());
        UpdateSelectedLive();
    }

    void OnListNotification(const NMHDR* hdr) {
        if (!hdr) return;
        if (hdr->hwndFrom == mainTab_ && hdr->code == TCN_SELCHANGE) {
            const int index = TabCtrl_GetCurSel(mainTab_);
            SwitchMainTab(index);
            return;
        }
        if (hdr->hwndFrom == dungeonTeamList_ && hdr->code == LVN_ITEMCHANGED) {
            const auto* n = reinterpret_cast<const NMLISTVIEW*>(hdr);
            if ((n->uChanged & LVIF_STATE) != 0 && (n->uNewState & LVIS_SELECTED) != 0) {
                RefreshDungeonStepList();
                const int teamIndex = SelectedDungeonTeamIndex();
                if (teamIndex >= 0 && teamIndex < static_cast<int>(dungeonTeams_.size()) && dungeonStatus_)
                    SetText(dungeonStatus_, L"ĐỘI " + std::to_wstring(dungeonTeams_[static_cast<std::size_t>(teamIndex)].config.id) +
                                            L" • " + dungeonTeams_[static_cast<std::size_t>(teamIndex)].status);
            }
            return;
        }
        if (hdr->hwndFrom == dungeonAccountList_ && hdr->code == LVN_ITEMCHANGED) {
            const auto* n = reinterpret_cast<const NMLISTVIEW*>(hdr);
            if ((n->uChanged & LVIF_STATE) != 0 && (n->uNewState & LVIS_SELECTED) != 0)
                RefreshDungeonSellThresholdEditor();
            return;
        }
        if (hdr->hwndFrom == clientList_ && hdr->code == LVN_ITEMCHANGED) {
            const auto* n = reinterpret_cast<const NMLISTVIEW*>(hdr);
            if ((n->uChanged & LVIF_STATE) != 0 && (n->uNewState & LVIS_SELECTED) != 0) {
                PersistSelectedEditorSafeBeforeSwitch(n->iItem);
                LoadSelectedProfileToUi();
            }
            return;
        }
        if (hdr->hwndFrom == autoPkStepList_ && hdr->code == LVN_ITEMCHANGED) {
            if (autoPkUiLoading_) return;
            const auto* n = reinterpret_cast<const NMLISTVIEW*>(hdr);
            if (n->iItem >= 0 && n->iItem < static_cast<int>(autoPkSteps_.size())) {
                if ((n->uChanged & LVIF_STATE) != 0 && ((n->uOldState ^ n->uNewState) & LVIS_STATEIMAGEMASK) != 0) {
                    autoPkSteps_[static_cast<std::size_t>(n->iItem)].enabled = ListView_GetCheckState(autoPkStepList_, n->iItem) != FALSE;
                    SaveAutoPkSettings();
                }
                if ((n->uChanged & LVIF_STATE) != 0 && (n->uNewState & LVIS_SELECTED) != 0) LoadAutoPkStepEditor(n->iItem);
            }
            return;
        }
        if (hdr->hwndFrom == autoPkClickList_ && hdr->code == LVN_ITEMCHANGED) {
            const auto* n = reinterpret_cast<const NMLISTVIEW*>(hdr);
            if ((n->uChanged & LVIF_STATE) != 0 && (n->uNewState & LVIS_SELECTED) != 0) LoadAutoPkClickEditor(n->iItem);
            return;
        }
        if (hdr->hwndFrom == sellMacroList_ && hdr->code == LVN_ITEMCHANGED) {
            const auto* n = reinterpret_cast<const NMLISTVIEW*>(hdr);
            if ((n->uChanged & LVIF_STATE) != 0 && (n->uNewState & LVIS_SELECTED) != 0) {
                ListView_SetItemState(sellMacroList_, n->iItem, LVIS_FOCUSED, LVIS_FOCUSED);
                LoadSelectedMacroEditor();
            }
            return;
        }
        if (hdr->hwndFrom == rotationList_ && hdr->code == LVN_ITEMCHANGED) {
            if (rotationUiLoading_) return;
            const auto* n = reinterpret_cast<const NMLISTVIEW*>(hdr);
            if ((n->uChanged & LVIF_STATE) != 0 && ((n->uOldState ^ n->uNewState) & LVIS_STATEIMAGEMASK) != 0) {
                Account* a = SelectedAccount();
                if (a) {
                    const std::wstring oldSpot = a->profile.selectedSpot;
                    PersistRotationListFromUi(*a);
                    SaveProfile(a->profile);
                    RefreshRotationList();
                    if (_wcsicmp(oldSpot.c_str(), a->profile.selectedSpot.c_str()) != 0) {
                        ApplyAutoSellerForTrainingTarget(*a);
                        SaveProfile(a->profile);
                        const DWORD now = GetTickCount();
                        ResetRotationWindow(*a, now);
                        if (a->runtime.running) BeginTrainRecovery(*a, now);
                        LogAccount(*a, L"Đổi pool → bãi hiện tại chuyển sang " + a->profile.selectedSpot);
                    }
                    const int row = SelectedIndex();
                    if (row >= 0) UpdateAccountRow(row, *a);
                }
            }
            return;
        }
    }

    void ToggleGlobalPause() {
        globalPaused_ = !globalPaused_;
        if (globalPaused_) {
            for (auto& item : accounts_) {
                Account& a = *item;
                if (!a.runtime.running && !a.pk.active && !a.dungeonOwned) continue;
                if (a.bridge.Attached() && !a.runtime.clientFreezeActive) {
                    Response r{}; std::wstring ignored;
                    (void)a.bridge.Call(Command::StopPath, 0, 0, 0, r, ignored, 700);
                }
                a.runtime.status = L"TẠM DỪNG F4";
            }
            Log(L"F4 → TẠM DỪNG toàn bộ acc đang RUN; StopPath đã gửi, không tự đổi combat.");
            if (telegramSettings_.enabled && telegramSettings_.notifyToolState)
                (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, L"⏸ F4 PAUSE\nThời gian: " + LocalDateTimeText(), L"F4 PAUSE", L"-");
        } else {
            for (auto& item : accounts_) if (item->runtime.running || item->pk.active || item->dungeonOwned) item->runtime.status = L"Tiếp tục sau F4";
            Log(L"F4 → TIẾP TỤC toàn bộ acc đang RUN.");
            if (telegramSettings_.enabled && telegramSettings_.notifyToolState)
                (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage, L"▶️ F4 RESUME\nThời gian: " + LocalDateTimeText(), L"F4 RESUME", L"-");
        }
    }

    void PersistSelectedEditorSafeBeforeSwitch(int newIndex) {
        // LVN_ITEMCHANGED arrives after selection state changes, so we cannot reliably know the old row here.
        // All meaningful editor mutations are persisted immediately on their own events/capture/save.
        (void)newIndex;
    }

    LRESULT Handle(UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case WM_CREATE:
                BuildUi();
                return 0;
            case kTelegramResultMessage:
                HandleTelegramWorkerResult(lp);
                return 0;
            case WM_NOTIFY: {
                const NMHDR* hdr = reinterpret_cast<const NMHDR*>(lp);
                if (hdr && hdr->hwndFrom == dungeonTeamList_ && hdr->code == NM_CUSTOMDRAW)
                    return DungeonTeamCustomDraw(reinterpret_cast<const NMLVCUSTOMDRAW*>(lp));
                OnListNotification(hdr);
                return 0;
            }
            case WM_COMMAND:
                switch (LOWORD(wp)) {
                    case IDC_TG_SHOW_TOKEN:
                        if (HIWORD(wp) == BN_CLICKED) ToggleTelegramTokenVisible();
                        break;
                    case IDC_TG_SAVE:
                        if (HIWORD(wp) == BN_CLICKED) (void)PersistTelegramSettingsFromUi(true);
                        break;
                    case IDC_TG_TEST_BOT:
                        if (HIWORD(wp) == BN_CLICKED) TelegramTestBot();
                        break;
                    case IDC_TG_DISCOVER_CHAT:
                        if (HIWORD(wp) == BN_CLICKED) TelegramDiscoverChatId();
                        break;
                    case IDC_TG_SEND_TEST:
                        if (HIWORD(wp) == BN_CLICKED) TelegramSendTest();
                        break;
                    case IDC_TG_SEND_SUMMARY:
                        if (HIWORD(wp) == BN_CLICKED) TelegramSendSummaryNow();
                        break;
                    case IDC_TG_CLEAR_LOG:
                        if (HIWORD(wp) == BN_CLICKED) ClearTelegramLog();
                        break;
                    case IDC_TG_COPY_LOG:
                        if (HIWORD(wp) == BN_CLICKED) CopyTelegramLog();
                        break;
                    case IDC_TG_ENABLED:
                    case IDC_TG_NOTIFY_DEATH:
                    case IDC_TG_NOTIFY_REVIVE:
                    case IDC_TG_NOTIFY_SELL_COMPLETE:
                    case IDC_TG_NOTIFY_SELL_SUMMARY:
                    case IDC_TG_NOTIFY_TRADE:
                    case IDC_TG_NOTIFY_FREEZE:
                    case IDC_TG_NOTIFY_FIFO:
                    case IDC_TG_NOTIFY_LAULAN:
                    case IDC_TG_NOTIFY_WORLDFLOW_TIMEOUT:
                    case IDC_TG_NOTIFY_TOOL_STATE:
                    case IDC_TG_NOTIFY_SESSION_SUMMARY:
                    case IDC_TG_NOTIFY_FUN_ALERTS:
                    case IDC_TG_MONEY_1M:
                    case IDC_TG_MONEY_5M:
                    case IDC_TG_MONEY_60M:
                    case IDC_TG_MONEY_6H:
                    case IDC_TG_MONEY_24H:
                    case IDC_TG_INTERVAL_ENABLED:
                    case IDC_TG_DAILY_ENABLED:
                        if (HIWORD(wp) == BN_CLICKED) (void)PersistTelegramSettingsFromUi(false);
                        break;
                    case IDC_TG_TOKEN:
                    case IDC_TG_CHAT_ID:
                    case IDC_TG_INTERVAL_MINUTES:
                    case IDC_TG_DAILY_TIME1:
                    case IDC_TG_DAILY_TIME2:
                    case IDC_TG_DAILY_TIME3:
                    case IDC_TG_DAILY_TIME4:
                    case IDC_TG_WORLDFLOW_TIMEOUT_SEC:
                        if (HIWORD(wp) == EN_KILLFOCUS) (void)PersistTelegramSettingsFromUi(false);
                        break;
                    case IDC_SCAN:
                        if (mainTabIndex_ == 1 && autoPkRunning_) StopAutoPk(L"quét lại client");
                        ScanClients();
                        break;
                    case IDC_TRADE_ROLE:
                        if (HIWORD(wp) == CBN_SELCHANGE) ApplySelectedTradeRole();
                        break;
                    case IDC_CONSOLIDATE_TOGGLE:
                        if (HIWORD(wp) == BN_CLICKED) ToggleConsolidationMode();
                        break;
                    case IDC_COMPACT_TOGGLE:
                        if (HIWORD(wp) == BN_CLICKED) ToggleCompactMode();
                        break;
                    case IDC_EXPORT_CLICK_CONFIG:
                        if (HIWORD(wp) == BN_CLICKED) ExportPortableMasterConfig();
                        break;
                    case IDC_IMPORT_CLICK_CONFIG:
                        if (HIWORD(wp) == BN_CLICKED) ImportPortableMasterConfig();
                        break;
                    case IDC_SELL_SEQUENCE:
                        ToggleSellMacroEditor();
                        break;
                    case IDC_MAIN_TRADE_SEQUENCE:
                        OpenTradeSequenceEditor(1);
                        break;
                    case IDC_CHILD_TRADE_SEQUENCE:
                        OpenTradeSequenceEditor(2);
                        break;
                    case IDC_TRADE_RENDEZVOUS_CAPTURE:
                        CaptureTradeRendezvous();
                        break;
                    case IDC_COPY_CLICKS:
                        CopyClicksFromAnotherAccount();
                        break;
                    case IDC_BAG_FILTER_OPEN:
                        OpenInventoryFilterWindow();
                        break;
                    case IDC_DG_REFRESH: RefreshDungeonAccountList(); break;
                    case IDC_DG_SCAN_CLIENT: DungeonScanClients(); break;
                    case IDC_DG_CREATE_TEAM: CreateDungeonTeam(); break;
                    case IDC_DG_START: StartDungeonSelected(); break;
                    case IDC_DG_PAUSE: PauseResumeDungeonSelected(); break;
                    case IDC_DG_STOP: StopDungeonSelected(); break;
                    case IDC_DG_DELETE_TEAM: DeleteDungeonSelected(); break;
                    case IDC_DG_SCAN_MONSTER: DungeonManualScan(); break;
                    case IDC_DG_CONFIG: OpenDungeonEditor(); break;
                    case IDC_DG_SAVE_THRESHOLD: SaveDungeonSellThresholdSelected(); break;
                    case IDC_DG_PRESET:
                        if (HIWORD(wp) == CBN_SELCHANGE) RefreshDungeonStepList();
                        break;
                    case IDC_PK_START:
                        StartAutoPk();
                        break;
                    case IDC_PK_STOP:
                        StopAutoPk();
                        break;
                    case IDC_PK_LIFE:
                        if (HIWORD(wp) == BN_CLICKED) { autoPkLifeCheck_ = SendMessageW(autoPkLife_, BM_GETCHECK, 0, 0) == BST_CHECKED; SaveAutoPkSettings(); }
                        break;
                    case IDC_PK_LOOP:
                        if (HIWORD(wp) == BN_CLICKED) { autoPkLoop_ = SendMessageW(autoPkLoopCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED; SaveAutoPkSettings(); }
                        break;
                    case IDC_PK_STEP_UP: MoveAutoPkStep(-1); break;
                    case IDC_PK_STEP_DOWN: MoveAutoPkStep(1); break;
                    case IDC_PK_TARGET_CAPTURE: CaptureAutoPkTarget(); break;
                    case IDC_PK_STEP_SAVE: SaveAutoPkStepEditor(); break;
                    case IDC_PK_CLICK_ADD: AddAutoPkClick(); break;
                    case IDC_PK_CLICK_DELETE: DeleteAutoPkClick(); break;
                    case IDC_PK_CLICK_UP: MoveAutoPkClick(-1); break;
                    case IDC_PK_CLICK_DOWN: MoveAutoPkClick(1); break;
                    case IDC_PK_CLICK_SAVE: SaveAutoPkClickEditor(); break;
                    case IDC_PK_CLICK_CAPTURE: BeginAutoPkClickCapture(); break;
                    case IDC_PK_CLICK_TEST: TestAutoPkClick(); break;
                    case IDC_START_CHECKED:
                        StartChecked();
                        break;
                    case IDC_STOP_CHECKED:
                        StopChecked();
                        break;
                    case IDC_ROTATE_DEATH_LIMIT:
                    case IDC_ROTATE_DEATH_WINDOW:
                    case IDC_ROTATE_NO_BAG:
                        if (HIWORD(wp) == EN_KILLFOCUS) PersistSelectedEditor();
                        break;
                    case IDC_SAVE_TARGET:
                        SaveTargetForSelected();
                        break;
                    case IDC_DELETE_SPOT:
                        DeleteSelectedSharedSpot();
                        break;
                    case IDC_SPOT_COMBO:
                        if (HIWORD(wp) == CBN_SELCHANGE) SelectSharedSpotForAccount();
                        break;
                    case IDC_CAPTURE_AUTO:
                        BeginCapture(ClickSlot::AutoMenu);
                        break;
                    case IDC_CAPTURE_ATTACK:
                        BeginCapture(ClickSlot::Attack);
                        break;
                    case IDC_CAPTURE_STOP_AUTO_2:
                        BeginCapture(ClickSlot::StopAuto2);
                        break;
                    case IDC_TEST_AUTO:
                        TestClick(ClickSlot::AutoMenu);
                        break;
                    case IDC_TEST_ATTACK:
                        TestClick(ClickSlot::Attack);
                        break;
                    case IDC_TEST_STOP_AUTO_2:
                        TestClick(ClickSlot::StopAuto2);
                        break;
                    case IDC_SELL_ADD:
                        AddSellMacroRow();
                        break;
                    case IDC_SELL_DELETE:
                        DeleteSellMacroRow();
                        break;
                    case IDC_SELL_SAVE:
                        SaveSellMacroRow();
                        break;
                    case IDC_SELL_CAPTURE:
                        BeginMacroCapture();
                        break;
                    case IDC_SELL_TEST:
                        TestSellMacroRow();
                        break;
                    case IDC_SELL_REC:
                        ToggleSellRecorder();
                        break;
                    case IDC_SELL_COPY:
                        CopySelectedSellRows();
                        break;
                    case IDC_SELL_PASTE:
                        PasteSellRows();
                        break;
                    case IDC_SELL_COPY_ACCOUNT:
                        CopySellSequenceFromAnotherAccount();
                        break;
                    case IDC_ENABLE_SHORTCUT:
                        if (HIWORD(wp) == BN_CLICKED) {
                            shortcutSettings_.enabled = SendMessageW(enableShortcut_, BM_GETCHECK, 0, 0) == BST_CHECKED;
                            SaveShortcutSettings(shortcutSettings_);
                            Log(std::wstring(L"ĐƯỜNG TẮT: ") + (shortcutSettings_.enabled ? L"BẬT" : L"TẮT"));
                            if (shortcutSettings_.enabled) OpenShortcutSettingsWindow();
                            else if (shortcutWindow_ && IsWindow(shortcutWindow_)) ShowWindow(shortcutWindow_, SW_HIDE);
                        }
                        break;
                    case IDC_SHORTCUT_SETTINGS:
                        if (HIWORD(wp) == BN_CLICKED) OpenShortcutSettingsWindow();
                        break;
                    case IDC_ENABLE_REVIVE:
                    case IDC_ENABLE_CONFIRM:
                    case IDC_ENABLE_FIGHT:
                    case IDC_ENABLE_SELL:
                        if (HIWORD(wp) == BN_CLICKED) PersistSelectedEditor();
                        break;
                    case IDC_SELL_NPC:
                        if (HIWORD(wp) == CBN_SELCHANGE) OnSellNpcSelectionChanged();
                        break;
                    case IDC_SELL_NPC_CAPTURE:
                        if (HIWORD(wp) == BN_CLICKED) CaptureSellNpcPosition();
                        break;
                    case IDC_SELL_NPC_X:
                    case IDC_SELL_NPC_Y:
                    case IDC_TOLERANCE:
                        if (HIWORD(wp) == EN_KILLFOCUS) PersistSelectedEditor();
                        break;
                }
                return 0;
            case WM_HOTKEY:
                if (static_cast<int>(wp) == kCaptureHotkeyId) {
                    CaptureHotkeyPoint();
                    return 0;
                }
                if (static_cast<int>(wp) == kPauseHotkeyId) {
                    ToggleGlobalPause();
                    return 0;
                }
                break;
            case WM_TIMER:
                if (wp == kRecordTimer) { PollRecorder(); return 0; }
                if (wp == kTimer) Tick();
                return 0;
            case WM_CLOSE:
                StopAutoPk(L"đóng ứng dụng");
                if (recorderMode_ != RecorderMode::None) StopRecorder(true);
                if (dungeonEditor_ && IsWindow(dungeonEditor_)) DestroyWindow(dungeonEditor_);
                DestroyWindow(hwnd_);
                return 0;
            case WM_DESTROY:
                if (recorderMode_ != RecorderMode::None) StopRecorder(false);
                // Auto-save every persistent input before exit. Captures already save
                // immediately; this final pass also commits the currently edited macro row.
                SaveSellMacroRow();
                SaveAutoPkSettings();
                SaveDungeonTeams();
                SaveInventoryFilterSettings(inventoryFilter_);
                PersistSelectedEditor();
                SaveSharedSellNpcPositions(sellNpcPositions_);
                for (auto& a : accounts_) SaveProfile(a->profile);
                (void)PersistTelegramSettingsFromUi(false);
                telegramWorker_.Stop();
                {
                    MSG pending{};
                    while (PeekMessageW(&pending, hwnd_, kTelegramResultMessage, kTelegramResultMessage, PM_REMOVE))
                        delete reinterpret_cast<telegram_notify::Result*>(pending.lParam);
                }
                FlushIni();
                UnregisterHotKey(hwnd_, kCaptureHotkeyId);
                UnregisterHotKey(hwnd_, kPauseHotkeyId);
                for (auto& a : accounts_) a->bridge.Close();
                if (shortcutDarkBrush_) { DeleteObject(shortcutDarkBrush_); shortcutDarkBrush_ = nullptr; }
                for (HFONT* font : {&aboutHeadingFont_, &aboutNameFont_, &aboutUpcomingFont_, &aboutBodyFont_}) {
                    if (*font) { DeleteObject(*font); *font = nullptr; }
                }
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProcW(hwnd_, msg, wp, lp);
    }

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    HWND clientList_ = nullptr;
    HWND selected_ = nullptr;
    HWND live_ = nullptr;
    HWND tradeRoleCombo_ = nullptr;
    HWND tradeEnable_ = nullptr;
    HWND tradeStatus_ = nullptr;
    HWND tradeEditor_ = nullptr;
    HWND tradeSeqList_ = nullptr;
    HWND tradeSeqTarget_ = nullptr;
    HWND tradeSeqDesc_ = nullptr;
    HWND tradeSeqDelay_ = nullptr;
    HWND tradeSeqRepeat_ = nullptr;
    HWND tradeSeqGroupRepeat_ = nullptr;
    HWND tradeRecordButton_ = nullptr;
    HWND tradeRecordStatus_ = nullptr;
    HWND targetName_ = nullptr;
    HWND spotCombo_ = nullptr;
    HWND targetText_ = nullptr;
    HWND tolerance_ = nullptr;
    HWND enableRevive_ = nullptr;
    HWND enableConfirm_ = nullptr;
    HWND enableShortcut_ = nullptr;
    HWND shortcutSettingsButton_ = nullptr;
    HWND rotationList_ = nullptr;
    HWND rotateDeathLimit_ = nullptr;
    HWND rotateDeathWindow_ = nullptr;
    HWND rotateNoFullBag_ = nullptr;
    HWND enableFight_ = nullptr;
    HWND enableSell_ = nullptr;
    HWND sellNpcCombo_ = nullptr;
    HWND sellNpcX_ = nullptr;
    HWND sellNpcY_ = nullptr;
    HWND sellNpcPosText_ = nullptr;
    HWND sellMacroList_ = nullptr;
    HWND sellDesc_ = nullptr;
    HWND sellDelay_ = nullptr;
    HWND sellRepeat_ = nullptr;
    HWND sellRecordButton_ = nullptr;
    HWND sellRecordStatus_ = nullptr;
    std::vector<HWND> sellMacroControls_{};
    bool sellMacroEditorVisible_ = false;
    HWND logCaption_ = nullptr;
    HWND sellSequenceButton_ = nullptr;
    HWND mainTradeSequenceButton_ = nullptr;
    HWND childTradeSequenceButton_ = nullptr;
    HWND tradeRendezvousCaptureButton_ = nullptr;
    HWND tradeRendezvousLabel_ = nullptr;
    HWND scanButton_ = nullptr;
    HWND startCheckedButton_ = nullptr;
    HWND stopCheckedButton_ = nullptr;
    HWND compactButton_ = nullptr;
    std::array<HWND, 5> pointLabels_{};
    HWND log_ = nullptr;
    HWND mainTab_ = nullptr;
    int mainTabIndex_ = 0;
    std::vector<HWND> aboutControls_{};
    std::vector<HWND> telegramControls_{};
    std::vector<HWND> dungeonControls_{};
    HWND dungeonAccountList_=nullptr;
    HWND dungeonLeaderCombo_=nullptr;
    HWND dungeonPresetCombo_=nullptr;
    HWND dungeonRunsEdit_=nullptr;
    HWND dungeonTeamList_=nullptr;
    HWND dungeonStepList_=nullptr;
    HWND dungeonStatus_=nullptr;
    HWND dungeonMonsterStatus_=nullptr;
    HWND dungeonSellThresholdEdit_=nullptr;
    std::vector<cleanroute_dungeon::Preset> dungeonPresets_{};
    std::vector<DungeonTeamRuntime> dungeonTeams_{};
    int dungeonNextTeamId_=1;

    // Modeless preset/step editor. Runtime teams freeze a copy of their preset at START,
    // so editing never mutates an already-running state machine.
    HWND dungeonEditor_=nullptr;
    HWND dungeonEditorList_=nullptr;
    HWND dungeonEditorKind_=nullptr;
    HWND dungeonEditorLabel_=nullptr;
    HWND dungeonEditorMap_=nullptr;
    HWND dungeonEditorX_=nullptr;
    HWND dungeonEditorY_=nullptr;
    HWND dungeonEditorTolerance_=nullptr;
    HWND dungeonEditorKills_=nullptr;
    HWND dungeonEditorRadius_=nullptr;
    HWND dungeonEditorDelay_=nullptr;
    HWND dungeonEditorTimeout_=nullptr;
    HWND dungeonEditorMonster_=nullptr;
    HWND dungeonEditorResID_=nullptr;
    HWND dungeonEditorGroup_=nullptr;
    HWND dungeonEditorBoss_=nullptr;
    HWND dungeonEditorAny_=nullptr;
    std::array<HWND,6> dungeonEditorSlots_{};
    HWND dungeonEditorGatherMap_=nullptr;
    HWND dungeonEditorNpc_=nullptr;
    HWND dungeonEditorGatherX_=nullptr;
    HWND dungeonEditorGatherY_=nullptr;
    HWND dungeonEditorDungeonMap_=nullptr;
    HWND dungeonEditorDialog_=nullptr;
    int dungeonEditorPresetIndex_=-1;
    HFONT aboutHeadingFont_ = nullptr;
    HFONT aboutNameFont_ = nullptr;
    HFONT aboutUpcomingFont_ = nullptr;
    HFONT aboutBodyFont_ = nullptr;
    std::vector<std::pair<HWND, bool>> autoTabVisibility_{};
    std::vector<std::pair<HWND, bool>> compactVisibility_{};
    bool compactMode_ = false;

    // Global shortcut router/settings. It is opt-in and does not alter unrelated direct routes when disabled.
    ShortcutSettings shortcutSettings_ = LoadShortcutSettings();
    HWND shortcutWindow_ = nullptr;
    HWND shortcutTheme_ = nullptr;
    HWND shortcutSellerCombo_ = nullptr;
    HWND shortcutSellerCoordLabel_ = nullptr;
    std::array<HWND, 28> shortcutCoordEdits_{};
    std::array<HWND, 3> shortcutClickLabels_{};
    std::array<HWND, 3> shortcutClickTimeEdits_{};
    std::array<HWND, 3> shortcutClickDelayEdits_{};
    int shortcutKunlunCaptureIndex_ = -1;
    HBRUSH shortcutDarkBrush_ = nullptr;

    // v1.3 semantic inventory filter UI + shared AUTO policy.
    inventory_filter_logic::Settings inventoryFilter_{};
    HWND inventoryFilterOpenButton_ = nullptr;
    HWND inventoryFilterWindow_ = nullptr;
    HWND inventoryFilterBagList_ = nullptr;
    HWND inventoryFilterRuleList_ = nullptr;
    HWND inventoryFilterStatus_ = nullptr;
    HWND inventoryFilterEnabled_ = nullptr;
    HWND inventoryFilterProtectBound_ = nullptr;
    HWND inventoryFilterKeepWeapon_ = nullptr;
    HWND inventoryFilterDropEquip_ = nullptr;
    HWND inventoryFilterSellEquip_ = nullptr;
    HWND inventoryFilterDropCommon_ = nullptr;
    HWND inventoryFilterSellCommon_ = nullptr;
    HWND inventoryFilterDropGem_ = nullptr;
    HWND inventoryFilterSellGem_ = nullptr;
    HWND inventoryFilterDropMedicine_ = nullptr;
    HWND inventoryFilterSellMedicine_ = nullptr;
    HWND inventoryFilterDropPetEquip_ = nullptr;
    HWND inventoryFilterSellPetEquip_ = nullptr;
    DWORD inventoryFilterPid_ = 0;
    std::vector<InventoryBagRow> inventoryFilterBagRows_{};

    // v1.3: Telegram observer/output subsystem; bound-gold reporting remains read-only. It owns no game input,
    // route, trade, AUTO or AutoPK scheduling state.
    HWND telegramEnabled_ = nullptr;
    HWND telegramToken_ = nullptr;
    HWND telegramShowToken_ = nullptr;
    HWND telegramChatId_ = nullptr;
    HWND telegramStatus_ = nullptr;
    HWND telegramLog_ = nullptr;
    HWND telegramNotifyDeath_ = nullptr;
    HWND telegramNotifyRevive_ = nullptr;
    HWND telegramNotifySellComplete_ = nullptr;
    HWND telegramNotifySellSummary_ = nullptr;
    HWND telegramNotifyTrade_ = nullptr;
    HWND telegramNotifyFreeze_ = nullptr;
    HWND telegramNotifyFifo_ = nullptr;
    HWND telegramNotifyLauLan_ = nullptr;
    HWND telegramNotifyWorldFlowTimeout_ = nullptr;
    HWND telegramNotifyToolState_ = nullptr;
    HWND telegramNotifySessionSummary_ = nullptr;
    HWND telegramNotifyFunAlerts_ = nullptr;
    std::array<HWND, 5> telegramMoneyMilestone_{};
    HWND telegramIntervalEnabled_ = nullptr;
    HWND telegramIntervalMinutes_ = nullptr;
    HWND telegramDailyEnabled_ = nullptr;
    std::array<HWND, 4> telegramDailyTime_{};
    HWND telegramWorldFlowTimeoutSec_ = nullptr;
    bool telegramTokenVisible_ = false;
    TelegramSettings telegramSettings_{};
    std::wstring telegramLoadWarning_{};
    telegram_notify::Worker telegramWorker_{};
    std::uint64_t telegramRequestCounter_ = 0;
    TelegramStats telegramStats_{};
    TelegramReportBaseline telegramReportBaseline_{};
    std::wstring telegramReportBaselineTime_{};
    std::map<DWORD, TelegramAccountWatch> telegramWatch_{};
    DWORD telegramLastIntervalSummaryTick_ = 0;
    std::array<std::uint64_t, 4> telegramLastDailyKeys_{};

    HWND autoPkStatus_ = nullptr;
    HWND autoPkLife_ = nullptr;
    HWND autoPkLoopCheck_ = nullptr;
    HWND autoPkStepList_ = nullptr;
    HWND autoPkTargetLabel_ = nullptr;
    HWND autoPkStepDesc_ = nullptr;
    HWND autoPkTolerance_ = nullptr;
    HWND autoPkStepDelay_ = nullptr;
    HWND autoPkStepRepeat_ = nullptr;
    HWND autoPkClickList_ = nullptr;
    HWND autoPkClickPhase_ = nullptr;
    HWND autoPkClickDesc_ = nullptr;
    HWND autoPkClickDelay_ = nullptr;
    HWND autoPkClickRepeat_ = nullptr;
    std::vector<HWND> autoPkControls_{};
    std::vector<AutoPkStep> autoPkSteps_{};
    bool autoPkRunning_ = false;
    bool autoPkLoop_ = false;
    bool autoPkLifeCheck_ = true;
    bool autoPkUiLoading_ = false;
    bool autoPkRecoveryActive_ = false;
    int autoPkCurrentStep_ = -1;
    int autoPkStepPass_ = 1;
    int autoPkEnterPhase_ = 0;
    DWORD autoPkDueTick_ = 0;
    int autoPkDragStartRow_ = -1;

    std::vector<std::unique_ptr<Account>> accounts_;
    std::vector<TargetProfile> spots_;
    std::array<SellNpcPosition, kSellNpcs.size()> sellNpcPositions_ = LoadSharedSellNpcPositions();
    ClickSlot captureSlot_ = ClickSlot::None;
    int captureMacroIndex_ = -1;
    DWORD capturePid_ = 0;
    int captureTradeSequenceIndex_ = -1;
    int captureTradeSequenceMode_ = 0;
    int captureTradeSequenceMainRef_ = -1;
    int capturePkStepIndex_ = -1;
    int capturePkClickIndex_ = -1;
    bool globalPaused_ = false;
    bool rotationUiLoading_ = false;

    std::vector<TradeSequenceStep> mainTradeSequence_{};
    std::vector<TradeSequenceStep> childTradeSequence_{}; // v0.2.7 one GLOBAL workflow used by whichever CON is active.
    std::vector<DWORD> tradeQueuePids_{}; // R7: max 3; immutable entry tickets are authoritative FIFO.
    std::uint64_t tradeWorkflowEntryCounter_ = 0;
    std::vector<TradeSequenceStep> legacyChildTradeTemplate_{};
    bool sharedChildTradeMigrationDone_ = false;
    int tradeEditorMode_ = 0; // 1=MAIN shared sequence, 2=GLOBAL ACC CON workflow; selected CON is capture/test donor only.
    DWORD tradeEditorChildPid_ = 0;
    struct TradeTxn {
        TradePhase phase = TradePhase::Idle;
        DWORD mainPid = 0; DWORD childPid = 0; int childSlot = 0;
        DWORD cooldownUntil = 0;
        DWORD targetStartedTick = 0;
        DWORD targetRetryTick = 0;
        DWORD targetLastSelectTick = 0;
        int targetAttempts = 0;
        std::size_t sequenceIndex = 0;
        int sequenceRepeatDone = 0;
        int sequenceGroupRepeatDone = 0;
        int sequencePass = 1;
        DWORD sequenceDueTick = 0;
        int sequenceMainFreeBeforePass = -1;
        DWORD sequenceBagVerifyStartedTick = 0;
        DWORD sequenceBagStableSinceTick = 0;
        int sequenceBagLastFree = -1;
    } tradeTxn_{};
    bool tradeEnabled_ = true;
    TargetProfile tradeRendezvous_{};
    int tradeRendezvousTolerance_ = kPreciseWorldTolerance;
    RecorderMode recorderMode_ = RecorderMode::None;
    DWORD recorderPrimaryPid_ = 0;
    bool recorderMouseDown_ = false;
    std::vector<RecordedClick> recorderClicks_{};
    std::vector<SellMacroStep> sellClipboard_{};
    std::vector<TradeSequenceStep> tradeClipboard_{};
    int tradeClipboardMode_ = 0;
    bool tradeSeqDragSelecting_ = false;
    bool tradeSeqDragUpdating_ = false;
    int tradeSeqDragStartRow_ = -1;

};

} // namespace

void EnableDpiAwareness() {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    using SetContextFn = BOOL (WINAPI*)(HANDLE);
    SetContextFn setContext = nullptr;
    if (user32) ResolveProc(user32, "SetProcessDpiAwarenessContext", setContext);
    if (setContext) {
        // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 == (HANDLE)-4. Dynamic lookup keeps old SDKs buildable.
        (void)setContext(reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4)));
    } else {
        (void)SetProcessDPIAware();
    }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    // Prevent DPI virtualization from corrupting cursor->client coordinate capture on scaled displays.
    EnableDpiAwareness();
    App app;
    if (!app.Create(instance)) return 2;
    app.Show(show);
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
