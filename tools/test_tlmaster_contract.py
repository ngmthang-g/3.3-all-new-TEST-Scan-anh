from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
C = (ROOT / 'src/controller.cpp').read_text(encoding='utf-8')
H = (ROOT / 'src/thdc_route_logic.h').read_text(encoding='utf-8')

def esc(s):
    return s.replace('\\', '\\\\').replace('\t', '\\t').replace('\r', '\\r').replace('\n', '\\n')

def unesc(s):
    out = []
    i = 0
    while i < len(s):
        if s[i] != '\\':
            out.append(s[i]); i += 1; continue
        i += 1
        if i >= len(s):
            raise ValueError('dangling escape')
        mapping = {'\\': '\\', 't': '\t', 'r': '\r', 'n': '\n'}
        if s[i] not in mapping:
            raise ValueError('bad escape')
        out.append(mapping[s[i]]); i += 1
    return ''.join(out)

map_blob = 'TLMAPCFG\t1\r\nMAP_COUNT\t1\r\nMAP\tBãi Côn Lôn\\ttest\t75\t123\t456\r\n'
click_blob = 'TLCLICKCFG\t2\r\nPOINT\tAUTO\t1\t10\t20\t1000\t700\r\nCHILD_COUNT\t2\r\n'
for blob in (map_blob, click_blob):
    assert unesc(esc(blob)) == blob

for token in [
    'TLMASTERCFG\\t3', 'MAP_BLOB', 'CLICK_BLOB', 'RENDEZVOUS',
    'SELLNPC_COUNT', 'SHORTCUT_COUNT', 'THDC_COUNT\\t7',
    'KUNLUN_EXIT_CLICK', 'POST_TRADE_CLICK', 'ApplyThdcCoordinatePairs',
    'shortcutSettings_.kunlunExitClicks=incoming.kunlunExitClicks', 'shortcutSettings_.postTradeClick=incoming.postTradeClick', 'END'
]:
    assert token in C, token

keys = [
    ('M10000_TO_M10014', 10000, 8257, 148110),
    ('M10014_UP_M10015', 10014, 890, 6895),
    ('M10015_UP_M10016', 10015, 3080, 2900),
    ('M10015_DOWN_M10014', 10015, 7450, 966),
    ('M10016_UP_M10017', 10016, 4256, 7120),
    ('M10016_DOWN_M10015', 10016, 7620, 1242),
    ('M10017_DOWN_M10016', 10017, 690, 7200),
]
for key, source, x, y in keys:
    assert f'L"{key}"' in C
    assert str(source) in C
    assert f'= {x}' in C and f'= {y}' in C

for snippet in [
    'return {true, 10000, 10014, 0, true}',
    'return {true, 10014, 10015, 1, false}',
    'GatePlan{true, 10015, 10016, 2, false}',
    'GatePlan{true, 10015, 10014, 3, false}',
    'GatePlan{true, 10016, 10017, 4, false}',
    'GatePlan{true, 10016, 10015, 5, false}',
    'return {true, 10017, 10016, 6, false}',
]:
    assert snippet in H, snippet

assert '(masterVersion!=1&&masterVersion!=2&&masterVersion!=3)' in C
assert 'masterVersion==1 && sawLegacyKunlunClick' in C
assert 'masterVersion==2 && expectedThdc==7 && thdcSeen==7 && kunlunClickSeen==3' in C
assert 'masterVersion==3 && expectedThdc==7 && thdcSeen==7 && kunlunClickSeen==3 && sawPostTradeClick' in C

fn = re.search(r'void ImportPortableMasterConfig\(\) \{(.*?)\n    \}\n\n    void LoadTradeSettings', C, re.S)
assert fn, 'cannot isolate ImportPortableMasterConfig'
body = fn.group(1)
assert body.find('ParsePortableMasterConfig') < body.find('MessageBoxW(hwnd_,confirm.c_str()') < body.find('spots_=std::move(incoming.maps)')

assert C.count('L"XUẤT TẤT CẢ", BS_PUSHBUTTON') == 1
assert C.count('L"NHẬP TẤT CẢ", BS_PUSHBUTTON') == 1
for old in ['L"XUẤT MAP", BS_PUSHBUTTON', 'L"NHẬP MAP", BS_PUSHBUTTON',
            'L"XUẤT CFG", BS_PUSHBUTTON', 'L"NHẬP CFG", BS_PUSHBUTTON']:
    assert old not in C

assert '1190,1603' not in C and '1190, 1603' not in C

# Dồn đồ CON→MAIN v9.9: four admitted travellers, arrived-only FIFO,
# stationary MAIN, >=9 capacity gate, and one hidden idle/capacity click.
TC = (ROOT / 'src/trade_coordinator_logic.h').read_text(encoding='utf-8')
FS = (ROOT / 'src/fixed_slot_sell_logic.h').read_text(encoding='utf-8')
assert 'kReceivedSlotsFinishThreshold = 8' in TC
assert 'kTradePassMaxItems = kReceivedSlotsFinishThreshold + 1' in TC
assert 'kMaxTravelingChildren = 4' in TC
assert 'enum class PassDecision { FinishChild, RepeatSameChild };' in TC
assert 'enum class PostPassAction { SellPauseSameChild, FinishChild, RepeatSameChild };' in TC
assert 'ReceivedSlots(beforeFree, afterFree) <= kReceivedSlotsFinishThreshold' in TC
assert 'if (MainNeedsCapacitySell(afterFree)) return PostPassAction::SellPauseSameChild;' in TC
assert 'return freeBagSpace >= kTradePassMaxItems;' in TC
assert 'freeBagSpace < kTradePassMaxItems' in TC
assert 'ShouldAssignArrivalTicket' in TC
assert 'CoordinatorInternalPointAction(' in C
assert 'PostBackgroundClientClick' not in C
for forbidden in ['InvalidSnapshot', 'WaitForSnapshot', 'IsFreshPostTradeSnapshot',
                  'IsFullTradePassDelta', 'RepeatPreparation', 'EffectiveMainSellThreshold',
                  'TradeTransferRepeatLimit', 'CHUYỂN ĐỒ', 'HasChildTransferStep',
                  'TradeStepKindLabel', 'tradeSeqKind_', 'IDC_SEQ_KIND',
                  'MainSellThreshold', 'mainSellThreshold_', 'YieldActiveTradeForMainSell']:
    assert forbidden not in TC and forbidden not in C, forbidden
assert 'NormalizeChildPreClickContract' not in C
assert 'firstStored' not in C
assert 'preClickError' not in C
assert 'tradeTxn_.sequenceIndex = 1;' not in C
assert 'if (childTradeSequence_.empty())' in C
assert 'for (std::size_t i = 0; i < childTradeSequence_.size(); ++i)' in C
assert 'while (index > 0 && seq[index - 1].groupId == id)' in C
assert 'kTradeBagStableMs = 1000' in C
assert 'kTradeBagVerifyMaxMs = 2000' in C
assert 'if (tradeTxn_.phase == TradePhase::Sequence)' in C
assert 'tradeTxn_.phase != TradePhase::TargetMain' in C
assert 'ArmMainGapClickAfterPostPass(now, main' in C
assert 'ArmMainGapClickAfterPostPass(now, *activeMain' in C
assert 'savedSequenceFinished' not in C
assert 'const int repeatLimit = std::max(1, effective->repeat);' in C
assert 'capByMain' not in C
assert 'TLCLICKCFG\\t2' in C
assert 'if (f.size()!=14)' in C
assert 'lastTradePassFreeBagSpace' not in C
assert 'EffectiveClickCount' not in C and 'EffectiveClickCount' not in FS
assert 'TickMainStationarySell' not in C
assert 'TickMainIdleClick' in C and 'TickMainFullSellBatch' in C
assert 'LegacyIdleClickSourceIndex' in FS
assert 'kDefaultFullBatchClickCount = 90' in FS
assert 'const PostPassAction postPass = DecidePostPass(beforeFree, afterFree);' in C
assert 'postPass == PostPassAction::SellPauseSameChild' in C
assert 'PauseTradeForMainSell(TradePhase::TargetMain' in C
assert 'GIỮ NGUYÊN GD DỞ' in C
assert 'tradeTravelPids_' in C and 'tradeQueuePids_' in C
assert 'CHƯA có số FIFO' in C and 'ĐÃ TỚI TỌA GD → nhận FIFO' in C
assert 'tradeQueuePids_.empty() && main->runtime.sellPhase == 0' in C
assert 'MainNeedsCapacitySell(main->snapshot.freeBagSpace)' in C
assert 'CON tới sẽ ưu tiên GD' in C
assert 'rt.sellMacroRepeatDone >= rt.sellMacroPass' in C
assert 'if (CanStartTradePass(free))' in C
assert 'if (!MainBagIsFull(main.snapshot.freeBagSpace))' not in C
assert 'MAIN VẪN <9 Ô sau đủ' in C
assert 'a.snapshotValid && (a.snapshot.validMask & ValidAutoPath) && a.snapshot.autoPathing' in C
assert 'a.bridge.Call(Command::StopPath' in C
assert 'Command::ClickInternalPoint' in C
assert 'child.snapshot.freeBagSpace' not in C

rv_start = C.find('if (tradeTxn_.phase == TradePhase::Rendezvous) {')
rv_end = C.find('if (tradeTxn_.phase == TradePhase::TargetMain) {', rv_start)
rv = C[rv_start:rv_end] if rv_start >= 0 and rv_end > rv_start else ''
assert rv, 'cannot isolate Rendezvous -> TargetMain'
assert 'CoordinatorInternalPointAction' not in rv
assert 'tradeTxn_.phase = TradePhase::TargetMain;' in rv
assert 'MAIN <9 ô trước pass mới' in rv

target_start = rv_end
target_end = C.find('if (tradeTxn_.phase == TradePhase::Sequence) {', target_start)
target = C[target_start:target_end] if target_start >= 0 and target_end > target_start else ''
assert target, 'cannot isolate TargetMain -> Sequence'
assert 'Command::SelectTargetByRoleID' in target
assert 'tradeTxn_.sequenceIndex = 0;' in target
assert 'MAIN <9 ô trước khi target/click pass' in target

print('TLMASTER v3 + POST-TRADE + THDC + v9.9 trade contract: PASS')
