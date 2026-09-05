from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]

with tempfile.TemporaryDirectory(prefix='v99_scan_v4_auto_tab_') as td:
    out = Path(td)
    scripts = [
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
        'apply_v99_automation_cp6.py',
        'apply_v99_automation_cp7.py',
    ]
    for script in scripts:
        path = ROOT / 'tools' / script
        assert path.exists(), f'missing generated-source patch: {script}'
        subprocess.run(
            [sys.executable, str(path), '--source-root', str(ROOT), '--output-dir', str(out)],
            check=True,
        )
    C = (out / 'controller.cpp').read_text(encoding='utf-8-sig')

# AUTO must be selectable again; AUTO PHÓ BẢN remains the removed/blocked tab.
assert 'if (index == 1 || index == 2)' not in C
assert 'if (index == 2)' in C
assert 'L"AUTO PHÓ BẢN"' in C

# SCAN LỌC VK V4 is an AUTO-tab control, not a Developer unlocked control.
assert 'imageScanButton_ = Make(L"BUTTON", L"SCAN LỌC VK V4", BS_PUSHBUTTON, 18, 62, 180, 30, IDC_TEST_IMAGE_SCAN);' in C
assert 'developerUnlockedControls_ = {testOpenBagButton_, shortcutSettingsButton_, logCaption_, log_};' in C
assert 'developerUnlockedControls_ = {testOpenBagButton_, shortcutSettingsButton_, imageScanButton_, logCaption_, log_};' not in C

# Hidden on startup/DỒN ĐỒ, shown only while AUTO is active, hidden again when leaving AUTO.
assert 'if (imageScanButton_) ShowWindow(imageScanButton_, SW_HIDE); // AUTO-tab only' in C
assert 'else if (mainTabIndex_ == 1) {' in C
assert 'if (imageScanButton_) ShowWindow(imageScanButton_, SW_HIDE);' in C
assert 'else if (index == 1) {' in C
assert 'if (imageScanButton_) ShowWindow(imageScanButton_, SW_SHOW);' in C

# Existing V4 password/runtime behavior must remain intact.
assert 'bool scanV4Unlocked_ = false;' in C
assert 'RequestImageScanTest();' in C
assert 'DialogBoxParamW(instance_, MAKEINTRESOURCEW(IDD_V4_PASSWORD)' in C
assert 'L"961912"' in C
assert 'image_scan_test::RunDialog(MakeImageScanTarget(*account));' in C

# CP7 must be the final generated-source patch after CP6.
cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8-sig')
pos6 = cmake.find('apply_v99_automation_cp6.py')
pos7 = cmake.find('apply_v99_automation_cp7.py')
assert 0 <= pos6 < pos7, 'CP7 must run after CP6 in CMake generated-source chain'

print('V99 SCAN V4 AUTO TAB CONTRACT: PASS')
