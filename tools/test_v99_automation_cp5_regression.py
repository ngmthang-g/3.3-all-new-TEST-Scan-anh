from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CP5 = ROOT / 'tools' / 'apply_v99_automation_cp5.py'
if not CP5.exists():
    raise SystemExit('CP5 REGRESSION FAIL: missing tools/apply_v99_automation_cp5.py')

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
        'apply_v99_automation_cp5.py',
    ]
    for script in steps:
        subprocess.run([
            sys.executable, str(ROOT / 'tools' / script),
            '--source-root', str(ROOT), '--output-dir', str(out)
        ], check=True, stdout=subprocess.DEVNULL)

    controller = (out / 'controller.cpp').read_text(encoding='utf-8-sig')
    bridge = (out / 'bridge.cpp').read_text(encoding='utf-8-sig')
    protocol = (out / 'protocol.h').read_text(encoding='utf-8-sig')
    image_test = (out / 'image_scan_test.cpp').read_text(encoding='utf-8-sig')
    image_auto = (out / 'image_scan_auto_ext.inl').read_text(encoding='utf-8-sig')

cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8-sig')
workflow = (ROOT / '.github/workflows/build-v99-special.yml').read_text(encoding='utf-8-sig')
party_logic = (ROOT / 'src' / 'party_build_logic.h').read_text(encoding='utf-8-sig')

checks = {
    # CP1 trade sequence remains intact.
    'trade semantic preserved': 'TravelSemantic::Trade' in protocol and 'key == L"giaodich"' in bridge,
    'trade target/opener/semantic/sequence wiring preserved': all(x in controller for x in [
        'Command::SelectTargetByRoleID', 'postTradeClick', 'TravelSemantic::Trade', 'TradePhase::Sequence'
    ]),

    # CP2 precheck persistence / boundary survives later patches.
    'precheck UI preserved': 'PRECHECK TAY NẢI' in image_test and 'LẤY F8 PRECHECK' in image_test,
    'precheck persistence preserved': all(x in image_auto for x in ['precheck_path=', 'precheck_roi=', 'precheck_switch=']),
    'precheck runtime preserved': all(x in image_auto for x in ['WaitPrecheckSwitch', 'startPrecheckCompleted', 'PRECHECK TAY NẢI PASS', 'PRECHECK TAY NẢI MISS']),

    # CP3 bulk/gather behavior survives CP4/CP5.
    'bulk controls preserved': all(x in controller for x in ['ÁP BÃI PT', 'ÁP ALL CON']),
    'gather exclusive preserved': all(x in controller for x in [
        'StopAllNormalAutomationForExclusiveMode(L"TẬP TRUNG")',
        'EligibleForGather(isMain)',
        'TẬP TRUNG OFF • tool IDLE • không tự START lại auto cũ'
    ]),

    # CP4 party persistence / developer layout / exclusive matrix.
    'party key persistence': all(x in controller for x in ['L"PartyKey"', 'partyKey =', 'SaveProfile(a.profile)']),
    'party developer persistence': all(x in controller for x in [
        'LoadPartyBuildSettings()', 'SavePartyBuildSettings', 'KeyClick1', 'KeyClick2', 'MemberFace',
        'TargetRetry', 'InviteRetry'
    ]),
    'party dev controls stay developer-only': 'partyBuildDevControls_' in controller and 'developerUnlockedControls_.insert' in controller,
    'quick management compact row': all(x in controller for x in [
        'L"ĐẶT KEY"', 'L"TỰ TẠO PT: OFF"', 'L"TẬP TRUNG: OFF"', 'L"ÁP BÃI PT"', 'L"ÁP ALL CON"'
    ]),
    'party semantic exact': 'TravelSemantic::InviteParty' in protocol and 'key == L"moivaonhom"' in bridge,
    'party excludes key itself': 'member != &key' in controller and 'memberRoleId != keyRoleId' in party_logic,
    'party start stops gather': 'if (gatherModeActive_) StopGatherMode();' in controller,
    'gather start stops party': 'if (partyBuildModeActive_) StopPartyBuildMode(L"chuyển sang TẬP TRUNG")' in controller,
    'normal start blocked by exclusive modes': 'if (gatherModeActive_ || partyBuildModeActive_)' in controller,
    'exclusive modes end idle': all(x in controller for x in [
        'TỰ TẠO PT OFF • tool IDLE', 'TẬP TRUNG OFF • tool IDLE'
    ]),

    # CP5 hardening: failed invite must reopen player menu before another semantic retry.
    'invite retry reopens face menu': 'AUTO PT • retry mở lại menu member' in controller and
                                      's.phase = PartyBuildPhase::MemberFace;' in controller,
    'invite retry counter not reset by face reopen': 's.inviteAttempts = 0;' not in controller.split('if (s.phase == PartyBuildPhase::MemberFace) {', 1)[1].split('if (s.phase == PartyBuildPhase::MemberInvite) {', 1)[0],

    # Build/CI integration order.
    'cp5 build order after cp4': cmake.find('apply_v99_automation_cp5.py') > cmake.find('apply_v99_automation_cp4.py') >= 0,
    'workflow runs cp5 regression': 'test_v99_automation_cp5_regression.py' in workflow,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('CP5 REGRESSION FAIL: ' + '; '.join(failed))
print('V9.9 CP5 FULL REGRESSION CONTRACT: PASS')
