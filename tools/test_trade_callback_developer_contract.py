from pathlib import Path

root = Path(__file__).resolve().parents[1]
protocol = (root / 'src/protocol.h').read_text(encoding='utf-8-sig')
bridge = (root / 'src/bridge.cpp').read_text(encoding='utf-8-sig')
controller = (root / 'src/controller.cpp').read_text(encoding='utf-8-sig')

checks = {
    'trade semantic enum': 'Trade = 4,' in protocol,
    'trade semantic exact key': 'case TravelSemantic::Trade: return key == L"giaodich";' in bridge,
    'developer tab label': 'L"DEVELOPER"' in controller,
    'developer tab has 6 items': 'TabCtrl_SetItemSize(mainTab_, (kMainTabWidth - 8) / 6, 28);' in controller,
    'developer password control': 'IDC_DEVELOPER_PASSWORD' in controller and 'ES_PASSWORD' in controller,
    'developer password literal': 'L"191296"' in controller,
    'open bag button stored for developer': 'testOpenBagButton_' in controller,
    'developer controls include current log': 'developerUnlockedControls_' in controller and 'logCaption_' in controller and 'log_' in controller,
    'target main invokes trade callback': 'Command::ClickTravelSemantic, static_cast<int>(TravelSemantic::Trade)' in controller,
    'trade callback gates macro start': 'GIAO DỊCH CALLBACK PASS' in controller,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('FAIL trade/developer contract: ' + ', '.join(failed))

phase_line = next((line for line in controller.splitlines() if 'enum class TradePhase' in line), '')
if 'TradeMenu' in phase_line or 'OpenMenu' in phase_line or 'SelectTrade' in phase_line:
    raise SystemExit('FAIL scope: unexpected new trade phase added')

target_anchor = 'if (tradeTxn_.phase == TradePhase::TargetMain) {'
start = controller.find(target_anchor)
if start < 0:
    raise SystemExit('FAIL: TargetMain block missing')
end = controller.find('if (tradeTxn_.phase == TradePhase::Sequence) {', start)
if end < 0:
    raise SystemExit('FAIL: Sequence block missing')
block = controller[start:end]
callback_pos = block.find('Command::ClickTravelSemantic, static_cast<int>(TravelSemantic::Trade)')
sequence_pos = block.find('tradeTxn_.phase = TradePhase::Sequence;')
if callback_pos < 0 or sequence_pos < 0 or callback_pos > sequence_pos:
    raise SystemExit('FAIL ordering: trade callback must succeed before Sequence starts')

print('PASS trade callback + developer tab contract')
