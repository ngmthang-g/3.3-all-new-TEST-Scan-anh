from pathlib import Path
import argparse

ap=argparse.ArgumentParser()
ap.add_argument('--source-root', required=True)
ap.add_argument('--output-dir', required=True)
a=ap.parse_args()
out=Path(a.output_dir)

def load(path):
    raw=path.read_bytes(); text=raw.decode('utf-8-sig')
    nl='\r\n' if '\r\n' in text else '\n'
    return text.replace('\r\n','\n').replace('\r','\n'), nl

def save(path,text,nl): path.write_bytes(text.replace('\n',nl).encode('utf-8'))

def once(text,old,new,label):
    c=text.count(old)
    if c!=1: raise SystemExit(f'{label}: expected one marker, found {c}')
    return text.replace(old,new,1)

# ---- generated FILTER V5 core / config / Developer UI ----
cpp_path=out/'image_scan_test.cpp'; cpp,nl=load(cpp_path)
cpp=once(cpp,
'''constexpr int IDC_PRE_CLOSE_FULL = 1114;\n''',
'''constexpr int IDC_PRE_CLOSE_FULL = 1114;\nconstexpr int IDC_PRECHECK_TEMPLATE = 1130;\nconstexpr int IDC_PRECHECK_PICK = 1131;\nconstexpr int IDC_PRECHECK_REGION = 1132;\nconstexpr int IDC_PRECHECK_PREVIEW = 1133;\nconstexpr int IDC_PRECHECK_FULL = 1134;\nconstexpr int IDC_PRECHECK_X = 1135;\nconstexpr int IDC_PRECHECK_Y = 1136;\nconstexpr int IDC_PRECHECK_DELAY = 1137;\nconstexpr int IDC_PRECHECK_CAPTURE = 1138;\n''','precheck ids')
cpp=once(cpp,
'''    std::wstring closeTemplatePath;\n''',
'''    std::wstring closeTemplatePath;\n    // CP2: ảnh chỉ dùng để xác nhận client đang ở đúng giao diện Tay nải trước OPEN BAG.\n    std::wstring bagUiTemplatePath;\n''','precheck template field')
cpp=once(cpp,
'''    ScanRoi preCloseRoi{};\n''',
'''    ScanRoi preCloseRoi{};\n    ScanRoi bagUiRoi{};\n''','precheck roi field')
cpp=once(cpp,
'''    ClickStep closeBag;\n''',
'''    ClickStep closeBag;\n    // Nếu PRECHECK không thấy ảnh Tay nải, click ẩn tọa này rồi mới chạy OPEN BAG cũ.\n    ClickStep bagUiSwitch;\n''','precheck click field')
cpp=once(cpp,
'''    RebaseNamedRoi(c.preCloseRoi, cw, ch);\n    for (ClickStep& step : c.steps) RebaseClickStep(step, cw, ch);\n    RebaseClickStep(c.afterDiscard, cw, ch);\n''',
'''    RebaseNamedRoi(c.preCloseRoi, cw, ch);\n    RebaseNamedRoi(c.bagUiRoi, cw, ch);\n    for (ClickStep& step : c.steps) RebaseClickStep(step, cw, ch);\n    RebaseClickStep(c.afterDiscard, cw, ch);\n    RebaseClickStep(c.bagUiSwitch, cw, ch);\n''','precheck rebase')

cpp=once(cpp,
'''    s.config.closeTemplatePath = ReadText(s.hwnd, IDC_TEMPLATE_CLOSE);\n''',
'''    s.config.closeTemplatePath = ReadText(s.hwnd, IDC_TEMPLATE_CLOSE);\n    s.config.bagUiTemplatePath = ReadText(s.hwnd, IDC_PRECHECK_TEMPLATE);\n''','sync precheck path')
cpp=once(cpp,
'''    const int closeBagX = ReadInt(s.hwnd, IDC_BAG_CLOSE_X, s.config.closeBag.x);\n    const int closeBagY = ReadInt(s.hwnd, IDC_BAG_CLOSE_Y, s.config.closeBag.y);\n    s.config.openBag1.delayMs = std::clamp(ReadInt(s.hwnd, IDC_OPEN1_DELAY, s.config.openBag1.delayMs), 50, 10000);\n''',
'''    const int closeBagX = ReadInt(s.hwnd, IDC_BAG_CLOSE_X, s.config.closeBag.x);\n    const int closeBagY = ReadInt(s.hwnd, IDC_BAG_CLOSE_Y, s.config.closeBag.y);\n    const int precheckX = ReadInt(s.hwnd, IDC_PRECHECK_X, s.config.bagUiSwitch.x);\n    const int precheckY = ReadInt(s.hwnd, IDC_PRECHECK_Y, s.config.bagUiSwitch.y);\n    s.config.openBag1.delayMs = std::clamp(ReadInt(s.hwnd, IDC_OPEN1_DELAY, s.config.openBag1.delayMs), 50, 10000);\n''','sync precheck coordinates')
cpp=once(cpp,
'''    s.config.closeBag.delayMs = std::clamp(ReadInt(s.hwnd, IDC_BAG_CLOSE_DELAY, s.config.closeBag.delayMs), 50, 10000);\n''',
'''    s.config.closeBag.delayMs = std::clamp(ReadInt(s.hwnd, IDC_BAG_CLOSE_DELAY, s.config.closeBag.delayMs), 50, 10000);\n    s.config.bagUiSwitch.delayMs = std::clamp(ReadInt(s.hwnd, IDC_PRECHECK_DELAY, s.config.bagUiSwitch.delayMs), 50, 10000);\n''','sync precheck delay')
cpp=once(cpp,
'''        s.config.closeRoi.baseW = cw;\n        s.config.closeRoi.baseH = ch;\n''',
'''        s.config.closeRoi.baseW = cw;\n        s.config.closeRoi.baseH = ch;\n        s.config.bagUiRoi.baseW = cw;\n        s.config.bagUiRoi.baseH = ch;\n''','sync precheck roi base')
cpp=once(cpp,
'''        syncPoint(s.config.closeBag, closeBagX, closeBagY, true);\n''',
'''        syncPoint(s.config.closeBag, closeBagX, closeBagY, true);\n        syncPoint(s.config.bagUiSwitch, precheckX, precheckY, true);\n''','sync precheck point')
cpp=once(cpp,
'''void PickCloseTemplate(State& s) { PickImageFile(s, IDC_TEMPLATE_CLOSE, s.config.closeTemplatePath); }\n''',
'''void PickCloseTemplate(State& s) { PickImageFile(s, IDC_TEMPLATE_CLOSE, s.config.closeTemplatePath); }\nvoid PickPrecheckTemplate(State& s) { PickImageFile(s, IDC_PRECHECK_TEMPLATE, s.config.bagUiTemplatePath); }\n''','precheck image picker')

# refresh imported settings
cpp=once(cpp,
'''    setPoint(IDC_BAG_CLOSE_X,IDC_BAG_CLOSE_Y,IDC_BAG_CLOSE_DELAY,s.config.closeBag);\n''',
'''    setPoint(IDC_BAG_CLOSE_X,IDC_BAG_CLOSE_Y,IDC_BAG_CLOSE_DELAY,s.config.closeBag);\n    setPoint(IDC_PRECHECK_X,IDC_PRECHECK_Y,IDC_PRECHECK_DELAY,s.config.bagUiSwitch);\n''','refresh precheck point')
cpp=once(cpp,
'''    SetDlgItemTextW(s.hwnd,IDC_TEMPLATE_CLOSE,s.config.closeTemplatePath.c_str());\n''',
'''    SetDlgItemTextW(s.hwnd,IDC_TEMPLATE_CLOSE,s.config.closeTemplatePath.c_str());\n    SetDlgItemTextW(s.hwnd,IDC_PRECHECK_TEMPLATE,s.config.bagUiTemplatePath.c_str());\n''','refresh precheck image')

# manual export/import copies all four images beside .tlscan, keeping config portable
cpp=once(cpp,
'''    const auto close=CopyTemplateBesideConfig(cfgPath,s.config.closeTemplatePath,L"_close",error);if(close.empty()){SetStatus(s.hwnd,L"XUẤT SCAN FAIL • "+error);return;}\n''',
'''    const auto close=CopyTemplateBesideConfig(cfgPath,s.config.closeTemplatePath,L"_close",error);if(close.empty()){SetStatus(s.hwnd,L"XUẤT SCAN FAIL • "+error);return;}\n    const auto precheck=CopyTemplateBesideConfig(cfgPath,s.config.bagUiTemplatePath,L"_precheck",error);if(precheck.empty()){SetStatus(s.hwnd,L"XUẤT SCAN FAIL • "+error);return;}\n''','manual export precheck copy')
cpp=once(cpp,
'''    out<<"good_file="<<Utf8(good.filename().wstring())<<"\\n"<<"discard_file="<<Utf8(discard.filename().wstring())<<"\\n"<<"close_file="<<Utf8(close.filename().wstring())<<"\\n";\n''',
'''    out<<"good_file="<<Utf8(good.filename().wstring())<<"\\n"<<"discard_file="<<Utf8(discard.filename().wstring())<<"\\n"<<"close_file="<<Utf8(close.filename().wstring())<<"\\n";\n    out<<"precheck_file="<<Utf8(precheck.filename().wstring())<<"\\n";\n''','manual export precheck file')
cpp=once(cpp,
'''    out<<"pre_close_roi="<<RoiLine(s.config.preCloseRoi.x,s.config.preCloseRoi.y,s.config.preCloseRoi.w,s.config.preCloseRoi.h,s.config.preCloseRoi.baseW,s.config.preCloseRoi.baseH)<<"\\n";\n''',
'''    out<<"pre_close_roi="<<RoiLine(s.config.preCloseRoi.x,s.config.preCloseRoi.y,s.config.preCloseRoi.w,s.config.preCloseRoi.h,s.config.preCloseRoi.baseW,s.config.preCloseRoi.baseH)<<"\\n";\n    out<<"precheck_roi="<<RoiLine(s.config.bagUiRoi.x,s.config.bagUiRoi.y,s.config.bagUiRoi.w,s.config.bagUiRoi.h,s.config.bagUiRoi.baseW,s.config.bagUiRoi.baseH)<<"\\n";\n    out<<"precheck_switch="<<StepLine(s.config.bagUiSwitch)<<"\\n";\n''','manual export precheck settings')
cpp=once(cpp,
'''    out.close();SetStatus(s.hwnd,L"XUẤT SCAN PASS • config + 3 ảnh mẫu đã copy cạnh file .tlscan");\n''',
'''    out.close();SetStatus(s.hwnd,L"XUẤT SCAN PASS • config + 4 ảnh mẫu đã copy cạnh file .tlscan");\n''','manual export status')
cpp=once(cpp,
'''        else if(key=="close_file")c.closeTemplatePath=(cfgPath.parent_path()/std::filesystem::path(FromUtf8(val))).wstring();\n''',
'''        else if(key=="close_file")c.closeTemplatePath=(cfgPath.parent_path()/std::filesystem::path(FromUtf8(val))).wstring();\n        else if(key=="precheck_file")c.bagUiTemplatePath=(cfgPath.parent_path()/std::filesystem::path(FromUtf8(val))).wstring();\n''','manual import precheck file')
cpp=once(cpp,
'''        else if(key=="pre_close_roi"){std::array<int,6>v{};if(ParseSix(val,v)){c.preCloseRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}}\n''',
'''        else if(key=="pre_close_roi"){std::array<int,6>v{};if(ParseSix(val,v)){c.preCloseRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}}\n        else if(key=="precheck_roi"){std::array<int,6>v{};if(ParseSix(val,v)){c.bagUiRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}}\n        else if(key=="precheck_switch")ParseStepLine(val,c.bagUiSwitch);\n''','manual import precheck settings')

# ROI handlers reuse the same proven picker/preview helpers
cpp=once(cpp,
'''void ResetPreCloseRegion(State& s){int cw=0,ch=0;CurrentClientSize(s.target.gameWindow,cw,ch);s.config.preCloseRoi={};s.config.preCloseRoi.baseW=cw;s.config.preCloseRoi.baseH=ch;g_lastConfig=s.config;SetStatus(s.hwnd,L"ROI X TRƯỚC CLOSE BAG = FULL CLIENT");}\n''',
'''void ResetPreCloseRegion(State& s){int cw=0,ch=0;CurrentClientSize(s.target.gameWindow,cw,ch);s.config.preCloseRoi={};s.config.preCloseRoi.baseW=cw;s.config.preCloseRoi.baseH=ch;g_lastConfig=s.config;SetStatus(s.hwnd,L"ROI X TRƯỚC CLOSE BAG = FULL CLIENT");}\nvoid SelectPrecheckRegion(State& s){SelectNamedRegion(s,s.config.bagUiRoi,0,0,0,0,L"ROI PRECHECK TAY NẢI");}\nvoid PreviewPrecheckRegion(State& s){PreviewNamedRegion(s,s.config.bagUiRoi,L"ROI PRECHECK TAY NẢI");}\nvoid ResetPrecheckRegion(State& s){int cw=0,ch=0;CurrentClientSize(s.target.gameWindow,cw,ch);s.config.bagUiRoi={};s.config.bagUiRoi.baseW=cw;s.config.bagUiRoi.baseH=ch;g_lastConfig=s.config;SetStatus(s.hwnd,L"ROI PRECHECK TAY NẢI = FULL CLIENT");}\n''','precheck roi handlers')

# extend the existing proven negative-code F8 capture (-6 = PRECHECK switch)
cpp=once(cpp,
'''            }else if(s.captureRow==-3||s.captureRow==-4||s.captureRow==-5){\n                ClickStep* step=s.captureRow==-3?&s.config.openBag1:(s.captureRow==-4?&s.config.openBag2:&s.config.closeBag);\n                const int xId=s.captureRow==-3?IDC_OPEN1_X:(s.captureRow==-4?IDC_OPEN2_X:IDC_BAG_CLOSE_X);\n                const int yId=s.captureRow==-3?IDC_OPEN1_Y:(s.captureRow==-4?IDC_OPEN2_Y:IDC_BAG_CLOSE_Y);\n                const wchar_t* label=s.captureRow==-3?L"OPEN 1":(s.captureRow==-4?L"OPEN 2":L"CLOSE BAG");\n''',
'''            }else if(s.captureRow==-3||s.captureRow==-4||s.captureRow==-5||s.captureRow==-6){\n                ClickStep* step=s.captureRow==-3?&s.config.openBag1:(s.captureRow==-4?&s.config.openBag2:(s.captureRow==-5?&s.config.closeBag:&s.config.bagUiSwitch));\n                const int xId=s.captureRow==-3?IDC_OPEN1_X:(s.captureRow==-4?IDC_OPEN2_X:(s.captureRow==-5?IDC_BAG_CLOSE_X:IDC_PRECHECK_X));\n                const int yId=s.captureRow==-3?IDC_OPEN1_Y:(s.captureRow==-4?IDC_OPEN2_Y:(s.captureRow==-5?IDC_BAG_CLOSE_Y:IDC_PRECHECK_Y));\n                const wchar_t* label=s.captureRow==-3?L"OPEN 1":(s.captureRow==-4?L"OPEN 2":(s.captureRow==-5?L"CLOSE BAG":L"PRECHECK TAY NẢI"));\n''','precheck F8 mode')

# UI appended under existing auto scanner controls, inside password-protected V4 dialog
cpp=once(cpp,
'''    Add(s.hwnd,L"STATIC",L"Sẵn sàng. AUTO CON chỉ chạy sau khi tới bãi + AutoFight ON; FULL finish current item → X popup one-shot → CloseBag → nhường điều phối.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,824,987,75,IDC_STATUS);\n    Add(s.hwnd,L"BUTTON",L"ĐÓNG",BS_PUSHBUTTON,885,912,120,30,IDCANCEL);\n''',
'''    Add(s.hwnd,L"BUTTON",L"PRECHECK TAY NẢI • 1 lần mỗi START • trước OPEN BAG",BS_GROUPBOX,18,824,987,118,0);\n    Add(s.hwnd,L"STATIC",L"Ảnh data:",SS_LEFT|SS_CENTERIMAGE,32,846,72,25,0);\n    Add(s.hwnd,L"EDIT",s.config.bagUiTemplatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,106,846,458,25,IDC_PRECHECK_TEMPLATE);\n    Add(s.hwnd,L"BUTTON",L"CHỌN ẢNH",BS_PUSHBUTTON,572,846,100,25,IDC_PRECHECK_PICK);\n    Add(s.hwnd,L"BUTTON",L"CHỌN ROI",BS_PUSHBUTTON,680,846,96,25,IDC_PRECHECK_REGION);\n    Add(s.hwnd,L"BUTTON",L"XEM ROI",BS_PUSHBUTTON,784,846,88,25,IDC_PRECHECK_PREVIEW);\n    Add(s.hwnd,L"BUTTON",L"FULL",BS_PUSHBUTTON,880,846,72,25,IDC_PRECHECK_FULL);\n    Add(s.hwnd,L"STATIC",L"Tọa chuyển UI X",SS_LEFT|SS_CENTERIMAGE,32,880,102,25,0);\n    Add(s.hwnd,L"EDIT",s.config.bagUiSwitch.valid?std::to_wstring(s.config.bagUiSwitch.x).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,138,880,62,25,IDC_PRECHECK_X);\n    Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,204,880,18,25,0);\n    Add(s.hwnd,L"EDIT",s.config.bagUiSwitch.valid?std::to_wstring(s.config.bagUiSwitch.y).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,225,880,62,25,IDC_PRECHECK_Y);\n    Add(s.hwnd,L"STATIC",L"ms",SS_CENTER|SS_CENTERIMAGE,292,880,25,25,0);\n    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.bagUiSwitch.delayMs).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,320,880,62,25,IDC_PRECHECK_DELAY);\n    Add(s.hwnd,L"BUTTON",L"LẤY F8 PRECHECK",BS_PUSHBUTTON,392,880,160,25,IDC_PRECHECK_CAPTURE);\n    Add(s.hwnd,L"STATIC",L"Ảnh đúng → bỏ click chuyển UI; ảnh sai → click tọa này → chạy OPEN BAG cũ.",SS_LEFT|SS_CENTERIMAGE,565,880,420,25,0);\n\n    Add(s.hwnd,L"STATIC",L"Sẵn sàng. AUTO CON: PRECHECK Tay nải → OPEN BAG → scan; FULL finish current item → X popup one-shot → CloseBag.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,950,987,62,IDC_STATUS);\n    Add(s.hwnd,L"BUTTON",L"ĐÓNG",BS_PUSHBUTTON,885,1020,120,30,IDCANCEL);\n''','precheck UI')
cpp=once(cpp,
'''                case IDC_PRE_CLOSE_FULL:ResetPreCloseRegion(*s);return 0;\n''',
'''                case IDC_PRE_CLOSE_FULL:ResetPreCloseRegion(*s);return 0;\n                case IDC_PRECHECK_PICK:PickPrecheckTemplate(*s);return 0;\n                case IDC_PRECHECK_REGION:SelectPrecheckRegion(*s);return 0;\n                case IDC_PRECHECK_PREVIEW:PreviewPrecheckRegion(*s);return 0;\n                case IDC_PRECHECK_FULL:ResetPrecheckRegion(*s);return 0;\n''','precheck ui commands')
cpp=once(cpp,
'''                case IDC_BAG_CLOSE_CAPTURE:ArmBagF8(*s,-5,L"CLOSE BAG");return 0;\n''',
'''                case IDC_BAG_CLOSE_CAPTURE:ArmBagF8(*s,-5,L"CLOSE BAG");return 0;\n                case IDC_PRECHECK_CAPTURE:ArmBagF8(*s,-6,L"PRECHECK TAY NẢI");return 0;\n''','precheck capture command')
cpp=once(cpp,
'''                              CW_USEDEFAULT,CW_USEDEFAULT,1000,980,target.owner,nullptr,GetModuleHandleW(nullptr),&state);\n''',
'''                              CW_USEDEFAULT,CW_USEDEFAULT,1000,1090,target.owner,nullptr,GetModuleHandleW(nullptr),&state);\n''','precheck dialog height')
save(cpp_path,cpp,nl)

# ---- background AUTO runtime ----
auto_path=out/'image_scan_auto_ext.inl'; auto,anl=load(auto_path)
auto=once(auto,
'''    Idle,\n    WaitOpen1,\n''',
'''    Idle,\n    WaitPrecheckSwitch,\n    WaitOpen1,\n''','precheck runtime phase')
auto=once(auto,
'''    bool bagOpened = false;\n''',
'''    bool bagOpened = false;\n    // START boundary only: ClearAutoSessionRuntime intentionally does NOT reset this.\n    bool startPrecheckCompleted = false;\n''','precheck latch')
# validate mandatory CP2 settings only when this scanner is enabled
auto=once(auto,
'''    if (s.scan.config.closeTemplatePath.empty()) { error=L"chưa chọn ảnh X"; return false; }\n''',
'''    if (s.scan.config.closeTemplatePath.empty()) { error=L"chưa chọn ảnh X"; return false; }\n    if (s.scan.config.bagUiTemplatePath.empty()) { error=L"PRECHECK TAY NẢI chưa chọn ảnh data"; return false; }\n    if (!s.scan.config.bagUiSwitch.valid) { error=L"PRECHECK TAY NẢI chưa gán tọa chuyển UI bằng F8"; return false; }\n''','precheck validation')

pre_fn='''bool AutoRunStartPrecheck(AutoSession& s, ULONGLONG now, std::wstring& error) {\n    if(s.startPrecheckCompleted)return AutoOpenNext(s,now,1,error);\n\n    Image tpl{};\n    if(!LoadImageWic(s.scan.config.bagUiTemplatePath,tpl,error)){error=L"PRECHECK TAY NẢI ảnh data • "+error;return false;}\n    Image frame{};std::wstring backend;\n    if(!CaptureClient(s.scan.target.gameWindow,frame,backend,error)){error=L"PRECHECK TAY NẢI capture • "+error;return false;}\n    int rx=0,ry=0,rw=0,rh=0;ResolveNamedRoi(s.scan.config.bagUiRoi,frame.width,frame.height,rx,ry,rw,rh);\n    Match m=FindTemplate(frame,tpl,rx,ry,rw,rh,\n                         static_cast<double>(std::clamp(s.scan.config.thresholdPercent,1,100))/100.0,error);\n    if(m.score<0.0){error=L"PRECHECK TAY NẢI scan ROI • "+error;return false;}\n\n    if(m.found){\n        s.startPrecheckCompleted=true;\n        s.status=L"PRECHECK TAY NẢI PASS • đúng giao diện → OPEN BAG";\n        return AutoOpenNext(s,now,1,error);\n    }\n\n    if(!AutoClickStep(s,s.scan.config.bagUiSwitch,error))return false;\n    s.startPrecheckCompleted=true;\n    AutoArm(s,AutoPhase::WaitPrecheckSwitch,now,\n            static_cast<ULONGLONG>(std::clamp(s.scan.config.bagUiSwitch.delayMs,50,10000)),false);\n    s.status=L"PRECHECK TAY NẢI MISS • click chuyển giao diện → OPEN BAG";\n    return true;\n}\n\n'''
auto=once(auto,
'''bool InitializeAutoSession(AutoSession& s, const Target& target, ULONGLONG now, std::wstring& error) {\n''',
pre_fn+'''bool InitializeAutoSession(AutoSession& s, const Target& target, ULONGLONG now, std::wstring& error) {\n''','precheck helper')
auto=once(auto,
'''    if(!AutoOpenNext(s,now,1,error)){AutoSetError(s,error);return false;}\n''',
'''    if(!AutoRunStartPrecheck(s,now,error)){AutoSetError(s,error);return false;}\n''','precheck before open bag')
# Include PRECHECK delay in FULL immediate close set to avoid waiting if bag becomes full then.
auto=once(auto,
'''       (s.phase==AutoPhase::WaitOpen1||s.phase==AutoPhase::WaitOpen2||s.phase==AutoPhase::WaitEmptyRetry)){\n''',
'''       (s.phase==AutoPhase::WaitPrecheckSwitch||s.phase==AutoPhase::WaitOpen1||s.phase==AutoPhase::WaitOpen2||s.phase==AutoPhase::WaitEmptyRetry)){\n''','full during precheck')
auto=once(auto,
'''    if(s.phase==AutoPhase::WaitOpen1){\n''',
'''    if(s.phase==AutoPhase::WaitPrecheckSwitch){\n        if(s.fullExitPending){if(!AutoStartClose(s,now,AutoClosePurpose::Full,error))AutoSetError(s,error);return MakeAutoResult(s);}\n        if(!AutoOpenNext(s,now,1,error))AutoSetError(s,error);return MakeAutoResult(s);\n    }\n    if(s.phase==AutoPhase::WaitOpen1){\n''','precheck wait')
# persistent config
auto=once(auto,
'''    out<<"good_path="<<Utf8(c.templatePath)<<"\\n"<<"discard_path="<<Utf8(c.discardTemplatePath)<<"\\n"<<"close_path="<<Utf8(c.closeTemplatePath)<<"\\n";\n''',
'''    out<<"good_path="<<Utf8(c.templatePath)<<"\\n"<<"discard_path="<<Utf8(c.discardTemplatePath)<<"\\n"<<"close_path="<<Utf8(c.closeTemplatePath)<<"\\n";\n    out<<"precheck_path="<<Utf8(c.bagUiTemplatePath)<<"\\n";\n''','persistent precheck path')
auto=once(auto,
'''    out<<"pre_close_roi="<<RoiLine(c.preCloseRoi.x,c.preCloseRoi.y,c.preCloseRoi.w,c.preCloseRoi.h,c.preCloseRoi.baseW,c.preCloseRoi.baseH)<<"\\n";\n''',
'''    out<<"pre_close_roi="<<RoiLine(c.preCloseRoi.x,c.preCloseRoi.y,c.preCloseRoi.w,c.preCloseRoi.h,c.preCloseRoi.baseW,c.preCloseRoi.baseH)<<"\\n";\n    out<<"precheck_roi="<<RoiLine(c.bagUiRoi.x,c.bagUiRoi.y,c.bagUiRoi.w,c.bagUiRoi.h,c.bagUiRoi.baseW,c.bagUiRoi.baseH)<<"\\n";\n    out<<"precheck_switch="<<StepLine(c.bagUiSwitch)<<"\\n";\n''','persistent precheck settings')
auto=once(auto,
'''        if(key=="good_path")c.templatePath=FromUtf8(val);else if(key=="discard_path")c.discardTemplatePath=FromUtf8(val);else if(key=="close_path")c.closeTemplatePath=FromUtf8(val);\n''',
'''        if(key=="good_path")c.templatePath=FromUtf8(val);else if(key=="discard_path")c.discardTemplatePath=FromUtf8(val);else if(key=="close_path")c.closeTemplatePath=FromUtf8(val);\n        else if(key=="precheck_path")c.bagUiTemplatePath=FromUtf8(val);\n''','persistent import precheck path')
auto=once(auto,
'''        else if(key=="pre_close_roi"){std::array<int,6>v{};if(ParseSix(val,v))c.preCloseRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}\n''',
'''        else if(key=="pre_close_roi"){std::array<int,6>v{};if(ParseSix(val,v))c.preCloseRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}\n        else if(key=="precheck_roi"){std::array<int,6>v{};if(ParseSix(val,v))c.bagUiRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}\n        else if(key=="precheck_switch")ParseStepLine(val,c.bagUiSwitch);\n''','persistent import precheck settings')

# Explicit invariant: Reset/new train/revive may clear phase, never START latch.
if 'startPrecheckCompleted = false' in auto.replace('bool startPrecheckCompleted = false;','') or 'startPrecheckCompleted=false' in auto:
    raise SystemExit('CP2 invariant: generic runtime reset clears START precheck latch')
for token in ['bagUiTemplatePath','bagUiRoi','bagUiSwitch','WaitPrecheckSwitch','AutoRunStartPrecheck','PRECHECK TAY NẢI PASS','PRECHECK TAY NẢI MISS']:
    if token not in cpp+auto: raise SystemExit('CP2 final assertion missing: '+token)
save(auto_path,auto,anl)
print('apply_v99_automation_cp2_impl.py: PASS')
