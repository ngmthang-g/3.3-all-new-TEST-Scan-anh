from pathlib import Path

p = Path('src/image_scan_filter_v4.inl')
if not p.exists():
    raise SystemExit('missing src/image_scan_filter_v4.inl')
text = p.read_text(encoding='utf-8')


def replace_once(old: str, new: str):
    global text
    if old not in text:
        raise SystemExit(f'delay-controls anchor missing: {old[:120]}')
    text = text.replace(old, new, 1)


# Separate configurable delays for the three special gameplay clicks.
replace_once('constexpr int IDC_CLOSE_FULL = 1066;\n', '''constexpr int IDC_CLOSE_FULL = 1066;\nconstexpr int IDC_DELAY_DISCARD = 1070;\nconstexpr int IDC_DELAY_AFTER_DISCARD = 1071;\nconstexpr int IDC_DELAY_CLOSE = 1072;\n''')

replace_once('    int thresholdPercent = 90;\n    std::vector<ClickStep> steps;\n    ClickStep afterDiscard;\n', '''    int thresholdPercent = 90;\n    std::vector<ClickStep> steps;\n    ClickStep afterDiscard;\n\n    // Delay cố định sau từng loại click đặc biệt.\n    int discardClickDelayMs = 500;\n    int afterDiscardClickDelayMs = 500;\n    int closeClickDelayMs = 500;\n''')

replace_once('enum class RunPhase {\n    Idle,\n    WaitItemReady,\n    WaitDiscardGone,\n    WaitPopupGoneAfterConfirm,\n    WaitCloseGone,\n};\n', '''enum class RunPhase {\n    Idle,\n    WaitItemReady,\n    WaitDiscardGone,\n    WaitPopupGoneAfterConfirm,\n    WaitCloseGone,\n};\n\nenum class DelayKind {\n    Slot,\n    Discard,\n    AfterDiscard,\n    Close,\n};\n''')

replace_once('    bool runningChain = false;\n\n    // Runtime v4:', '    bool runningChain = false;\n    DelayKind delayKind = DelayKind::Slot;\n\n    // Runtime v4:')

# Read all three special delays. Same allowed range as current per-slot delay.
threshold_anchor = '    s.config.thresholdPercent = std::clamp(ReadInt(s.hwnd, IDC_THRESHOLD, 90), 1, 100);\n'
replace_once(threshold_anchor, threshold_anchor + '''\n    s.config.discardClickDelayMs = std::clamp(ReadInt(s.hwnd, IDC_DELAY_DISCARD, s.config.discardClickDelayMs), 50, 10000);\n    s.config.afterDiscardClickDelayMs = std::clamp(ReadInt(s.hwnd, IDC_DELAY_AFTER_DISCARD, s.config.afterDiscardClickDelayMs), 50, 10000);\n    s.config.closeClickDelayMs = std::clamp(ReadInt(s.hwnd, IDC_DELAY_CLOSE, s.config.closeClickDelayMs), 50, 10000);\n''')

# Keep the existing six Sleep(StepDelayMs(s)) call sites so source contracts stay stable,
# but StepDelayMs now returns the delay for the exact action that just happened.
old_step_delay = '''int StepDelayMs(const State& s){\n    if(s.slotIndex<s.config.steps.size())return std::clamp(s.config.steps[s.slotIndex].delayMs,50,10000);\n    return 500;\n}\n'''
new_step_delay = '''int StepDelayMs(const State& s){\n    switch(s.delayKind){\n        case DelayKind::Discard: return std::clamp(s.config.discardClickDelayMs,50,10000);\n        case DelayKind::AfterDiscard: return std::clamp(s.config.afterDiscardClickDelayMs,50,10000);\n        case DelayKind::Close: return std::clamp(s.config.closeClickDelayMs,50,10000);\n        case DelayKind::Slot:\n        default:\n            if(s.slotIndex<s.config.steps.size())return std::clamp(s.config.steps[s.slotIndex].delayMs,50,10000);\n            return 500;\n    }\n}\n'''
replace_once(old_step_delay, new_step_delay)

# Assign the right delay kind immediately before each existing Sleep call.
action_replacements = [
    ('if(!RawClick(s,cx,cy,frame.width,frame.height,error)){FinishRun(s,L"CLICK TÂM X FAIL • "+error);return;}\n                Sleep(static_cast<DWORD>(StepDelayMs(s)));',
     'if(!RawClick(s,cx,cy,frame.width,frame.height,error)){FinishRun(s,L"CLICK TÂM X FAIL • "+error);return;}\n                s.delayKind=DelayKind::Close;\n                Sleep(static_cast<DWORD>(StepDelayMs(s)));'),
    ('if(!RawClick(s,dx,dy,frame.width,frame.height,error)){FinishRun(s,L"CLICK TÂM VỨT FAIL • "+error);return;}\n            Sleep(static_cast<DWORD>(StepDelayMs(s)));',
     'if(!RawClick(s,dx,dy,frame.width,frame.height,error)){FinishRun(s,L"CLICK TÂM VỨT FAIL • "+error);return;}\n            s.delayKind=DelayKind::Discard;\n            Sleep(static_cast<DWORD>(StepDelayMs(s)));'),
    ('if(!ClickAfterDiscard(s,error)){FinishRun(s,L"CLICK SAU VỨT FAIL • "+error);return;}\n            Sleep(static_cast<DWORD>(StepDelayMs(s)));',
     'if(!ClickAfterDiscard(s,error)){FinishRun(s,L"CLICK SAU VỨT FAIL • "+error);return;}\n            s.delayKind=DelayKind::AfterDiscard;\n            Sleep(static_cast<DWORD>(StepDelayMs(s)));'),
    ('if(!ClickCurrentSlot(s,error)){FinishRun(s,L"LẶP CLICK "+std::to_wstring(s.slotIndex+1)+L" FAIL • "+error);return;}\n            Sleep(static_cast<DWORD>(StepDelayMs(s)));',
     'if(!ClickCurrentSlot(s,error)){FinishRun(s,L"LẶP CLICK "+std::to_wstring(s.slotIndex+1)+L" FAIL • "+error);return;}\n            s.delayKind=DelayKind::Slot;\n            Sleep(static_cast<DWORD>(StepDelayMs(s)));'),
    ('if(!ClickCurrentSlot(s,error)){FinishRun(s,L"CLICK "+std::to_wstring(s.slotIndex+1)+L" FAIL • "+error);return;}\n            Sleep(static_cast<DWORD>(StepDelayMs(s)));',
     'if(!ClickCurrentSlot(s,error)){FinishRun(s,L"CLICK "+std::to_wstring(s.slotIndex+1)+L" FAIL • "+error);return;}\n            s.delayKind=DelayKind::Slot;\n            Sleep(static_cast<DWORD>(StepDelayMs(s)));'),
    ('if(!ClickCurrentSlot(s,error)){FinishRun(s,L"CLICK 1 FAIL • "+error);return;}\n    Sleep(static_cast<DWORD>(StepDelayMs(s)));',
     'if(!ClickCurrentSlot(s,error)){FinishRun(s,L"CLICK 1 FAIL • "+error);return;}\n    s.delayKind=DelayKind::Slot;\n    Sleep(static_cast<DWORD>(StepDelayMs(s)));'),
]
for old, new in action_replacements:
    replace_once(old, new)

# Make add/remove/reorder controls visible; logic already existed in WndProc.
old_group = '    Add(s.hwnd,L"BUTTON",L"DANH SÁCH CLICK Ô ĐỒ • mặc định 20 • +THÊM / -XÓA không giới hạn cứng • Delay = Sleep sau mỗi thao tác",BS_GROUPBOX,18,238,987,330,0);\n'
new_group = '''    Add(s.hwnd,L"BUTTON",L"DANH SÁCH CLICK Ô ĐỒ • mặc định 20 • Delay riêng từng CLICK N",BS_GROUPBOX,18,238,987,330,0);\n    Add(s.hwnd,L"BUTTON",L"+ THÊM",BS_PUSHBUTTON,650,236,78,26,IDC_STEP_ADD);\n    Add(s.hwnd,L"BUTTON",L"- XÓA",BS_PUSHBUTTON,733,236,78,26,IDC_STEP_DELETE);\n    Add(s.hwnd,L"BUTTON",L"LÊN",BS_PUSHBUTTON,816,236,70,26,IDC_STEP_UP);\n    Add(s.hwnd,L"BUTTON",L"XUỐNG",BS_PUSHBUTTON,891,236,90,26,IDC_STEP_DOWN);\n'''
replace_once(old_group, new_group)

# Compact special-delay controls on the existing CLICK SAU VỨT row; no extra window height required.
old_after_info = '    Add(s.hwnd,L"STATIC",L"Sau VỨT: tool chờ ảnh VỨT biến mất rồi mới click tọa này.",SS_LEFT|SS_CENTERIMAGE,640,522,345,27,0);\n'
new_after_info = '''    Add(s.hwnd,L"STATIC",L"VỨT ms",SS_LEFT|SS_CENTERIMAGE,640,522,48,27,0);\n    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.discardClickDelayMs).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,689,522,55,27,IDC_DELAY_DISCARD);\n    Add(s.hwnd,L"STATIC",L"SAU ms",SS_LEFT|SS_CENTERIMAGE,750,522,50,27,0);\n    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.afterDiscardClickDelayMs).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,801,522,55,27,IDC_DELAY_AFTER_DISCARD);\n    Add(s.hwnd,L"STATIC",L"X ms",SS_LEFT|SS_CENTERIMAGE,862,522,38,27,0);\n    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.closeClickDelayMs).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,901,522,55,27,IDC_DELAY_CLOSE);\n'''
replace_once(old_after_info, new_after_info)

# Clarify that delays are independent by action.
text = text.replace('Mặc định 500ms; Sleep sau MỖI click để game không nhận thao tác quá nhanh.',
                    'Delay N chỉ áp dụng CLICK N; VỨT / SAU VỨT / X có delay riêng bên dưới.')
text = text.replace('Sẵn sàng. ESC = dừng khẩn. V4 SLEEP: mỗi RAW click đều chờ Delay ms trước bước tiếp theo.',
                    'Sẵn sàng. V4 SLEEP: CLICK N, VỨT, SAU VỨT và X đều có Delay riêng; 50-10000 ms.')

required = [
    'IDC_DELAY_DISCARD = 1070',
    'IDC_DELAY_AFTER_DISCARD = 1071',
    'IDC_DELAY_CLOSE = 1072',
    'DelayKind::Discard',
    'DelayKind::AfterDiscard',
    'DelayKind::Close',
    'discardClickDelayMs',
    'afterDiscardClickDelayMs',
    'closeClickDelayMs',
    'L"+ THÊM"',
    'L"- XÓA"',
]
for needle in required:
    if needle not in text:
        raise SystemExit(f'delay-controls required marker missing: {needle}')
if text.count('Sleep(static_cast<DWORD>(StepDelayMs(s)))') < 6:
    raise SystemExit('delay-controls patch must preserve Sleep after every gameplay click action')

p.write_text(text, encoding='utf-8')
print('FILTER v4 DELAY CONTROLS PASS • CLICK N / VỨT / SAU VỨT / X configurable independently')
