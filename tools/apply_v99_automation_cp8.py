from pathlib import Path
import argparse

ap = argparse.ArgumentParser()
ap.add_argument('--source-root', required=True)
ap.add_argument('--output-dir', required=True)
a = ap.parse_args()
out = Path(a.output_dir)
p = out / 'controller.cpp'
raw = p.read_bytes()
text = raw.decode('utf-8-sig')
nl = '\r\n' if '\r\n' in text else '\n'
t = text.replace('\r\n', '\n').replace('\r', '\n')


def once(old: str, new: str, label: str) -> None:
    global t
    count = t.count(old)
    if count != 1:
        raise SystemExit(f'{label}: expected one marker, found {count}')
    t = t.replace(old, new, 1)


# ---------------------------------------------------------------------------
# CP8-A: once TradePhase::Sequence begins, the saved trade macro is atomic.
# Gameplay state/position/queue snapshots must not abort or release the active
# CON between macro rows. Only technical loss of the active process/window is
# allowed to abort; click execution errors remain owned by the macro runner.
# ---------------------------------------------------------------------------
once(
'''        main->tradeHeld = true;\n\n        if (!tradeRendezvous_.valid) {\n''',
'''        main->tradeHeld = true;\n\n        // CP8 ATOMIC TRADE SEQUENCE FAST PATH\n        // Once Sequence starts, it is a pure saved click macro. Do not run\n        // rendezvous/AutoPath/queue/gameplay guards between macro rows.\n        if (tradeTxn_.phase == TradePhase::Sequence) {\n            Account* sequenceMain = AccountByPid(tradeTxn_.mainPid);\n            Account* sequenceChild = AccountByPid(tradeTxn_.childPid);\n            const bool technicalReady = sequenceMain && sequenceChild &&\n                sequenceMain->runtime.running && sequenceChild->runtime.running &&\n                IsWindow(sequenceMain->game.window) && IsWindow(sequenceChild->game.window);\n            if (!technicalReady) {\n                AbortTrade(L"mất MAIN/CON kỹ thuật trong chuỗi giao dịch", now);\n                return;\n            }\n            sequenceMain->tradeHeld = true;\n            sequenceChild->tradeHeld = true;\n            (void)ExecuteTradeSequenceTick(*sequenceMain, *sequenceChild, now);\n            return;\n        }\n\n        if (!tradeRendezvous_.valid) {\n''',
    'atomic sequence fast path',
)

once(
'''        if (tradeTxn_.phase == TradePhase::Sequence) {\n            if (!activeMain || !activeChild || !TradeQueueContains(activeChild->game.pid) ||\n                !TradePairReadyForPreparation(*activeMain, *activeChild)) {\n                AbortTrade(L"mất acc/state trong chuỗi giao dịch", now);\n                return;\n            }\n            if (activeMain->snapshot.autoPathing || activeChild->snapshot.autoPathing ||\n                !TradeAccountAtRendezvous(*activeMain) ||\n                !TradeAccountAtRendezvous(*activeChild)) {\n                AbortTrade(L"MAIN/CON rời TỌA GD giữa chuỗi", now);\n                return;\n            }\n            // Atomicity: even if MAIN drops below 9 free slots here, finish the current saved pass.\n            // Capacity batch clicking is allowed only after the pass snapshot has been recorded.\n            (void)ExecuteTradeSequenceTick(*activeMain, *activeChild, now);\n            return;\n        }\n\n''',
'''        // CP8: Sequence is handled by the atomic fast path above before any gameplay guard.\n\n''',
    'remove old sequence gameplay guards',
)

# ---------------------------------------------------------------------------
# CP8-B: Developer LOG clear button. It only clears the visible EDIT contents.
# No runtime/trade/account state is reset.
# ---------------------------------------------------------------------------
once(
'''constexpr int IDC_PB_INVITE_RETRY = 236;\n''',
'''constexpr int IDC_PB_INVITE_RETRY = 236;\nconstexpr int IDC_CLEAR_LOG = 237;\n''',
    'clear log control id',
)

once(
'''        logCaption_ = Make(L"STATIC", L"LOG / BỘ ĐIỀU PHỐI", 0, 18, 320, 190, 20, 0); addFont(logCaption_);\n        log_ = Make(L"EDIT", L"", WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, 18, 344, 1005, 570, IDC_LOG); addFont(log_);\n        if (logCaption_) ShowWindow(logCaption_, SW_HIDE);\n        if (log_) ShowWindow(log_, SW_HIDE); // Developer-only display; logging stays active while hidden.\n''',
'''        logCaption_ = Make(L"STATIC", L"LOG / BỘ ĐIỀU PHỐI", 0, 18, 320, 190, 20, 0); addFont(logCaption_);\n        clearLogButton_ = Make(L"BUTTON", L"XÓA LOG", BS_PUSHBUTTON, 212, 316, 105, 25, IDC_CLEAR_LOG); addFont(clearLogButton_);\n        log_ = Make(L"EDIT", L"", WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, 18, 344, 1005, 570, IDC_LOG); addFont(log_);\n        if (logCaption_) ShowWindow(logCaption_, SW_HIDE);\n        if (clearLogButton_) ShowWindow(clearLogButton_, SW_HIDE);\n        if (log_) ShowWindow(log_, SW_HIDE); // Developer-only display; logging stays active while hidden.\n''',
    'add clear log button beside log caption',
)

once(
'''        developerUnlockedControls_ = {testOpenBagButton_, shortcutSettingsButton_, logCaption_, log_};\n''',
'''        developerUnlockedControls_ = {testOpenBagButton_, shortcutSettingsButton_, logCaption_, clearLogButton_, log_};\n''',
    'developer clear log visibility',
)

once(
'''                    case IDC_DEVELOPER_PASSWORD:\n                        if (HIWORD(wp) == EN_CHANGE) TryUnlockDeveloper();\n                        break;\n                    case IDC_TG_SHOW_TOKEN:\n''',
'''                    case IDC_DEVELOPER_PASSWORD:\n                        if (HIWORD(wp) == EN_CHANGE) TryUnlockDeveloper();\n                        break;\n                    case IDC_CLEAR_LOG:\n                        if (HIWORD(wp) == BN_CLICKED && log_) SetWindowTextW(log_, L"");\n                        break;\n                    case IDC_TG_SHOW_TOKEN:\n''',
    'clear log command',
)

once(
'''    HWND logCaption_ = nullptr;\n''',
'''    HWND logCaption_ = nullptr;\n    HWND clearLogButton_ = nullptr;\n''',
    'clear log member',
)

required = [
    '// CP8 ATOMIC TRADE SEQUENCE FAST PATH',
    'const bool technicalReady = sequenceMain && sequenceChild &&',
    'sequenceMain->tradeHeld = true;',
    'sequenceChild->tradeHeld = true;',
    'ExecuteTradeSequenceTick(*sequenceMain, *sequenceChild, now);',
    'constexpr int IDC_CLEAR_LOG = 237;',
    'clearLogButton_ = Make(L"BUTTON", L"XÓA LOG"',
    'case IDC_CLEAR_LOG:',
    'SetWindowTextW(log_, L"");',
]
for token in required:
    if token not in t:
        raise SystemExit(f'CP8 final assertion missing: {token}')

if 'AbortTrade(L"mất acc/state trong chuỗi giao dịch", now);' in t:
    raise SystemExit('CP8 final assertion: old sequence state abort remains')
if 'AbortTrade(L"MAIN/CON rời TỌA GD giữa chuỗi", now);' in t:
    raise SystemExit('CP8 final assertion: old sequence rendezvous abort remains')
coord_start = t.index('    void TickTradeCoordinator(DWORD now) {')
coord_end = t.index('    void TickAccount(Account& a)', coord_start) if '    void TickAccount(Account& a)' in t[coord_start:] else t.index('    void OnTimer()', coord_start)
coord = t[coord_start:coord_end]
if coord.count('if (tradeTxn_.phase == TradePhase::Sequence) {') != 1:
    raise SystemExit('CP8 final assertion: coordinator must have exactly one atomic Sequence path')

p.write_bytes(t.replace('\n', nl).encode('utf-8'))
print('apply_v99_automation_cp8.py: PASS')
