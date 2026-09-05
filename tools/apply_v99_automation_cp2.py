from pathlib import Path
import argparse
import re

ap = argparse.ArgumentParser()
ap.add_argument('--source-root', required=True)
ap.add_argument('--output-dir', required=True)
a = ap.parse_args()
out = Path(a.output_dir)


def load(path: Path):
    raw = path.read_bytes()
    text = raw.decode('utf-8-sig')
    nl = '\r\n' if '\r\n' in text else '\n'
    return text.replace('\r\n', '\n').replace('\r', '\n'), nl


def save(path: Path, text: str, nl: str):
    path.write_bytes(text.replace('\n', nl).encode('utf-8'))


def once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f'{label}: expected one marker, found {count}')
    return text.replace(old, new, 1)


def regex_once(text: str, pattern: str, repl: str, label: str, flags=0) -> str:
    out_text, count = re.subn(pattern, repl, text, count=1, flags=flags)
    if count != 1:
        raise SystemExit(f'{label}: expected one regex marker, found {count}')
    return out_text


# ---------------------------------------------------------------------------
# Generated FILTER V5 UI/config. CP2 adds one independent image+ROI and one
# F8 coordinate. It does not alter GOOD/VUT/X slot logic.
# ---------------------------------------------------------------------------
cpp_path = out / 'image_scan_test.cpp'
cpp, cpp_nl = load(cpp_path)

cpp = once(cpp,
'''constexpr int IDC_PRE_CLOSE_FULL = 1114;\n''',
'''constexpr int IDC_PRE_CLOSE_FULL = 1114;\nconstexpr int IDC_PRECHECK_TEMPLATE = 1130;\nconstexpr int IDC_PRECHECK_PICK = 1131;\nconstexpr int IDC_PRECHECK_REGION = 1132;\nconstexpr int IDC_PRECHECK_PREVIEW = 1133;\nconstexpr int IDC_PRECHECK_FULL = 1134;\nconstexpr int IDC_PRECHECK_X = 1135;\nconstexpr int IDC_PRECHECK_Y = 1136;\nconstexpr int IDC_PRECHECK_DELAY = 1137;\nconstexpr int IDC_PRECHECK_CAPTURE = 1138;\n''', 'precheck ids')

cpp = regex_once(cpp,
    r'(\s*std::wstring closeTemplatePath;\n)',
    r'\1    std::wstring bagUiTemplatePath; // CP2: dấu hiệu đang ở giao diện Tay nải\n',
    'precheck template config')
cpp = once(cpp,
'''    ScanRoi preCloseRoi{};\n''',
'''    ScanRoi preCloseRoi{};\n    ScanRoi bagUiRoi{};\n''', 'precheck roi config')
cpp = regex_once(cpp,
    r'(\s*ClickStep closeBag;\n)',
    r'\1    ClickStep bagUiSwitch; // click chuyển về giao diện Tay nải khi PRECHECK miss\n',
    'precheck click config')
cpp = once(cpp,
'''    RebaseNamedRoi(c.preCloseRoi, cw, ch);\n''',
'''    RebaseNamedRoi(c.preCloseRoi, cw, ch);\n    RebaseNamedRoi(c.bagUiRoi, cw, ch);\n''', 'precheck roi rebase')
cpp = regex_once(cpp,
    r'(\s*RebaseClickStep\(c\.closeBag, cw, ch\);\n)',
    r'\1    RebaseClickStep(c.bagUiSwitch, cw, ch);\n',
    'precheck click rebase')

# State-local F8 capture is separate from slot/OPEN/CLOSE capture modes.
cpp = regex_once(cpp,
    r'(\s*bool f8WasDown = false;\n)',
    r'\1    bool precheckCaptureArmed = false;\n    bool precheckF8WasDown = false;\n',
    'precheck state flags')

# Sync text/X/Y/delay into config. ROI itself is edited directly by picker helpers.
cpp = once(cpp,
'''    s.config.closeTemplatePath = ReadText(s.hwnd, IDC_TEMPLATE_CLOSE);\n''',
'''    s.config.closeTemplatePath = ReadText(s.hwnd, IDC_TEMPLATE_CLOSE);\n    s.config.bagUiTemplatePath = ReadText(s.hwnd, IDC_PRECHECK_TEMPLATE);\n''', 'sync precheck path')

sync_anchor = '''    g_lastConfig = s.config;\n}\n\nvoid PickImageFile(State& s, int editId, std::wstring& dest) {\n'''
sync_block = '''    {\n        const int px = ReadInt(s.hwnd, IDC_PRECHECK_X, s.config.bagUiSwitch.x);\n        const int py = ReadInt(s.hwnd, IDC_PRECHECK_Y, s.config.bagUiSwitch.y);\n        const int delay = std::clamp(ReadInt(s.hwnd, IDC_PRECHECK_DELAY, s.config.bagUiSwitch.delayMs), 50, 10000);\n        int cw = 0, ch = 0;\n        if (CurrentClientSize(s.target.gameWindow, cw, ch) && px >= 0 && py >= 0 && px < cw && py < ch) {\n            s.config.bagUiSwitch.x = px; s.config.bagUiSwitch.y = py;\n            s.config.bagUiSwitch.baseW = cw; s.config.bagUiSwitch.baseH = ch;\n            s.config.bagUiSwitch.delayMs = delay; s.config.bagUiSwitch.repeat = 1; s.config.bagUiSwitch.valid = true;\n        }\n    }\n    g_lastConfig = s.config;\n}\n\nvoid PickImageFile(State& s, int editId, std::wstring& dest) {\n'''
cpp = once(cpp, sync_anchor, sync_block, 'sync precheck point')

cpp = once(cpp,
'''void PickCloseTemplate(State& s) { PickImageFile(s, IDC_TEMPLATE_CLOSE, s.config.closeTemplatePath); }\n''',
'''void PickCloseTemplate(State& s) { PickImageFile(s, IDC_TEMPLATE_CLOSE, s.config.closeTemplatePath); }\nvoid PickPrecheckTemplate(State& s) { PickImageFile(s, IDC_PRECHECK_TEMPLATE, s.config.bagUiTemplatePath); }\n''', 'precheck image picker')

cpp = once(cpp,
'''void ResetPreCloseRegion(State& s){int cw=0,ch=0;CurrentClientSize(s.target.gameWindow,cw,ch);s.config.preCloseRoi={};s.config.preCloseRoi.baseW=cw;s.config.preCloseRoi.baseH=ch;g_lastConfig=s.config;SetStatus(s.hwnd,L"ROI X TRƯỚC CLOSE BAG = FULL CLIENT");}\n''',
'''void ResetPreCloseRegion(State& s){int cw=0,ch=0;CurrentClientSize(s.target.gameWindow,cw,ch);s.config.preCloseRoi={};s.config.preCloseRoi.baseW=cw;s.config.preCloseRoi.baseH=ch;g_lastConfig=s.config;SetStatus(s.hwnd,L"ROI X TRƯỚC CLOSE BAG = FULL CLIENT");}\nvoid SelectPrecheckRegion(State& s){SelectNamedRegion(s,s.config.bagUiRoi,0,0,0,0,L"ROI PRECHECK TAY NẢI");}\nvoid PreviewPrecheckRegion(State& s){PreviewNamedRegion(s,s.config.bagUiRoi,L"ROI PRECHECK TAY NẢI");}\nvoid ResetPrecheckRegion(State& s){int cw=0,ch=0;CurrentClientSize(s.target.gameWindow,cw,ch);s.config.bagUiRoi={};s.config.bagUiRoi.baseW=cw;s.config.bagUiRoi.baseH=ch;g_lastConfig=s.config;SetStatus(s.hwnd,L"ROI PRECHECK TAY NẢI = FULL CLIENT");}\n\nvoid ArmPrecheckF8(State& s){\n    s.precheckCaptureArmed=true;s.precheckF8WasDown=(GetAsyncKeyState(VK_F8)&0x8000)!=0;\n    SetTimer(s.hwnd,kF8PollTimer,30,nullptr);\n    SetStatus(s.hwnd,L"PRECHECK TAY NẢI: đưa chuột vào tọa chuyển giao diện rồi F8");\n}\n''', 'precheck roi and f8 handlers')

# PRECHECK gets first refusal on the shared F8 timer; existing slot/open/close logic remains unchanged.
cpp = once(cpp,
'''void PollF8(State& s){\n    const bool down=(GetAsyncKeyState(VK_F8)&0x8000)!=0;\n''',
'''void PollF8(State& s){\n    const bool down=(GetAsyncKeyState(VK_F8)&0x8000)!=0;\n    if(s.precheckCaptureArmed){\n        if(down&&!s.precheckF8WasDown){\n            POINT p{};GetCursorPos(&p);POINT client=p;int cw=0,ch=0;\n            if(ScreenToClient(s.target.gameWindow,&client)&&CurrentClientSize(s.target.gameWindow,cw,ch)&&\n               client.x>=0&&client.y>=0&&client.x<cw&&client.y<ch){\n                ClickStep& step=s.config.bagUiSwitch;step.x=client.x;step.y=client.y;step.baseW=cw;step.baseH=ch;step.valid=true;step.repeat=1;\n                step.delayMs=std::clamp(ReadInt(s.hwnd,IDC_PRECHECK_DELAY,step.delayMs),50,10000);\n                SetEditInt(s.hwnd,IDC_PRECHECK_X,step.x);SetEditInt(s.hwnd,IDC_PRECHECK_Y,step.y);\n                s.precheckCaptureArmed=false;KillTimer(s.hwnd,kF8PollTimer);g_lastConfig=s.config;\n                SetStatus(s.hwnd,L"F8 PASS • PRECHECK TAY NẢI = "+std::to_wstring(step.x)+L","+std::to_wstring(step.y));\n            }else{SetStatus(s.hwnd,L"F8 PRECHECK: chuột chưa nằm trong client game");}\n        }\n        s.precheckF8WasDown=down;return;\n    }\n''', 'precheck f8 poll')

# Manual SCAN export/import.
cpp = regex_once(cpp,
    r'(\s*out<<"close_path="<<Utf8\(s\.config\.closeTemplatePath\)<<"\\n";\n)',
    r'\1    out<<"precheck_path="<<Utf8(s.config.bagUiTemplatePath)<<"\\n";\n',
    'manual export precheck path')
cpp = once(cpp,
'''    out<<"pre_close_roi="<<RoiLine(s.config.preCloseRoi.x,s.config.preCloseRoi.y,s.config.preCloseRoi.w,s.config.preCloseRoi.h,s.config.preCloseRoi.baseW,s.config.preCloseRoi.baseH)<<"\\n";\n''',
'''    out<<"pre_close_roi="<<RoiLine(s.config.preCloseRoi.x,s.config.preCloseRoi.y,s.config.preCloseRoi.w,s.config.preCloseRoi.h,s.config.preCloseRoi.baseW,s.config.preCloseRoi.baseH)<<"\\n";\n    out<<"precheck_roi="<<RoiLine(s.config.bagUiRoi.x,s.config.bagUiRoi.y,s.config.bagUiRoi.w,s.config.bagUiRoi.h,s.config.bagUiRoi.baseW,s.config.bagUiRoi.baseH)<<"\\n";\n    out<<"precheck_switch="<<StepLine(s.config.bagUiSwitch)<<"\\n";\n''', 'manual export precheck roi/step')
cpp = regex_once(cpp,
    r'(\s*else if\(key=="close_path"\)c\.closeTemplatePath=FromUtf8\(val\);\n)',
    r'\1        else if(key=="precheck_path")c.bagUiTemplatePath=FromUtf8(val);\n',
    'manual import precheck path')
cpp = once(cpp,
'''        else if(key=="pre_close_roi"){std::array<int,6>v{};if(ParseSix(val,v)){c.preCloseRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}}\n''',
'''        else if(key=="pre_close_roi"){std::array<int,6>v{};if(ParseSix(val,v)){c.preCloseRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}}\n        else if(key=="precheck_roi"){std::array<int,6>v{};if(ParseSix(val,v)){c.bagUiRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}}\n        else if(key=="precheck_switch")ParseStepLine(val,c.bagUiSwitch);\n''', 'manual import precheck roi/step')

# UI is appended below the existing AUTO section; existing controls keep their positions.
old_status = '''    Add(s.hwnd,L"STATIC",L"Sẵn sàng. AUTO CON chỉ chạy sau khi tới bãi + AutoFight ON; FULL finish current item → X popup one-shot → CloseBag → nhường điều phối.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,824,987,75,IDC_STATUS);\n    Add(s.hwnd,L"BUTTON",L"ĐÓNG",BS_PUSHBUTTON,885,912,120,30,IDCANCEL);\n'''
new_status = '''    Add(s.hwnd,L"BUTTON",L"PRECHECK TAY NẢI • 1 lần mỗi START • trước OPEN BAG",BS_GROUPBOX,18,824,987,118,0);\n    Add(s.hwnd,L"STATIC",L"Ảnh data:",SS_LEFT|SS_CENTERIMAGE,32,846,72,25,0);\n    Add(s.hwnd,L"EDIT",s.config.bagUiTemplatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,106,846,458,25,IDC_PRECHECK_TEMPLATE);\n    Add(s.hwnd,L"BUTTON",L"CHỌN ẢNH",BS_PUSHBUTTON,572,846,100,25,IDC_PRECHECK_PICK);\n    Add(s.hwnd,L"BUTTON",L"CHỌN ROI",BS_PUSHBUTTON,680,846,96,25,IDC_PRECHECK_REGION);\n    Add(s.hwnd,L"BUTTON",L"XEM ROI",BS_PUSHBUTTON,784,846,88,25,IDC_PRECHECK_PREVIEW);\n    Add(s.hwnd,L"BUTTON",L"FULL",BS_PUSHBUTTON,880,846,72,25,IDC_PRECHECK_FULL);\n    Add(s.hwnd,L"STATIC",L"Tọa chuyển UI  X",SS_LEFT|SS_CENTERIMAGE,32,880,105,25,0);\n    Add(s.hwnd,L"EDIT",s.config.bagUiSwitch.valid?std::to_wstring(s.config.bagUiSwitch.x).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,140,880,62,25,IDC_PRECHECK_X);\n    Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,207,880,18,25,0);\n    Add(s.hwnd,L"EDIT",s.config.bagUiSwitch.valid?std::to_wstring(s.config.bagUiSwitch.y).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,228,880,62,25,IDC_PRECHECK_Y);\n    Add(s.hwnd,L"STATIC",L"Delay ms",SS_LEFT|SS_CENTERIMAGE,300,880,62,25,0);\n    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.bagUiSwitch.delayMs).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,365,880,66,25,IDC_PRECHECK_DELAY);\n    Add(s.hwnd,L"BUTTON",L"LẤY F8 PRECHECK",BS_PUSHBUTTON,442,880,160,25,IDC_PRECHECK_CAPTURE);\n    Add(s.hwnd,L"STATIC",L"PASS ảnh → OPEN BAG; MISS ảnh → click tọa trên → OPEN BAG.",SS_LEFT|SS_CENTERIMAGE,615,880,350,25,0);\n\n    Add(s.hwnd,L"STATIC",L"Sẵn sàng. AUTO CON: PRECHECK Tay nải → OPEN BAG → scan; FULL finish current item → X popup one-shot → CloseBag.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,950,987,62,IDC_STATUS);\n    Add(s.hwnd,L"BUTTON",L"ĐÓNG",BS_PUSHBUTTON,885,1020,120,30,IDCANCEL);\n'''
cpp = once(cpp, old_status, new_status, 'precheck UI block')
cpp = once(cpp,
'''                              CW_USEDEFAULT,CW_USEDEFAULT,1000,980,target.owner,nullptr,GetModuleHandleW(nullptr),&state);\n''',
'''                              CW_USEDEFAULT,CW_USEDEFAULT,1000,1090,target.owner,nullptr,GetModuleHandleW(nullptr),&state);\n''', 'precheck dialog height')

cpp = once(cpp,
'''                case IDC_PRE_CLOSE_FULL:ResetPreCloseRegion(*s);return 0;\n''',
'''                case IDC_PRE_CLOSE_FULL:ResetPreCloseRegion(*s);return 0;\n                case IDC_PRECHECK_PICK:PickPrecheckTemplate(*s);return 0;\n                case IDC_PRECHECK_REGION:SelectPrecheckRegion(*s);return 0;\n                case IDC_PRECHECK_PREVIEW:PreviewPrecheckRegion(*s);return 0;\n                case IDC_PRECHECK_FULL:ResetPrecheckRegion(*s);return 0;\n                case IDC_PRECHECK_CAPTURE:ArmPrecheckF8(*s);return 0;\n''', 'precheck commands')

save(cpp_path, cpp, cpp_nl)


# ---------------------------------------------------------------------------
# AUTO runtime. PRECHECK is a START-session latch: ResetAutoFilter (new train,
# revive/life reset) does not clear it; StopAutoFilter erases the session, so
# the next explicit START performs PRECHECK again.
# ---------------------------------------------------------------------------
auto_path = out / 'image_scan_auto_ext.inl'
auto, auto_nl = load(auto_path)

auto = once(auto,
'''    Idle,\n    WaitOpen1,\n''',
'''    Idle,\n    WaitPrecheckSwitch,\n    WaitOpen1,\n''', 'precheck phase')
auto = once(auto,
'''    bool bagOpened = false;\n''',
'''    bool bagOpened = false;\n    bool startPrecheckCompleted = false; // intentionally survives ResetAutoFilter; Stop erases session\n''', 'precheck start latch')

# Persist the extra CP2 config in LOCALAPPDATA as well.
auto = regex_once(auto,
    r'(\s*out<<"good_path="<<Utf8\(c\.templatePath\)<<"\\n"<<"discard_path="<<Utf8\(c\.discardTemplatePath\)<<"\\n"<<"close_path="<<Utf8\(c\.closeTemplatePath\)<<"\\n";\n)',
    r'\1    out<<"precheck_path="<<Utf8(c.bagUiTemplatePath)<<"\\n";\n',
    'persistent precheck path export')
auto = once(auto,
'''    out<<"pre_close_roi="<<RoiLine(c.preCloseRoi.x,c.preCloseRoi.y,c.preCloseRoi.w,c.preCloseRoi.h,c.preCloseRoi.baseW,c.preCloseRoi.baseH)<<"\\n";\n''',
'''    out<<"pre_close_roi="<<RoiLine(c.preCloseRoi.x,c.preCloseRoi.y,c.preCloseRoi.w,c.preCloseRoi.h,c.preCloseRoi.baseW,c.preCloseRoi.baseH)<<"\\n";\n    out<<"precheck_roi="<<RoiLine(c.bagUiRoi.x,c.bagUiRoi.y,c.bagUiRoi.w,c.bagUiRoi.h,c.bagUiRoi.baseW,c.bagUiRoi.baseH)<<"\\n";\n    out<<"precheck_switch="<<StepLine(c.bagUiSwitch)<<"\\n";\n''', 'persistent precheck roi export')
auto = regex_once(auto,
    r'(\s*if\(key=="good_path"\)c\.templatePath=FromUtf8\(val\);else if\(key=="discard_path"\)c\.discardTemplatePath=FromUtf8\(val\);else if\(key=="close_path"\)c\.closeTemplatePath=FromUtf8\(val\);\n)',
    r'\1        else if(key=="precheck_path")c.bagUiTemplatePath=FromUtf8(val);\n',
    'persistent precheck path import')
auto = once(auto,
'''        else if(key=="pre_close_roi"){std::array<int,6>v{};if(ParseSix(val,v))c.preCloseRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}\n''',
'''        else if(key=="pre_close_roi"){std::array<int,6>v{};if(ParseSix(val,v))c.preCloseRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}\n        else if(key=="precheck_roi"){std::array<int,6>v{};if(ParseSix(val,v))c.bagUiRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}\n        else if(key=="precheck_switch")ParseStepLine(val,c.bagUiSwitch);\n''', 'persistent precheck roi import')

# Mandatory when AUTO FILTER is enabled: explicit error instead of silently scanning wrong UI.
auto = once(auto,
'''    if (s.scan.config.closeTemplatePath.empty()) { error=L"chưa chọn ảnh X"; return false; }\n''',
'''    if (s.scan.config.closeTemplatePath.empty()) { error=L"chưa chọn ảnh X"; return false; }\n    if (s.scan.config.bagUiTemplatePath.empty()) { error=L"PRECHECK TAY NẢI chưa chọn ảnh data"; return false; }\n    if (!s.scan.config.bagUiSwitch.valid) { error=L"PRECHECK TAY NẢI chưa gán tọa chuyển UI bằng F8"; return false; }\n''', 'precheck validation')

precheck_fn = '''\nbool AutoRunStartPrecheck(AutoSession& s, ULONGLONG now, std::wstring& error) {\n    if (s.startPrecheckCompleted) return AutoOpenNext(s,now,1,error);\n    Image tpl{};\n    if(!LoadImageWic(s.scan.config.bagUiTemplatePath,tpl,error)){error=L"PRECHECK TAY NẢI ảnh data • "+error;return false;}\n    Image frame{};std::wstring backend;\n    if(!CaptureClient(s.scan.target.gameWindow,frame,backend,error)){error=L"PRECHECK TAY NẢI capture • "+error;return false;}\n    int rx=0,ry=0,rw=0,rh=0;ResolveNamedRoi(s.scan.config.bagUiRoi,frame.width,frame.height,rx,ry,rw,rh);\n    Match m=FindTemplate(frame,tpl,rx,ry,rw,rh,static_cast<double>(std::clamp(s.scan.config.thresholdPercent,1,100))/100.0,error);\n    if(m.score<0.0){error=L"PRECHECK TAY NẢI scan ROI • "+error;return false;}\n    s.startPrecheckCompleted=true;\n    if(m.found){\n        s.status=L"PRECHECK TAY NẢI PASS • đúng giao diện → OPEN BAG";\n        return AutoOpenNext(s,now,1,error);\n    }\n    if(!AutoClickStep(s,s.scan.config.bagUiSwitch,error))return false;\n    AutoArm(s,AutoPhase::WaitPrecheckSwitch,now,static_cast<ULONGLONG>(std::clamp(s.scan.config.bagUiSwitch.delayMs,50,10000)),false);\n    s.status=L"PRECHECK TAY NẢI MISS • click chuyển giao diện → chờ OPEN BAG";\n    return true;\n}\n\n'''
auto = once(auto,
'''bool InitializeAutoSession(AutoSession& s, const Target& target, ULONGLONG now, std::wstring& error) {\n''',
precheck_fn + '''bool InitializeAutoSession(AutoSession& s, const Target& target, ULONGLONG now, std::wstring& error) {\n''', 'precheck runtime function')
auto = once(auto,
'''    if(!AutoOpenNext(s,now,1,error)){AutoSetError(s,error);return false;}\n''',
'''    if(!AutoRunStartPrecheck(s,now,error)){AutoSetError(s,error);return false;}\n''', 'precheck before open bag')
auto = once(auto,
'''    if(s.phase==AutoPhase::WaitOpen1){\n''',
'''    if(s.phase==AutoPhase::WaitPrecheckSwitch){\n        if(s.fullExitPending){if(!AutoStartClose(s,now,AutoClosePurpose::Full,error))AutoSetError(s,error);return MakeAutoResult(s);}\n        if(!AutoOpenNext(s,now,1,error))AutoSetError(s,error);return MakeAutoResult(s);\n    }\n    if(s.phase==AutoPhase::WaitOpen1){\n''', 'precheck wait phase')

required = [
    'bagUiTemplatePath', 'bagUiRoi', 'bagUiSwitch', 'PRECHECK TAY NẢI',
    'WaitPrecheckSwitch', 'startPrecheckCompleted', 'AutoRunStartPrecheck',
]
for token in required:
    if token not in cpp + auto:
        raise SystemExit('CP2 final assertion missing: ' + token)
# The START latch must not be zeroed by generic reset; only object creation/erase controls it.
if 's.startPrecheckCompleted = false' in auto or 's.startPrecheckCompleted=false' in auto:
    raise SystemExit('CP2 final assertion: generic runtime reset clears START precheck latch')

save(auto_path, auto, auto_nl)
print('apply_v99_automation_cp2.py: PASS')
