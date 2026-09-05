from pathlib import Path

root = Path(__file__).resolve().parents[2]

def load(rel):
    p = root / rel
    data = p.read_bytes()
    nl = b'\r\n' if b'\r\n' in data else b'\n'
    return p, data, nl

def replace_once(data, old, new, label):
    count = data.count(old)
    if count != 1:
        raise SystemExit(f'{label}: expected exactly 1 anchor, got {count}')
    return data.replace(old, new, 1)

# 1) Reuse the exact Côn Lôn semantic callback machinery; only add the Trade token.
p, data, nl = load('src/protocol.h')
old = b'    DenCacMonPhai = 3,' + nl + b'};'
new = b'    DenCacMonPhai = 3,' + nl + b'    Trade = 4,' + nl + b'};'
data = replace_once(data, old, new, 'TravelSemantic::Trade')
p.write_bytes(data)

p, data, nl = load('src/bridge.cpp')
old = b'        case TravelSemantic::DenCacMonPhai: return key == L"dencacmonphai";' + nl + b'        default: return false;'
new = b'        case TravelSemantic::DenCacMonPhai: return key == L"dencacmonphai";' + nl + b'        case TravelSemantic::Trade: return key == L"giaodich";' + nl + b'        default: return false;'
data = replace_once(data, old, new, 'trade semantic key')
p.write_bytes(data)

# 2) Developer tab: move the existing open-bag test + existing internal log there.
p, data, nl = load('src/controller.cpp')
old = b'constexpr int IDC_TEST_OPEN_BAG = 5000; // TEST-ONLY semantic open bag probe'
new = old + nl + b'constexpr int IDC_DEVELOPER_PASSWORD = 5001;'
data = replace_once(data, old, new, 'developer password id')

old = (b'            tab.pszText = const_cast<wchar_t*>(L"GI' + 'Ớ'.encode('utf-8') + b'I THI' + 'Ệ'.encode('utf-8') + b'U"); TabCtrl_InsertItem(mainTab_, 4, &tab);' + nl +
       b'            TabCtrl_SetItemSize(mainTab_, (kMainTabWidth - 8) / 5, 28);')
new = (b'            tab.pszText = const_cast<wchar_t*>(L"GI' + 'Ớ'.encode('utf-8') + b'I THI' + 'Ệ'.encode('utf-8') + b'U"); TabCtrl_InsertItem(mainTab_, 4, &tab);' + nl +
       b'            tab.pszText = const_cast<wchar_t*>(L"DEVELOPER"); TabCtrl_InsertItem(mainTab_, 5, &tab);' + nl +
       b'            TabCtrl_SetItemSize(mainTab_, (kMainTabWidth - 8) / 6, 28);')
data = replace_once(data, old, new, 'developer tab item')

old = '        addFont(Make(L"BUTTON", L"TEST MỞ TAY NẢI • KHÔNG CLICK", BS_PUSHBUTTON, 18, 386, 290, 30, IDC_TEST_OPEN_BAG));'.encode('utf-8')
new = ('        testOpenBagButton_ = Make(L"BUTTON", L"TEST MỞ TAY NẢI • KHÔNG CLICK", BS_PUSHBUTTON, 18, 112, 290, 30, IDC_TEST_OPEN_BAG); addFont(testOpenBagButton_);' +
       '\r\n        if (testOpenBagButton_) ShowWindow(testOpenBagButton_, SW_HIDE);').encode('utf-8') if nl == b'\r\n' else ('        testOpenBagButton_ = Make(L"BUTTON", L"TEST MỞ TAY NẢI • KHÔNG CLICK", BS_PUSHBUTTON, 18, 112, 290, 30, IDC_TEST_OPEN_BAG); addFont(testOpenBagButton_);' +
       '\n        if (testOpenBagButton_) ShowWindow(testOpenBagButton_, SW_HIDE);').encode('utf-8')
data = replace_once(data, old, new, 'move open bag button')

old = ('        logCaption_ = Make(L"STATIC", L"LOG / BỘ ĐIỀU PHỐI", 0, 18, 742, 190, 20, 0); addFont(logCaption_);\r\n'
       '        log_ = Make(L"EDIT", L"", WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, 18, 764, 1005, 159, IDC_LOG); addFont(log_);\r\n'
       '        if (logCaption_) ShowWindow(logCaption_, SW_HIDE);\r\n'
       '        if (log_) ShowWindow(log_, SW_HIDE); // vẫn ghi log nội bộ nhưng không hiện vùng debug').encode('utf-8') if nl == b'\r\n' else ('        logCaption_ = Make(L"STATIC", L"LOG / BỘ ĐIỀU PHỐI", 0, 18, 742, 190, 20, 0); addFont(logCaption_);\n'
       '        log_ = Make(L"EDIT", L"", WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, 18, 764, 1005, 159, IDC_LOG); addFont(log_);\n'
       '        if (logCaption_) ShowWindow(logCaption_, SW_HIDE);\n'
       '        if (log_) ShowWindow(log_, SW_HIDE); // vẫn ghi log nội bộ nhưng không hiện vùng debug').encode('utf-8')
new_text = '''        logCaption_ = Make(L"STATIC", L"LOG / BỘ ĐIỀU PHỐI", 0, 18, 160, 190, 20, 0); addFont(logCaption_);
        log_ = Make(L"EDIT", L"", WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, 18, 184, 1005, 730, IDC_LOG); addFont(log_);
        if (logCaption_) ShowWindow(logCaption_, SW_HIDE);
        if (log_) ShowWindow(log_, SW_HIDE); // Developer-only display; logging stays active while hidden.

        developerPasswordLabel_ = Make(L"STATIC", L"DEVELOPER • nhập mật khẩu", SS_LEFT | SS_CENTERIMAGE, 18, 62, 250, 24, 0); addFont(developerPasswordLabel_);
        developerPassword_ = Make(L"EDIT", L"", WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL, 18, 88, 290, 27, IDC_DEVELOPER_PASSWORD); addFont(developerPassword_);
        developerControls_ = {developerPasswordLabel_, developerPassword_};
        developerUnlockedControls_ = {testOpenBagButton_, logCaption_, log_};
        for (HWND h : developerControls_) if (h) ShowWindow(h, SW_HIDE);
        for (HWND h : developerUnlockedControls_) if (h) ShowWindow(h, SW_HIDE);'''
new = new_text.replace('\n', '\r\n' if nl == b'\r\n' else '\n').encode('utf-8')
data = replace_once(data, old, new, 'developer log/auth controls')

old = (b'    bool IsTelegramControl(HWND h) const {' + nl +
       b'        return std::find(telegramControls_.begin(), telegramControls_.end(), h) != telegramControls_.end();' + nl +
       b'    }' + nl + nl +
       b'    void SwitchMainTab(int index) {')
new_text = '''    bool IsTelegramControl(HWND h) const {
        return std::find(telegramControls_.begin(), telegramControls_.end(), h) != telegramControls_.end();
    }

    bool IsDeveloperControl(HWND h) const {
        return std::find(developerControls_.begin(), developerControls_.end(), h) != developerControls_.end() ||
               std::find(developerUnlockedControls_.begin(), developerUnlockedControls_.end(), h) != developerUnlockedControls_.end();
    }

    void ShowDeveloperControls(bool showPage) {
        const bool showAuth = showPage && !developerUnlocked_;
        const bool showUnlocked = showPage && developerUnlocked_;
        for (HWND h : developerControls_) if (h) ShowWindow(h, showAuth ? SW_SHOW : SW_HIDE);
        for (HWND h : developerUnlockedControls_) if (h) ShowWindow(h, showUnlocked ? SW_SHOW : SW_HIDE);
        if (showAuth && developerPassword_) SetFocus(developerPassword_);
    }

    void TryUnlockDeveloper() {
        if (developerUnlocked_ || !developerPassword_) return;
        wchar_t password[32]{};
        GetWindowTextW(developerPassword_, password, static_cast<int>(_countof(password)));
        if (std::wstring(password) != L"191296") return;
        developerUnlocked_ = true;
        SetWindowTextW(developerPassword_, L"");
        if (mainTabIndex_ == 5) ShowDeveloperControls(true);
        Log(L"DEVELOPER: mở khóa thành công.");
    }

    void SwitchMainTab(int index) {'''
new = new_text.replace('\n', '\r\n' if nl == b'\r\n' else '\n').encode('utf-8')
data = replace_once(data, old, new, 'developer helper methods')

data = replace_once(data, b'        index = std::clamp(index, 0, 4);', b'        index = std::clamp(index, 0, 5);', 'tab clamp')
data = replace_once(data,
    b'                if (child == mainTab_ || IsAboutControl(child) || IsTelegramControl(child)) continue;',
    b'                if (child == mainTab_ || IsAboutControl(child) || IsTelegramControl(child) || IsDeveloperControl(child)) continue;',
    'exclude developer from main visibility snapshot')
old = (b'        } else if (mainTabIndex_ == 4) {' + nl +
       b'            for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_HIDE);' + nl +
       b'        }')
new = (b'        } else if (mainTabIndex_ == 4) {' + nl +
       b'            for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_HIDE);' + nl +
       b'        } else if (mainTabIndex_ == 5) {' + nl +
       b'            ShowDeveloperControls(false);' + nl +
       b'        }')
data = replace_once(data, old, new, 'leave developer tab')
old = (b'        if (index == 0) {' + nl +
       b'            for (HWND h : telegramControls_) if (h) ShowWindow(h, SW_HIDE);' + nl +
       b'            for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_HIDE);')
new = (b'        if (index == 0) {' + nl +
       b'            for (HWND h : telegramControls_) if (h) ShowWindow(h, SW_HIDE);' + nl +
       b'            for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_HIDE);' + nl +
       b'            ShowDeveloperControls(false);')
data = replace_once(data, old, new, 'hide developer on main tab')
old = (b'        } else if (index == 3) {' + nl +
       b'            for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_HIDE);' + nl +
       b'            for (HWND h : telegramControls_) if (h) ShowWindow(h, SW_SHOW);' + nl +
       b'        } else {' + nl +
       b'            for (HWND h : telegramControls_) if (h) ShowWindow(h, SW_HIDE);' + nl +
       b'            for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_SHOW);' + nl +
       b'        }')
new = (b'        } else if (index == 3) {' + nl +
       b'            for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_HIDE);' + nl +
       b'            ShowDeveloperControls(false);' + nl +
       b'            for (HWND h : telegramControls_) if (h) ShowWindow(h, SW_SHOW);' + nl +
       b'        } else if (index == 4) {' + nl +
       b'            for (HWND h : telegramControls_) if (h) ShowWindow(h, SW_HIDE);' + nl +
       b'            ShowDeveloperControls(false);' + nl +
       b'            for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_SHOW);' + nl +
       b'        } else {' + nl +
       b'            for (HWND h : telegramControls_) if (h) ShowWindow(h, SW_HIDE);' + nl +
       b'            for (HWND h : aboutControls_) if (h) ShowWindow(h, SW_HIDE);' + nl +
       b'            ShowDeveloperControls(true);' + nl +
       b'        }')
data = replace_once(data, old, new, 'enter developer tab')

old = b'                switch (LOWORD(wp)) {' + nl + b'                    case IDC_TG_SHOW_TOKEN:'
new = b'                switch (LOWORD(wp)) {' + nl + b'                    case IDC_DEVELOPER_PASSWORD:' + nl + b'                        if (HIWORD(wp) == EN_CHANGE) TryUnlockDeveloper();' + nl + b'                        break;' + nl + b'                    case IDC_TG_SHOW_TOKEN:'
data = replace_once(data, old, new, 'developer password WM_COMMAND')

old = (b'    HWND log_ = nullptr;' + nl +
       b'    HWND mainTab_ = nullptr;' + nl +
       b'    int mainTabIndex_ = 0;' + nl +
       b'    std::vector<HWND> aboutControls_{};' + nl +
       b'    std::vector<HWND> telegramControls_{};')
new = (b'    HWND log_ = nullptr;' + nl +
       b'    HWND testOpenBagButton_ = nullptr;' + nl +
       b'    HWND developerPasswordLabel_ = nullptr;' + nl +
       b'    HWND developerPassword_ = nullptr;' + nl +
       b'    HWND mainTab_ = nullptr;' + nl +
       b'    int mainTabIndex_ = 0;' + nl +
       b'    bool developerUnlocked_ = false;' + nl +
       b'    std::vector<HWND> aboutControls_{};' + nl +
       b'    std::vector<HWND> telegramControls_{};' + nl +
       b'    std::vector<HWND> developerControls_{};' + nl +
       b'    std::vector<HWND> developerUnlockedControls_{};')
data = replace_once(data, old, new, 'developer members')

# 3) Exact requested trade ordering: after target MAIN PASS, callback "Giao dịch";
# only then start the untouched existing auto-click sequence. No new TradePhase.
old = (b'            if (response.resultCode != static_cast<std::int32_t>(ActionResult::ActionInvoked)) {' + nl +
       b'                tradeTxn_.targetRetryTick = GetTickCount() + kTradeTargetRetryMs;' + nl +
       b'                return;' + nl +
       b'            }' + nl + nl +
       b'            tradeTxn_.phase = TradePhase::Sequence;')
new_text = '''            if (response.resultCode != static_cast<std::int32_t>(ActionResult::ActionInvoked)) {
                tradeTxn_.targetRetryTick = GetTickCount() + kTradeTargetRetryMs;
                return;
            }

            Response tradeCallbackResponse{};
            std::wstring tradeCallbackError;
            const bool tradeCallbackOk = activeChild->bridge.Call(
                Command::ClickTravelSemantic, static_cast<int>(TravelSemantic::Trade),
                0, 0, tradeCallbackResponse, tradeCallbackError, 1100);
            if (!tradeCallbackOk ||
                tradeCallbackResponse.resultCode != static_cast<std::int32_t>(ActionResult::ActionInvoked)) {
                tradeTxn_.targetRetryTick = GetTickCount() + kTradeTargetRetryMs;
                const std::wstring detail = !tradeCallbackError.empty() ? tradeCallbackError :
                    (tradeCallbackResponse.detail[0] ? std::wstring(tradeCallbackResponse.detail) : L"chưa thấy dòng Giao dịch");
                LogAccount(*activeChild, L"GIAO DỊCH CALLBACK WAIT • " + detail);
                return;
            }
            LogAccount(*activeChild, L"GIAO DỊCH CALLBACK PASS • " + std::wstring(tradeCallbackResponse.detail));

            tradeTxn_.phase = TradePhase::Sequence;'''
new = new_text.replace('\n', '\r\n' if nl == b'\r\n' else '\n').encode('utf-8')
data = replace_once(data, old, new, 'target main -> trade callback -> sequence')

p.write_bytes(data)

# 4) Keep the contract in the normal v9.9 Windows build.
p, data, nl = load('.github/workflows/build-v99-special.yml')
needle = b"          python tools/test_image_scan_v5_contract.py" + nl + b"          if ($LASTEXITCODE -ne 0) { throw 'image scan v5 contract verifier failed' }"
replacement = needle + nl + b"          python tools/test_trade_callback_developer_contract.py" + nl + b"          if ($LASTEXITCODE -ne 0) { throw 'trade callback/developer contract verifier failed' }"
data = replace_once(data, needle, replacement, 'build workflow trade/developer contract')
p.write_bytes(data)

print('PASS applied trade callback + developer tab patch')
