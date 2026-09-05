from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
auto = (ROOT/'src/image_scan_auto_ext.inl').read_text(encoding='utf-8-sig')
header = (ROOT/'src/image_scan_test.h').read_text(encoding='utf-8-sig')
gen = (ROOT/'tools/generate_image_scan_v4_sources.py').read_text(encoding='utf-8-sig')
workflow = (ROOT/'.github/workflows/build-v99-special.yml').read_text(encoding='utf-8-sig')

with tempfile.TemporaryDirectory() as td:
    out = Path(td)
    subprocess.run([
        sys.executable, str(ROOT/'tools/generate_image_scan_v4_sources.py'),
        '--source-root', str(ROOT), '--output-dir', str(out)
    ], check=True, stdout=subprocess.DEVNULL)
    subprocess.run([
        sys.executable, str(ROOT/'tools/apply_pre_close_x_patch.py'),
        '--source-root', str(ROOT), '--output-dir', str(out)
    ], check=True, stdout=subprocess.DEVNULL)
    for patch in [
        'apply_ui30_controller_base.py',
        'apply_ui30_controller_groups.py',
        'apply_ui30_controller_runtime.py',
        'apply_ui30_scanner.py',
    ]:
        subprocess.run([
            sys.executable, str(ROOT/'tools'/patch),
            '--source-root', str(ROOT), '--output-dir', str(out)
        ], check=True, stdout=subprocess.DEVNULL)
    core = (out/'image_scan_test.cpp').read_text(encoding='utf-8-sig')
    generated_auto = (out/'image_scan_auto_ext.inl').read_text(encoding='utf-8-sig')
    controller = (out/'controller.cpp').read_text(encoding='utf-8-sig')

checks = {
    '42 is default only': 'kDefaultInitialSteps = 42' in core and 'kAutoRequiredSlots' not in core + auto,
    'dynamic runtime slot count': 's.scan.config.steps.size()' in auto and 'AUTO chưa có tọa scan' in auto,
    'EMPTY waits 10s same slot': 'kEmptyRetryMs = 10000' in core and 'NO GOOD + NO VỨT = Ô TRỐNG' in core and 'WaitEmptyRetry' in auto,
    'no BagReady extra guard': 'BagReady' not in core+auto and 'InventoryReady' not in core+auto,
    'per-CON 30 toggles': 'std::array<bool, 30> childEnabled' in core and '1200..1229 = CON1..CON30' in core and 'childSlot <= 30' in generated_auto,
    'main UI 30/PT/PID quick controls': 'kChildTradeCount = 30' in controller and 'CHỌN TẤT CẢ' in controller and 'BỎ TẤT CẢ' in controller and 'DisplayParty' in controller and 'LVN_LINKCLICK' in controller and 'PID ◀' in controller,
    'taller list keeps NPC controls visible below settings': '18, 66, 1005, 220, IDC_CLIENT_LIST' in controller and '82, 468, 465, 260, IDC_SELL_NPC' in controller and '90, 498, 860, 27, IDC_SELL_NPC_POS' in controller,
    'two optional open + one close': 'openBag1' in core and 'openBag2' in core and 'closeBag' in core,
    'separate scan export/import': 'TL_SCAN_V5' in core and 'XUẤT SCAN' in core and 'NHẬP SCAN' in core,
    'actual templates copied beside export config': 'CopyTemplateBesideConfig' in core and '_good' in core and '_discard' in core and '_close' in core,
    'background CON API': 'TickAutoFilter' in header and 'IsChildAutoFilterEnabled' in header and 'FullBagYieldReady' in header,
    'FULL latch survives bag-space change': 'FullBagTravelLatched' in auto and 'fullExitPending' in auto and 'FullBagTravelLatched(child->game.window, slot)' in controller,
    'scan error cannot clear FULL latch': 's.phase=AutoPhase::Error;s.currentItemActive=false;s.fullExitPending=false' not in auto,
    'FULL coordinator uses live-or-latched request': 'const bool fullRequested = child->snapshot.freeBagSpace == 0 ||' in controller,
    'FULL finishes current then closes': 'AutoClosePurpose::Full' in auto and 'WaitPopupGoneAfterConfirm' in auto,
    'last configured slot completes until full': 's.slotIndex+1>=s.scan.config.steps.size()' in auto and 'CompletedUntilFull' in auto,
    'FULL does not auto-restart when one slot becomes free': 'awaitingNewTrain' not in auto,
    'new train resets scan explicitly': controller.count('image_scan_test::ResetAutoFilter(a.game.window, scanSlot)') >= 2,
    'death gives revive priority then closes': 'NotifyDeath' in auto and 'NotifyReviveClicked' in auto and 'sau ĐẦU THAI' in auto,
    'periodic 60s disabled in steady train': 'periodic tọa/AutoFight 60s OFF' in controller and 'TickAutoFilter(MakeImageScanTarget(a), scanSlot, bagFull)' in controller,
    'persistent scan settings': 'WeaponScan.auto.tlscan' in auto and 'EnsurePersistentConfigLoaded' in core+auto and 'SavePersistentConfig' in core+auto,
    'workflow runs contract': 'test_image_scan_v5_contract.py' in workflow,
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('IMAGE SCAN V5 CONTRACT FAIL: ' + '; '.join(failed))
print('IMAGE SCAN V5 AUTO-CON CONTRACT: PASS')
