from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
CTRL = ROOT / "src" / "controller.cpp"
TEST = ROOT / "src" / "route_logic_test.cpp"
VERSION = ROOT / "VERSION.txt"

c = CTRL.read_text(encoding="utf-8")
t = TEST.read_text(encoding="utf-8")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly 1 match, got {count}")
    return text.replace(old, new, 1)


def replace_block(text: str, pattern: str, replacement: str, label: str) -> str:
    out, count = re.subn(pattern, replacement, text, count=1, flags=re.S)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly 1 block, got {count}")
    return out


# Idempotent success path for later manual workflow_dispatch runs.
if VERSION.read_text(encoding="utf-8").strip() == "1.6" and "PORTAL SPECIAL" in c and "kPreciseWorldTolerance = 20" in c:
    print("v1.6 patch already applied; verification-only run")
    sys.exit(0)

# Visible identity + diagnostics support.
c = replace_once(c, "#include <cwchar>\n", "#include <cwchar>\n#include <cmath>\n", "add <cmath>")
c = replace_once(
    c,
    'constexpr wchar_t kTitle[] = L"Thần Long Item Consolidator v1.5.4 • MAP CFG + PRE-CLICK GD + SELL NPC + AUTO PK + LỌC ĐỒ + TELEGRAM";',
    'constexpr wchar_t kTitle[] = L"AUTO Thần Long đa tính năng Pro v1.6";',
    "main title",
)
c = replace_once(
    c,
    "constexpr DWORD kTradeTargetRetryMs = 500;\n",
    "constexpr DWORD kTradeTargetRetryMs = 500;\n"
    "constexpr int kPreciseWorldTolerance = 20; // v1.6: GD/NPC gần như tuyệt đối; train vẫn dùng profile tolerance.\n"
    "constexpr DWORD kShortcutPathAcceptMs = 1500; // Bridge PASS nhưng phải thấy AutoPath/movement thực.\n"
    "constexpr int kShortcutPathMaxDispatch = 5;\n",
    "precision constants",
)
c = replace_once(
    c,
    'L"Thần Long Item Consolidator • v1.5.4 • MAP CFG + PRE-CLICK GD + SELL NPC + Auto PK + Lọc đồ + Telegram"',
    'L"AUTO Thần Long đa tính năng Pro • v1.6"',
    "about title",
)
c = replace_once(
    c,
    'L"Thần Long v1.5.4 • TÙY CHỈNH TỌA ĐỘ"',
    'L"AUTO Thần Long đa tính năng Pro v1.6 • TÙY CHỈNH TỌA ĐỘ"',
    "shortcut window title",
)
c = replace_once(
    c,
    'L"Thần Long v1.5.1 • LỌC ĐỒ TAY NẢI"',
    'L"AUTO Thần Long đa tính năng Pro v1.6 • LỌC ĐỒ TAY NẢI"',
    "inventory window title",
)

# Trade rendezvous: migrate every old/default tolerance to the approved near-exact 20.
c = replace_once(
    c,
    'tradeRendezvousTolerance_ = std::clamp(ReadIniInt(L"Global", L"TradeRendezvousTolerance", 120), 20, 500);',
    'tradeRendezvousTolerance_ = kPreciseWorldTolerance; // v1.6 migration: do not reuse legacy 120 for GD.',
    "trade tolerance load migration",
)
c = replace_once(c, "int tradeRendezvousTolerance_ = 120;", "int tradeRendezvousTolerance_ = kPreciseWorldTolerance;", "trade tolerance member")

# Raw-coordinate capture logs: make runtime A/B proof explicit without changing coordinate domain.
c = replace_once(
    c,
    'LogAccount(*a, L"Đã lưu/cập nhật bãi CHUNG: " + name + L" • M" + std::to_wstring(s.mapID) + L" • " +\n                       std::to_wstring(s.x) + L"," + std::to_wstring(s.y));',
    'LogAccount(*a, L"COORD CAPTURE RAW TRAIN • đã lưu/cập nhật bãi CHUNG: " + name + L" • M" + std::to_wstring(s.mapID) + L" • " +\n                       std::to_wstring(s.x) + L"," + std::to_wstring(s.y));',
    "train capture diagnostic",
)
c = replace_once(
    c,
    'LogAccount(*source, L"Đã GET TỌA GD = M" + std::to_wstring(tradeRendezvous_.mapID) + L" • " +\n                            std::to_wstring(tradeRendezvous_.x) + L"," + std::to_wstring(tradeRendezvous_.y));',
    'LogAccount(*source, L"COORD CAPTURE RAW GD • M" + std::to_wstring(tradeRendezvous_.mapID) + L" • " +\n                            std::to_wstring(tradeRendezvous_.x) + L"," + std::to_wstring(tradeRendezvous_.y) +\n                            L" • TOL=" + std::to_wstring(tradeRendezvousTolerance_));',
    "trade capture diagnostic",
)
c = replace_once(
    c,
    'LogAccount(*a, std::wstring(L"ĐÃ GÁN TỌA • ") + labels[index] + L" • M" + std::to_wstring(snap.mapID) + L" • " +\n                       std::to_wstring(snap.x) + L"," + std::to_wstring(snap.y));',
    'LogAccount(*a, std::wstring(L"COORD CAPTURE RAW SHORTCUT • ") + labels[index] + L" • M" + std::to_wstring(snap.mapID) + L" • " +\n                       std::to_wstring(snap.x) + L"," + std::to_wstring(snap.y));',
    "shortcut capture diagnostic",
)

# SendDecision must never claim AutoPath when Bridge dispatch failed. Also log current/target/delta/distance.
send_match = re.search(
    r"    bool SendDecision\(Account& a, Action action, const TargetProfile& t, const wchar_t\* context\) \{.*?\n    \}\n\n    bool CompleteToolOwnedRoute",
    c,
    flags=re.S,
)
if not send_match:
    raise RuntimeError("SendDecision block not found")
send = send_match.group(0)
send = send.replace(
    "bool SendDecision(Account& a, Action action, const TargetProfile& t, const wchar_t* context) {",
    "bool SendDecision(Account& a, Action action, const TargetProfile& t, const wchar_t* context, int diagnosticTolerance = 0) {",
    1,
)
send = send.replace('                rt.status = L"Đang lên ngựa • " + where;', '                if (ok) rt.status = L"Đang lên ngựa • " + where;', 1)
send = send.replace('                rt.status = L"Tới " + where + L" • xuống ngựa";', '                if (ok) rt.status = L"Tới " + where + L" • xuống ngựa";', 1)
send = send.replace('                rt.status = L"Đang AutoPath tới " + where;', '                if (ok) rt.status = L"Đang AutoPath tới " + where;', 1)
send = send.replace('                rt.status = L"Tới " + where + L" • StopPath";', '                if (ok) rt.status = L"Tới " + where + L" • StopPath";', 1)
send = send.replace(
    "        rt.lastActionTick = now;\n",
    "        if (ok) {\n"
    "            rt.lastActionTick = now;\n"
    "        } else if (action == Action::StartPath) {\n"
    "            // v1.6: a transient Bridge rejection must not create a fake 5-second AutoPath success window.\n"
    "            rt.lastActionTick = now > 3500 ? now - 3500 : 0;\n"
    "        } else {\n"
    "            rt.lastActionTick = now;\n"
    "        }\n"
    "        if (ok && action == Action::StartPath) {\n"
    "            const long long dx = static_cast<long long>(a.snapshot.x) - t.x;\n"
    "            const long long dy = static_cast<long long>(a.snapshot.y) - t.y;\n"
    "            const long long d2 = dx * dx + dy * dy;\n"
    "            const long long distance = static_cast<long long>(std::llround(std::sqrt(static_cast<long double>(d2))));\n"
    "            const std::wstring tolText = diagnosticTolerance < 0\n"
    "                ? L\"PORTAL_SPECIAL\"\n"
    "                : std::to_wstring(diagnosticTolerance > 0 ? diagnosticTolerance : a.profile.tolerance);\n"
    "            LogAccount(a, L\"COORD STARTPATH PASS • CURRENT M\" + std::to_wstring(a.snapshot.mapID) +\n"
    "                          L\"/\" + std::to_wstring(a.snapshot.x) + L\",\" + std::to_wstring(a.snapshot.y) +\n"
    "                          L\" • TARGET M\" + std::to_wstring(t.mapID) + L\"/\" + std::to_wstring(t.x) + L\",\" + std::to_wstring(t.y) +\n"
    "                          L\" • DX=\" + std::to_wstring(dx) + L\" DY=\" + std::to_wstring(dy) +\n"
    "                          L\" DISTANCE=\" + std::to_wstring(distance) + L\" TOL=\" + tolText +\n"
    "                          L\" • BRIDGE=\" + std::wstring(r.detail));\n"
    "        }\n",
    1,
)
send = send.replace(
    '        if (!ok) LogAccount(a, L"Route action fail-closed: " + error);',
    '        if (!ok) {\n'
    '            rt.status = L"ROUTE ACTION FAIL • " + where + L" • " + error;\n'
    '            LogAccount(a, L"Route action fail-closed: " + error);\n'
    '        }',
    1,
)
c = c[:send_match.start()] + send + c[send_match.end():]

# Normal robust travel keeps train tolerance; pass it only for diagnostics when StartPath really dispatches.
robust_match = re.search(
    r"    bool HandleRobustTravelDirect\(.*?\n    \}\n\n    void ResetShortcutRoute",
    c,
    flags=re.S,
)
if not robust_match:
    raise RuntimeError("HandleRobustTravelDirect block not found")
robust = robust_match.group(0)
robust = robust.replace(
    "SendDecision(a, Action::StartPath, targetProfile, context)",
    "SendDecision(a, Action::StartPath, targetProfile, context, travelTolerance)",
)
c = c[:robust_match.start()] + robust + c[robust_match.end():]

# Shortcut NPC legs are same-map waypoint travel in the affected cases. Dispatch StartPath immediately
# after Travel Guard instead of waiting through the generic mount recovery state machine.
shortcut_leg = r'''    bool ShortcutTravelLeg(Account& a, DWORD now, const TargetProfile& leg, const wchar_t* label, bool& arrived) {
        arrived = false;
        if (!leg.valid) { FailShortcutRoute(a, std::wstring(L"chưa gán tọa: ") + label + L" • mở TÙY CHỈNH và bấm LẤY TỌA"); return true; }
        RuntimeState& rt = a.runtime;
        const Snapshot& s = a.snapshot;
        if (!a.snapshotValid || !s.mapReady || s.waitingChangeMap ||
            (s.validMask & (ValidMap | ValidPosition | ValidAutoPath)) != (ValidMap | ValidPosition | ValidAutoPath)) {
            rt.status = std::wstring(L"ĐƯỜNG TẮT • chờ state ổn định trước StartPath tới ") + label;
            return true;
        }

        // If the waypoint is not on the current map, preserve the existing robust cross-map machinery.
        if (s.mapID != leg.mapID) {
            return HandleRobustTravelDirect(a, now, leg, label, arrived, kPreciseWorldTolerance);
        }

        State logic{};
        logic.valid = true; logic.mapReady = true; logic.waitingMap = false;
        logic.mapID = s.mapID; logic.x = s.x; logic.y = s.y;
        logic.riding = s.riding != 0; logic.autoPathing = s.autoPathing != 0;
        Target precise{leg.mapID, leg.x, leg.y, kPreciseWorldTolerance};
        if (AtTarget(logic, precise)) {
            if (s.autoPathing) {
                (void)SendDecision(a, Action::StopPath, leg, label, kPreciseWorldTolerance);
                return true;
            }
            if (s.riding) {
                (void)SendDecision(a, Action::Dismount, leg, label, kPreciseWorldTolerance);
                return true;
            }
            ResetRobustTravel(rt);
            ResetTravelFightGuard(rt);
            arrived = true;
            rt.shortcutAttempts = 0;
            rt.shortcutTick = 0;
            return true;
        }

        if (s.autoPathing) {
            rt.status = std::wstring(L"ĐƯỜNG TẮT • AutoPath THỰC đang ON tới ") + label +
                        L" • target=" + std::to_wstring(leg.x) + L"," + std::to_wstring(leg.y);
            return true;
        }

        if (rt.shortcutAttempts >= kShortcutPathMaxDispatch && rt.shortcutTick != 0 &&
            Elapsed(now, rt.shortcutTick, kShortcutPathAcceptMs)) {
            FailShortcutRoute(a, std::wstring(L"StartPath Bridge đã PASS ") + std::to_wstring(rt.shortcutAttempts) +
                                 L" lần nhưng AutoPath không ON/không nhận waypoint: " + label);
            return true;
        }
        if (rt.shortcutTick != 0 && !Elapsed(now, rt.shortcutTick, kShortcutPathAcceptMs)) {
            rt.status = std::wstring(L"ĐƯỜNG TẮT • STARTPATH PASS • chờ verify AutoPath thực tới ") + label;
            return true;
        }
        if (rt.shortcutTick != 0 && rt.lastAction == Action::StartPath) rt.lastActionTick = 0;
        if (!EnsureAutoFightOffForTravel(a, now, label)) return true;
        if (SendDecision(a, Action::StartPath, leg, label, kPreciseWorldTolerance)) {
            ++rt.shortcutAttempts;
            rt.shortcutTick = now;
            rt.lastObservedX = s.x;
            rt.lastObservedY = s.y;
            rt.lastMovementTick = now;
            rt.status = std::wstring(L"ĐƯỜNG TẮT • STARTPATH PASS thực gửi tới ") + label +
                        L" • lần " + std::to_wstring(rt.shortcutAttempts) + L"/" + std::to_wstring(kShortcutPathMaxDispatch);
        }
        return true;
    }

    bool HandleShortcutNpcRoute'''
c = replace_block(
    c,
    r"    bool ShortcutTravelLeg\(.*?\n    \}\n\n    bool HandleShortcutNpcRoute",
    shortcut_leg,
    "shortcut direct waypoint travel",
)

# Interserver portal is a special state machine: never StopPath merely for entering tolerance 120.
# It starts the saved M10000 gate waypoint immediately, accepts either precise arrival (~20) OR
# an actually-running AutoPath that has stalled for 3s, then tries semantic ConfirmMap.
interserver = r'''    bool HandleShortcutInterserverGate(Account& a, DWORD now, const TargetProfile& finalTarget,
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
                (s.validMask & (ValidMap | ValidPosition | ValidAutoPath)) != (ValidMap | ValidPosition | ValidAutoPath)) {
                rt.status = L"LIÊN-SERVER • chờ state M/X/Y/AutoPath ổn định";
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
            const bool stalledThreeSeconds = rt.shortcutAttempts > 0 && s.autoPathing &&
                rt.lastMovementTick != 0 && Elapsed(now, rt.lastMovementTick, kLauLanGateStallMs);

            if (preciseAtGate || stalledThreeSeconds) {
                rt.shortcutPhase = 2;
                rt.shortcutTick = now;
                rt.shortcutAttempts = 0;
                rt.status = preciseAtGate
                    ? L"LIÊN-SERVER • PORTAL SPECIAL tới sát tọa cổng • KHÔNG StopPath theo radius 120 • chờ popup"
                    : L"LIÊN-SERVER • PORTAL SPECIAL AutoPath THỰC đã đứng ~3s • chờ popup";
                LogAccount(a, preciseAtGate
                    ? L"PORTAL SPECIAL: vị trí đã vào tolerance 20; giữ nguyên path, không StopPath sớm, chuyển sang ConfirmMap."
                    : L"PORTAL SPECIAL: AutoPath đã ON và vị trí không đổi ~3s; chuyển sang ConfirmMap dù không dùng AtTarget(120)." );
                return true;
            }

            if (s.autoPathing) {
                rt.status = L"LIÊN-SERVER • PORTAL SPECIAL AutoPath THỰC đang chạy tới cổng " +
                            std::to_wstring(gate.x) + L"," + std::to_wstring(gate.y);
                return true;
            }

            if (rt.shortcutAttempts >= kShortcutPathMaxDispatch && rt.shortcutTick != 0 &&
                Elapsed(now, rt.shortcutTick, kShortcutPathAcceptMs)) {
                FailShortcutRoute(a, L"PORTAL SPECIAL: Bridge báo StartPath PASS nhiều lần nhưng AutoPath không ON; xem log COORD STARTPATH PASS");
                return true;
            }
            if (rt.shortcutTick != 0 && !Elapsed(now, rt.shortcutTick, kShortcutPathAcceptMs)) {
                rt.status = L"LIÊN-SERVER • STARTPATH PASS • chờ verify AutoPath THỰC tới cổng";
                return true;
            }
            if (rt.shortcutTick != 0 && rt.lastAction == Action::StartPath) rt.lastActionTick = 0;
            if (!EnsureAutoFightOffForTravel(a, now, L"cổng liên-server M10000")) return true;
            if (SendDecision(a, Action::StartPath, gate, L"cổng liên-server M10000", -1)) {
                ++rt.shortcutAttempts;
                rt.shortcutTick = now;
                rt.lastObservedX = s.x;
                rt.lastObservedY = s.y;
                rt.lastMovementTick = now;
                rt.status = L"LIÊN-SERVER • PORTAL SPECIAL STARTPATH PASS • lần " +
                            std::to_wstring(rt.shortcutAttempts) + L"/" + std::to_wstring(kShortcutPathMaxDispatch) +
                            L" • chờ AutoPath THỰC ON";
            }
            return true;
        }

        if (rt.shortcutPhase == 2) {
            if (!Elapsed(now, rt.shortcutTick, 500)) return true;
            if (ShortcutBridgeCall(a, Command::ConfirmMap, 0, L"Xác nhận popup cổng liên-server", now)) {
                rt.shortcutPhase = 3;
                rt.shortcutTick = now;
                rt.shortcutAttempts = 0;
                rt.shortcutExpectedMap = finalTarget.mapID;
                return true;
            }
            ++rt.shortcutAttempts;
            rt.shortcutTick = now;
            if (rt.shortcutAttempts >= 12) {
                FailShortcutRoute(a, L"PORTAL SPECIAL đã tới/stall nhưng ~6s vẫn không thấy đúng popup Xác nhận");
            } else {
                rt.status = L"LIÊN-SERVER • PORTAL SPECIAL đang chờ popup Xác nhận " +
                            std::to_wstring(rt.shortcutAttempts) + L"/12";
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

    bool HandleShortcutTravel'''
c = replace_block(
    c,
    r"    bool HandleShortcutInterserverGate\(.*?\n    \}\n\n    bool HandleShortcutTravel",
    interserver,
    "interserver portal state machine",
)

# Seller NPC travel is precise; normal train/rotation keeps the user profile tolerance.
c = replace_once(
    c,
    '(void)HandleRobustTravel(a, now, npcTarget, L"NPC bán", arrived);',
    '(void)HandleRobustTravel(a, now, npcTarget, L"NPC bán", arrived, kPreciseWorldTolerance);',
    "seller NPC tolerance",
)

# v1.6 route test explicitly proves that train tolerance 120 and precision tolerance 20 are different contracts.
insert_test = '''\n    State preciseState{true, true, false, 50, 1050, 2000, false, false};\n    Target trainTolerance{50, 1000, 2000, 120};\n    Target preciseTolerance{50, 1000, 2000, 20};\n    Check(AtTarget(preciseState, trainTolerance), "tolerance-split:train-120-allows-near");\n    Check(!AtTarget(preciseState, preciseTolerance), "tolerance-split:precise-20-rejects-50-away");\n'''
t = replace_once(
    t,
    '    std::printf("RESULT %d/15 PASS\\n", 15 - g_fail);',
    insert_test + '    std::printf("RESULT %d/17 PASS\\n", 17 - g_fail);',
    "route tolerance tests",
)

# Final invariants before writing.
for required in [
    'AUTO Thần Long đa tính năng Pro v1.6',
    'kPreciseWorldTolerance = 20',
    'COORD STARTPATH PASS',
    'PORTAL SPECIAL',
    'STARTPATH PASS thực gửi tới',
    'HandleRobustTravel(a, now, npcTarget, L"NPC bán", arrived, kPreciseWorldTolerance)',
    'tradeRendezvousTolerance_ = kPreciseWorldTolerance',
]:
    if required not in c:
        raise RuntimeError(f"required v1.6 marker missing after patch: {required}")
if 'DisplayCoordToAutoPath' in c:
    raise RuntimeError('forbidden coordinate scaling token reintroduced')

CTRL.write_text(c, encoding="utf-8")
TEST.write_text(t, encoding="utf-8")
VERSION.write_text("1.6\n", encoding="utf-8")
print("v1.6 patch applied: precise GD/NPC, direct shortcut waypoint dispatch, M10000 portal no early StopPath, diagnostics enabled")
