from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    print(f"V9.9 SPECIAL VERIFY FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def need(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


controller = read("src/controller.cpp")
bridge = read("src/bridge.cpp")
trade = read("src/trade_coordinator_logic.h")
fixed_sell = read("src/fixed_slot_sell_logic.h")
cmake = read("CMakeLists.txt")
launcher = read("src/license_launcher.cpp")
resource = read("resources/app.rc")

need(read("VERSION.txt").strip() == "9.9 ĐẶC BIỆT", "VERSION.txt mismatch")
need("project(ThanLongItemConsolidator VERSION 9.9" in cmake, "CMake version mismatch")
need("FILEVERSION 9,9,0,0" in resource, "Windows file version mismatch")
need("9.9-special" in launcher, "license API appVersion mismatch")
need("Auto dồn đồ thần Long phiên bản PRO MAX by Thắng Nguyễn S2" in controller,
     "product title mismatch")

need(not (ROOT / "src/auto_pk_logic.h").exists(), "Auto PK source still exists")
need(not (ROOT / "src/auto_pk_logic_test.cpp").exists(), "Auto PK test still exists")
need(not (ROOT / "src/rotation_logic.h").exists(), "rotation source still exists")
need("AutoPk" not in controller and "auto_pk" not in controller, "Auto PK remains in controller")
need("rotationSpots" not in controller and "UpdateRotation" not in controller,
     "train-spot rotation remains in controller")

need("kMaxTravelingChildren = 4" in trade, "four-CON workflow cap missing")
need("ShouldAssignArrivalTicket" in trade, "arrival-only ticket rule missing")
need("PostPassAction::SellPauseSameChild" in trade,
     "post-pass MAIN capacity hold rule missing")
need("return freeBagSpace >= kTradePassMaxItems;" in trade,
     "MAIN trade pass can start below 9 free slots")
need("MainNeedsCapacitySell" in trade and
     "freeBagSpace < kTradePassMaxItems" in trade,
     "MAIN <9 capacity-sell rule missing")
need("if (MainNeedsCapacitySell(afterFree)) return PostPassAction::SellPauseSameChild;" in trade,
     "post-pass <9 must retain the same CON for sell pause")
need("HasConclusivePostPassBagSnapshot" in trade and
     "CanFinalizeUnchangedPostPassBagSnapshot" in trade,
     "bounded unchanged post-pass snapshot contract missing")
need("TRADE SNAPSHOT ZERO" in controller and "X→X sau timeout" in controller,
     "X→X timeout does not finalize as a zero-item pass")
need("snapshot MAIN FreeBagSpace không hợp lệ quá timeout" in controller,
     "invalid post-pass snapshot can still retain a transaction forever")
need("TRADE SNAPSHOT WAIT" not in controller and "KHÔNG FinishChild" not in controller,
     "old infinite X→X hold contract still remains")
need(controller.count("CoordinatorRawMacroPointAction(") >= 3 and
     "rawMacro ? 1 : 0" in controller,
     "trade raw-macro controller path missing")
need("g_shared->request.arg2 != 0" in bridge and
     "không gate theo raycast/_uiDragging" in bridge,
     "Bridge raw-macro mode missing")
need("if (!rawMacro)" in bridge and
     "InputSyncManager raycast không bắt được UI tại tọa độ đã gán" in bridge,
     "guarded UI behavior for non-trade callers was unintentionally removed")
need("std::vector<DWORD> tradeTravelPids_" in controller, "travel/admission set missing")
need("std::vector<DWORD> tradeQueuePids_" in controller, "arrived FIFO set missing")
need("CHƯA có số FIFO" in controller and "ĐÃ TỚI TỌA GD → nhận FIFO" in controller,
     "arrival FIFO behavior missing")

need("MAIN ĐỨNG IM" in controller, "stationary MAIN contract missing")
need("a.snapshotValid && (a.snapshot.validMask & ValidAutoPath) && a.snapshot.autoPathing" in controller and
     "a.bridge.Call(Command::StopPath" in controller,
     "MAIN no-AutoPath start guard missing")
need("DỒN ĐỒ: BẮT BUỘC" in controller, "mandatory consolidation UI missing")
need("Tính năng đã được cắt bỏ khỏi ver 9.9 ĐẶC BIỆT" in controller,
     "cut-feature popup missing")

# Approved MAIN click contract: one editable hidden point. With no arrived CON,
# MAIN clicks it forever regardless of bag capacity. An arrived CON takes
# priority; MAIN must have >=9 free slots before starting/resuming a trade pass.
need("Click khi không giao dịch" in controller, "single MAIN idle click label missing")
need("TickMainIdleClick" in controller, "MAIN infinite idle click runtime missing")
need("TickMainFullSellBatch" in controller, "MAIN capacity batch runtime missing")
need("tradeQueuePids_.empty() && main->runtime.sellPhase == 0" in controller,
     "idle click is not explicitly gated by arrived-CON FIFO")
need("MainNeedsCapacitySell(main->snapshot.freeBagSpace)" in controller,
     "idle coordinator does not enforce <9 capacity sell with a waiting CON")
idle_pos = controller.find("tradeQueuePids_.empty() && main->runtime.sellPhase == 0")
capacity_pos = controller.find("const bool mainNeedsCapacity", idle_pos)
need(idle_pos >= 0 and capacity_pos > idle_pos,
     "MAIN capacity check incorrectly runs before no-CON infinite idle click")
need("CON tới sẽ ưu tiên GD" in controller,
     "arrived-CON priority over idle click missing")
need("Đang đứng chơi nên click bán liên tục" in controller,
     "simple MAIN idle status missing")
need("Đang giao dịch với CON" in controller,
     "simple MAIN trading status missing")
need("MAIN <9 • đang click bán" in controller,
     "MAIN active capacity-sell status missing")

# Full-batch atomicity: once Repeat starts, no intermediate FreeBag value may
# stop the batch. Capacity is checked only in phase 2 after Repeat is complete.
need("rt.sellMacroRepeatDone >= rt.sellMacroPass" in controller,
     "configured Repeat completion boundary missing")
need("if (rt.sellPhase == 2)" in controller and "if (CanStartTradePass(free))" in controller,
     "post-batch >=9 capacity evaluation missing")
need("MAIN VẪN <9 Ô sau đủ" in controller,
     "repeat-another-full-batch path missing")
need("if (!MainBagIsFull(main.snapshot.freeBagSpace))" not in controller,
     "capacity batch can still stop early after the first newly-free slot")
need("MAIN <9 ô trước pass mới" in controller and
     "MAIN <9 ô trước khi target/click pass" in controller,
     "pre-pass <9 sell-pause guard wording missing")
need("MAIN đã có >=9 ô trống" in controller,
     "trade resume is not explicitly gated by >=9 free slots")

need("LegacyIdleClickSourceIndex" in fixed_sell,
     "legacy Step-5 coordinate migration helper missing")
need("ClampFullBatchClickCount" in fixed_sell and "kDefaultFullBatchClickCount = 90" in fixed_sell,
     "editable/default-90 full batch contract missing")
need("ClampClickDelayMs" in fixed_sell,
     "editable MAIN click delay contract missing")
need("Command::ClickInternalPoint" in controller,
     "hidden internal point click missing")
need("GIỮ NGUYÊN GD DỞ" in controller and "TradePhase::SellPause" in controller,
     "sell-pause/resume state contract missing")
need("const PostPassAction postPass = DecidePostPass(beforeFree, afterFree);" in controller,
     "controller does not apply capacity-first post-pass decision")

# Old sell state/adaptive learning must stay gone.
need("TickMainStationarySell" not in controller,
     "old Step-5 seller state machine remains")
need("EffectiveClickCount" not in controller and "EffectiveClickCount" not in fixed_sell,
     "old adaptive sell-count learning remains")
need("lastTradePassFreeBagSpace" not in controller,
     "old post-trade sell learning field remains")

need("AddLocalReport(L\"SESSION START\"" in controller, "local session report missing")
need("AddLocalReport(L\"MONEY MILESTONE\"" in controller, "local bound-gold report missing")
need("if (!telegramStats_.active) return;" in controller,
     "local report scheduler must not require Telegram enabled")

print("V9.9 SPECIAL VERIFY PASS")
