from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
cp2 = ROOT / 'tools' / 'apply_v99_automation_cp2.py'
impl = ROOT / 'tools' / 'apply_v99_automation_cp2_impl.py'
if not cp2.exists() or not impl.exists():
    raise SystemExit('V4 PRECHECK CONTRACT FAIL: missing CP2 patch implementation')

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
    ]
    for script in steps:
        subprocess.run([
            sys.executable, str(ROOT / 'tools' / script),
            '--source-root', str(ROOT), '--output-dir', str(out)
        ], check=True, stdout=subprocess.DEVNULL)

    core = (out / 'image_scan_test.cpp').read_text(encoding='utf-8-sig')
    auto = (out / 'image_scan_auto_ext.inl').read_text(encoding='utf-8-sig')

cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8-sig')
workflow = (ROOT / '.github/workflows/build-v99-special.yml').read_text(encoding='utf-8-sig')

checks = {
    'precheck config fields': all(x in core for x in ['bagUiTemplatePath', 'bagUiRoi', 'bagUiSwitch']),
    'portable manual scan export': all(x in core for x in ['precheck_file=', '_precheck', 'precheck_roi=', 'precheck_switch=', 'config + 4 ảnh mẫu']),
    'precheck persistent settings': all(x in auto for x in ['precheck_path=', 'precheck_roi=', 'precheck_switch=']),
    'precheck developer UI': 'PRECHECK TAY NẢI' in core and 'LẤY F8 PRECHECK' in core,
    'precheck image + roi UI': all(x in core for x in ['IDC_PRECHECK_PICK', 'IDC_PRECHECK_REGION', 'IDC_PRECHECK_PREVIEW', 'IDC_PRECHECK_FULL']),
    'reuse proven F8 capture': 'ArmBagF8(*s,-6,L"PRECHECK TAY NẢI")' in core and 's.captureRow==-6' in core,
    'runtime precheck phase': 'WaitPrecheckSwitch' in auto and 'startPrecheckCompleted' in auto,
    'runtime scan before open bag': 'AutoRunStartPrecheck' in auto and 'if(!AutoRunStartPrecheck(s,now,error))' in auto,
    'match skips switch click': 'PRECHECK TAY NẢI PASS' in auto,
    'mismatch clicks configured point': 'PRECHECK TAY NẢI MISS' in auto and 'AutoClickStep(s,s.scan.config.bagUiSwitch,error)' in auto,
    'reset keeps one-per-start latch': 'void ResetAutoFilter(HWND gameWindow, int childSlot)' in auto and 's.startPrecheckCompleted=false' not in auto and 's.startPrecheckCompleted = false' not in auto,
    'stop creates next start boundary': 'g_autoSessions.erase(key)' in auto,
    'full can preempt precheck delay': 'WaitPrecheckSwitch||s.phase==AutoPhase::WaitOpen1' in auto,
    'cp2 build order after cp1': cmake.find('apply_v99_automation_cp2.py') > cmake.find('apply_v99_automation_cp1.py') >= 0,
    'workflow runs precheck contract': 'test_v4_precheck_contract.py' in workflow,
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('V4 PRECHECK CONTRACT FAIL: ' + '; '.join(failed))
print('V4 PRECHECK TAY NAI CONTRACT: PASS')
