from pathlib import Path
import argparse

ap = argparse.ArgumentParser()
ap.add_argument('--source-root', required=True)
ap.add_argument('--output-dir', required=True)
a = ap.parse_args()
root = Path(a.source_root)
out = Path(a.output_dir)

def once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise SystemExit(f'{label}: expected one marker, found {count}')
    return text.replace(old, new, 1)

cpp_path = out / 'image_scan_test.cpp'
auto_src = root / 'src' / 'image_scan_auto_ext.inl'
auto_out = out / 'image_scan_auto_ext.inl'
cpp = cpp_path.read_text(encoding='utf-8-sig')

cpp = once(cpp,
'''constexpr int IDC_CHILD_SCAN_BASE = 1100; // 1100..1111 = CON1..CON12\n''',
'''constexpr int IDC_CHILD_SCAN_BASE = 1100; // 1100..1111 = CON1..CON12\nconstexpr int IDC_PRE_CLOSE_REGION = 1112;\nconstexpr int IDC_PRE_CLOSE_PREVIEW = 1113;\nconstexpr int IDC_PRE_CLOSE_FULL = 1114;\n''', 'pre-close ids')
cpp = once(cpp,
'''    ScanRoi discardRoi{};\n    ScanRoi closeRoi{};\n''',
'''    ScanRoi discardRoi{};\n    ScanRoi closeRoi{};\n    // Dùng chung ảnh DẤU X, chỉ ROI riêng cho one-shot ngay trước CloseBag.\n    ScanRoi preCloseRoi{};\n''', 'pre-close config')
cpp = once(cpp,
'''    RebaseNamedRoi(c.discardRoi, cw, ch);\n    RebaseNamedRoi(c.closeRoi, cw, ch);\n''',
'''    RebaseNamedRoi(c.discardRoi, cw, ch);\n    RebaseNamedRoi(c.closeRoi, cw, ch);\n    RebaseNamedRoi(c.preCloseRoi, cw, ch);\n''', 'pre-close rebase')
cpp = once(cpp,
'''    out<<"close_roi="<<RoiLine(s.config.closeRoi.x,s.config.closeRoi.y,s.config.closeRoi.w,s.config.closeRoi.h,s.config.closeRoi.baseW,s.config.closeRoi.baseH)<<"\\n";\n''',
'''    out<<"close_roi="<<RoiLine(s.config.closeRoi.x,s.config.closeRoi.y,s.config.closeRoi.w,s.config.closeRoi.h,s.config.closeRoi.baseW,s.config.closeRoi.baseH)<<"\\n";\n    out<<"pre_close_roi="<<RoiLine(s.config.preCloseRoi.x,s.config.preCloseRoi.y,s.config.preCloseRoi.w,s.config.preCloseRoi.h,s.config.preCloseRoi.baseW,s.config.preCloseRoi.baseH)<<"\\n";\n''', 'scan export')
cpp = once(cpp,
'''        else if(key=="close_roi"){std::array<int,6>v{};if(ParseSix(val,v)){c.closeRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}}\n''',
'''        else if(key=="close_roi"){std::array<int,6>v{};if(ParseSix(val,v)){c.closeRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}}\n        else if(key=="pre_close_roi"){std::array<int,6>v{};if(ParseSix(val,v)){c.preCloseRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}}\n''', 'scan import')
cpp = once(cpp,
'''    SetEditInt(s.hwnd,xId,roi.x);SetEditInt(s.hwnd,yId,roi.y);SetEditInt(s.hwnd,wId,roi.w);SetEditInt(s.hwnd,hId,roi.h);\n''',
'''    if(xId>0)SetEditInt(s.hwnd,xId,roi.x);if(yId>0)SetEditInt(s.hwnd,yId,roi.y);if(wId>0)SetEditInt(s.hwnd,wId,roi.w);if(hId>0)SetEditInt(s.hwnd,hId,roi.h);\n''', 'optional editors')
cpp = once(cpp,
'''void ResetCloseRegion(State& s){ResetNamedRegion(s,s.config.closeRoi,IDC_CLOSE_X,IDC_CLOSE_Y,IDC_CLOSE_W,IDC_CLOSE_H,L"ROI DẤU X");}\n''',
'''void ResetCloseRegion(State& s){ResetNamedRegion(s,s.config.closeRoi,IDC_CLOSE_X,IDC_CLOSE_Y,IDC_CLOSE_W,IDC_CLOSE_H,L"ROI DẤU X");}\nvoid SelectPreCloseRegion(State& s){SelectNamedRegion(s,s.config.preCloseRoi,0,0,0,0,L"ROI X TRƯỚC CLOSE BAG");}\nvoid PreviewPreCloseRegion(State& s){PreviewNamedRegion(s,s.config.preCloseRoi,L"ROI X TRƯỚC CLOSE BAG");}\nvoid ResetPreCloseRegion(State& s){int cw=0,ch=0;CurrentClientSize(s.target.gameWindow,cw,ch);s.config.preCloseRoi={};s.config.preCloseRoi.baseW=cw;s.config.preCloseRoi.baseH=ch;g_lastConfig=s.config;SetStatus(s.hwnd,L"ROI X TRƯỚC CLOSE BAG = FULL CLIENT");}\n''', 'pre-close handlers')
cpp = once(cpp,
'''    Add(s.hwnd,L"BUTTON",L"AUTO LỌC VK CHỈ CON • tick CON + 2 click mở tùy chọn + 1 click đóng",BS_GROUPBOX,18,628,987,112,0);\n''',
'''    Add(s.hwnd,L"BUTTON",L"AUTO LỌC VK CHỈ CON • tick CON + 2 click mở tùy chọn + 1 click đóng",BS_GROUPBOX,18,628,987,145,0);\n''', 'group height')
cpp = once(cpp,
'''    Add(s.hwnd,L"BUTTON",L"XUẤT SCAN",BS_PUSHBUTTON,482,710,145,25,IDC_EXPORT_SCAN);Add(s.hwnd,L"BUTTON",L"NHẬP SCAN",BS_PUSHBUTTON,637,710,145,25,IDC_IMPORT_SCAN);\n\n    Add(s.hwnd,L"STATIC",L"Sẵn sàng. AUTO CON chỉ chạy sau khi tới bãi + AutoFight ON; FULL finish current item → CloseBag → nhường điều phối.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,750,987,75,IDC_STATUS);\n    Add(s.hwnd,L"BUTTON",L"ĐÓNG",BS_PUSHBUTTON,885,838,120,30,IDCANCEL);\n''',
'''    Add(s.hwnd,L"BUTTON",L"XUẤT SCAN",BS_PUSHBUTTON,482,710,145,25,IDC_EXPORT_SCAN);Add(s.hwnd,L"BUTTON",L"NHẬP SCAN",BS_PUSHBUTTON,637,710,145,25,IDC_IMPORT_SCAN);\n    Add(s.hwnd,L"STATIC",L"ROI X TRƯỚC CLOSE BAG • dùng chung ảnh DẤU X + ngưỡng hiện tại:",SS_LEFT|SS_CENTERIMAGE,32,742,430,25,0);\n    Add(s.hwnd,L"BUTTON",L"CHỌN VÙNG",BS_PUSHBUTTON,470,742,112,25,IDC_PRE_CLOSE_REGION);\n    Add(s.hwnd,L"BUTTON",L"XEM",BS_PUSHBUTTON,590,742,68,25,IDC_PRE_CLOSE_PREVIEW);\n    Add(s.hwnd,L"BUTTON",L"FULL",BS_PUSHBUTTON,666,742,68,25,IDC_PRE_CLOSE_FULL);\n\n    Add(s.hwnd,L"STATIC",L"Sẵn sàng. AUTO CON chỉ chạy sau khi tới bãi + AutoFight ON; FULL finish current item → X popup one-shot → CloseBag → nhường điều phối.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,780,987,75,IDC_STATUS);\n    Add(s.hwnd,L"BUTTON",L"ĐÓNG",BS_PUSHBUTTON,885,868,120,30,IDCANCEL);\n''', 'pre-close UI')
cpp = once(cpp,
'''                case IDC_CLOSE_FULL:ResetCloseRegion(*s);return 0;\n''',
'''                case IDC_CLOSE_FULL:ResetCloseRegion(*s);return 0;\n                case IDC_PRE_CLOSE_REGION:SelectPreCloseRegion(*s);return 0;\n                case IDC_PRE_CLOSE_PREVIEW:PreviewPreCloseRegion(*s);return 0;\n                case IDC_PRE_CLOSE_FULL:ResetPreCloseRegion(*s);return 0;\n''', 'pre-close commands')
cpp = once(cpp,
'''                              CW_USEDEFAULT,CW_USEDEFAULT,1000,890,target.owner,nullptr,GetModuleHandleW(nullptr),&state);\n''',
'''                              CW_USEDEFAULT,CW_USEDEFAULT,1000,920,target.owner,nullptr,GetModuleHandleW(nullptr),&state);\n''', 'dialog height')
cpp_path.write_text(cpp, encoding='utf-8', newline='\n')

auto = auto_src.read_text(encoding='utf-8-sig').replace('\r\n','\n').replace('\r','\n')
auto = once(auto,
'''    WaitEmptyRetry,\n    WaitBagClose,\n''',
'''    WaitEmptyRetry,\n    WaitPreCloseX,\n    WaitBagClose,\n''', 'runtime phase')
auto = once(auto,
'''bool AutoStartClose(AutoSession& s, ULONGLONG now, AutoClosePurpose purpose, std::wstring& error) {\n    if(!AutoClickStep(s,s.scan.config.closeBag,error))return false;\n    s.bagOpened=false;s.currentItemActive=false;s.closePurpose=purpose;\n    AutoArm(s,AutoPhase::WaitBagClose,now,static_cast<ULONGLONG>(std::clamp(s.scan.config.closeBag.delayMs,50,10000)),false);\n    s.status=purpose==AutoClosePurpose::Full\n        ? L"SCAN VK • FULL → CLOSE BAG → nhường điều phối"\n        : L"SCAN VK • SLOT CUỐI GOOD → CLOSE BAG → chờ FULL";\n    return true;\n}\n''',
'''bool AutoClickCloseBagNow(AutoSession& s, ULONGLONG now, AutoClosePurpose purpose, std::wstring& error) {\n    if(!AutoClickStep(s,s.scan.config.closeBag,error))return false;\n    s.bagOpened=false;s.currentItemActive=false;s.closePurpose=purpose;\n    AutoArm(s,AutoPhase::WaitBagClose,now,static_cast<ULONGLONG>(std::clamp(s.scan.config.closeBag.delayMs,50,10000)),false);\n    s.status=purpose==AutoClosePurpose::Full\n        ? L"SCAN VK • FULL → CLOSE BAG → nhường điều phối"\n        : L"SCAN VK • SLOT CUỐI GOOD → CLOSE BAG → chờ FULL";\n    return true;\n}\n\nbool AutoStartClose(AutoSession& s, ULONGLONG now, AutoClosePurpose purpose, std::wstring& error) {\n    // One-shot cleanup immediately before CloseBag. Reuse the existing DẤU X image + threshold,\n    // but scan only the separately configured preCloseRoi. NOT FOUND/capture fail never blocks yield.\n    const ScanRoi& roi=s.scan.config.preCloseRoi;\n    if(roi.baseW>0&&roi.baseH>0&&!s.scan.closeTpl.bgra.empty()){\n        Image frame{};std::wstring backend,scanError;\n        if(CaptureClient(s.scan.target.gameWindow,frame,backend,scanError)){\n            int rx=0,ry=0,rw=0,rh=0;ResolveNamedRoi(roi,frame.width,frame.height,rx,ry,rw,rh);\n            Match close=FindTemplate(frame,s.scan.closeTpl,rx,ry,rw,rh,\n                                     static_cast<double>(std::clamp(s.scan.config.thresholdPercent,1,100))/100.0,scanError);\n            if(close.found){\n                const int cx=close.x+s.scan.closeTpl.width/2,cy=close.y+s.scan.closeTpl.height/2;\n                std::wstring clickError;\n                if(RawClick(s.scan,cx,cy,frame.width,frame.height,clickError)){\n                    s.closePurpose=purpose;s.currentItemActive=false;\n                    AutoArm(s,AutoPhase::WaitPreCloseX,now,static_cast<ULONGLONG>(std::clamp(s.scan.config.closeClickDelayMs,50,10000)),false);\n                    s.status=L"SCAN VK • trước CLOSE BAG thấy X → click tâm";\n                    return true;\n                }\n            }\n        }\n    }\n    return AutoClickCloseBagNow(s,now,purpose,error);\n}\n''', 'one-shot runtime')
auto = once(auto,
'''    out<<"close_roi="<<RoiLine(c.closeRoi.x,c.closeRoi.y,c.closeRoi.w,c.closeRoi.h,c.closeRoi.baseW,c.closeRoi.baseH)<<"\\n";\n''',
'''    out<<"close_roi="<<RoiLine(c.closeRoi.x,c.closeRoi.y,c.closeRoi.w,c.closeRoi.h,c.closeRoi.baseW,c.closeRoi.baseH)<<"\\n";\n    out<<"pre_close_roi="<<RoiLine(c.preCloseRoi.x,c.preCloseRoi.y,c.preCloseRoi.w,c.preCloseRoi.h,c.preCloseRoi.baseW,c.preCloseRoi.baseH)<<"\\n";\n''', 'persistent export')
auto = once(auto,
'''        else if(key=="close_roi"){std::array<int,6>v{};if(ParseSix(val,v))c.closeRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}\n''',
'''        else if(key=="close_roi"){std::array<int,6>v{};if(ParseSix(val,v))c.closeRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}\n        else if(key=="pre_close_roi"){std::array<int,6>v{};if(ParseSix(val,v))c.preCloseRoi={v[0],v[1],v[2],v[3],v[4],v[5]};}\n''', 'persistent import')
auto = once(auto,
'''    if(s.phase==AutoPhase::WaitBagClose){\n''',
'''    if(s.phase==AutoPhase::WaitPreCloseX){\n        const AutoClosePurpose purpose=s.closePurpose;\n        if(!AutoClickCloseBagNow(s,now,purpose,error))AutoSetError(s,L"CLOSE BAG sau scan X • "+error);\n        return MakeAutoResult(s);\n    }\n    if(s.phase==AutoPhase::WaitBagClose){\n''', 'runtime wait')
auto = once(auto,
'''    if(cfg.closeBag.valid){s->scan.config=cfg;(void)AutoClickStep(*s,cfg.closeBag,error);}\n    ClearAutoSessionRuntime(*s);s->status=error.empty()?L"SCAN VK • sau ĐẦU THAI đã click CLOSE BAG":L"SCAN VK • CLOSE BAG sau ĐẦU THAI fail • "+error;\n''',
'''    if(cfg.closeBag.valid){\n        s->scan.config=cfg;s->scan.target=target;\n        if(cfg.preCloseRoi.baseW>0&&cfg.preCloseRoi.baseH>0&&!cfg.closeTemplatePath.empty()){\n            std::wstring loadError;Image tpl{};\n            if(LoadImageWic(cfg.closeTemplatePath,tpl,loadError)){\n                Image frame{};std::wstring backend,scanError;\n                if(CaptureClient(target.gameWindow,frame,backend,scanError)){\n                    int rx=0,ry=0,rw=0,rh=0;ResolveNamedRoi(cfg.preCloseRoi,frame.width,frame.height,rx,ry,rw,rh);\n                    Match m=FindTemplate(frame,tpl,rx,ry,rw,rh,static_cast<double>(std::clamp(cfg.thresholdPercent,1,100))/100.0,scanError);\n                    if(m.found){std::wstring ignored;(void)RawClick(s->scan,m.x+tpl.width/2,m.y+tpl.height/2,frame.width,frame.height,ignored);}\n                }\n            }\n        }\n        (void)AutoClickStep(*s,cfg.closeBag,error);\n    }\n    ClearAutoSessionRuntime(*s);s->status=error.empty()?L"SCAN VK • sau ĐẦU THAI đã dọn X one-shot + click CLOSE BAG":L"SCAN VK • CLOSE BAG sau ĐẦU THAI fail • "+error;\n''', 'death cleanup')
auto_out.write_text(auto, encoding='utf-8', newline='\n')

required = [
    'preCloseRoi', 'WaitPreCloseX', 'FindTemplate(frame,s.scan.closeTpl',
    'ROI X TRƯỚC CLOSE BAG', 'pre_close_roi=', 'AutoClickCloseBagNow'
]
combined = cpp + auto
missing = [x for x in required if x not in combined]
if missing:
    raise SystemExit('PRE-CLOSE X PATCH FAIL: ' + ', '.join(missing))
print('PRE-CLOSE X one-shot patch PASS')
