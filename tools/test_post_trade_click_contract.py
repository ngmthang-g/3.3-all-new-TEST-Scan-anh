from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASE = (ROOT / 'src/controller.cpp').read_text(encoding='utf-8-sig')

# The existing persisted keys must remain unchanged so the user's saved F8 point,
# Delay and Repeat migrate without any reconfiguration.
for token in [
    'bool postTradeClickEnabled = false;',
    'ClickPoint postTradeClick{};',
    'int postTradeClickDelayMs = 200;',
    'int postTradeClickRepeat = 1;',
    'PostTradeClickEnabled', 'PostTradeClickValid', 'PostTradeClickX', 'PostTradeClickY',
    'PostTradeClickW', 'PostTradeClickH', 'PostTradeClickDelayMs', 'PostTradeClickRepeat',
    'std::clamp(ReadIniInt(section, L"PostTradeClickRepeat", 1), 0, 999)',
]:
    assert token in BASE, token

# Build exactly the generated source chain CMake uses, then inspect the final controller.
with tempfile.TemporaryDirectory(prefix='v99_cp1_contract_') as td:
    out = Path(td)
    scripts = [
        'generate_image_scan_v4_sources.py',
        'apply_pre_close_x_patch.py',
        'apply_ui30_controller_base.py',
        'apply_ui30_controller_groups.py',
        'apply_ui30_controller_runtime.py',
        'apply_ui30_scanner.py',
        'apply_v99_automation_cp1.py',
    ]
    for script in scripts:
        path = ROOT / 'tools' / script
        assert path.exists(), f'missing generated-source patch: {script}'
        subprocess.run(
            [sys.executable, str(path), '--source-root', str(ROOT), '--output-dir', str(out)],
            check=True,
        )
    C = (out / 'controller.cpp').read_text(encoding='utf-8-sig')

# Legacy INI/master fields stay compatible, but the visible meaning changes to the
# pre-trade menu opener on the active CON only.
assert 'CLICK SAU TARGET MAIN' in C
assert 'MAIN chạy trước → đúng CON vừa giao dịch chạy sau' not in C
for token in [
    'IDC_SC_POST_TRADE_ENABLED', 'IDC_SC_POST_TRADE_DELAY',
    'IDC_SC_POST_TRADE_REPEAT', 'IDC_SC_POST_TRADE_CAPTURE',
    'BeginPostTradeClickCapture()', 'shortcutSettings_.postTradeClick = captured;',
]:
    assert token in C, token

# The executor is now a pre-trade opener on the active CON only. It must remain
# a hidden InputSync point action and must not touch business-state decisions.
fn = re.search(
    r'bool ExecuteTradeMenuOpenClickTick\(Account& child, DWORD now\) \{(.*?)\n    \}\n\n    bool ExecuteTradeSequenceTick',
    C,
    re.S,
)
assert fn, 'cannot isolate ExecuteTradeMenuOpenClickTick'
body = fn.group(1)
for token in [
    'CoordinatorInternalPointAction(',
    'child, shortcutSettings_.postTradeClick',
    '++tradeTxn_.postTradeClickRepeatDone;',
    'tradeTxn_.postTradeClickCompleted = true;',
]:
    assert token in body, token
for forbidden in [
    'Account& main', 'postTradeClickTarget == 0 ? main', '++tradeTxn_.postTradeClickTarget',
    'AbortTrade(', 'FinishTradeChild(', 'freeBagSpace', 'receivedSlots',
    'MainNeedsCapacitySell', 'mapID', 'npcID', 'TradeAccountAtRendezvous',
]:
    assert forbidden not in body, forbidden

# The old post-sequence execution boundary must be gone completely.
seq = re.search(
    r'bool ExecuteTradeSequenceTick\(Account& main, Account& child, DWORD now\) \{(.*?)\n    \}\n\n    bool',
    C,
    re.S,
)
assert seq, 'cannot isolate ExecuteTradeSequenceTick'
seq_body = seq.group(1)
assert 'ExecutePostTradeClickTick(main, child, now)' not in seq_body
assert 'ExecuteTradeMenuOpenClickTick(' not in seq_body

# Exact required order in TargetMain:
# TARGET MAIN PASS -> configured hidden click on CON -> semantic Giao dịch -> existing macro.
target_anchor = 'if (tradeTxn_.phase == TradePhase::TargetMain) {'
start = C.find(target_anchor)
assert start >= 0, 'TargetMain block missing'
end = C.find('if (tradeTxn_.phase == TradePhase::Sequence) {', start)
assert end > start, 'Sequence block missing after TargetMain'
target = C[start:end]
pos_open = target.find('ExecuteTradeMenuOpenClickTick(*activeChild, now)')
pos_trade = target.find('Command::ClickTravelSemantic, static_cast<int>(TravelSemantic::Trade)')
pos_macro = target.find('tradeTxn_.phase = TradePhase::Sequence;')
assert 0 <= pos_open < pos_trade < pos_macro, 'required TargetMain -> click -> Giao dịch -> macro ordering broken'
assert 'GIAO DỊCH CALLBACK WAIT' in target
assert 'tradeTxn_.postTradeClickCompleted = false;' in target
assert 'tradeTxn_.postTradeClickRepeatDone = 0;' in target

# No new trade phase is allowed for this bounded change.
phase = re.search(r'enum class TradePhase[^;]+;', C, re.S)
assert phase, 'TradePhase enum missing'
for forbidden in ['TradeMenu', 'OpenMenu', 'SelectTrade', 'PostTradeClick']:
    assert forbidden not in phase.group(0), forbidden

# TÙY CHỈNH and SCAN V4 belong to Developer-only controls. SCAN V4 has its own
# per-process masked password gate and the command must route through that gate.
assert 'developerUnlockedControls_ = {testOpenBagButton_, shortcutSettingsButton_, imageScanButton_, logCaption_, log_};' in C
assert 'HWND imageScanButton_ = nullptr;' in C
assert 'bool scanV4Unlocked_ = false;' in C
assert 'RequestImageScanTest();' in C
assert 'DialogBoxParamW' in C
assert 'L"961912"' in C
resource = (ROOT / 'resources' / 'app.rc').read_text(encoding='utf-8-sig')
for token in ['9100 DIALOGEX', 'EDITTEXT 9101', 'ES_PASSWORD']:
    assert token in resource, token

# Full master export/import still carries the legacy storage key so existing files work.
for token in [
    'TLMASTERCFG\\t3', 'POST_TRADE_CLICK',
    'masterVersion!=1&&masterVersion!=2&&masterVersion!=3',
    'shortcutSettings_.postTradeClick=incoming.postTradeClick;',
    'shortcutSettings_.postTradeClickDelayMs=incoming.postTradeClickDelayMs;',
    'shortcutSettings_.postTradeClickRepeat=incoming.postTradeClickRepeat;',
]:
    assert token in C, token

# Scope guard: CP1 must not pull in later Gather/PT/restart work.
for forbidden in ['AUTO RECOVERY LOCK', 'TỰ KHÔI PHỤC KHI BỊ KẸT', 'WatchdogRestart', 'AutoStopStart', 'TẬP TRUNG TẤT CẢ']:
    assert forbidden not in C, forbidden

print('CP1 trade opener + Developer move + V4 password contract: PASS')
