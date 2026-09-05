from pathlib import Path
import subprocess, sys, tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as td:
    out = Path(td)
    steps = [
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
    ]
    for script in steps:
        subprocess.run([sys.executable, str(ROOT / 'tools' / script), '--source-root', str(ROOT), '--output-dir', str(out)], check=True)
    controller = (out / 'controller.cpp').read_text(encoding='utf-8-sig')
    bridge = (out / 'bridge.cpp').read_text(encoding='utf-8-sig')
    protocol = (out / 'protocol.h').read_text(encoding='utf-8-sig')
cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8-sig')
workflow = (ROOT / '.github/workflows/build-v99-special.yml').read_text(encoding='utf-8-sig')

checks = {
    'party key persistence': 'partyKey' in controller and 'PartyKey' in controller,
    'compact key ui': 'ĐẶT KEY' in controller and 'TỰ TẠO PT: OFF' in controller,
    'developer party controls': 'AUTO TẠO PT • DEVELOPER' in controller and 'LẤY F8 CLICK 1' in controller and 'LẤY F8 MẶT' in controller,
    'exclusive party mode': 'partyBuildModeActive_' in controller and 'StopAllNormalAutomationForExclusiveMode(L"TỰ TẠO PT")' in controller,
    'key does not target self': 'IsInviteMember' in controller,
    'target member by role id': 'Command::SelectTargetByRoleID' in controller,
    'hidden face click': 'MEMBER FACE' in controller and 'CoordinatorInternalPointAction' in controller,
    'invite semantic enum': 'InviteParty = 5' in protocol,
    'invite semantic exact token': 'moivaonhom' in bridge,
    'invite callback': 'TravelSemantic::InviteParty' in controller,
    'completion popup': 'AUTO TẠO PT HOÀN TẤT' in controller,
    'off stays idle': 'TỰ TẠO PT OFF • tool IDLE' in controller,
    'no trade while party build': '!partyBuildModeActive_' in controller and 'TickTradeCoordinator' in controller,
    'cp4 build order after cp3': cmake.find('apply_v99_automation_cp4.py') > cmake.find('apply_v99_automation_cp3.py') >= 0,
    'party logic test target': 'party_build_logic_tests' in cmake,
    'workflow cp4 contract': 'test_v99_automation_cp4_contract.py' in workflow,
    'workflow party logic test': 'party_build_logic_tests' in workflow,
}
failed = [k for k,v in checks.items() if not v]
if failed:
    raise SystemExit('CP4 CONTRACT FAIL: ' + '; '.join(failed))
print('V9.9 AUTOMATION CP4 CONTRACT: PASS')
