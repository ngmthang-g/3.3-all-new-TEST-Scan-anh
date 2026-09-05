from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
patch = ROOT / 'tools' / 'apply_v99_automation_cp3.py'
if not patch.exists():
    raise SystemExit('V99 CP3 CONTRACT FAIL: missing tools/apply_v99_automation_cp3.py')

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
    ]
    for script in steps:
        subprocess.run([
            sys.executable, str(ROOT / 'tools' / script),
            '--source-root', str(ROOT), '--output-dir', str(out)
        ], check=True, stdout=subprocess.DEVNULL)
    controller = (out / 'controller.cpp').read_text(encoding='utf-8-sig')

cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8-sig')
workflow = (ROOT / '.github/workflows/build-v99-special.yml').read_text(encoding='utf-8-sig')

checks = {
    'bulk logic header included': '#include "automation_bulk_logic.h"' in controller,
    'quick management UI': all(x in controller for x in ['ÁP BÃI PT', 'ÁP BÃI ALL CON', 'TẬP TRUNG: OFF', 'LẤY TỌA TẬP TRUNG']),
    'bulk apply handlers': all(x in controller for x in ['ApplySpotToParty()', 'ApplySpotToAllCon()', 'EligibleForSpot']),
    'gather persistent target': all(x in controller for x in ['LoadGatherTarget()', 'SaveGatherTarget(', 'gatherTarget_']),
    'gather capture': 'CaptureGatherTarget()' in controller and 'TỌA TẬP TRUNG' in controller,
    'exclusive stop': 'StopAllNormalAutomationForExclusiveMode' in controller and 'ReleaseTradeHolds();' in controller,
    'gather excludes main': 'EligibleForGather' in controller,
    'gather tick reuses travel': 'TickGatherAccount' in controller and 'HandleRobustTravel(a, now, gatherTarget_' in controller,
    'gather keeps death logic': 'TickGatherAccount' in controller and 'HandleDeath(a, now)' in controller,
    'gather skips normal coordinator': '!gatherModeActive_' in controller and 'TickTradeCoordinator(GetTickCount())' in controller,
    'gather blocks normal start': 'TẬP TRUNG đang ON' in controller,
    'gather off stays idle': 'không tự START lại auto cũ' in controller,
    'cp3 after cp2': cmake.find('apply_v99_automation_cp3.py') > cmake.find('apply_v99_automation_cp2.py') >= 0,
    'bulk unit test target': 'automation_bulk_logic_tests' in cmake,
    'workflow runs cp3 contract': 'test_v99_automation_cp3_contract.py' in workflow,
    'workflow runs bulk unit test': 'automation_bulk_logic_tests' in workflow,
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('V99 CP3 CONTRACT FAIL: ' + '; '.join(failed))
print('V99 AUTOMATION CP3 CONTRACT: PASS')
