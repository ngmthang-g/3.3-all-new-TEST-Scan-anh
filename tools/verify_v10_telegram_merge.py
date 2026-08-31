from pathlib import Path
import hashlib
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTROLLER = (ROOT / 'src' / 'controller.cpp').read_text(encoding='utf-8')
CMAKE = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8')
VERSION = (ROOT / 'VERSION.txt').read_text(encoding='utf-8').strip()

errors = []
def need(cond, msg):
    if not cond: errors.append(msg)

def function_body(text, signature):
    pos = text.find(signature)
    if pos < 0: return None
    brace = text.find('{', pos)
    if brace < 0: return None
    depth = 0
    for i in range(brace, len(text)):
        if text[i] == '{': depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0: return text[pos:i+1]
    return None

# Version contract.
need(VERSION == '1.0', f'VERSION.txt must be 1.0, got {VERSION!r}')
need('project(ThanLongItemConsolidator VERSION 1.0.0' in CMAKE, 'CMake project version is not 1.0.0')
need('v1.0 • AUTO PK + TELEGRAM' in CONTROLLER, 'window title is not v1.0')

# Four tabs in exact order: existing AUTO + AUTO PK, donor TELEGRAM, existing ABOUT.
tab_lines = [line.strip() for line in CONTROLLER.splitlines() if 'TabCtrl_InsertItem(mainTab_' in line]
expected = ['L"AUTO"', 'L"AUTO PK"', 'L"TELEGRAM"', 'L"GIỚI THIỆU"']
need(len(tab_lines) == 4, f'expected 4 main tabs, got {len(tab_lines)}')
if len(tab_lines) == 4:
    for i, marker in enumerate(expected): need(marker in tab_lines[i], f'tab {i} is not {marker}')

# Internal control IDs must be disjoint. AUTO PK owns 400-431; Telegram owns 500-548.
pk_ids = [int(x) for x in re.findall(r'constexpr int IDC_PK_[A-Z0-9_]+ = (\d+);', CONTROLLER)]
tg_ids = [int(x) for x in re.findall(r'constexpr int IDC_TG_[A-Z0-9_]+ = (\d+);', CONTROLLER)]
need(pk_ids and all(400 <= x <= 431 for x in pk_ids), 'AutoPK ID range changed unexpectedly')
need(tg_ids and all(500 <= x <= 548 for x in tg_ids), 'Telegram IDs are not isolated in 500-548')
need(set(pk_ids).isdisjoint(tg_ids), 'AutoPK and Telegram control IDs overlap')

# Donor subsystem files: logic/header stay exact; notifier differs only by v1.0 user-agent text.
expected_hashes = {
    'telegram_logic.h': '1ece7c14bb1f7e119c923e0548d26c6851f918a77634a8581f680191392132e2',
    'telegram_logic_test.cpp': '5b41c54c1824a01bd1e7cb621b3fe2bdc56b67eba98fd6fdb781dc527a3280b1',
    'telegram_notifier.h': '750b63497e8abaa1f0d4f77b0dfe048cf846318c1c56a4b093fbad5123298c52',
}
for name, expected_hash in expected_hashes.items():
    actual = hashlib.sha256((ROOT/'src'/name).read_bytes()).hexdigest()
    need(actual == expected_hash, f'{name} differs from donor v0.6: {actual}')
need('WinHttpOpen(L"ThanLongItemConsolidator/1.0"' in (ROOT/'src/telegram_notifier.cpp').read_text(encoding='utf-8'),
     'Telegram notifier user-agent not updated to v1.0')

# Telegram is observer/output only: no bridge calls or game-input APIs inside transplanted method block.
start = CONTROLLER.find('    static std::wstring TrimWs')
end = CONTROLLER.find('    static std::wstring ProfileSection', start)
need(start >= 0 and end > start, 'Telegram method block not found')
if start >= 0 and end > start:
    tg_block = CONTROLLER[start:end]
    for forbidden in ['.bridge.Call(', 'CoordinatorInternalPointAction(', 'ClickInternalPoint', 'StartPath', 'StopPath', 'SetCursorPos(', 'SendInput(']:
        need(forbidden not in tg_block, f'Telegram block unexpectedly owns game/input action: {forbidden}')

# Critical v0.6.2.0 functions must remain byte-for-byte unchanged.
protected = {
    '    void TickAutoPk(': '1eb5447ae825ad9144c1cce2bbb770ca0c0bff04af99a6b02456ea4a36f29a48',
    '    bool EnsureAutoFightOffForTravel(': '582e438d5d7a86e46deb8e3ae5d2a5ab64a66cf83a618a4dcf23ea26e740117e',
    '    bool HandleRobustTravel(': 'd360b7f77bfc666843987adcc74fe7df0728d42c037c8fa853ea8650d064b7ca',
    '    bool PriorityReviveClick(': '934e7b2edf82c379eaec7d553e868ea16058a640e7183b179d93a3a5a94a2b4c',
    '    bool PriorityAutoClick(': 'fe754f63af9c1b967487519109b070b85c2049970b46aa8b85945948350b9439',
    '    void TickAccount(': '05b9c56ad2ef1545c53ae2279ed1b294d5c37e1c366b48bf188ba6e58f18c967',
    '    void ResetRuntimeForLifeBoundary(': 'd6bebaa3eb4d107385c73cd34fcf230bb397fcb58f56485334fb067680aebdb7',
    '    void BeginTrainRecovery(': 'f23b32173caf1eb7776317144ec79632a81484f0ab400bc758165470382ec66f',
    '    bool HandleDeath(': '21fb826e614cba4fa85a073b51a56d824263cf0ae310d1619bf77a3b99864642',
    '    void AbortTrade(': 'a2a88b182f99a97e4c6b9eb219aaf5701524c308650a223b00ccfcf65d74e899',
}
for sig, expected_hash in protected.items():
    body = function_body(CONTROLLER, sig)
    need(body is not None, f'protected function missing: {sig.strip()}')
    if body is not None:
        actual = hashlib.sha256(body.encode('utf-8')).hexdigest()
        need(actual == expected_hash, f'protected v0.6.2.0 function changed: {sig.strip()} -> {actual}')

# Required observer hook sites; these only report state and must not alter base actions.
for marker in [
    'TelegramRecordTradeCompleted(*main, *child, receivedSlots, tradeTxn_.sequencePass)',
    'TelegramRecordLauLanConfirm(a, a.runtime.confirmAttempts)',
    'TelegramRecordSellCompleted(a, s.freeBagSpace)',
    'ObserveTelegramAccountState(a, GetTickCount())',
    'TickTelegramSchedules(GetTickCount())',
    'telegramWorker_.Stop()',
]:
    need(marker in CONTROLLER, f'missing Telegram observer hook: {marker}')

# Build dependencies/test target.
for marker in ['src/telegram_notifier.cpp', 'winhttp crypt32', 'telegram_logic_tests']:
    need(marker in CMAKE, f'CMake missing Telegram dependency: {marker}')

if errors:
    for e in errors: print('FAIL:', e)
    sys.exit(1)
print('verify_v10_telegram_merge PASS')
