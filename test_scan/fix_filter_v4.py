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
p.write_text(text, encoding='utf-8')
print('FILTER v4 ROI editor fix PASS')
