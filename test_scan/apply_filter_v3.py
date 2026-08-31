from pathlib import Path

src_path = Path('test_scan/image_scan_test_v2.inl')
if not src_path.exists():
    raise SystemExit('missing v2 template')
text = src_path.read_text(encoding='utf-8')


def once(old: str, new: str, label: str):
    global text
    if old not in text:
        raise SystemExit(f'v3 patch anchor missing: {label}')
    text = text.replace(old, new, 1)


def between(start: str, end: str, replacement: str, label: str):
    global text
    a = text.find(start)
    if a < 0:
        raise SystemExit(f'v3 patch start missing: {label}')
    b = text.find(end, a)
    if b < 0:
        raise SystemExit(f'v3 patch end missing: {label}')
    text = text[:a] + replacement + text[b:]

once('constexpr wchar_t kClassName[] = L"ThanLongImageScanTestPocV2";',
     'constexpr wchar_t kClassName[] = L"ThanLongImageFilterTestPocV3";', 'class')
once('constexpr std::size_t kMaxClickSteps = 10;',
     'constexpr std::size_t kMaxClickSteps = 20;', 'max steps')
once('constexpr int IDC_STEP_TEST = 1031;\n', '''constexpr int IDC_STEP_TEST = 1031;
constexpr int IDC_TEMPLATE_DISCARD = 1040;
constexpr int IDC_PICK_DISCARD = 1041;
constexpr int IDC_TEMPLATE_CLOSE = 1042;
constexpr int IDC_PICK_CLOSE = 1043;
constexpr int IDC_AFTER_X = 1044;
constexpr int IDC_AFTER_Y = 1045;
constexpr int IDC_AFTER_CAPTURE = 1046;
constexpr int IDC_AFTER_TEST = 1047;
''', 'ids')

old_config = '''struct Config {
    std::wstring templatePath;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int roiBaseW = 0;
    int roiBaseH = 0;
    int thresholdPercent = 90;
    std::vector<ClickStep> steps;
};
'''
new_config = '''struct Config {
    // Ảnh 1 = dấu hiệu món đồ ĐÚNG. Chỉ scan, KHÔNG click ảnh 1.
    std::wstring templatePath;
    // Hai ảnh dùng chung cho cả 20 ô và được click đúng tâm match.
    std::wstring discardTemplatePath;
    std::wstring closeTemplatePath;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int roiBaseW = 0;
    int roiBaseH = 0;
    int thresholdPercent = 90;
    std::vector<ClickStep> steps;
    // Click ngay sau khi click tâm NÚT VỨT (ví dụ nút xác nhận), tự gán bằng F8.
    ClickStep afterDiscard;
};
'''
once(old_config, new_config, 'config')

# Refresh list/editor: 20 fixed slots; Repeat is deliberately removed from the filter UI.
between('void LoadStepEditor(State& s, int row) {', 'void SaveStepEditor(State& s) {', r'''void LoadStepEditor(State& s, int row) {
    if (row < 0 || row >= static_cast<int>(s.config.steps.size())) return;
    const ClickStep& step = s.config.steps[static_cast<std::size_t>(row)];
    if (step.valid) { SetEditInt(s.hwnd, IDC_STEP_X, step.x); SetEditInt(s.hwnd, IDC_STEP_Y, step.y); }
    else { SetDlgItemTextW(s.hwnd, IDC_STEP_X, L""); SetDlgItemTextW(s.hwnd, IDC_STEP_Y, L""); }
    SetEditInt(s.hwnd, IDC_STEP_DELAY, step.delayMs);
}

void RefreshStepList(State& s, int select = -1) {
    if (!s.stepList) return;
    ListView_DeleteAllItems(s.stepList);
    for (std::size_t i = 0; i < s.config.steps.size(); ++i) {
        const ClickStep& step = s.config.steps[i];
        std::wstring no = std::to_wstring(i + 1);
        LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = static_cast<int>(i); item.pszText = no.data();
        ListView_InsertItem(s.stepList, &item);
        std::wstring x = step.valid ? std::to_wstring(step.x) : L"CHƯA F8";
        std::wstring y = step.valid ? std::to_wstring(step.y) : L"-";
        std::wstring base = step.valid ? std::to_wstring(step.baseW) + L"x" + std::to_wstring(step.baseH) : L"-";
        std::wstring delay = std::to_wstring(step.delayMs);
        std::wstring state = step.valid ? L"READY • click ô rồi scan Ảnh 1" : L"CHƯA CÓ TỌA ĐỘ";
        ListView_SetItemText(s.stepList, static_cast<int>(i), 1, x.data());
        ListView_SetItemText(s.stepList, static_cast<int>(i), 2, y.data());
        ListView_SetItemText(s.stepList, static_cast<int>(i), 3, base.data());
        ListView_SetItemText(s.stepList, static_cast<int>(i), 4, delay.data());
        ListView_SetItemText(s.stepList, static_cast<int>(i), 5, state.data());
    }
    if (select < 0 && !s.config.steps.empty()) select = 0;
    if (select >= 0 && select < static_cast<int>(s.config.steps.size())) {
        ListView_SetItemState(s.stepList, select, LVIS_SELECTED | LVIS_FOCUSED,
                              LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(s.stepList, select, FALSE);
        LoadStepEditor(s, select);
    } else {
        SetDlgItemTextW(s.hwnd, IDC_STEP_X, L""); SetDlgItemTextW(s.hwnd, IDC_STEP_Y, L"");
    }
}

''', 'step editor/list')

# Replace SaveStepEditor and SyncConfig/PickTemplate block.
between('void SaveStepEditor(State& s) {', 'RECT NormalizedDragRect(const RegionPickerState& s) {', r'''void SaveStepEditor(State& s) {
    const int row = SelectedStep(s);
    if (row < 0 || row >= static_cast<int>(s.config.steps.size())) return;
    ClickStep& step = s.config.steps[static_cast<std::size_t>(row)];
    const int x = ReadInt(s.hwnd, IDC_STEP_X, -1);
    const int y = ReadInt(s.hwnd, IDC_STEP_Y, -1);
    step.delayMs = std::clamp(ReadInt(s.hwnd, IDC_STEP_DELAY, step.delayMs), 0, 60000);
    step.repeat = 1;
    int cw = 0, ch = 0;
    if (x >= 0 && y >= 0 && CurrentClientSize(s.target.gameWindow, cw, ch) && x < cw && y < ch) {
        step.x = x; step.y = y; step.baseW = cw; step.baseH = ch; step.valid = true;
    } else if (x < 0 || y < 0) {
        step.valid = false; step.x = -1; step.y = -1;
    }
    g_lastConfig = s.config;
}

void SyncConfig(State& s) {
    SaveStepEditor(s);
    s.config.templatePath = ReadText(s.hwnd, IDC_TEMPLATE);
    s.config.discardTemplatePath = ReadText(s.hwnd, IDC_TEMPLATE_DISCARD);
    s.config.closeTemplatePath = ReadText(s.hwnd, IDC_TEMPLATE_CLOSE);
    s.config.x = std::max(0, ReadInt(s.hwnd, IDC_X, 0));
    s.config.y = std::max(0, ReadInt(s.hwnd, IDC_Y, 0));
    s.config.w = std::max(0, ReadInt(s.hwnd, IDC_W, 0));
    s.config.h = std::max(0, ReadInt(s.hwnd, IDC_H, 0));
    s.config.thresholdPercent = std::clamp(ReadInt(s.hwnd, IDC_THRESHOLD, 90), 1, 100);
    int cw = 0, ch = 0;
    if (CurrentClientSize(s.target.gameWindow, cw, ch)) { s.config.roiBaseW = cw; s.config.roiBaseH = ch; }
    g_lastConfig = s.config;
}

void PickImageFile(State& s, int editId, std::wstring& dest) {
    SyncConfig(s);
    wchar_t file[MAX_PATH * 4]{};
    if (!dest.empty()) wcsncpy_s(file, dest.c_str(), _TRUNCATE);
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = s.hwnd;
    ofn.lpstrFilter = L"Ảnh mẫu (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0Tất cả file\0*.*\0\0";
    ofn.lpstrFile = file; ofn.nMaxFile = static_cast<DWORD>(std::size(file));
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;
    dest = file;
    SetDlgItemTextW(s.hwnd, editId, file);
    g_lastConfig = s.config;
}

void PickTemplate(State& s) { PickImageFile(s, IDC_TEMPLATE, s.config.templatePath); }
void PickDiscardTemplate(State& s) { PickImageFile(s, IDC_TEMPLATE_DISCARD, s.config.discardTemplatePath); }
void PickCloseTemplate(State& s) { PickImageFile(s, IDC_TEMPLATE_CLOSE, s.config.closeTemplatePath); }

''', 'config/image pick')

# F8 logic: rows 0..19, captureRow == -2 means the common post-discard point.
between('void ArmF8(State& s){', 'bool ResolveStepPoint(const ClickStep& step,int currentW,int currentH,int& x,int& y){', r'''void ArmF8(State& s){
    const int row=SelectedStep(s);if(row<0||row>=static_cast<int>(s.config.steps.size())){SetStatus(s.hwnd,L"F8: chọn một ô 1-20 trước");return;}
    s.captureRow=row;s.f8WasDown=(GetAsyncKeyState(VK_F8)&0x8000)!=0;SetTimer(s.hwnd,kF8PollTimer,30,nullptr);
    SetStatus(s.hwnd,L"ĐÃ ARM F8 CHO CLICK "+std::to_wstring(row+1)+L" • đưa chuột vào đúng ô đồ trong GAME rồi F8");
}

void ArmAfterDiscardF8(State& s){
    s.captureRow=-2;s.f8WasDown=(GetAsyncKeyState(VK_F8)&0x8000)!=0;SetTimer(s.hwnd,kF8PollTimer,30,nullptr);
    SetStatus(s.hwnd,L"ĐÃ ARM F8 CHO CLICK SAU VỨT • đưa chuột vào nút xác nhận/phía sau rồi F8");
}

void PollF8(State& s){
    const bool down=(GetAsyncKeyState(VK_F8)&0x8000)!=0;
    if(s.captureRow!=-1&&down&&!s.f8WasDown){
        POINT p{};GetCursorPos(&p);POINT client=p;
        if(!ScreenToClient(s.target.gameWindow,&client)){SetStatus(s.hwnd,L"F8 FAIL • ScreenToClient");}
        else{
            int cw=0,ch=0;if(!CurrentClientSize(s.target.gameWindow,cw,ch)||client.x<0||client.y<0||client.x>=cw||client.y>=ch){
                SetStatus(s.hwnd,L"F8: chuột chưa nằm trong client game • vẫn đang chờ F8");
            }else if(s.captureRow==-2){
                ClickStep& step=s.config.afterDiscard;step.x=client.x;step.y=client.y;step.baseW=cw;step.baseH=ch;step.valid=true;step.repeat=1;
                s.captureRow=-1;KillTimer(s.hwnd,kF8PollTimer);g_lastConfig=s.config;
                SetEditInt(s.hwnd,IDC_AFTER_X,step.x);SetEditInt(s.hwnd,IDC_AFTER_Y,step.y);
                SetStatus(s.hwnd,L"F8 PASS • CLICK SAU VỨT = "+std::to_wstring(step.x)+L","+std::to_wstring(step.y)+L" @ "+std::to_wstring(cw)+L"x"+std::to_wstring(ch));
            }else if(s.captureRow>=0&&s.captureRow<static_cast<int>(s.config.steps.size())){
                ClickStep& step=s.config.steps[static_cast<std::size_t>(s.captureRow)];
                step.x=client.x;step.y=client.y;step.baseW=cw;step.baseH=ch;step.valid=true;step.repeat=1;
                const int row=s.captureRow;s.captureRow=-1;KillTimer(s.hwnd,kF8PollTimer);g_lastConfig=s.config;RefreshStepList(s,row);
                SetStatus(s.hwnd,L"F8 PASS • CLICK "+std::to_wstring(row+1)+L" = "+std::to_wstring(step.x)+L","+std::to_wstring(step.y)+L" @ "+std::to_wstring(cw)+L"x"+std::to_wstring(ch));
            }
        }
    }
    s.f8WasDown=down;
}

''', 'f8')

# Replace validation + runtime.
between('bool ValidateChain(const State& s,std::wstring& error){', 'void BuildControls(State& s) {', r'''bool ValidateFilter(const State& s,std::wstring& error){
    if(s.config.templatePath.empty()){error=L"chưa chọn ẢNH 1 (đồ đúng)";return false;}
    if(s.config.discardTemplatePath.empty()){error=L"chưa chọn ảnh NÚT VỨT";return false;}
    if(s.config.closeTemplatePath.empty()){error=L"chưa chọn ảnh DẤU X";return false;}
    if(s.config.steps.size()!=kMaxClickSteps){error=L"phải có đúng 20 tọa click";return false;}
    for(std::size_t i=0;i<s.config.steps.size();++i){
        const ClickStep& step=s.config.steps[i];
        if(!step.valid||step.baseW<=0||step.baseH<=0){error=L"CLICK "+std::to_wstring(i+1)+L" chưa gán tọa F8";return false;}
        if(step.delayMs<0||step.delayMs>60000){error=L"Delay CLICK "+std::to_wstring(i+1)+L" không hợp lệ";return false;}
    }
    if(!s.config.afterDiscard.valid||s.config.afterDiscard.baseW<=0||s.config.afterDiscard.baseH<=0){
        error=L"chưa gán CLICK SAU VỨT bằng F8";return false;
    }
    return true;
}

void ResolveScanRoi(const Config& c,int currentW,int currentH,int& rx,int& ry,int& rw,int& rh){
    rx=c.x;ry=c.y;rw=c.w;rh=c.h;
    if(c.roiBaseW>0&&c.roiBaseH>0&&(c.roiBaseW!=currentW||c.roiBaseH!=currentH)){
        rx=ScaleCoord(rx,c.roiBaseW,currentW);ry=ScaleCoord(ry,c.roiBaseH,currentH);
        if(rw>0)rw=std::max(1,ScaleCoord(rw,c.roiBaseW,currentW));
        if(rh>0)rh=std::max(1,ScaleCoord(rh,c.roiBaseH,currentH));
    }
}

std::wstring ScoreText(double score){
    if(score<0.0)return L"N/A";wchar_t b[40]{};swprintf_s(b,L"%.2f%%",score*100.0);return b;
}

bool ScanLoaded(State& s,const Image& tpl,Match& match,std::wstring& backend,std::wstring& error,int& frameW,int& frameH){
    Image frame{};if(!CaptureClient(s.target.gameWindow,frame,backend,error))return false;
    frameW=frame.width;frameH=frame.height;int rx=0,ry=0,rw=0,rh=0;ResolveScanRoi(s.config,frame.width,frame.height,rx,ry,rw,rh);
    match=FindTemplate(frame,tpl,rx,ry,rw,rh,static_cast<double>(s.config.thresholdPercent)/100.0,error);
    return match.score>=0.0;
}

bool RawClick(State& s,int x,int y,int cw,int ch,std::wstring& error){
    if(!s.target.hiddenClick){error=L"không có RAW hidden-click callback";return false;}
    std::wstring detail;if(!s.target.hiddenClick(s.target.context,x,y,cw,ch,detail)){error=detail;return false;}
    return true;
}

void TestAfterDiscard(State& s){
    SyncConfig(s);int cw=0,ch=0,x=0,y=0;
    if(!CurrentClientSize(s.target.gameWindow,cw,ch)||!ResolveStepPoint(s.config.afterDiscard,cw,ch,x,y)){
        SetStatus(s.hwnd,L"TEST CLICK SAU VỨT FAIL • chưa F8 tọa hợp lệ");return;
    }
    std::wstring error;const bool ok=RawClick(s,x,y,cw,ch,error);
    SetStatus(s.hwnd,L"TEST CLICK SAU VỨT "+std::wstring(ok?L"PASS • ":L"FAIL • ")+std::to_wstring(x)+L","+std::to_wstring(y)+(error.empty()?L"":L" • "+error));
}

void FinishRun(State& s){s.runningChain=false;EnableWindow(GetDlgItem(s.hwnd,IDC_TEST),TRUE);}

void RunTest(State& s) {
    if(s.runningChain)return;SyncConfig(s);
    if(!s.target.gameWindow||!IsWindow(s.target.gameWindow)){SetStatus(s.hwnd,L"FAIL • cửa sổ game đã mất");return;}
    std::wstring error;if(!ValidateFilter(s,error)){SetStatus(s.hwnd,L"CHƯA THỂ CHẠY • "+error);return;}

    Image goodTpl{},discardTpl{},closeTpl{};
    if(!LoadImageWic(s.config.templatePath,goodTpl,error)){SetStatus(s.hwnd,L"ẢNH 1 FAIL • "+error);return;}
    if(!LoadImageWic(s.config.discardTemplatePath,discardTpl,error)){SetStatus(s.hwnd,L"ẢNH VỨT FAIL • "+error);return;}
    if(!LoadImageWic(s.config.closeTemplatePath,closeTpl,error)){SetStatus(s.hwnd,L"ẢNH X FAIL • "+error);return;}

    s.runningChain=true;EnableWindow(GetDlgItem(s.hwnd,IDC_TEST),FALSE);
    SetStatus(s.hwnd,L"BẮT ĐẦU LỌC 20 Ô • ESC để dừng khẩn • không MapID/combat/AUTO/state machine");

    for(std::size_t i=0;i<kMaxClickSteps;++i){
        std::size_t discardCount=0;
        for(;;){
            if((GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0){SetStatus(s.hwnd,L"ĐÃ DỪNG BẰNG ESC tại CLICK "+std::to_wstring(i+1));FinishRun(s);return;}
            int cw=0,ch=0,x=0,y=0;
            if(!CurrentClientSize(s.target.gameWindow,cw,ch)||!ResolveStepPoint(s.config.steps[i],cw,ch,x,y)){
                SetStatus(s.hwnd,L"FAIL • không resolve được CLICK "+std::to_wstring(i+1));FinishRun(s);return;
            }
            error.clear();
            SetStatus(s.hwnd,L"CLICK "+std::to_wstring(i+1)+L" • mở ô @ "+std::to_wstring(x)+L","+std::to_wstring(y)+L" • lượt vứt tại ô="+std::to_wstring(discardCount));
            if(!RawClick(s,x,y,cw,ch,error)){SetStatus(s.hwnd,L"RAW CLICK "+std::to_wstring(i+1)+L" FAIL • "+error);FinishRun(s);return;}
            const DWORD delay=static_cast<DWORD>(std::max(0,s.config.steps[i].delayMs));if(delay)Sleep(delay);

            Match good{};std::wstring backend;int fw=0,fh=0;error.clear();
            if(!ScanLoaded(s,goodTpl,good,backend,error,fw,fh)){
                SetStatus(s.hwnd,L"SCAN ẢNH 1 FAIL tại CLICK "+std::to_wstring(i+1)+L" • "+error);FinishRun(s);return;
            }

            if(good.found){
                // ĐÚNG ĐỒ: Ảnh 1 chỉ là điều kiện. Không click Ảnh 1. Tìm dấu X rồi click tâm X.
                Match close{};std::wstring backendX;int xw=0,xh=0;error.clear();
                if(!ScanLoaded(s,closeTpl,close,backendX,error,xw,xh)){
                    SetStatus(s.hwnd,L"ẢNH 1 PASS nhưng scan DẤU X FAIL tại CLICK "+std::to_wstring(i+1)+L" • "+error);FinishRun(s);return;
                }
                if(!close.found){
                    SetStatus(s.hwnd,L"ẢNH 1 PASS "+ScoreText(good.score)+L" • DẤU X NOT FOUND "+ScoreText(close.score)+L" tại CLICK "+std::to_wstring(i+1)+L" → DỪNG");FinishRun(s);return;
                }
                const int cx=close.x+closeTpl.width/2,cy=close.y+closeTpl.height/2;
                error.clear();if(!RawClick(s,cx,cy,xw,xh,error)){
                    SetStatus(s.hwnd,L"CLICK TÂM DẤU X FAIL tại CLICK "+std::to_wstring(i+1)+L" • "+error);FinishRun(s);return;
                }
                SetStatus(s.hwnd,L"CLICK "+std::to_wstring(i+1)+L" • ẢNH 1 PASS "+ScoreText(good.score)+L" → DẤU X PASS "+ScoreText(close.score)+L" → click tâm X → SANG CLICK "+std::to_wstring(i+2));
                if(delay)Sleep(delay);break;
            }

            // SAI ĐỒ: tìm NÚT VỨT, click tâm VỨT, click tọa sau VỨT, rồi quay lại đúng CLICK i.
            Match discard{};std::wstring backendD;int dw=0,dh=0;error.clear();
            if(!ScanLoaded(s,discardTpl,discard,backendD,error,dw,dh)){
                SetStatus(s.hwnd,L"ẢNH 1 NOT FOUND và scan NÚT VỨT FAIL tại CLICK "+std::to_wstring(i+1)+L" • "+error);FinishRun(s);return;
            }
            if(!discard.found){
                SetStatus(s.hwnd,L"CLICK "+std::to_wstring(i+1)+L" • ẢNH 1 NOT FOUND "+ScoreText(good.score)+L" • NÚT VỨT NOT FOUND "+ScoreText(discard.score)+L" → DỪNG");FinishRun(s);return;
            }
            const int dx=discard.x+discardTpl.width/2,dy=discard.y+discardTpl.height/2;
            error.clear();if(!RawClick(s,dx,dy,dw,dh,error)){
                SetStatus(s.hwnd,L"CLICK TÂM NÚT VỨT FAIL tại CLICK "+std::to_wstring(i+1)+L" • "+error);FinishRun(s);return;
            }
            if(delay)Sleep(delay);
            int ax=0,ay=0;int acw=0,ach=0;
            if(!CurrentClientSize(s.target.gameWindow,acw,ach)||!ResolveStepPoint(s.config.afterDiscard,acw,ach,ax,ay)){
                SetStatus(s.hwnd,L"CLICK SAU VỨT resolve FAIL");FinishRun(s);return;
            }
            error.clear();if(!RawClick(s,ax,ay,acw,ach,error)){
                SetStatus(s.hwnd,L"CLICK SAU VỨT FAIL tại CLICK "+std::to_wstring(i+1)+L" • "+error);FinishRun(s);return;
            }
            ++discardCount;
            SetStatus(s.hwnd,L"CLICK "+std::to_wstring(i+1)+L" • ẢNH 1 SAI → VỨT PASS "+ScoreText(discard.score)+L" → click sau VỨT → LẶP LẠI CLICK "+std::to_wstring(i+1));
            if(delay)Sleep(delay);
        }
    }
    FinishRun(s);
    SetStatus(s.hwnd,L"LỌC 20 CLICK HOÀN TẤT • chỉ tăng số thứ tự sau ẢNH 1 PASS + click tâm DẤU X PASS");
}

''', 'filter runtime')

# Replace the entire dialog layout with the approved 20-slot filter UI.
between('void BuildControls(State& s) {', 'LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){', r'''void BuildControls(State& s) {
    Add(s.hwnd,L"STATIC",(L"ACC TEST: "+s.target.accountLabel).c_str(),SS_LEFT|SS_CENTERIMAGE,18,10,930,24,0);

    Add(s.hwnd,L"STATIC",L"ẢNH 1 • ĐỒ ĐÚNG (chỉ scan, KHÔNG click):",SS_LEFT|SS_CENTERIMAGE,18,39,230,25,0);
    Add(s.hwnd,L"EDIT",s.config.templatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,250,39,585,25,IDC_TEMPLATE);
    Add(s.hwnd,L"BUTTON",L"CHỌN ẢNH 1",BS_PUSHBUTTON,842,38,116,27,IDC_PICK);
    Add(s.hwnd,L"STATIC",L"NÚT VỨT • scan rồi click đúng tâm:",SS_LEFT|SS_CENTERIMAGE,18,69,230,25,0);
    Add(s.hwnd,L"EDIT",s.config.discardTemplatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,250,69,585,25,IDC_TEMPLATE_DISCARD);
    Add(s.hwnd,L"BUTTON",L"CHỌN VỨT",BS_PUSHBUTTON,842,68,116,27,IDC_PICK_DISCARD);
    Add(s.hwnd,L"STATIC",L"DẤU X • scan rồi click đúng tâm:",SS_LEFT|SS_CENTERIMAGE,18,99,230,25,0);
    Add(s.hwnd,L"EDIT",s.config.closeTemplatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,250,99,585,25,IDC_TEMPLATE_CLOSE);
    Add(s.hwnd,L"BUTTON",L"CHỌN DẤU X",BS_PUSHBUTTON,842,98,116,27,IDC_PICK_CLOSE);

    Add(s.hwnd,L"STATIC",L"ROI chung: X",SS_LEFT|SS_CENTERIMAGE,18,132,70,25,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.x).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,88,132,58,25,IDC_X);
    Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,150,132,18,25,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.y).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,170,132,58,25,IDC_Y);
    Add(s.hwnd,L"STATIC",L"W",SS_CENTER|SS_CENTERIMAGE,232,132,18,25,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.w).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,252,132,62,25,IDC_W);
    Add(s.hwnd,L"STATIC",L"H",SS_CENTER|SS_CENTERIMAGE,318,132,18,25,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.h).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,338,132,62,25,IDC_H);
    Add(s.hwnd,L"STATIC",L"Ngưỡng %",SS_LEFT|SS_CENTERIMAGE,410,132,62,25,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.thresholdPercent).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,474,132,52,25,IDC_THRESHOLD);
    Add(s.hwnd,L"BUTTON",L"CHỌN VÙNG BẰNG CHUỘT",BS_PUSHBUTTON,538,129,185,30,IDC_PICK_REGION);
    Add(s.hwnd,L"BUTTON",L"XEM VÙNG",BS_PUSHBUTTON,730,129,104,30,IDC_PREVIEW_REGION);
    Add(s.hwnd,L"BUTTON",L"FULL CLIENT",BS_PUSHBUTTON,842,129,116,30,IDC_FULL_REGION);

    Add(s.hwnd,L"BUTTON",L"20 TỌA CLICK Ô ĐỒ • mỗi CLICK N: click ô → scan Ảnh 1 → giữ hoặc vứt → chỉ PASS mới sang N+1",BS_GROUPBOX,18,168,940,350,0);
    s.stepList=Add(s.hwnd,WC_LISTVIEWW,L"",LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS|WS_BORDER,32,193,912,225,IDC_STEP_LIST);
    ListView_SetExtendedListViewStyle(s.stepList,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);
    AddColumn(s.stepList,0,45,L"#");AddColumn(s.stepList,1,80,L"X");AddColumn(s.stepList,2,80,L"Y");
    AddColumn(s.stepList,3,110,L"Base size");AddColumn(s.stepList,4,90,L"Delay ms");AddColumn(s.stepList,5,485,L"Logic");
    Add(s.hwnd,L"STATIC",L"X:",SS_LEFT|SS_CENTERIMAGE,32,428,20,27,0);
    Add(s.hwnd,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,54,428,64,27,IDC_STEP_X);
    Add(s.hwnd,L"STATIC",L"Y:",SS_LEFT|SS_CENTERIMAGE,126,428,20,27,0);
    Add(s.hwnd,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,148,428,64,27,IDC_STEP_Y);
    Add(s.hwnd,L"STATIC",L"Delay:",SS_LEFT|SS_CENTERIMAGE,222,428,45,27,0);
    Add(s.hwnd,L"EDIT",L"500",WS_BORDER|ES_NUMBER|ES_CENTER,269,428,64,27,IDC_STEP_DELAY);
    Add(s.hwnd,L"BUTTON",L"LƯU CLICK",BS_PUSHBUTTON,344,428,105,27,IDC_STEP_SAVE);
    Add(s.hwnd,L"BUTTON",L"LẤY TỌA F8",BS_PUSHBUTTON,458,428,130,27,IDC_STEP_CAPTURE);
    Add(s.hwnd,L"BUTTON",L"TEST CLICK ẨN",BS_PUSHBUTTON,597,428,130,27,IDC_STEP_TEST);
    Add(s.hwnd,L"STATIC",L"Chọn CLICK 1-20 → đưa chuột vào ô đồ → F8.",SS_LEFT|SS_CENTERIMAGE,738,428,205,27,0);

    Add(s.hwnd,L"STATIC",L"CLICK SAU VỨT (tọa chung):",SS_LEFT|SS_CENTERIMAGE,32,467,160,27,0);
    Add(s.hwnd,L"STATIC",L"X",SS_CENTER|SS_CENTERIMAGE,194,467,16,27,0);
    Add(s.hwnd,L"EDIT",s.config.afterDiscard.valid?std::to_wstring(s.config.afterDiscard.x).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,212,467,65,27,IDC_AFTER_X);
    Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,283,467,16,27,0);
    Add(s.hwnd,L"EDIT",s.config.afterDiscard.valid?std::to_wstring(s.config.afterDiscard.y).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,301,467,65,27,IDC_AFTER_Y);
    Add(s.hwnd,L"BUTTON",L"LẤY F8 SAU VỨT",BS_PUSHBUTTON,377,467,155,27,IDC_AFTER_CAPTURE);
    Add(s.hwnd,L"BUTTON",L"TEST SAU VỨT",BS_PUSHBUTTON,541,467,135,27,IDC_AFTER_TEST);
    Add(s.hwnd,L"STATIC",L"Nút VỨT và DẤU X không dùng tọa cố định: luôn click tâm ảnh match.",SS_LEFT|SS_CENTERIMAGE,688,467,255,27,0);

    Add(s.hwnd,L"STATIC",L"SAI: CLICK N → Ảnh1 không thấy → tìm VỨT → click tâm VỨT → click SAU VỨT → quay lại CLICK N.  ĐÚNG: thấy Ảnh1 → tìm X → click tâm X → N+1.",SS_LEFT|SS_CENTERIMAGE,32,493,912,24,0);
    Add(s.hwnd,L"BUTTON",L"CHẠY TEST LỌC 20 CLICK ẨN",BS_DEFPUSHBUTTON,18,530,940,38,IDC_TEST);
    Add(s.hwnd,L"STATIC",L"Sẵn sàng. ESC = dừng khẩn khi chuỗi đang chạy.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,578,940,84,IDC_STATUS);
    Add(s.hwnd,L"BUTTON",L"ĐÓNG",BS_PUSHBUTTON,838,673,120,30,IDCANCEL);
    RefreshStepList(s);
}

''', 'build controls')

# Wire new buttons.
once('case IDC_PICK:PickTemplate(*s);return 0;\n', '''case IDC_PICK:PickTemplate(*s);return 0;
                case IDC_PICK_DISCARD:PickDiscardTemplate(*s);return 0;
                case IDC_PICK_CLOSE:PickCloseTemplate(*s);return 0;
''', 'button pick cases')
once('case IDC_STEP_TEST:TestSelectedStep(*s);return 0;\n', '''case IDC_STEP_TEST:TestSelectedStep(*s);return 0;
                case IDC_AFTER_CAPTURE:ArmAfterDiscardF8(*s);return 0;
                case IDC_AFTER_TEST:TestAfterDiscard(*s);return 0;
''', 'after discard cases')

# Make exactly 20 rows on open, preserving already captured rows if any.
once('State state{};state.target=target;state.config=g_lastConfig;RebaseForCurrentClient(state.config,target.gameWindow);',
     'State state{};state.target=target;state.config=g_lastConfig;RebaseForCurrentClient(state.config,target.gameWindow);if(state.config.steps.size()<kMaxClickSteps)state.config.steps.resize(kMaxClickSteps);if(state.config.steps.size()>kMaxClickSteps)state.config.steps.resize(kMaxClickSteps);', 'run init')
once('L"TEST SCAN ẢNH ẨN • VÙNG NHỎ + CHUỖI CLICK v2"',
     'L"TEST LỌC ĐỒ ẢNH ẨN • 20 CLICK • v3"', 'title')
once('CW_USEDEFAULT,CW_USEDEFAULT,910,715,',
     'CW_USEDEFAULT,CW_USEDEFAULT,1000,755,', 'window size')

# The old AddStep message is harmless/unreachable in v3 UI, but keep source semantics accurate.
text = text.replace('CHUỖI CLICK tối đa 10 dòng', 'BỘ LỌC cố định 20 CLICK')
text = text.replace('CHUỖI CLICK vượt 10 dòng', 'BỘ LỌC vượt 20 CLICK')

# Update common post-discard point from edit boxes on SyncConfig (F8 remains the canonical base-size capture).
insert_anchor = 's.config.thresholdPercent = std::clamp(ReadInt(s.hwnd, IDC_THRESHOLD, 90), 1, 100);\n'
insert = '''s.config.thresholdPercent = std::clamp(ReadInt(s.hwnd, IDC_THRESHOLD, 90), 1, 100);
    const int afterX = ReadInt(s.hwnd, IDC_AFTER_X, s.config.afterDiscard.x);
    const int afterY = ReadInt(s.hwnd, IDC_AFTER_Y, s.config.afterDiscard.y);
    int afterW=0,afterH=0;
    if(afterX>=0&&afterY>=0&&CurrentClientSize(s.target.gameWindow,afterW,afterH)&&afterX<afterW&&afterY<afterH){
        s.config.afterDiscard.x=afterX;s.config.afterDiscard.y=afterY;s.config.afterDiscard.baseW=afterW;s.config.afterDiscard.baseH=afterH;s.config.afterDiscard.valid=true;s.config.afterDiscard.repeat=1;
    }
'''
if insert_anchor not in text:
    raise SystemExit('sync after-discard anchor missing')
text = text.replace(insert_anchor, insert, 1)

required = [
    'kMaxClickSteps = 20',
    'ẢNH 1 • ĐỒ ĐÚNG',
    'NÚT VỨT • scan rồi click đúng tâm',
    'DẤU X • scan rồi click đúng tâm',
    'CLICK SAU VỨT',
    'LẶP LẠI CLICK',
    'RunTest(State& s)',
    'GetAsyncKeyState(VK_ESCAPE)',
]
for needle in required:
    if needle not in text:
        raise SystemExit(f'v3 generated contract missing: {needle}')

Path('src/image_scan_filter_v3.inl').write_text(text, encoding='utf-8')
print('FILTER V3 generated PASS • 20 slots • good/discard/X • retry same slot')
