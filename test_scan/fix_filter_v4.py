from pathlib import Path

p = Path('src/image_scan_filter_v4.inl')
if not p.exists():
    raise SystemExit('missing src/image_scan_filter_v4.inl')
text = p.read_text(encoding='utf-8')

start = text.find('void AddRoiEditors(State& s')
end = text.find('void BuildControls(State& s)', start)
if start < 0 or end < 0:
    raise SystemExit('cannot locate AddRoiEditors block')

fixed = r'''void AddRoiEditors(State& s,int y,int xId,int yId,int wId,int hId,int pickId,int previewId,int fullId,
                   int roiX,int roiY,int roiW,int roiH,const wchar_t* label){
    const int left=18;
    Add(s.hwnd,L"STATIC",label,SS_LEFT|SS_CENTERIMAGE,left,y,80,24,0);
    Add(s.hwnd,L"STATIC",L"X",SS_CENTER|SS_CENTERIMAGE,left+82,y,16,24,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(roiX).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,left+98,y,54,24,xId);
    Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,left+154,y,16,24,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(roiY).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,left+170,y,54,24,yId);
    Add(s.hwnd,L"STATIC",L"W",SS_CENTER|SS_CENTERIMAGE,left+226,y,16,24,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(roiW).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,left+242,y,58,24,wId);
    Add(s.hwnd,L"STATIC",L"H",SS_CENTER|SS_CENTERIMAGE,left+302,y,16,24,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(roiH).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,left+318,y,58,24,hId);
    Add(s.hwnd,L"BUTTON",L"CHỌN VÙNG",BS_PUSHBUTTON,left+386,y-2,112,28,pickId);
    Add(s.hwnd,L"BUTTON",L"XEM",BS_PUSHBUTTON,left+504,y-2,68,28,previewId);
    Add(s.hwnd,L"BUTTON",L"FULL",BS_PUSHBUTTON,left+578,y-2,68,28,fullId);
}

'''
text = text[:start] + fixed + text[end:]

# User-approved dynamic click list: keep 20 only as the initial default, never as a hard cap.
replacements = [
    ('constexpr std::size_t kMaxClickSteps = 20;',
     'constexpr std::size_t kDefaultInitialSteps = 20;'),
    ('    if(s.config.steps.size()>=kMaxClickSteps){SetStatus(s.hwnd,L"BỘ LỌC cố định 20 CLICK");return;}\n',
     ''),
    ('if(s.config.steps.size()!=kMaxClickSteps){error=L"phải có đúng 20 tọa click";return false;}',
     'if(s.config.steps.empty()){error=L"danh sách CLICK đang rỗng";return false;}'),
    ('if(s.slotIndex>=kMaxClickSteps){FinishRun(s,L"LỌC 20 CLICK HOÀN TẤT • 1 capture/probe • 3 ROI riêng • không Sleep khóa UI");return;}',
     'if(s.slotIndex>=s.config.steps.size()){FinishRun(s,L"LỌC "+std::to_wstring(s.config.steps.size())+L" CLICK HOÀN TẤT • 1 capture/probe • 3 ROI riêng • Sleep sau mỗi thao tác");return;}'),
    ('L"20 TỌA CLICK Ô ĐỒ • Timeout là thời gian tối đa chờ trạng thái, KHÔNG phải Sleep"',
     'L"DANH SÁCH CLICK Ô ĐỒ • mặc định 20 • +THÊM / -XÓA không giới hạn cứng • Delay = Sleep sau mỗi thao tác"'),
    ('L"CHẠY TEST LỌC 20 CLICK ẨN • V4 OPTIMIZED"',
     'L"CHẠY TEST LỌC CLICK ẨN • V4 SLEEP"'),
    ('L"F8: chọn một ô 1-20 trước"',
     'L"F8: chọn một dòng CLICK trước"'),
    ('State state{};state.target=target;state.config=g_lastConfig;RebaseForCurrentClient(state.config,target.gameWindow);if(state.config.steps.size()<kMaxClickSteps)state.config.steps.resize(kMaxClickSteps);if(state.config.steps.size()>kMaxClickSteps)state.config.steps.resize(kMaxClickSteps);',
     'State state{};state.target=target;state.config=g_lastConfig;RebaseForCurrentClient(state.config,target.gameWindow);if(state.config.steps.empty())state.config.steps.resize(kDefaultInitialSteps);'),
    ('L"TEST LỌC ĐỒ ẢNH ẨN • 20 CLICK • v3"',
     'L"TEST LỌC ĐỒ ẢNH ẨN • V4 SLEEP • CLICK ĐỘNG"'),
]
for old, new in replacements:
    if old not in text:
        raise SystemExit(f'FILTER v4 unlimited anchor missing: {old[:80]}')
    text = text.replace(old, new, 1)

# Make +THÊM feedback explicit and keep runtime list mutations out while running.
anchor = '    s.config.steps.push_back(step);g_lastConfig=s.config;RefreshStepList(s,static_cast<int>(s.config.steps.size()-1));\n}'
if anchor not in text:
    raise SystemExit('FILTER v4 add-step tail missing')
text = text.replace(anchor,
                    '    s.config.steps.push_back(step);g_lastConfig=s.config;RefreshStepList(s,static_cast<int>(s.config.steps.size()-1));\n'
                    '    SetStatus(s.hwnd,L"Đã thêm CLICK "+std::to_wstring(s.config.steps.size())+L" • không có giới hạn cứng");\n}', 1)

cmd_anchor = '        case WM_COMMAND:\n            switch(LOWORD(wp)){'
if cmd_anchor not in text:
    raise SystemExit('FILTER v4 WM_COMMAND anchor missing')
text = text.replace(cmd_anchor,
                    '        case WM_COMMAND:\n'
                    '            if(s->runningChain && LOWORD(wp)!=IDCANCEL){SetStatus(s->hwnd,L"Đang chạy FILTER v4 • ESC để dừng trước khi sửa cấu hình");return 0;}\n'
                    '            switch(LOWORD(wp)){', 1)

# User-requested rollback: fixed Sleep delay after EVERY click/action.
# Keep the optimized 1-capture/probe + 3-ROI scan path; only action pacing goes back to fixed delay.
sleep_replacements = [
    ('    int delayMs = 1500;', '    int delayMs = 500;'),
    ('step.delayMs = std::clamp(ReadInt(s.hwnd, IDC_STEP_DELAY, step.delayMs), 300, 10000);',
     'step.delayMs = std::clamp(ReadInt(s.hwnd, IDC_STEP_DELAY, step.delayMs), 50, 10000);'),
    ('if(step.delayMs<300||step.delayMs>10000){error=L"Delay CLICK "+std::to_wstring(i+1)+L" không hợp lệ";return false;}',
     'if(step.delayMs<50||step.delayMs>10000){error=L"Delay CLICK "+std::to_wstring(i+1)+L" không hợp lệ (50-10000 ms)";return false;}'),
    ('int SlotTimeoutMs(const State& s){\n    if(s.slotIndex<s.config.steps.size())return std::clamp(s.config.steps[s.slotIndex].delayMs,300,10000);\n    return 1500;\n}',
     'int StepDelayMs(const State& s){\n    if(s.slotIndex<s.config.steps.size())return std::clamp(s.config.steps[s.slotIndex].delayMs,50,10000);\n    return 500;\n}\n\nconstexpr UINT kStateTimeoutMs = 5000;'),
    ('const ULONGLONG now=GetTickCount64();s.runPhase=phase;s.nextProbeTick=now+firstProbeDelay;s.phaseDeadlineTick=now+static_cast<ULONGLONG>(SlotTimeoutMs(s));',
     'const ULONGLONG now=GetTickCount64();s.runPhase=phase;s.nextProbeTick=now+firstProbeDelay;s.phaseDeadlineTick=now+static_cast<ULONGLONG>(kStateTimeoutMs);'),
    ('AddColumn(s.stepList,0,45,L"#");AddColumn(s.stepList,1,80,L"X");AddColumn(s.stepList,2,80,L"Y");AddColumn(s.stepList,3,110,L"Base size");AddColumn(s.stepList,4,100,L"Timeout ms");AddColumn(s.stepList,5,515,L"Logic");',
     'AddColumn(s.stepList,0,45,L"#");AddColumn(s.stepList,1,80,L"X");AddColumn(s.stepList,2,80,L"Y");AddColumn(s.stepList,3,110,L"Base size");AddColumn(s.stepList,4,100,L"Delay ms");AddColumn(s.stepList,5,515,L"Logic");'),
    ('Add(s.hwnd,L"STATIC",L"Timeout:",SS_LEFT|SS_CENTERIMAGE,222,480,55,27,0);Add(s.hwnd,L"EDIT",L"1500",WS_BORDER|ES_NUMBER|ES_CENTER,279,480,70,27,IDC_STEP_DELAY);',
     'Add(s.hwnd,L"STATIC",L"Delay:",SS_LEFT|SS_CENTERIMAGE,222,480,55,27,0);Add(s.hwnd,L"EDIT",L"500",WS_BORDER|ES_NUMBER|ES_CENTER,279,480,70,27,IDC_STEP_DELAY);'),
    ('Add(s.hwnd,L"STATIC",L"Mặc định timeout 1500ms; máy nhanh thấy UI sớm thì xử lý ngay, không chờ đủ.",SS_LEFT|SS_CENTERIMAGE,752,480,235,27,0);',
     'Add(s.hwnd,L"STATIC",L"Mặc định 500ms; Sleep sau MỖI click để game không nhận thao tác quá nhanh.",SS_LEFT|SS_CENTERIMAGE,752,480,235,27,0);'),
    ('Add(s.hwnd,L"STATIC",L"Sẵn sàng. ESC = dừng khẩn. Không Sleep khóa UI; timer chỉ probe khi đến lượt.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,632,987,90,IDC_STATUS);',
     'Add(s.hwnd,L"STATIC",L"Sẵn sàng. ESC = dừng khẩn. V4 SLEEP: mỗi RAW click đều chờ Delay ms trước bước tiếp theo.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,632,987,90,IDC_STATUS);'),
    ('SetStatus(s.hwnd,L"V4 START • CLICK 1 → probe theo trạng thái • 1 PrintWindow/probe • ROI ẢNH1/VỨT/X riêng • Timeout fail-closed");',
     'SetStatus(s.hwnd,L"V4 SLEEP START • CLICK 1 → Sleep Delay → probe 1 frame • ROI ẢNH1/VỨT/X riêng");'),
]
for old, new in sleep_replacements:
    if old not in text:
        raise SystemExit(f'FILTER v4 Sleep anchor missing: {old[:100]}')
    text = text.replace(old, new, 1)

# Insert Sleep after every gameplay click action. Scans stay on the optimized shared frame.
action_replacements = [
    ('if(!RawClick(s,cx,cy,frame.width,frame.height,error)){FinishRun(s,L"CLICK TÂM X FAIL • "+error);return;}\n                SetStatus',
     'if(!RawClick(s,cx,cy,frame.width,frame.height,error)){FinishRun(s,L"CLICK TÂM X FAIL • "+error);return;}\n                Sleep(static_cast<DWORD>(StepDelayMs(s)));\n                SetStatus'),
    ('if(!RawClick(s,dx,dy,frame.width,frame.height,error)){FinishRun(s,L"CLICK TÂM VỨT FAIL • "+error);return;}\n            SetStatus',
     'if(!RawClick(s,dx,dy,frame.width,frame.height,error)){FinishRun(s,L"CLICK TÂM VỨT FAIL • "+error);return;}\n            Sleep(static_cast<DWORD>(StepDelayMs(s)));\n            SetStatus'),
    ('if(!ClickAfterDiscard(s,error)){FinishRun(s,L"CLICK SAU VỨT FAIL • "+error);return;}\n            SetStatus',
     'if(!ClickAfterDiscard(s,error)){FinishRun(s,L"CLICK SAU VỨT FAIL • "+error);return;}\n            Sleep(static_cast<DWORD>(StepDelayMs(s)));\n            SetStatus'),
    ('if(!ClickCurrentSlot(s,error)){FinishRun(s,L"LẶP CLICK "+std::to_wstring(s.slotIndex+1)+L" FAIL • "+error);return;}\n            SetStatus',
     'if(!ClickCurrentSlot(s,error)){FinishRun(s,L"LẶP CLICK "+std::to_wstring(s.slotIndex+1)+L" FAIL • "+error);return;}\n            Sleep(static_cast<DWORD>(StepDelayMs(s)));\n            SetStatus'),
    ('if(!ClickCurrentSlot(s,error)){FinishRun(s,L"CLICK "+std::to_wstring(s.slotIndex+1)+L" FAIL • "+error);return;}\n            SetStatus(s.hwnd,L"DẤU X đã biến mất',
     'if(!ClickCurrentSlot(s,error)){FinishRun(s,L"CLICK "+std::to_wstring(s.slotIndex+1)+L" FAIL • "+error);return;}\n            Sleep(static_cast<DWORD>(StepDelayMs(s)));\n            SetStatus(s.hwnd,L"DẤU X đã biến mất'),
    ('if(!ClickCurrentSlot(s,error)){FinishRun(s,L"CLICK 1 FAIL • "+error);return;}\n    ArmRunPhase',
     'if(!ClickCurrentSlot(s,error)){FinishRun(s,L"CLICK 1 FAIL • "+error);return;}\n    Sleep(static_cast<DWORD>(StepDelayMs(s)));\n    ArmRunPhase'),
]
for old, new in action_replacements:
    if old not in text:
        raise SystemExit(f'FILTER v4 action Sleep anchor missing: {old[:110]}')
    text = text.replace(old, new, 1)

# Update remaining UI/status wording from adaptive/no-Sleep terminology.
text = text.replace('Timeout là thời gian tối đa chờ trạng thái', 'Delay là Sleep sau mỗi thao tác')
text = text.replace('Timeout fail-closed', 'Sleep theo Delay + fail-closed')
text = text.replace('không Sleep khóa UI', 'Sleep sau mỗi thao tác')
text = text.replace('Timeout ms', 'Delay ms')

for forbidden in ['kMaxClickSteps', 'BỘ LỌC cố định 20 CLICK', 'phải có đúng 20 tọa click', 'LỌC 20 CLICK HOÀN TẤT', 'SlotTimeoutMs(']:
    if forbidden in text:
        raise SystemExit(f'FILTER v4 cleanup failed: {forbidden}')
for required in [
    'kDefaultInitialSteps = 20',
    's.slotIndex>=s.config.steps.size()',
    'không có giới hạn cứng',
    'int StepDelayMs(',
    'kStateTimeoutMs = 5000',
    'Sleep(static_cast<DWORD>(StepDelayMs(s)))',
    'Delay ms',
    'V4 SLEEP',
]:
    if required not in text:
        raise SystemExit(f'FILTER v4 required marker missing: {required}')
if text.count('Sleep(static_cast<DWORD>(StepDelayMs(s)))') < 6:
    raise SystemExit('FILTER v4 expected Sleep after every action')

p.write_text(text, encoding='utf-8')
print('FILTER v4 ROI editor fix PASS')
print('FILTER v4 dynamic click-list PASS • default 20, add/remove unlimited')
print('FILTER v4 SLEEP PASS • fixed per-action delay restored • default 500ms')
