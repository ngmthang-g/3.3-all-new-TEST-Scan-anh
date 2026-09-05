from pathlib import Path
import argparse

ap = argparse.ArgumentParser()
ap.add_argument('--source-root', required=True)
ap.add_argument('--output-dir', required=True)
a = ap.parse_args()
out = Path(a.output_dir)
p = out / 'controller.cpp'
raw = p.read_bytes()
text = raw.decode('utf-8-sig')
nl = '\r\n' if '\r\n' in text else '\n'
t = text.replace('\r\n', '\n').replace('\r', '\n')


def once(old: str, new: str, label: str) -> None:
    global t
    count = t.count(old)
    if count != 1:
        raise SystemExit(f'{label}: expected one marker, found {count}')
    t = t.replace(old, new, 1)

# Move SCAN LỌC VK V4 out of DEVELOPER and make it an AUTO-tab-only control.
once(
    '        imageScanButton_ = Make(L"BUTTON", L"SCAN LỌC VK V4", BS_PUSHBUTTON, 456, 112, 180, 30, IDC_TEST_IMAGE_SCAN); addFont(imageScanButton_);\n',
    '        imageScanButton_ = Make(L"BUTTON", L"SCAN LỌC VK V4", BS_PUSHBUTTON, 18, 62, 180, 30, IDC_TEST_IMAGE_SCAN); addFont(imageScanButton_);\n'
    '        if (imageScanButton_) ShowWindow(imageScanButton_, SW_HIDE); // AUTO-tab only\n',
    'move Scan V4 button to AUTO tab position',
)

once(
    '        developerUnlockedControls_ = {testOpenBagButton_, shortcutSettingsButton_, imageScanButton_, logCaption_, log_};\n',
    '        developerUnlockedControls_ = {testOpenBagButton_, shortcutSettingsButton_, logCaption_, log_};\n',
    'remove Scan V4 from Developer controls',
)

once(
'''        if (index == 1 || index == 2) {
            const wchar_t* feature = index == 1 ? L"AUTO" : L"AUTO PHÓ BẢN";
            MessageBoxW(hwnd_,
                        (std::wstring(feature) + L"\\n\\nTính năng đã được cắt bỏ khỏi ver 9.9 ĐẶC BIỆT.").c_str(),
                        kTitle, MB_OK | MB_ICONINFORMATION);
            TabCtrl_SetCurSel(mainTab_, mainTabIndex_);
            return;
        }
''',
'''        if (index == 2) {
            MessageBoxW(hwnd_,
                        L"AUTO PHÓ BẢN\\n\\nTính năng đã được cắt bỏ khỏi ver 9.9 ĐẶC BIỆT.",
                        kTitle, MB_OK | MB_ICONINFORMATION);
            TabCtrl_SetCurSel(mainTab_, mainTabIndex_);
            return;
        }
''',
    'allow AUTO tab while keeping AUTO PHÓ BẢN blocked',
)

once(
'''        } else if (mainTabIndex_ == 3) {
            for (HWND h : telegramControls_) if (h) ShowWindow(h, SW_HIDE);
''',
'''        } else if (mainTabIndex_ == 1) {
            if (imageScanButton_) ShowWindow(imageScanButton_, SW_HIDE);
        } else if (mainTabIndex_ == 3) {
            for (HWND h : telegramControls_) if (h) ShowWindow(h, SW_HIDE);
''',
    'hide Scan V4 when leaving AUTO tab',
)

once(
'''        } else if (index == 3) {
            for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_HIDE);
            ShowDeveloperControls(false);
            for (HWND h : telegramControls_) if (h) ShowWindow(h, SW_SHOW);
''',
'''        } else if (index == 1) {
            for (HWND h : telegramControls_) if (h) ShowWindow(h, SW_HIDE);
            for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_HIDE);
            ShowDeveloperControls(false);
            if (imageScanButton_) ShowWindow(imageScanButton_, SW_SHOW);
        } else if (index == 3) {
            for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_HIDE);
            ShowDeveloperControls(false);
            for (HWND h : telegramControls_) if (h) ShowWindow(h, SW_SHOW);
''',
    'show Scan V4 on AUTO tab',
)

required = [
    'imageScanButton_ = Make(L"BUTTON", L"SCAN LỌC VK V4", BS_PUSHBUTTON, 18, 62, 180, 30, IDC_TEST_IMAGE_SCAN);',
    'developerUnlockedControls_ = {testOpenBagButton_, shortcutSettingsButton_, logCaption_, log_};',
    'if (index == 2)',
    'else if (mainTabIndex_ == 1) {',
    'else if (index == 1) {',
    'RequestImageScanTest();',
    'L"961912"',
]
for token in required:
    if token not in t:
        raise SystemExit(f'CP7 final assertion missing: {token}')
if 'developerUnlockedControls_ = {testOpenBagButton_, shortcutSettingsButton_, imageScanButton_, logCaption_, log_};' in t:
    raise SystemExit('CP7 final assertion: Scan V4 still belongs to Developer controls')
if 'if (index == 1 || index == 2)' in t:
    raise SystemExit('CP7 final assertion: AUTO tab is still blocked')

p.write_bytes(t.replace('\n', nl).encode('utf-8'))
print('apply_v99_automation_cp7.py: PASS')
