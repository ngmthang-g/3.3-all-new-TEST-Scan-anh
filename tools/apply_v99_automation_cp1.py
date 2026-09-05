from pathlib import Path
import argparse
import re

ap = argparse.ArgumentParser()
ap.add_argument('--source-root', required=True)
ap.add_argument('--output-dir', required=True)
a = ap.parse_args()
out_dir = Path(a.output_dir)
p = out_dir / 'controller.cpp'
raw = p.read_bytes()
text = raw.decode('utf-8-sig')
nl = '\r\n' if '\r\n' in text else '\n'
t = text.replace('\r\n', '\n')


def once(old: str, new: str, label: str) -> None:
    global t
    count = t.count(old)
    if count != 1:
        raise SystemExit(f'{label}: expected 1 marker, got {count}')
    t = t.replace(old, new, 1)


def regex_once(pattern: str, replacement: str, label: str, flags: int = 0) -> None:
    global t
    new_text, count = re.subn(pattern, replacement, t, count=1, flags=flags)
    if count != 1:
        raise SystemExit(f'{label}: expected 1 regex marker, got {count}')
    t = new_text


def replace_between(start: str, end: str, replacement: str, label: str) -> None:
    global t
    if t.count(start) != 1:
        raise SystemExit(f'{label}: start marker count={t.count(start)}')
    i = t.index(start)
    j = t.find(end, i + len(start))
    if j < 0:
        raise SystemExit(f'{label}: end marker missing')
    t = t[:i] + replacement + t[j:]


# ---------------------------------------------------------------------------
# CP1-A: repurpose the existing persisted PostTradeClick setting as the
# configured hidden click that runs on the active CON after TARGET MAIN PASS,
# before the already-verified semantic "Giao dịch" callback and before macro.
# Storage keys intentionally stay unchanged so existing user config migrates.
# ---------------------------------------------------------------------------

executor_start = '    bool ExecutePostTradeClickTick(Account& main, Account& child, DWORD now) {\n'
executor_end = '    bool ExecuteTradeSequenceTick(Account& main, Account& child, DWORD now) {\n'
new_executor = '''    bool ExecuteTradeMenuOpenClickTick(Account& child, DWORD now) {
        if (tradeTxn_.postTradeClickCompleted) return false;

        const int repeat = std::clamp(shortcutSettings_.postTradeClickRepeat, 0, 999);
        const DWORD delay = static_cast<DWORD>(std::clamp(shortcutSettings_.postTradeClickDelayMs, 0, 60000));
        if (!shortcutSettings_.postTradeClickEnabled || repeat == 0) {
            tradeTxn_.postTradeClickCompleted = true;
            tradeTxn_.postTradeClickDueTick = 0;
            return false;
        }
        if (!shortcutSettings_.postTradeClick.valid) {
            if (!tradeTxn_.postTradeClickSkipReported) {
                tradeTxn_.postTradeClickSkipReported = true;
                LogAccount(child, L"CLICK SAU TARGET MAIN bỏ qua • đã BẬT nhưng chưa có tọa F8 hợp lệ.");
            }
            tradeTxn_.postTradeClickCompleted = true;
            tradeTxn_.postTradeClickDueTick = 0;
            return false;
        }
        if (tradeTxn_.postTradeClickDueTick != 0 &&
            static_cast<LONG>(now - tradeTxn_.postTradeClickDueTick) < 0) return true;

        // Delay after the final click is deliberate: the player interaction menu
        // needs one UI frame/window before the semantic "Giao dịch" callback scans it.
        if (tradeTxn_.postTradeClickRepeatDone >= repeat) {
            tradeTxn_.postTradeClickCompleted = true;
            tradeTxn_.postTradeClickDueTick = 0;
            SetTradeStatus(L"TRADE • CLICK SAU TARGET MAIN xong CON" +
                           std::to_wstring(tradeTxn_.childSlot) + L" • chuẩn bị chọn Giao dịch");
            return false;
        }

        std::wstring error;
        const bool ok = CoordinatorInternalPointAction(
            child, shortcutSettings_.postTradeClick,
            L"CLICK SAU TARGET MAIN • CON" + std::to_wstring(tradeTxn_.childSlot),
            error);
        if (!ok) {
            // This click is only the menu opener. The following semantic callback is
            // still the authoritative gate; on failure that gate resets this opener
            // so the next retry performs the click again instead of spinning on text.
            LogAccount(child, L"CLICK SAU TARGET MAIN click lỗi • sẽ retry lại trước callback • " + error);
        }

        ++tradeTxn_.postTradeClickRepeatDone;
        tradeTxn_.postTradeClickDueTick = GetTickCount() + delay;
        SetTradeStatus(L"TRADE • CLICK SAU TARGET MAIN → CON" +
                       std::to_wstring(tradeTxn_.childSlot) + L" • " +
                       std::to_wstring(tradeTxn_.postTradeClickRepeatDone) + L"/" +
                       std::to_wstring(repeat));
        return true;
    }

'''
replace_between(executor_start, executor_end, new_executor, 'replace old post-trade executor')

once(
'''            // Separate module boundary: the saved trade sequence is already complete.
            // Run the optional free click on MAIN, then the active CON, before any
            // post-pass bag snapshot reasoning. It is NOT a TradeSequenceStep.
            if (!tradeTxn_.postTradeClickCompleted && ExecutePostTradeClickTick(main, child, now)) return true;
''',
'''            // CLICK SAU TARGET MAIN moved before semantic Giao dịch. Nothing is
            // injected after the saved trade sequence anymore.
''',
'post-sequence click removal',
)

once(
'''            Response tradeCallbackResponse{};
''',
'''            if (!tradeTxn_.postTradeClickCompleted &&
                ExecuteTradeMenuOpenClickTick(*activeChild, now)) {
                return;
            }

            Response tradeCallbackResponse{};
''',
'pre-trade click before semantic',
)

once(
'''                tradeTxn_.targetRetryTick = GetTickCount() + kTradeTargetRetryMs;
                const std::wstring detail = !tradeCallbackError.empty() ? tradeCallbackError :
''',
'''                tradeTxn_.targetRetryTick = GetTickCount() + kTradeTargetRetryMs;
                // Menu may have failed to open or may have been rebuilt. Force the
                // configured CON click to run again before the next semantic retry.
                tradeTxn_.postTradeClickCompleted = false;
                tradeTxn_.postTradeClickRepeatDone = 0;
                tradeTxn_.postTradeClickDueTick = tradeTxn_.targetRetryTick;
                tradeTxn_.postTradeClickSkipReported = false;
                const std::wstring detail = !tradeCallbackError.empty() ? tradeCallbackError :
''',
'rearm opener on semantic failure',
)

# Make the target-pass log describe the real order; do not imply macro has started.
t = t.replace(
    'L" • macro bắt đầu từ dòng 1 • FreeBag MAIN=" +',
    'L" • chuẩn bị CLICK SAU TARGET → Giao dịch → macro dòng 1 • FreeBag MAIN=" +',
)

# Keep legacy field/INI names but update all user-visible wording.
t = t.replace(
    'AUTO CLICK SAU GIAO DỊCH • ĐỘC LẬP, KHÔNG NẰM TRONG CHUỖI GD • MAIN chạy trước → đúng CON vừa giao dịch chạy sau',
    'CLICK SAU TARGET MAIN • CHỈ CON • mở menu trước callback Giao dịch, sau đó mới chạy CHUỖI GD',
)
t = t.replace('AUTO CLICK SAU GD', 'CLICK SAU TARGET MAIN')
t = t.replace('Auto Click Sau Giao Dịch', 'Click Sau Target Main')
t = t.replace('dùng chung MAIN + CON', 'dùng cho CON')


# ---------------------------------------------------------------------------
# CP1-B: move the deep settings buttons to DEVELOPER. They remain the same
# controls/handlers; only ownership/visibility and coordinates change.
# ---------------------------------------------------------------------------
regex_once(
    r'shortcutSettingsButton_ = Make\(L"BUTTON", L"TÙY CHỈNH", BS_PUSHBUTTON,\s*\d+,\s*\d+,\s*\d+,\s*\d+,\s*IDC_SHORTCUT_SETTINGS\); addFont\(shortcutSettingsButton_\);',
    'shortcutSettingsButton_ = Make(L"BUTTON", L"TÙY CHỈNH", BS_PUSHBUTTON, 320, 112, 124, 30, IDC_SHORTCUT_SETTINGS); addFont(shortcutSettingsButton_);',
    'move shortcut settings button',
)
regex_once(
    r'addFont\(Make\(L"BUTTON", L"SCAN LỌC VK V4", BS_PUSHBUTTON,\s*\d+,\s*\d+,\s*\d+,\s*\d+,\s*IDC_TEST_IMAGE_SCAN\)\);',
    'imageScanButton_ = Make(L"BUTTON", L"SCAN LỌC VK V4", BS_PUSHBUTTON, 456, 112, 180, 30, IDC_TEST_IMAGE_SCAN); addFont(imageScanButton_);',
    'move/store image scan button',
)
once(
    '        developerUnlockedControls_ = {testOpenBagButton_, logCaption_, log_};\n',
    '        developerUnlockedControls_ = {testOpenBagButton_, shortcutSettingsButton_, imageScanButton_, logCaption_, log_};\n',
    'developer unlocked control set',
)


# ---------------------------------------------------------------------------
# CP1-C: SCAN V4 has a second, session-only password gate. The password dialog
# uses ES_PASSWORD in resources/app.rc. Restarting the tool resets the bool.
# ---------------------------------------------------------------------------
once(
    'constexpr int IDC_DEVELOPER_PASSWORD = 5001;\n',
    'constexpr int IDC_DEVELOPER_PASSWORD = 5001;\nconstexpr int IDD_V4_PASSWORD = 9100;\nconstexpr int IDC_V4_PASSWORD_EDIT = 9101;\n',
    'v4 password resource ids',
)

password_methods = '''    static INT_PTR CALLBACK ScanV4PasswordDialogProc(HWND dialog, UINT msg, WPARAM wp, LPARAM) {
        switch (msg) {
            case WM_INITDIALOG:
                SetFocus(GetDlgItem(dialog, IDC_V4_PASSWORD_EDIT));
                return FALSE;
            case WM_COMMAND:
                if (LOWORD(wp) == IDOK) {
                    wchar_t password[32]{};
                    GetDlgItemTextW(dialog, IDC_V4_PASSWORD_EDIT, password, static_cast<int>(_countof(password)));
                    if (std::wstring(password) == L"961912") {
                        SecureZeroMemory(password, sizeof(password));
                        EndDialog(dialog, IDOK);
                        return TRUE;
                    }
                    SecureZeroMemory(password, sizeof(password));
                    SetDlgItemTextW(dialog, IDC_V4_PASSWORD_EDIT, L"");
                    MessageBoxW(dialog, L"Sai mật khẩu SCAN V4.", L"SCAN LỌC VK V4", MB_OK | MB_ICONWARNING);
                    SetFocus(GetDlgItem(dialog, IDC_V4_PASSWORD_EDIT));
                    return TRUE;
                }
                if (LOWORD(wp) == IDCANCEL) {
                    EndDialog(dialog, IDCANCEL);
                    return TRUE;
                }
                break;
        }
        return FALSE;
    }

    void RequestImageScanTest() {
        if (!scanV4Unlocked_) {
            const INT_PTR result = DialogBoxParamW(instance_, MAKEINTRESOURCEW(IDD_V4_PASSWORD), hwnd_,
                                                   ScanV4PasswordDialogProc, 0);
            if (result != IDOK) return;
            scanV4Unlocked_ = true;
            Log(L"SCAN LỌC VK V4: mở khóa phiên hiện tại.");
        }
        OpenImageScanTest();
    }

'''
once(
    '    void OpenImageScanTest() {\n',
    password_methods + '    void OpenImageScanTest() {\n',
    'v4 password methods',
)
once(
    '''                    case IDC_TEST_IMAGE_SCAN:
                        OpenImageScanTest();
                        break;
''',
    '''                    case IDC_TEST_IMAGE_SCAN:
                        RequestImageScanTest();
                        break;
''',
    'v4 command gate',
)
once(
    '    HWND testOpenBagButton_ = nullptr;\n',
    '    HWND testOpenBagButton_ = nullptr;\n    HWND imageScanButton_ = nullptr;\n    bool scanV4Unlocked_ = false;\n',
    'v4 member state',
)

# Final scope assertions inside the patcher catch accidental drift immediately.
required = [
    'ExecuteTradeMenuOpenClickTick(*activeChild, now)',
    'Command::ClickTravelSemantic, static_cast<int>(TravelSemantic::Trade)',
    'developerUnlockedControls_ = {testOpenBagButton_, shortcutSettingsButton_, imageScanButton_, logCaption_, log_};',
    'DialogBoxParamW(instance_, MAKEINTRESOURCEW(IDD_V4_PASSWORD)',
    'L"961912"',
]
for token in required:
    if token not in t:
        raise SystemExit(f'CP1 final assertion missing: {token}')
if 'ExecutePostTradeClickTick(main, child, now)' in t:
    raise SystemExit('CP1 final assertion: old post-sequence executor call remains')

p.write_bytes(t.replace('\n', nl).encode('utf-8'))
print('apply_v99_automation_cp1.py: PASS')
