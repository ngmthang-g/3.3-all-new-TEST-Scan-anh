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

# User-approved v4 follow-up: keep 20 as the initial default only; remove all hard limits.
replacements = [
    ('constexpr std::size_t kMaxClickSteps = 20;',
     'constexpr std::size_t kDefaultInitialSteps = 20;'),
    ('    if(s.config.steps.size()>=kMaxClickSteps){SetStatus(s.hwnd,L"BỘ LỌC cố định 20 CLICK");return;}\n',
     ''),
    ('if(s.config.steps.size()!=kMaxClickSteps){error=L"phải có đúng 20 tọa click";return false;}',
     'if(s.config.steps.empty()){error=L"danh sách CLICK đang rỗng";return false;}'),
    ('if(s.slotIndex>=kMaxClickSteps){FinishRun(s,L"LỌC 20 CLICK HOÀN TẤT • 1 capture/probe • 3 ROI riêng • không Sleep khóa UI");return;}',
     'if(s.slotIndex>=s.config.steps.size()){FinishRun(s,L"LỌC "+std::to_wstring(s.config.steps.size())+L" CLICK HOÀN TẤT • 1 capture/probe • 3 ROI riêng • không Sleep khóa UI");return;}'),
    ('L"20 TỌA CLICK Ô ĐỒ • Timeout là thời gian tối đa chờ trạng thái, KHÔNG phải Sleep"',
     'L"DANH SÁCH CLICK Ô ĐỒ • mặc định 20 • +THÊM / -XÓA không giới hạn cứng • Timeout không phải Sleep"'),
    ('L"CHẠY TEST LỌC 20 CLICK ẨN • V4 OPTIMIZED"',
     'L"CHẠY TEST LỌC CLICK ẨN • V4 OPTIMIZED"'),
    ('L"F8: chọn một ô 1-20 trước"',
     'L"F8: chọn một dòng CLICK trước"'),
    ('State state{};state.target=target;state.config=g_lastConfig;RebaseForCurrentClient(state.config,target.gameWindow);if(state.config.steps.size()<kMaxClickSteps)state.config.steps.resize(kMaxClickSteps);if(state.config.steps.size()>kMaxClickSteps)state.config.steps.resize(kMaxClickSteps);',
     'State state{};state.target=target;state.config=g_lastConfig;RebaseForCurrentClient(state.config,target.gameWindow);if(state.config.steps.empty())state.config.steps.resize(kDefaultInitialSteps);'),
    ('L"TEST LỌC ĐỒ ẢNH ẨN • 20 CLICK • v3"',
     'L"TEST LỌC ĐỒ ẢNH ẨN • V4 OPTIMIZED • CLICK ĐỘNG"'),
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

# Safety: ignore config-changing button commands while the timer/state chain is running.
cmd_anchor = '        case WM_COMMAND:\n            switch(LOWORD(wp)){'
if cmd_anchor not in text:
    raise SystemExit('FILTER v4 WM_COMMAND anchor missing')
text = text.replace(cmd_anchor,
                    '        case WM_COMMAND:\n'
                    '            if(s->runningChain && LOWORD(wp)!=IDCANCEL){SetStatus(s->hwnd,L"Đang chạy FILTER v4 • ESC để dừng trước khi sửa cấu hình");return 0;}\n'
                    '            switch(LOWORD(wp)){', 1)

for forbidden in ['kMaxClickSteps', 'BỘ LỌC cố định 20 CLICK', 'phải có đúng 20 tọa click', 'LỌC 20 CLICK HOÀN TẤT']:
    if forbidden in text:
        raise SystemExit(f'FILTER v4 unlimited cleanup failed: {forbidden}')
for required in ['kDefaultInitialSteps = 20', 's.slotIndex>=s.config.steps.size()', 'không có giới hạn cứng']:
    if required not in text:
        raise SystemExit(f'FILTER v4 unlimited required marker missing: {required}')

p.write_text(text, encoding='utf-8')
print('FILTER v4 ROI editor fix PASS')
print('FILTER v4 unlimited click-list patch PASS • default 20, add/remove dynamic')
