from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
C = (ROOT / 'src/controller.cpp').read_text(encoding='utf-8')
B = (ROOT / 'src/bridge.cpp').read_text(encoding='utf-8')
P = (ROOT / 'src/protocol.h').read_text(encoding='utf-8')
H = (ROOT / 'src/thdc_route_logic.h').read_text(encoding='utf-8')
R = (ROOT / 'src/route_logic_test.cpp').read_text(encoding='utf-8')
TN = (ROOT / 'src/travel_network_logic.h').read_text(encoding='utf-8')
TNT = (ROOT / 'src/travel_network_logic_test.cpp').read_text(encoding='utf-8')
W = (ROOT / '.github/workflows/build-v41.yml').read_text(encoding='utf-8')
CM = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8')
RC = (ROOT / 'resources/app.rc').read_text(encoding='utf-8')
ALL = '\n'.join([C, B, P, H, R, TN, TNT])

def need(ok, message):
    if not ok:
        print('FAIL:', message)
        sys.exit(1)

# Product/version contract.
need((ROOT / 'VERSION.txt').read_text(encoding='utf-8').strip() == '4.2', 'VERSION.txt must be 4.2')
need('project(ThanLongItemConsolidator VERSION 4.2' in CM, 'CMake version must be 4.2')
need('FILEVERSION 4,2,0,0' in RC and 'PRODUCTVERSION 4,2,0,0' in RC, 'VERSIONINFO must be 4.2')
need('AUTO Thần Long đa tính năng Pro v4.2' in C, 'visible product title v4.2 missing')
need('AUTO_Than_Long_da_tinh_nang_Pro_v4.2.exe' in RC and
     'AUTO-Than-Long-da-tinh-nang-Pro-v4.2-win-x64.zip' in W, 'v4.2 artifact names missing')
need('kProtocolVersion = 0x00040200u' in P, 'EXE/DLL protocol must be v4.2')

# Existing critical safety/role contracts stay present.
for token in ['RequestTrade', 'OtherRolePopup', 'TradeInvite', 'IDC_TEST_GD', 'BeginTestTrade']:
    need(token not in ALL, f'forbidden legacy trade/test token remains: {token}')
need('SelectTargetByRoleID = 22' in P, 'target-only command missing')
need('ExactMethod(c.gameApi, "SelectTarget", 1, true, "System.Int32")' in B, 'verified SelectTarget(Int32) missing')
need('enum class TradePhase { Idle, Rendezvous, TargetMain, Sequence };' in C, 'trade production phases changed')
need('tradeTxn_.sequenceIndex = 1;' not in C, 'obsolete row-1 skip remains')
need('a.bridge.Call(Command::ConfirmMap, 0, 0, 0, response, error, 2200)' in C, 'normal ConfirmMap changed')
need('a.bridge.Call(Command::Revive, 0, 0, 0, response, error, 2200)' in C, 'normal Revive changed')
need('constexpr int kChildTradeCount = 12;' in C and 'CON1→CON12' in C, 'CON1-CON12 role range missing')
need('CanStartPath(a.snapshot.riding != 0)' in C and 'KHÔNG chạy bộ AutoPath' in C, 'riding-only StartPath invariant missing')

# Coordinate schema/UI/export.
need('WriteIniInt(section, L"CoordinateVersion", 4);' in C and 'if (coordinateVersion >= 4)' in C,
     'shortcut CoordinateVersion 4 missing')
for token in [
    'std::array<TimedClickPoint, 3> kunlunExitClicks',
    'IDC_SC_CAPTURE_KUNLUN_CLICK_0', 'IDC_SC_CAPTURE_KUNLUN_CLICK_2',
    'IDC_SC_KUNLUN_TIME_0', 'IDC_SC_KUNLUN_DELAY_2',
    'Time (ms)=chờ trước click', 'Delay (ms)=chờ sau click',
    'std::array<HWND, 28> shortcutCoordEdits_',
    'IDC_SC_CAPTURE_THDC_0', 'IDC_SC_CAPTURE_THDC_6',
    'TLMASTERCFG\\t2', 'THDC_COUNT\\t7', 'KUNLUN_EXIT_CLICK',
]:
    need(token in C, f'v3.3 config/UI/export token missing: {token}')
need('ApplyThdcCoordinatePairs(shortcutSettings_,incoming.thdcCoords)' in C and
     'shortcutSettings_.kunlunExitClicks=incoming.kunlunExitClicks' in C,
     'master import does not apply THDC/3-click timing')
need('1190,1603' not in C and '1190, 1603' not in C, 'discarded M10017 train coordinate leaked into portal config')

# Côn Lôn exit: only three configured TryClickUI steps; no semantic Dai Ly path.
for forbidden in [
    'ReservedDaiLy', 'selectionID=200001',
    'TravelSelectionTag', 'IsV018XaTruyenTitle', 'v018DaiLy',
    'kunlunOpenClick', 'useKunlunTryClick',
]:
    need(forbidden not in ALL, f'old Côn Lôn semantic callback residue remains: {forbidden}')
cls = re.search(r'bool HandleKunLunExitTryClickRoute\(.*?\n    \}\n\n    bool HandleShortcutNpcRoute', C, re.S)
need(cls is not None, 'cannot isolate dedicated three-click Côn Lôn route')
cb = cls.group(0)
need(cb.count('DispatchInternalPointActionDirect') == 1 and
     'shortcutClickIndex' in cb and 'rt.shortcutClickIndex == 3' in cb,
     'Côn Lôn route must serialize exactly one dispatcher site across 3 configured rows')
need('TravelSemantic::DaiLy' not in cb, 'Côn Lôn exit must remain three-click only; v4.2 DaiLy semantic is for proven Xa Truyen return routes')
need('previousDelay' in cb and 'click.timeMs' in cb and 'kunlunExitClicks[2].delayMs' in cb,
     'per-click Time/Delay scheduling missing')
need('Mở NPC → Đại Lý → Xác nhận' in cb and 'không còn callback Đại Lý/Xác nhận' in cb,
     'three-click identity/order not explicit')
need('Command::ClickTravelSemantic' not in cb and 'Command::ConfirmTravelSemantic' not in cb,
     'Côn Lôn exit still calls semantic destination/confirm callback')
need(cb.find('rt.shortcutClickIndex == 3') < cb.find('s.mapID != rt.shortcutSourceMap'),
     'MapID must only be evaluated after all three clicks')
need('HandleKunLunExitTryClickRoute(a, now, finalTarget, npc)' in C,
     'KunLunExit switch is not routed to the dedicated three-click FSM')
need('Xa Truyền Bình ID387 đi vào Côn Lôn' in cb, 'enter/exit Côn Lôn NPC distinction missing')

# Tinh Tuc fix remains no-confirm.
need('DenCacMonPhai = 3' in P and 'TravelSemantic::DenCacMonPhai' in C,
     'two-level Tinh Túc semantic contract missing')
phase6 = re.search(r'if \(rt\.shortcutPhase == 6\) \{.*?\n        \}\n\n        if \(rt\.shortcutPhase == 4\)', C, re.S)
need(phase6 is not None and 'HỎA DIỆM NO-CONFIRM' in phase6.group(0) and
     'Command::ConfirmTravelSemantic' not in phase6.group(0),
     'Tinh Túc must still transition directly without confirmation')

# THDC exact source-map gate topology and defaults.
defaults = [
    'int thdcEntryX = 8257, thdcEntryY = 148110',
    'int thdcFloor1UpX = 890, thdcFloor1UpY = 6895',
    'int thdcFloor2UpX = 3080, thdcFloor2UpY = 2900',
    'int thdcFloor2DownX = 7450, thdcFloor2DownY = 966',
    'int thdcFloor3UpX = 4256, thdcFloor3UpY = 7120',
    'int thdcFloor3DownX = 7620, thdcFloor3DownY = 1242',
    'int thdcFloor4DownX = 690, thdcFloor4DownY = 7200',
]
for token in defaults:
    need(token in C, f'THDC default missing/wrong: {token}')
plans = [
    'return {true, 10000, 10014, 0, true}',
    'return {true, 10014, 10015, 1, false}',
    'GatePlan{true, 10015, 10016, 2, false}',
    'GatePlan{true, 10015, 10014, 3, false}',
    'GatePlan{true, 10016, 10017, 4, false}',
    'GatePlan{true, 10016, 10015, 5, false}',
    'return {true, 10017, 10016, 6, false}',
]
for token in plans:
    need(token in H, f'THDC adjacent-floor plan missing: {token}')
need(R.count('thdc:') >= 9, 'THDC topology tests missing')
thdc = re.search(r'bool HandleThdcRoute\(.*?\n    \}\n\n    bool HandleShortcutTravel', C, re.S)
need(thdc is not None, 'cannot isolate THDC FSM')
tb = thdc.group(0)
need('NextGate(s.mapID, finalTarget.mapID)' in tb and
     'plan.expectedMap != rt.shortcutExpectedMap' in tb,
     'THDC FSM does not enforce next adjacent map')
need('if (s.mapID != 10014 || !s.mapReady || s.waitingChangeMap)' in tb and
     tb.find('if (s.mapID != 10014') < tb.find('Command::ConfirmTravelSemantic'),
     'THDC confirmation must happen only after stable M10014')
need('không xác nhận ở M10000' in tb and 'THĐC ENTRY CONFIRM PASS' in tb,
     'THDC entry confirmation timing contract missing')
need('mandatoryThdc' in C and 'ShortcutKind::ThdcRoute' in C,
     'THDC route must be mandatory on M10000/THDC floor traversal')

# Remaining generic semantic/confirm paths stay available and fail closed.
for token in ['EnumerateActiveUiObjects', 'ExactMethod(row.klass, "HandleClickEvent", 0, false)',
              'SEMANTIC_SCAN CALLBACK PASS', 'CONFIRM CALLBACK ĐÃ GỌI',
              'g_travelBaselineActiveControls']:
    need(token in B, f'generic semantic/confirm resolver missing: {token}')
need(C.count('Command::ConfirmTravelSemantic') >= 2,
     'remaining portal/NPC confirmation paths unexpectedly removed')
need('kShortcutNpcUiReadyMs = 2000' in C and 'kShortcutConfirmUiReadyMs = 2000' in C,
     'semantic UI pacing changed')
need('V3.0 SHORTCUT FIRST' in C, 'initial train shortcut routing marker missing')

# Seller identities remain unchanged.
for token in [
    'Ba Nhĩ • Lâu Lan M5 • ID 328", 5, 328',
    'Xa Truyền Bình • Lâu Lan M5 • ID 387", 5, 387',
    'Dược Đại Phu • Hỏa Diệm Sơn M55 • ID 279", 55, 279',
    'A Lạp Bá Nhân • Hỏa Diệm Sơn M55 • ID 275", 55, 275',
]:
    need(token in C, f'seller preset changed: {token}')


# Dồn đồ CON→MAIN: v1.4 pass decision + simple positional macro; no reserved first row.
TC = (ROOT / 'src/trade_coordinator_logic.h').read_text(encoding='utf-8')
need('PostBackgroundClientClick' not in C and 'PostMessageW(game.window' not in C,
     'dedicated outside-window trade pre-click path must be removed')
need('CoordinatorInternalPointAction(' in C,
     'saved trade macro must still use the unified hidden InputSync action')
for token in ['NormalizeChildPreClickContract', 'firstStored', 'preClickError',
              'kTradePassSequenceStartIndex', 'dòng #1 bắt buộc', 'từ dòng 2']:
    need(token not in C and token not in TC, f'obsolete first-row contract remains: {token}')
need('if (childTradeSequence_.empty())' in C and
     'for (std::size_t i = 0; i < childTradeSequence_.size(); ++i)' in C,
     'dynamic trade sequence must validate every current row from index 0')
need('while (index > 0 && seq[index - 1].groupId == id)' in C,
     'repeat groups must be allowed to start at current first row')

# Exact v1.4 MAIN-delta rule.
need('kReceivedSlotsFinishThreshold = 8' in TC and
     'enum class PassDecision { FinishChild, RepeatSameChild };' in TC,
     'v1.4 pass threshold/decision enum missing')
need('ReceivedSlots(beforeFree, afterFree) <= kReceivedSlotsFinishThreshold' in TC,
     'v1.4 <=8 finish / >8 repeat rule missing')
for forbidden in ['WaitForSnapshot', 'InvalidSnapshot', 'IsFreshPostTradeSnapshot',
                  'IsFullTradePassDelta', 'RepeatPreparation', 'EffectiveMainSellThreshold',
                  'TradeTransferRepeatLimit']:
    need(forbidden not in TC and forbidden not in C,
         f'v3.3 snapshot/macro special residue remains: {forbidden}')

# v3.3 bag observation timing: same v1.4 before/after delta, slower stabilization only.
need('kTradeBagStableMs = 1500' in C and 'kTradeBagVerifyMaxMs = 3200' in C,
     'v3.3 1500/3200ms bag timing missing')
need('kTradeBagRepeatStableMs' not in C and 'kTradeBagFinishStableMs' not in C,
     'split snapshot timing remains')
need('if (!stableEnough && !verifyTimedOut) return true;' in C and
     'DecidePass(beforeFree, afterFree) == PassDecision::RepeatSameChild' in C,
     'controller is not using stable-or-timeout MAIN delta decision')

# Trade macro is intentionally dumb: target MAIN/CON + point + delay + configured repeat/group only.
for forbidden in ['CHUYỂN ĐỒ', 'HasChildTransferStep', 'TradeStepKindLabel',
                  'tradeSeqKind_', 'IDC_SEQ_KIND', 'MainSellThreshold',
                  'mainSellThreshold_', 'mainSellThresholdEdit_']:
    need(forbidden not in C and forbidden not in TC,
         f'v3.3 obsolete trade-kind/threshold residue remains: {forbidden}')
need('const int repeatLimit = effective->repeat;' in C,
     'macro must use configured row repeat unchanged')
need('capByMain' not in C,
     'old transfer repeat cap must stay removed')
need('TLCLICKCFG\\t2' in C and 'if (f.size()!=14)' in C,
     'TLCLICKCFG v2 CHILD format without kind is missing')

# FULL is admission-only. Same CON stays active after a >8 MAIN delta while MAIN still has >=9 slots.
need('ShouldAdmitFullChild(TradeStateReady(*child), child->snapshot.freeBagSpace' in C,
     'initial FULL admission gate missing')
repeat_start = C.find('if (DecidePass(beforeFree, afterFree) == PassDecision::RepeatSameChild) {')
repeat_end = C.find('LogAccount(main, L"GD PASS CUỐI v1.4', repeat_start)
repeat = C[repeat_start:repeat_end] if repeat_start >= 0 and repeat_end > repeat_start else ''
need(bool(repeat), 'cannot isolate v1.4 repeat branch')
need('tradeTxn_.sequenceIndex = 0;' in repeat and
     'tradeTxn_.phase = TradePhase::TargetMain;' in repeat and
     'TARGET LẠI MAIN ID' in repeat,
     'repeat must keep same CON then retarget MAIN before next full macro')
need('child.snapshot.freeBagSpace' not in repeat and 'ShouldAdmitFullChild' not in repeat,
     'active CON fullness must never be re-checked between passes')

# MAIN <9 is a normal sell handoff, not AbortTrade: active CON goes train, queued CONs remain held.
need('YieldActiveTradeForMainSell' in C and
     'CON hiện tại về train' in C and 'CON FIFO đứng chờ' in C,
     'selective MAIN-sell handoff missing')
need('tradeTxn_.phase != TradePhase::Sequence && !CanStartTradePass(main->snapshot.freeBagSpace)' in C,
     'MAIN pass-start capacity gate missing or can still interrupt a running macro')
need('tradeTxn_.phase == TradePhase::Idle && !main->tradeHeld && !TradeStateReady(*main)' in C and
     'MAIN đang AUTO SELL • giữ CON FIFO đứng chờ' in C,
     'queued CON FIFO must stay held while MAIN finishes Auto Sell')
need('DecidePass(beforeFree, afterFree) == PassDecision::RepeatSameChild &&' in C and
     '!CanStartTradePass(afterFree)' in C,
     'post-pass >=9 delta + MAIN<9 capacity handoff missing')
need('ShouldAutoSell(tradeEnabled_, a.profile.tradeRole, a.profile.enableSell,' in C and
     's.freeBagSpace)) {' in C,
     'MAIN Auto Sell must use fixed 9-slot gate')
need('if (tradeRole == 1) return !CanStartTradePass(freeBagSpace);' in TC,
     'MAIN Auto Sell must trigger below 9 slots while consolidation is enabled')

# Target MAIN before first pass and every repeat. No special first row.
rv_start = C.find('if (tradeTxn_.phase == TradePhase::Rendezvous) {')
rv_end = C.find('if (tradeTxn_.phase == TradePhase::TargetMain) {', rv_start)
rv = C[rv_start:rv_end] if rv_start >= 0 and rv_end > rv_start else ''
need(bool(rv), 'cannot isolate Rendezvous -> TargetMain')
need('CoordinatorInternalPointAction' not in rv and 'tradeTxn_.phase = TradePhase::TargetMain;' in rv,
     'Rendezvous must target MAIN only; no special row/pre-click may run')
target_start = rv_end
target_end = C.find('if (tradeTxn_.phase == TradePhase::Sequence) {', target_start)
target = C[target_start:target_end] if target_start >= 0 and target_end > target_start else ''
need(bool(target) and 'Command::SelectTargetByRoleID' in target,
     'verified TargetMain phase missing')
need('tradeTxn_.sequenceIndex = 0;' in target and 'bắt đầu macro động từ bước đầu' in target,
     'TargetMain must start saved macro from row 0')
need('tradeTxn_.sequencePass = 1;' not in target,
     'TargetMain must preserve repeat pass number')


# v4.1 Auto Dungeon contracts.
D = (ROOT / 'src/dungeon_logic.h').read_text(encoding='utf-8')
DP = (ROOT / 'src/dungeon_presets.h').read_text(encoding='utf-8')
DA = (ROOT / 'src/dungeon_app_methods.inl').read_text(encoding='utf-8')
for token in ['AUTO PHÓ BẢN', 'dungeonOwned', 'TickDungeon(', 'StartDungeonSelected', 'PauseResumeDungeonSelected', 'StopDungeonSelected']:
    need(token in C or token in DA, f'v4.0 dungeon control missing: {token}')
need('constexpr std::size_t kMaxTeamMembers = 6' in D, 'dungeon max 6 team contract missing')
need('TeamsOverlap' in D and 'đang thuộc tổ đội RUN/PAUSE khác' in DA, 'dungeon PID ownership invariant missing')
need('ScanNearbyMonsters = 25' in P and 'ClickDialogText = 26' in P, 'dungeon bridge commands missing')
need('GMonster' in B and 'GRole' in B and 'ClickDialogTextExact' in B, 'strict dungeon scanner/dialog semantic resolver missing')
need('CanonicalPresets' in DP and DP.count('Base(L"') >= 10, 'canonical dungeon presets missing')
need('L"TucCau_1"' in DP and 'L"Tiêu diệt túc cầu",103,1312,3072,200' in DP, 'canonical Túc Cầu 200 missing')
need('dungeon_logic_tests' in CM and 'dungeon_progress_logic_tests' in CM, 'dungeon/progress logic tests missing from CMake')
need('team.kills >= step.requiredKills' in DA and '3 scan' not in DA, 'dungeon fight must not pass from empty AOI scans')
need('DungeonPostSellNeeded(Account& account, bool& valid' in DA and 'không đọc được FreeBagSpace' in DA,
     'post-dungeon sell must fail closed on invalid bag proof')
need('L"Q3_ToChau",L"Dã Ngoại Trại Phỉ",95,4,674,4500,8190' in DP,
     'Q3 Tô Châu canonical gather coordinate mismatch')
need('Command::CloseBackgroundSell' in DA, 'dungeon STOP/FAIL must close active background sell')
need('participantMask' in D and 'IDC_DGE_SLOT6' in C, 'dungeon 1-6 participant mask editor missing')
need('AUTO_DUNGEON_RUNTIME_TEST_PLAN.md' in [x.name for x in ROOT.iterdir()], 'donor runtime test plan missing')


# v4.1 task-board + scanner hybrid contracts.
DPROG = (ROOT / 'src/dungeon_progress_logic.h').read_text(encoding='utf-8')
need('ReadDungeonProgress = 27' in P and 'DungeonProgressSnapshot' in P, 'task progress protocol snapshot missing')
need('GetDoingTasks' in B and 'DungeonReadTaskParameters' in B and 'TaskID' in B and 'Parameters' in B,
     'bridge does not read semantic task snapshot')
need('Command::ReadDungeonProgress' in DA and 'TASK+SCAN' in DA and 'chờ UpdateTask' in DA,
     'dungeon FSM is not using TASK + scanner hybrid proof')
need('DungeonTaskLearn_' in DA and 'SaveDungeonTaskLearn' in DA and 'RestoreDungeonTaskLearn' in DA,
     'TASK + MONSTER LEARN persistence missing')
need('WaitForServerSync' in DPROG and 'Conflict' in DPROG and 'scannerKills >= in.required' in DPROG,
     'server-first progress decision policy missing')
need('fubenAvailable = false' in DA and '200168..200174' in DA,
     'FuBen packet proof must remain reserved/fail-closed until response store is proven')
need('OCR' not in B, 'bridge task progress must not depend on OCR')

# v4.2 Xa Truyền travel-network contracts.
need('NamHai = 4' in P and 'MieuCuong = 5' in P and 'HoangLongPhu = 6' in P and
     'ThachLam = 7' in P and 'DaiLy = 8' in P, 'v4.2 travel semantics missing')
for token in ['namhai', 'mieucuong', 'hoanglongphu', 'thachlam', 'daili']:
    need(token in B, f'v4.2 exact semantic token missing: {token}')
need('kXaTruyenBinhNpcId = 387' in TN and 'kXaTruyenTinNpcId = 522' in TN,
     'v4.2 proven Xa Truyen NPC IDs missing')
need('kXaTruyenTinX = 7236' in TN and 'kXaTruyenTinY = 1908' in TN,
     'v4.2 Xa Truyen Tin coordinate mismatch')
need('SelectNpcTeleport' in TN and 'IsThachLamPortalNetworkMap' in TN,
     'v4.2 travel-network planner missing')
need('TravelNetwork = 7' in C and 'ShortcutKind::TravelNetwork' in C,
     'v4.2 controller travel-network integration missing')
need('useSharedXaTruyenBinhPosition' in C and 'sellNpcPositions_' in C,
     'v4.2 must reuse the existing Xa Truyen Binh coordinate source')
need('travel_network_logic_tests' in CM and 'travel_network_logic_tests' in W,
     'v4.2 travel-network test not wired into build')
need('!SelectNpcTeleport(kMieuCuongMap, kDaiLyMap).valid' in TNT and
     '!SelectNpcTeleport(kHoangLongPhuMap, kDaiLyMap).valid' in TNT and
     '!SelectNpcTeleport(kThachLamMap, kDaiLyMap).valid' in TNT,
     'v4.2 unresolved return NPC fail-closed tests missing')

print('AUTO Than Long da tinh nang Pro v4.2 verifier: PASS')
