from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]

with tempfile.TemporaryDirectory(prefix='v99_cp8_atomic_trade_') as td:
    out = Path(td)
    scripts = [
        'generate_image_scan_v4_sources.py',
        'apply_pre_close_x_patch.py',
        'apply_ui30_controller_base.py',
        'apply_ui30_controller_groups.py',
        'apply_ui30_controller_runtime.py',
        'apply_ui30_scanner.py',
        'apply_v99_automation_cp1.py',
        'apply_v99_automation_cp2.py',
        'apply_v99_automation_cp3.py',
        'apply_v99_automation_cp4.py',
        'apply_v99_automation_cp5.py',
        'apply_v99_automation_cp6.py',
        'apply_v99_automation_cp7.py',
        'apply_v99_automation_cp8.py',
    ]
    for script in scripts:
        path = ROOT / 'tools' / script
        assert path.exists(), f'missing generated-source patch: {script}'
        subprocess.run(
            [sys.executable, str(path), '--source-root', str(ROOT), '--output-dir', str(out)],
            check=True,
        )
    C = (out / 'controller.cpp').read_text(encoding='utf-8-sig')

# Once Sequence begins it is a pure saved click macro. Gameplay snapshots/queue/position
# must not be allowed to abort and release the CON between macro rows.
seq_start = C.index('        // CP8 ATOMIC TRADE SEQUENCE FAST PATH')
seq_end = C.index('        if (!tradeRendezvous_.valid) {', seq_start)
seq_block = C[seq_start:seq_end]
assert 'TradeQueueContains(' not in seq_block
assert 'TradePairReadyForPreparation(' not in seq_block
assert 'snapshot.autoPathing' not in seq_block
assert 'TradeAccountAtRendezvous(' not in seq_block
assert 'mất acc/state trong chuỗi giao dịch' not in seq_block
assert 'MAIN/CON rời TỌA GD giữa chuỗi' not in seq_block
assert 'ExecuteTradeSequenceTick(*sequenceMain, *sequenceChild, now);' in seq_block
assert 'sequenceMain->tradeHeld = true;' in seq_block
assert 'sequenceChild->tradeHeld = true;' in seq_block

# There must be no second Sequence gameplay gate later in TickTradeCoordinator.
coord_start = C.index('    void TickTradeCoordinator(DWORD now) {')
coord_end = C.index('    void TickAccount(Account& a)', coord_start) if '    void TickAccount(Account& a)' in C[coord_start:] else C.index('    void OnTimer()', coord_start)
coord = C[coord_start:coord_end]
assert coord.count('if (tradeTxn_.phase == TradePhase::Sequence) {') == 1

# The macro runner still owns click execution failures; no unrelated gameplay guard
# should be reintroduced inside ExecuteTradeSequenceTick itself.
fn_start = C.index('    bool ExecuteTradeSequenceTick(Account& main, Account& child, DWORD now) {')
fn_end = C.index('    bool KeepMainStationary(', fn_start)
fn = C[fn_start:fn_end]
assert 'TradeAccountAtRendezvous(' not in fn
assert 'snapshot.autoPathing' not in fn
assert 'TradeQueueContains(' not in fn

# Developer log gets one dedicated clear button near the log. It only clears the edit
# control and must not reset trade/runtime state.
assert 'constexpr int IDC_CLEAR_LOG = 237;' in C
assert 'L"XÓA LOG"' in C
assert 'clearLogButton_ = Make(L"BUTTON", L"XÓA LOG"' in C
assert 'developerUnlockedControls_ = {testOpenBagButton_, shortcutSettingsButton_, logCaption_, clearLogButton_, log_};' in C
assert 'case IDC_CLEAR_LOG:' in C
assert 'SetWindowTextW(log_, L"");' in C
clear_case = C[C.index('                    case IDC_CLEAR_LOG:'):C.index('                    case IDC_TG_SHOW_TOKEN:', C.index('                    case IDC_CLEAR_LOG:'))]
assert 'ResetTradeTxn' not in clear_case
assert 'StopAccount' not in clear_case
assert 'ReleaseTradeHolds' not in clear_case

# CP8 is wired after CP7 and the CI contract is mandatory.
cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8-sig')
pos7 = cmake.find('apply_v99_automation_cp7.py')
pos8 = cmake.find('apply_v99_automation_cp8.py')
assert 0 <= pos7 < pos8, 'CP8 must run after CP7 in CMake generated-source chain'
workflow = (ROOT / '.github/workflows/build-v99-special.yml').read_text(encoding='utf-8-sig')
assert 'python tools/test_v99_automation_cp8_trade_atomic_log_contract.py' in workflow

print('V99 CP8 ATOMIC TRADE + CLEAR LOG CONTRACT: PASS')
