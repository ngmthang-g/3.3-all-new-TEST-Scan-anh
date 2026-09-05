from pathlib import Path
import argparse

ap=argparse.ArgumentParser()
ap.add_argument('--source-root',required=True)
ap.add_argument('--output-dir',required=True)
a=ap.parse_args()
out_dir=Path(a.output_dir)

# auto_ext runtime 12 -> 30 exact child loops / validator, preserve CRLF
p=out_dir/'image_scan_auto_ext.inl'; raw=p.read_bytes(); x=raw.decode('utf-8-sig').replace('\r\n','\n').replace('\r','\n')
repls=[
('bool ValidChildSlot(int childSlot) { return childSlot >= 1 && childSlot <= 12; }','bool ValidChildSlot(int childSlot) { return childSlot >= 1 && childSlot <= 30; }'),
('out<<"children=";for(int i=0;i<12;++i){if(i)out<<\',\';out<<(c.childEnabled[static_cast<std::size_t>(i)]?1:0);}out<<"\\n";','out<<"children=";for(int i=0;i<30;++i){if(i)out<<\',\';out<<(c.childEnabled[static_cast<std::size_t>(i)]?1:0);}out<<"\\n";'),
('else if(key=="children"){std::stringstream ss(val);std::string x;int i=0;while(std::getline(ss,x,\',\')&&i<12)c.childEnabled[static_cast<std::size_t>(i++)]=(x=="1");}','else if(key=="children"){std::stringstream ss(val);std::string x;int i=0;while(std::getline(ss,x,\',\')&&i<30)c.childEnabled[static_cast<std::size_t>(i++)]=(x=="1");}')]
for old,new in repls:
    if x.count(old)!=1: raise SystemExit('auto marker fail '+old[:40]+' count='+str(x.count(old)))
    x=x.replace(old,new,1)
p.write_bytes(x.replace('\n','\r\n').encode('utf-8'))
print('UI30 generated scanner runtime CON1..CON30 patch PASS')

# Generated FILTER UI: only expand CON toggles/layout; matching engine stays untouched.
cpp_path=out_dir/'image_scan_test.cpp'
cpp=cpp_path.read_text(encoding='utf-8-sig').replace('\r\n','\n').replace('\r','\n')

def once(text,old,new,label):
    c=text.count(old)
    if c!=1: raise SystemExit(f'{label}: expected one marker, found {c}')
    return text.replace(old,new,1)

cpp=once(cpp,
'''constexpr int IDC_CHILD_SCAN_BASE = 1100; // 1100..1111 = CON1..CON12\n''',
'''constexpr int IDC_CHILD_SCAN_BASE = 1200; // 1200..1229 = CON1..CON30; giữ tách khỏi PRE-CLOSE X 1112..1114\n''','child ids')
cpp=once(cpp,'std::array<bool, 12> childEnabled{};','std::array<bool, 30> childEnabled{};','child array')
cpp=once(cpp,
'''    for (int i = 0; i < 12; ++i)\n        s.config.childEnabled[static_cast<std::size_t>(i)] =\n            IsDlgButtonChecked(s.hwnd, IDC_CHILD_SCAN_BASE + i) == BST_CHECKED;\n''',
'''    for (int i = 0; i < 30; ++i)\n        s.config.childEnabled[static_cast<std::size_t>(i)] =\n            IsDlgButtonChecked(s.hwnd, IDC_CHILD_SCAN_BASE + i) == BST_CHECKED;\n''','sync children')
cpp=once(cpp,
'''    for(int i=0;i<12;++i) CheckDlgButton(s.hwnd,IDC_CHILD_SCAN_BASE+i,s.config.childEnabled[static_cast<std::size_t>(i)]?BST_CHECKED:BST_UNCHECKED);\n''',
'''    for(int i=0;i<30;++i) CheckDlgButton(s.hwnd,IDC_CHILD_SCAN_BASE+i,s.config.childEnabled[static_cast<std::size_t>(i)]?BST_CHECKED:BST_UNCHECKED);\n''','refresh children')
cpp=once(cpp,
'''    out<<"children=";for(int i=0;i<12;++i){if(i)out<<',';out<<(s.config.childEnabled[static_cast<std::size_t>(i)]?1:0);}out<<"\\n";\n''',
'''    out<<"children=";for(int i=0;i<30;++i){if(i)out<<',';out<<(s.config.childEnabled[static_cast<std::size_t>(i)]?1:0);}out<<"\\n";\n''','export children')
cpp=once(cpp,
'''        else if(key=="children"){std::stringstream ss(val);std::string x;int i=0;while(std::getline(ss,x,',')&&i<12){c.childEnabled[static_cast<std::size_t>(i++)]=(x=="1");}}\n''',
'''        else if(key=="children"){std::stringstream ss(val);std::string x;int i=0;while(std::getline(ss,x,',')&&i<30){c.childEnabled[static_cast<std::size_t>(i++)]=(x=="1");}}\n''','import children')

old='''    Add(s.hwnd,L"BUTTON",L"AUTO LỌC VK CHỈ CON • tick CON + 2 click mở tùy chọn + 1 click đóng",BS_GROUPBOX,18,628,987,145,0);\n    Add(s.hwnd,L"STATIC",L"CON:",SS_LEFT|SS_CENTERIMAGE,32,648,40,24,0);\n    for(int i=0;i<12;++i){const std::wstring label=L"C"+std::to_wstring(i+1);HWND cb=Add(s.hwnd,L"BUTTON",label.c_str(),BS_AUTOCHECKBOX,74+i*69,648,66,24,IDC_CHILD_SCAN_BASE+i);CheckDlgButton(s.hwnd,IDC_CHILD_SCAN_BASE+i,s.config.childEnabled[static_cast<std::size_t>(i)]?BST_CHECKED:BST_UNCHECKED);(void)cb;}\n\n    auto addBagRow=[&](int y,const wchar_t* label,int xId,int yId,int dId,int capId,const ClickStep& p){\n        Add(s.hwnd,L"STATIC",label,SS_LEFT|SS_CENTERIMAGE,32,y,82,25,0);Add(s.hwnd,L"STATIC",L"X",SS_CENTER|SS_CENTERIMAGE,116,y,16,25,0);\n        Add(s.hwnd,L"EDIT",p.valid?std::to_wstring(p.x).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,134,y,60,25,xId);Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,198,y,16,25,0);\n        Add(s.hwnd,L"EDIT",p.valid?std::to_wstring(p.y).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,216,y,60,25,yId);Add(s.hwnd,L"STATIC",L"ms",SS_CENTER|SS_CENTERIMAGE,280,y,25,25,0);\n        Add(s.hwnd,L"EDIT",std::to_wstring(p.delayMs).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,306,y,60,25,dId);Add(s.hwnd,L"BUTTON",L"LẤY F8",BS_PUSHBUTTON,372,y,82,25,capId);\n    };\n    addBagRow(680,L"OPEN 1",IDC_OPEN1_X,IDC_OPEN1_Y,IDC_OPEN1_DELAY,IDC_OPEN1_CAPTURE,s.config.openBag1);\n    addBagRow(710,L"OPEN 2",IDC_OPEN2_X,IDC_OPEN2_Y,IDC_OPEN2_DELAY,IDC_OPEN2_CAPTURE,s.config.openBag2);\n    Add(s.hwnd,L"STATIC",L"CLOSE BAG",SS_LEFT|SS_CENTERIMAGE,482,680,88,25,0);Add(s.hwnd,L"STATIC",L"X",SS_CENTER|SS_CENTERIMAGE,572,680,16,25,0);\n    Add(s.hwnd,L"EDIT",s.config.closeBag.valid?std::to_wstring(s.config.closeBag.x).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,590,680,60,25,IDC_BAG_CLOSE_X);Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,654,680,16,25,0);\n    Add(s.hwnd,L"EDIT",s.config.closeBag.valid?std::to_wstring(s.config.closeBag.y).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,672,680,60,25,IDC_BAG_CLOSE_Y);Add(s.hwnd,L"STATIC",L"ms",SS_CENTER|SS_CENTERIMAGE,736,680,25,25,0);\n    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.closeBag.delayMs).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,762,680,60,25,IDC_BAG_CLOSE_DELAY);Add(s.hwnd,L"BUTTON",L"LẤY F8",BS_PUSHBUTTON,828,680,82,25,IDC_BAG_CLOSE_CAPTURE);\n    Add(s.hwnd,L"BUTTON",L"XUẤT SCAN",BS_PUSHBUTTON,482,710,145,25,IDC_EXPORT_SCAN);Add(s.hwnd,L"BUTTON",L"NHẬP SCAN",BS_PUSHBUTTON,637,710,145,25,IDC_IMPORT_SCAN);\n    Add(s.hwnd,L"STATIC",L"ROI X TRƯỚC CLOSE BAG • dùng chung ảnh DẤU X + ngưỡng hiện tại:",SS_LEFT|SS_CENTERIMAGE,32,742,430,25,0);\n    Add(s.hwnd,L"BUTTON",L"CHỌN VÙNG",BS_PUSHBUTTON,470,742,112,25,IDC_PRE_CLOSE_REGION);\n    Add(s.hwnd,L"BUTTON",L"XEM",BS_PUSHBUTTON,590,742,68,25,IDC_PRE_CLOSE_PREVIEW);\n    Add(s.hwnd,L"BUTTON",L"FULL",BS_PUSHBUTTON,666,742,68,25,IDC_PRE_CLOSE_FULL);\n\n    Add(s.hwnd,L"STATIC",L"Sẵn sàng. AUTO CON chỉ chạy sau khi tới bãi + AutoFight ON; FULL finish current item → X popup one-shot → CloseBag → nhường điều phối.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,780,987,75,IDC_STATUS);\n    Add(s.hwnd,L"BUTTON",L"ĐÓNG",BS_PUSHBUTTON,885,868,120,30,IDCANCEL);\n'''
new='''    Add(s.hwnd,L"BUTTON",L"AUTO LỌC VK CHỈ CON • tick CON + 2 click mở tùy chọn + 1 click đóng",BS_GROUPBOX,18,628,987,190,0);\n    Add(s.hwnd,L"STATIC",L"CON:",SS_LEFT|SS_CENTERIMAGE,32,648,40,24,0);\n    for(int i=0;i<30;++i){\n        const std::wstring label=L"C"+std::to_wstring(i+1);\n        const int col=i%10,row=i/10;\n        HWND cb=Add(s.hwnd,L"BUTTON",label.c_str(),BS_AUTOCHECKBOX,74+col*88,648+row*24,82,24,IDC_CHILD_SCAN_BASE+i);\n        CheckDlgButton(s.hwnd,IDC_CHILD_SCAN_BASE+i,s.config.childEnabled[static_cast<std::size_t>(i)]?BST_CHECKED:BST_UNCHECKED);(void)cb;\n    }\n\n    auto addBagRow=[&](int y,const wchar_t* label,int xId,int yId,int dId,int capId,const ClickStep& p){\n        Add(s.hwnd,L"STATIC",label,SS_LEFT|SS_CENTERIMAGE,32,y,82,25,0);Add(s.hwnd,L"STATIC",L"X",SS_CENTER|SS_CENTERIMAGE,116,y,16,25,0);\n        Add(s.hwnd,L"EDIT",p.valid?std::to_wstring(p.x).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,134,y,60,25,xId);Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,198,y,16,25,0);\n        Add(s.hwnd,L"EDIT",p.valid?std::to_wstring(p.y).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,216,y,60,25,yId);Add(s.hwnd,L"STATIC",L"ms",SS_CENTER|SS_CENTERIMAGE,280,y,25,25,0);\n        Add(s.hwnd,L"EDIT",std::to_wstring(p.delayMs).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,306,y,60,25,dId);Add(s.hwnd,L"BUTTON",L"LẤY F8",BS_PUSHBUTTON,372,y,82,25,capId);\n    };\n    addBagRow(724,L"OPEN 1",IDC_OPEN1_X,IDC_OPEN1_Y,IDC_OPEN1_DELAY,IDC_OPEN1_CAPTURE,s.config.openBag1);\n    addBagRow(754,L"OPEN 2",IDC_OPEN2_X,IDC_OPEN2_Y,IDC_OPEN2_DELAY,IDC_OPEN2_CAPTURE,s.config.openBag2);\n    Add(s.hwnd,L"STATIC",L"CLOSE BAG",SS_LEFT|SS_CENTERIMAGE,482,724,88,25,0);Add(s.hwnd,L"STATIC",L"X",SS_CENTER|SS_CENTERIMAGE,572,724,16,25,0);\n    Add(s.hwnd,L"EDIT",s.config.closeBag.valid?std::to_wstring(s.config.closeBag.x).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,590,724,60,25,IDC_BAG_CLOSE_X);Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,654,724,16,25,0);\n    Add(s.hwnd,L"EDIT",s.config.closeBag.valid?std::to_wstring(s.config.closeBag.y).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,672,724,60,25,IDC_BAG_CLOSE_Y);Add(s.hwnd,L"STATIC",L"ms",SS_CENTER|SS_CENTERIMAGE,736,724,25,25,0);\n    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.closeBag.delayMs).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,762,724,60,25,IDC_BAG_CLOSE_DELAY);Add(s.hwnd,L"BUTTON",L"LẤY F8",BS_PUSHBUTTON,828,724,82,25,IDC_BAG_CLOSE_CAPTURE);\n    Add(s.hwnd,L"BUTTON",L"XUẤT SCAN",BS_PUSHBUTTON,482,754,145,25,IDC_EXPORT_SCAN);Add(s.hwnd,L"BUTTON",L"NHẬP SCAN",BS_PUSHBUTTON,637,754,145,25,IDC_IMPORT_SCAN);\n    Add(s.hwnd,L"STATIC",L"ROI X TRƯỚC CLOSE BAG • dùng chung ảnh DẤU X + ngưỡng hiện tại:",SS_LEFT|SS_CENTERIMAGE,32,786,430,25,0);\n    Add(s.hwnd,L"BUTTON",L"CHỌN VÙNG",BS_PUSHBUTTON,470,786,112,25,IDC_PRE_CLOSE_REGION);\n    Add(s.hwnd,L"BUTTON",L"XEM",BS_PUSHBUTTON,590,786,68,25,IDC_PRE_CLOSE_PREVIEW);\n    Add(s.hwnd,L"BUTTON",L"FULL",BS_PUSHBUTTON,666,786,68,25,IDC_PRE_CLOSE_FULL);\n\n    Add(s.hwnd,L"STATIC",L"Sẵn sàng. AUTO CON chỉ chạy sau khi tới bãi + AutoFight ON; FULL finish current item → X popup one-shot → CloseBag → nhường điều phối.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,824,987,75,IDC_STATUS);\n    Add(s.hwnd,L"BUTTON",L"ĐÓNG",BS_PUSHBUTTON,885,912,120,30,IDCANCEL);\n'''
cpp=once(cpp,old,new,'30 checkbox layout')
cpp=once(cpp,
'''                              CW_USEDEFAULT,CW_USEDEFAULT,1000,920,target.owner,nullptr,GetModuleHandleW(nullptr),&state);\n''',
'''                              CW_USEDEFAULT,CW_USEDEFAULT,1000,980,target.owner,nullptr,GetModuleHandleW(nullptr),&state);\n''','dialog height')

cpp_path.write_text(cpp,encoding='utf-8',newline='\n')
required=['std::array<bool, 30> childEnabled','i<30','1200..1229 = CON1..CON30','const int col=i%10,row=i/10']
missing=[x for x in required if x not in cpp]
if missing: raise SystemExit('UI30 SCAN PATCH FAIL: '+', '.join(missing))
print('UI30 SCANNER CON1..CON30 patch PASS')
