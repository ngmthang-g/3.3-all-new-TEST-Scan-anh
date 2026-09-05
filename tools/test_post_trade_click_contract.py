from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
C = (ROOT / 'src/controller.cpp').read_text(encoding='utf-8')

# Persistent shared settings. Repeat=0 must remain a deliberate no-op.
for token in [
    'bool postTradeClickEnabled = false;',
    'ClickPoint postTradeClick{};',
    'int postTradeClickDelayMs = 200;',
    'int postTradeClickRepeat = 1;',
    'PostTradeClickEnabled', 'PostTradeClickValid', 'PostTradeClickX', 'PostTradeClickY',
    'PostTradeClickW', 'PostTradeClickH', 'PostTradeClickDelayMs', 'PostTradeClickRepeat',
    'std::clamp(ReadIniInt(section, L"PostTradeClickRepeat", 1), 0, 999)',
]:
    assert token in C, token

# UI belongs to TÙY CHỈNH and is explicitly not part of the trade sequence editor.
assert 'AUTO CLICK SAU GIAO DỊCH • ĐỘC LẬP, KHÔNG NẰM TRONG CHUỖI GD' in C
for token in ['IDC_SC_POST_TRADE_ENABLED', 'IDC_SC_POST_TRADE_DELAY',
              'IDC_SC_POST_TRADE_REPEAT', 'IDC_SC_POST_TRADE_CAPTURE',
              'BeginPostTradeClickCapture()', 'shortcutSettings_.postTradeClick = captured;']:
    assert token in C, token

# Isolate the independent executor. It is best-effort and must never abort/finish
# a trade or inspect business state such as bag, item delta, MapID or NPC.
fn = re.search(r'bool ExecutePostTradeClickTick\(Account& main, Account& child, DWORD now\) \{(.*?)\n    \}\n\n    bool ExecuteTradeSequenceTick', C, re.S)
assert fn, 'cannot isolate ExecutePostTradeClickTick'
body = fn.group(1)
for token in [
    'tradeTxn_.postTradeClickTarget == 0 ? main : child',
    'CoordinatorInternalPointAction(',
    'target, shortcutSettings_.postTradeClick',
    '++tradeTxn_.postTradeClickRepeatDone;',
    '++tradeTxn_.postTradeClickTarget;',
    'postTradeClickTarget == 0',
    'postTradeClickTarget >= 2',
]:
    assert token in body, token
for forbidden in ['AbortTrade(', 'FinishTradeChild(', 'freeBagSpace', 'receivedSlots',
                  'MainNeedsCapacitySell', 'mapID', 'npcID', 'TradeAccountAtRendezvous']:
    assert forbidden not in body, forbidden

# MAIN must complete all repeats before stage increment, then exactly the active child
# supplied by the current TradeTxn executes the same shared point.
pos_action = body.find('CoordinatorInternalPointAction(')
pos_inc_repeat = body.find('++tradeTxn_.postTradeClickRepeatDone;')
pos_stage = body.find('++tradeTxn_.postTradeClickTarget;')
assert 0 <= pos_action < pos_inc_repeat < pos_stage
assert 'MAIN all repeats first, then active CON all repeats' in body

# Boundary: only after the saved TradeSequence is complete, before the bounded
# post-pass MAIN bag snapshot verification window starts.
seq = re.search(r'bool ExecuteTradeSequenceTick\(Account& main, Account& child, DWORD now\) \{(.*?)\n    \}\n\n    bool', C, re.S)
assert seq, 'cannot isolate ExecuteTradeSequenceTick'
seq_body = seq.group(1)
complete = seq_body.find('if (tradeTxn_.sequenceIndex >= seq.size())')
post = seq_body.find('ExecutePostTradeClickTick(main, child, now)')
bag = seq_body.find('if (tradeTxn_.sequenceBagVerifyStartedTick == 0)')
assert 0 <= complete < post < bag
assert 'It is NOT a TradeSequenceStep.' in seq_body

# Every pass resets only this post-trade substate; no FIFO/capacity phase is introduced.
for token in [
    'tradeTxn_.postTradeClickCompleted = false;',
    'tradeTxn_.postTradeClickTarget = 0;',
    'tradeTxn_.postTradeClickRepeatDone = 0;',
    'tradeTxn_.postTradeClickDueTick = 0;',
]:
    assert C.count(token) >= 2, token
assert 'enum class TradePhase' in C
assert 'PostTradeClick' not in re.search(r'enum class TradePhase[^;]+;', C, re.S).group(0)

# Full master export/import carries the setting, while v1/v2 readers remain supported.
for token in [
    'TLMASTERCFG\\t3', 'POST_TRADE_CLICK',
    'masterVersion!=1&&masterVersion!=2&&masterVersion!=3',
    'shortcutSettings_.postTradeClick=incoming.postTradeClick;',
    'shortcutSettings_.postTradeClickDelayMs=incoming.postTradeClickDelayMs;',
    'shortcutSettings_.postTradeClickRepeat=incoming.postTradeClickRepeat;',
]:
    assert token in C, token

# Scope guard: this feature iteration must not introduce the deferred watchdog/restart feature.
for forbidden in ['AUTO RECOVERY LOCK', 'TỰ KHÔI PHỤC KHI BỊ KẸT', 'WatchdogRestart', 'AutoStopStart']:
    assert forbidden not in C, forbidden

print('POST-TRADE CLICK independent MAIN + active CON contract: PASS')
