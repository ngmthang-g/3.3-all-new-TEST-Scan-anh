from pathlib import Path
import hashlib
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTROLLER = (ROOT/'src/controller.cpp').read_text(encoding='utf-8')
BRIDGE = (ROOT/'src/bridge.cpp').read_text(encoding='utf-8')
PROTOCOL = (ROOT/'src/protocol.h').read_text(encoding='utf-8')
LOGIC = (ROOT/'src/telegram_logic.h').read_text(encoding='utf-8')
CMAKE = (ROOT/'CMakeLists.txt').read_text(encoding='utf-8')
VERSION = (ROOT/'VERSION.txt').read_text(encoding='utf-8').strip()
errors=[]
def need(cond,msg):
    if not cond: errors.append(msg)

def function_body(text, signature):
    pos=text.find(signature)
    if pos<0: return None
    brace=text.find('{',pos)
    if brace<0: return None
    depth=0
    for i in range(brace,len(text)):
        if text[i]=='{': depth+=1
        elif text[i]=='}':
            depth-=1
            if depth==0: return text[pos:i+1]
    return None

need(VERSION=='1.1', f'VERSION must be 1.1, got {VERSION!r}')
need('VERSION 1.1.0' in CMAKE, 'CMake version not 1.1.0')
need('v1.1 • AUTO PK + TELEGRAM' in CONTROLLER, 'v1.1 title missing')
need('kProtocolVersion = 0x00010621u' in PROTOCOL, 'protocol must bump exactly once to 0x00010621')
need('ReadCurrency = 18' in PROTOCOL, 'read-only currency command missing')
need('std::int64_t value64_0' in PROTOCOL and 'std::int64_t value64_1' in PROTOCOL, '64-bit currency response missing')

pk_ids=[int(x) for x in re.findall(r'constexpr int IDC_PK_[A-Z0-9_]+ = (\d+);', CONTROLLER)]
tg_ids=[int(x) for x in re.findall(r'constexpr int IDC_TG_[A-Z0-9_]+ = (\d+);', CONTROLLER)]
need(pk_ids and all(400 <= x <= 431 for x in pk_ids), 'AutoPK ID range changed')
need(tg_ids and all(500 <= x <= 554 for x in tg_ids), 'Telegram IDs escaped 500-554')
need(set(pk_ids).isdisjoint(tg_ids), 'AutoPK/Telegram IDs overlap')

# Core AUTO/AutoPK functions stay byte-identical to verified v0.6.2.0/v1.0 base.
protected={
'    void TickAutoPk(':'1eb5447ae825ad9144c1cce2bbb770ca0c0bff04af99a6b02456ea4a36f29a48',
'    bool EnsureAutoFightOffForTravel(':'582e438d5d7a86e46deb8e3ae5d2a5ab64a66cf83a618a4dcf23ea26e740117e',
'    bool HandleRobustTravel(':'d360b7f77bfc666843987adcc74fe7df0728d42c037c8fa853ea8650d064b7ca',
'    bool PriorityReviveClick(':'934e7b2edf82c379eaec7d553e868ea16058a640e7183b179d93a3a5a94a2b4c',
'    bool PriorityAutoClick(':'fe754f63af9c1b967487519109b070b85c2049970b46aa8b85945948350b9439',
'    void TickAccount(':'05b9c56ad2ef1545c53ae2279ed1b294d5c37e1c366b48bf188ba6e58f18c967',
'    void ResetRuntimeForLifeBoundary(':'d6bebaa3eb4d107385c73cd34fcf230bb397fcb58f56485334fb067680aebdb7',
'    void BeginTrainRecovery(':'f23b32173caf1eb7776317144ec79632a81484f0ab400bc758165470382ec66f',
'    bool HandleDeath(':'21fb826e614cba4fa85a073b51a56d824263cf0ae310d1619bf77a3b99864642',
'    void AbortTrade(':'a2a88b182f99a97e4c6b9eb219aaf5701524c308650a223b00ccfcf65d74e899',
}
for sig, expected in protected.items():
    body=function_body(CONTROLLER,sig)
    need(body is not None, 'missing protected '+sig.strip())
    if body is not None:
        actual=hashlib.sha256(body.encode()).hexdigest()
        need(actual==expected, f'protected core changed {sig.strip()} -> {actual}')

# Telegram remains an observer. Its only bridge command is the read-only currency probe.
start=CONTROLLER.find('    static std::wstring TrimWs')
end=CONTROLLER.find('    static std::wstring ProfileSection', start)
need(start>=0 and end>start, 'Telegram block not found')
if start>=0 and end>start:
    block=CONTROLLER[start:end]
    calls=re.findall(r'\.bridge\.Call\(Command::([A-Za-z0-9_]+)',block)
    need(calls and set(calls)=={'ReadCurrency'}, f'Telegram owns unexpected bridge command(s): {calls}')
    for bad in ['CoordinatorInternalPointAction(', 'SetCursorPos(', 'SendInput(', 'Command::StartPath', 'Command::StopPath', 'Command::ClickInternalPoint']:
        need(bad not in block, 'Telegram gained mutable/input ownership: '+bad)

# Currency proof is optional/fail-closed: exact semantic getters only, Leader then roleData backing.
for marker in ['"get_Money"', '"get_BoundMoney"', 'RoleDataBacking(', 'Command::ReadCurrency', 'kCurrencySampleMs = 30000']:
    need(marker in BRIDGE or marker in CONTROLLER, 'currency contract missing: '+marker)
need('if (!validMask)' in BRIDGE, 'currency read must fail closed when neither getter resolves')

# Requested alert behavior and anti-spam gates.
for marker in ['FunnyAutoTrainOffText', '20u * 60u * 1000u', 'FunnySellText', 'FunnyDeathBurstText',
               '10u * 60u * 1000u', 'FunnyTradeText', 'FunnyCurrencyText', 'CurrencyMilestoneDue',
               'Cảnh báo vui + mốc vàng', 'MoneyMilestone1m', 'MoneyMilestone5m', 'MoneyMilestone60m', 'MoneyMilestone6h', 'MoneyMilestone24h']:
    need(marker in CONTROLLER or marker in LOGIC, 'requested Telegram v1.1 marker missing: '+marker)
need('deathsInTenMinutes >= 3' in LOGIC, 'death burst threshold must be 3/10m')
need('kCurrencyMilestoneCount = 5u' in LOGIC and '1ULL * kCurrencyMinuteMs' in LOGIC and '5ULL * kCurrencyMinuteMs' in LOGIC and '24ULL * 60ULL * kCurrencyMinuteMs' in LOGIC, 'currency milestone thresholds missing')
need('telegram_logic_tests' in CMAKE, 'Telegram tests missing from build')

if errors:
    for e in errors: print('FAIL:',e)
    sys.exit(1)
print('verify_v11_telegram_observer PASS')
