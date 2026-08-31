from pathlib import Path

src_path = Path('src/image_scan_filter_v3.inl')
if not src_path.exists():
    raise SystemExit('missing src/image_scan_filter_v3.inl; generate v3 first')

text = src_path.read_text(encoding='utf-8')


def replace_span(source: str, start: str, end: str, replacement: str) -> str:
    i = source.find(start)
    if i < 0:
        raise SystemExit(f'missing start anchor: {start}')
    j = source.find(end, i)
    if j < 0:
        raise SystemExit(f'missing end anchor: {end}')
    return source[:i] + replacement.rstrip() + '\n\n' + source[j:]


text = text.replace('ThanLongImageFilterTestPocV3', 'ThanLongImageFilterTestPocV4')
text = text.replace('constexpr UINT_PTR kF8PollTimer = 91;\n',
                    'constexpr UINT_PTR kF8PollTimer = 91;\nconstexpr UINT_PTR kRunTimer = 92;\nconstexpr UINT kProbeIntervalMs = 80;\n')
text = text.replace('constexpr std::size_t kMaxClickSteps = 20;', 'constexpr std::size_t kMaxClickSteps = 20;')
text = text.replace('    int delayMs = 500;', '    int delayMs = 1500;')
text = text.replace('constexpr int IDC_AFTER_TEST = 1047;\n', '''constexpr int IDC_AFTER_TEST = 1047;
constexpr int IDC_DISCARD_X = 1050;
constexpr int IDC_DISCARD_Y = 1051;
constexpr int IDC_DISCARD_W = 1052;
constexpr int IDC_DISCARD_H = 1053;
constexpr int IDC_DISCARD_REGION = 1054;
constexpr int IDC_DISCARD_PREVIEW = 1055;
constexpr int IDC_DISCARD_FULL = 1056;
constexpr int IDC_CLOSE_X = 1060;
constexpr int IDC_CLOSE_Y = 1061;
constexpr int IDC_CLOSE_W = 1062;
constexpr int IDC_CLOSE_H = 1063;
constexpr int IDC_CLOSE_REGION = 1064;
constexpr int IDC_CLOSE_PREVIEW = 1065;
constexpr int IDC_CLOSE_FULL = 1066;
''')

config_block = r'''struct ScanRoi {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int baseW = 0;
    int baseH = 0;
};

struct Config {
    // Ảnh 1 = dấu hiệu món đồ ĐÚNG. Chỉ scan, KHÔNG click ảnh 1.
    std::wstring templatePath;
    std::wstring discardTemplatePath;
    std::wstring closeTemplatePath;

    // ROI ẢNH 1 giữ các field cũ để tương thích cấu hình v3.
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int roiBaseW = 0;
    int roiBaseH = 0;

    // V4: NÚT VỨT và DẤU X có ROI riêng. Cả 3 ROI dùng trên CÙNG MỘT frame capture.
    ScanRoi discardRoi{};
    ScanRoi closeRoi{};

    int thresholdPercent = 90;
    std::vector<ClickStep> steps;
    ClickStep afterDiscard;
};'''
text = replace_span(text, 'struct Config {', 'Config g_lastConfig{};', config_block)

state_block = r'''enum class RunPhase {
    Idle,
    WaitItemReady,
    WaitDiscardGone,
    WaitPopupGoneAfterConfirm,
    WaitCloseGone,
};

struct State {
    Target target{};
    Config config{};
    HWND hwnd = nullptr;
    HWND stepList = nullptr;
    int captureRow = -1;
    bool f8WasDown = false;
    bool runningChain = false;

    // Runtime v4: template load đúng 1 lần khi Start; timer/state không khóa UI thread bằng Sleep.
    Image goodTpl{};
    Image discardTpl{};
    Image closeTpl{};
    RunPhase runPhase = RunPhase::Idle;
    std::size_t slotIndex = 0;
    std::size_t discardCount = 0;
    ULONGLONG nextProbeTick = 0;
    ULONGLONG phaseDeadlineTick = 0;
};'''
text = replace_span(text, 'struct State {', 'struct RegionPickerState {', state_block)

rebase_block = r'''void RebaseNamedRoi(ScanRoi& r, int cw, int ch) {
    if (r.baseW > 0 && r.baseH > 0 && (r.baseW != cw || r.baseH != ch)) {
        r.x = std::max(0, ScaleCoord(r.x, r.baseW, cw));
        r.y = std::max(0, ScaleCoord(r.y, r.baseH, ch));
        if (r.w > 0) r.w = std::max(1, ScaleCoord(r.w, r.baseW, cw));
        if (r.h > 0) r.h = std::max(1, ScaleCoord(r.h, r.baseH, ch));
    }
    r.baseW = cw;
    r.baseH = ch;
}

void RebaseClickStep(ClickStep& step, int cw, int ch) {
    if (!step.valid || step.baseW <= 0 || step.baseH <= 0) return;
    if (step.baseW != cw || step.baseH != ch) {
        step.x = ScaleCoord(step.x, step.baseW, cw);
        step.y = ScaleCoord(step.y, step.baseH, ch);
        step.baseW = cw;
        step.baseH = ch;
    }
}

void RebaseForCurrentClient(Config& c, HWND gameWindow) {
    int cw = 0, ch = 0;
    if (!CurrentClientSize(gameWindow, cw, ch)) return;
    if (c.roiBaseW > 0 && c.roiBaseH > 0 && (c.roiBaseW != cw || c.roiBaseH != ch)) {
        c.x = std::max(0, ScaleCoord(c.x, c.roiBaseW, cw));
        c.y = std::max(0, ScaleCoord(c.y, c.roiBaseH, ch));
        if (c.w > 0) c.w = std::max(1, ScaleCoord(c.w, c.roiBaseW, cw));
        if (c.h > 0) c.h = std::max(1, ScaleCoord(c.h, c.roiBaseH, ch));
    }
    c.roiBaseW = cw;
    c.roiBaseH = ch;
    RebaseNamedRoi(c.discardRoi, cw, ch);
    RebaseNamedRoi(c.closeRoi, cw, ch);
    for (ClickStep& step : c.steps) RebaseClickStep(step, cw, ch);
    RebaseClickStep(c.afterDiscard, cw, ch);
}'''
text = replace_span(text, 'void RebaseForCurrentClient(Config& c, HWND gameWindow)', 'bool LoadImageWic', rebase_block)

sync_block = r'''void SyncConfig(State& s) {
    SaveStepEditor(s);
    s.config.templatePath = ReadText(s.hwnd, IDC_TEMPLATE);
    s.config.discardTemplatePath = ReadText(s.hwnd, IDC_TEMPLATE_DISCARD);
    s.config.closeTemplatePath = ReadText(s.hwnd, IDC_TEMPLATE_CLOSE);

    s.config.x = std::max(0, ReadInt(s.hwnd, IDC_X, 0));
    s.config.y = std::max(0, ReadInt(s.hwnd, IDC_Y, 0));
    s.config.w = std::max(0, ReadInt(s.hwnd, IDC_W, 0));
    s.config.h = std::max(0, ReadInt(s.hwnd, IDC_H, 0));

    s.config.discardRoi.x = std::max(0, ReadInt(s.hwnd, IDC_DISCARD_X, 0));
    s.config.discardRoi.y = std::max(0, ReadInt(s.hwnd, IDC_DISCARD_Y, 0));
    s.config.discardRoi.w = std::max(0, ReadInt(s.hwnd, IDC_DISCARD_W, 0));
    s.config.discardRoi.h = std::max(0, ReadInt(s.hwnd, IDC_DISCARD_H, 0));

    s.config.closeRoi.x = std::max(0, ReadInt(s.hwnd, IDC_CLOSE_X, 0));
    s.config.closeRoi.y = std::max(0, ReadInt(s.hwnd, IDC_CLOSE_Y, 0));
    s.config.closeRoi.w = std::max(0, ReadInt(s.hwnd, IDC_CLOSE_W, 0));
    s.config.closeRoi.h = std::max(0, ReadInt(s.hwnd, IDC_CLOSE_H, 0));

    s.config.thresholdPercent = std::clamp(ReadInt(s.hwnd, IDC_THRESHOLD, 90), 1, 100);

    const int afterX = ReadInt(s.hwnd, IDC_AFTER_X, s.config.afterDiscard.x);
    const int afterY = ReadInt(s.hwnd, IDC_AFTER_Y, s.config.afterDiscard.y);
    int cw = 0, ch = 0;
    if (CurrentClientSize(s.target.gameWindow, cw, ch)) {
        s.config.roiBaseW = cw;
        s.config.roiBaseH = ch;
        s.config.discardRoi.baseW = cw;
        s.config.discardRoi.baseH = ch;
        s.config.closeRoi.baseW = cw;
        s.config.closeRoi.baseH = ch;
        if (afterX >= 0 && afterY >= 0 && afterX < cw && afterY < ch) {
            s.config.afterDiscard.x = afterX;
            s.config.afterDiscard.y = afterY;
            s.config.afterDiscard.baseW = cw;
            s.config.afterDiscard.baseH = ch;
            s.config.afterDiscard.valid = true;
            s.config.afterDiscard.repeat = 1;
        }
    }
    g_lastConfig = s.config;
}'''
text = replace_span(text, 'void SyncConfig(State& s)', 'void PickImageFile', sync_block)

region_block = r'''void SelectRegion(State& s) {
    SyncConfig(s);
    Image frame{}; std::wstring backend, error;
    if (!CaptureClient(s.target.gameWindow, frame, backend, error)) {
        SetStatus(s.hwnd, L"CHỌN ROI ẢNH 1 FAIL • " + error); return;
    }
    RECT region{};
    if (!PickRegionModal(s.hwnd, frame, region)) {
        SetStatus(s.hwnd, L"ROI ẢNH 1: đã hủy, giữ nguyên vùng cũ"); return;
    }
    s.config.x = region.left; s.config.y = region.top;
    s.config.w = region.right - region.left; s.config.h = region.bottom - region.top;
    s.config.roiBaseW = frame.width; s.config.roiBaseH = frame.height;
    SetEditInt(s.hwnd, IDC_X, s.config.x); SetEditInt(s.hwnd, IDC_Y, s.config.y);
    SetEditInt(s.hwnd, IDC_W, s.config.w); SetEditInt(s.hwnd, IDC_H, s.config.h);
    g_lastConfig = s.config;
    SetStatus(s.hwnd, L"ROI ẢNH 1 PASS • " + std::to_wstring(s.config.x) + L"," + std::to_wstring(s.config.y) +
                       L"/" + std::to_wstring(s.config.w) + L"x" + std::to_wstring(s.config.h) + L" • " + backend);
}

void PreviewRegion(State& s) {
    SyncConfig(s);
    Image frame{}, crop{}; std::wstring backend, error;
    if (!CaptureClient(s.target.gameWindow, frame, backend, error)) { SetStatus(s.hwnd, L"XEM ROI ẢNH 1 FAIL • " + error); return; }
    if (!CropImage(frame, s.config.x, s.config.y, s.config.w, s.config.h, crop, error)) { SetStatus(s.hwnd, L"XEM ROI ẢNH 1 FAIL • " + error); return; }
    ShowPreviewModal(s.hwnd, crop, L"ROI ẢNH 1 • " + std::to_wstring(crop.width) + L"x" + std::to_wstring(crop.height) + L" • " + backend);
}

void ResetFullRegion(State& s) {
    int cw=0,ch=0;CurrentClientSize(s.target.gameWindow,cw,ch);
    s.config.x=0;s.config.y=0;s.config.w=0;s.config.h=0;s.config.roiBaseW=cw;s.config.roiBaseH=ch;
    SetEditInt(s.hwnd,IDC_X,0);SetEditInt(s.hwnd,IDC_Y,0);SetEditInt(s.hwnd,IDC_W,0);SetEditInt(s.hwnd,IDC_H,0);
    g_lastConfig=s.config;SetStatus(s.hwnd,L"ROI ẢNH 1 = FULL CLIENT");
}

void SelectNamedRegion(State& s, ScanRoi& roi, int xId, int yId, int wId, int hId, const wchar_t* label) {
    SyncConfig(s);
    Image frame{}; std::wstring backend, error;
    if (!CaptureClient(s.target.gameWindow, frame, backend, error)) {
        SetStatus(s.hwnd, std::wstring(L"CHỌN ") + label + L" FAIL • " + error); return;
    }
    RECT region{};
    if (!PickRegionModal(s.hwnd, frame, region)) {
        SetStatus(s.hwnd, std::wstring(label) + L": đã hủy, giữ nguyên vùng cũ"); return;
    }
    roi.x = region.left; roi.y = region.top; roi.w = region.right-region.left; roi.h = region.bottom-region.top;
    roi.baseW = frame.width; roi.baseH = frame.height;
    SetEditInt(s.hwnd,xId,roi.x);SetEditInt(s.hwnd,yId,roi.y);SetEditInt(s.hwnd,wId,roi.w);SetEditInt(s.hwnd,hId,roi.h);
    g_lastConfig=s.config;
    SetStatus(s.hwnd,std::wstring(label)+L" PASS • "+std::to_wstring(roi.x)+L","+std::to_wstring(roi.y)+L"/"+std::to_wstring(roi.w)+L"x"+std::to_wstring(roi.h)+L" • "+backend);
}

void PreviewNamedRegion(State& s, const ScanRoi& roi, const wchar_t* label) {
    SyncConfig(s);
    Image frame{},crop{};std::wstring backend,error;
    if(!CaptureClient(s.target.gameWindow,frame,backend,error)){SetStatus(s.hwnd,std::wstring(L"XEM ")+label+L" FAIL • "+error);return;}
    int rx=roi.x,ry=roi.y,rw=roi.w,rh=roi.h;
    if(roi.baseW>0&&roi.baseH>0&&(roi.baseW!=frame.width||roi.baseH!=frame.height)){
        rx=ScaleCoord(rx,roi.baseW,frame.width);ry=ScaleCoord(ry,roi.baseH,frame.height);
        if(rw>0)rw=std::max(1,ScaleCoord(rw,roi.baseW,frame.width));
        if(rh>0)rh=std::max(1,ScaleCoord(rh,roi.baseH,frame.height));
    }
    if(!CropImage(frame,rx,ry,rw,rh,crop,error)){SetStatus(s.hwnd,std::wstring(L"XEM ")+label+L" FAIL • "+error);return;}
    ShowPreviewModal(s.hwnd,crop,std::wstring(label)+L" • "+std::to_wstring(crop.width)+L"x"+std::to_wstring(crop.height)+L" • "+backend);
}

void ResetNamedRegion(State& s, ScanRoi& roi, int xId, int yId, int wId, int hId, const wchar_t* label) {
    int cw=0,ch=0;CurrentClientSize(s.target.gameWindow,cw,ch);roi={};roi.baseW=cw;roi.baseH=ch;
    SetEditInt(s.hwnd,xId,0);SetEditInt(s.hwnd,yId,0);SetEditInt(s.hwnd,wId,0);SetEditInt(s.hwnd,hId,0);
    g_lastConfig=s.config;SetStatus(s.hwnd,std::wstring(label)+L" = FULL CLIENT");
}

void SelectDiscardRegion(State& s){SelectNamedRegion(s,s.config.discardRoi,IDC_DISCARD_X,IDC_DISCARD_Y,IDC_DISCARD_W,IDC_DISCARD_H,L"ROI VỨT");}
void PreviewDiscardRegion(State& s){PreviewNamedRegion(s,s.config.discardRoi,L"ROI VỨT");}
void ResetDiscardRegion(State& s){ResetNamedRegion(s,s.config.discardRoi,IDC_DISCARD_X,IDC_DISCARD_Y,IDC_DISCARD_W,IDC_DISCARD_H,L"ROI VỨT");}
void SelectCloseRegion(State& s){SelectNamedRegion(s,s.config.closeRoi,IDC_CLOSE_X,IDC_CLOSE_Y,IDC_CLOSE_W,IDC_CLOSE_H,L"ROI DẤU X");}
void PreviewCloseRegion(State& s){PreviewNamedRegion(s,s.config.closeRoi,L"ROI DẤU X");}
void ResetCloseRegion(State& s){ResetNamedRegion(s,s.config.closeRoi,IDC_CLOSE_X,IDC_CLOSE_Y,IDC_CLOSE_W,IDC_CLOSE_H,L"ROI DẤU X");}'''
text = replace_span(text, 'void SelectRegion(State& s)', 'void AddStep(State& s)', region_block)

# Delay cũ trở thành timeout fail-closed của từng CLICK, không còn Sleep.
text = text.replace('step.delayMs = std::clamp(ReadInt(s.hwnd, IDC_STEP_DELAY, step.delayMs), 0, 60000);',
                    'step.delayMs = std::clamp(ReadInt(s.hwnd, IDC_STEP_DELAY, step.delayMs), 300, 10000);')
text = text.replace('if(step.delayMs<0||step.delayMs>60000)', 'if(step.delayMs<300||step.delayMs>10000)')

runtime_block = r'''void ResolveGoodRoi(const Config& c,int currentW,int currentH,int& rx,int& ry,int& rw,int& rh){
    rx=c.x;ry=c.y;rw=c.w;rh=c.h;
    if(c.roiBaseW>0&&c.roiBaseH>0&&(c.roiBaseW!=currentW||c.roiBaseH!=currentH)){
        rx=ScaleCoord(rx,c.roiBaseW,currentW);ry=ScaleCoord(ry,c.roiBaseH,currentH);
        if(rw>0)rw=std::max(1,ScaleCoord(rw,c.roiBaseW,currentW));
        if(rh>0)rh=std::max(1,ScaleCoord(rh,c.roiBaseH,currentH));
    }
}

void ResolveNamedRoi(const ScanRoi& r,int currentW,int currentH,int& rx,int& ry,int& rw,int& rh){
    rx=r.x;ry=r.y;rw=r.w;rh=r.h;
    if(r.baseW>0&&r.baseH>0&&(r.baseW!=currentW||r.baseH!=currentH)){
        rx=ScaleCoord(rx,r.baseW,currentW);ry=ScaleCoord(ry,r.baseH,currentH);
        if(rw>0)rw=std::max(1,ScaleCoord(rw,r.baseW,currentW));
        if(rh>0)rh=std::max(1,ScaleCoord(rh,r.baseH,currentH));
    }
}

std::wstring ScoreText(double score){
    if(score<0.0)return L"N/A";wchar_t b[40]{};swprintf_s(b,L"%.2f%%",score*100.0);return b;
}

bool ScanFrameRect(const State& s,const Image& frame,const Image& tpl,int rx,int ry,int rw,int rh,Match& match,std::wstring& error){
    match=FindTemplate(frame,tpl,rx,ry,rw,rh,static_cast<double>(s.config.thresholdPercent)/100.0,error);
    return match.score>=0.0;
}

bool ScanGoodOnFrame(const State& s,const Image& frame,Match& match,std::wstring& error){
    int rx=0,ry=0,rw=0,rh=0;ResolveGoodRoi(s.config,frame.width,frame.height,rx,ry,rw,rh);
    return ScanFrameRect(s,frame,s.goodTpl,rx,ry,rw,rh,match,error);
}

bool ScanDiscardOnFrame(const State& s,const Image& frame,Match& match,std::wstring& error){
    int rx=0,ry=0,rw=0,rh=0;ResolveNamedRoi(s.config.discardRoi,frame.width,frame.height,rx,ry,rw,rh);
    return ScanFrameRect(s,frame,s.discardTpl,rx,ry,rw,rh,match,error);
}

bool ScanCloseOnFrame(const State& s,const Image& frame,Match& match,std::wstring& error){
    int rx=0,ry=0,rw=0,rh=0;ResolveNamedRoi(s.config.closeRoi,frame.width,frame.height,rx,ry,rw,rh);
    return ScanFrameRect(s,frame,s.closeTpl,rx,ry,rw,rh,match,error);
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

int SlotTimeoutMs(const State& s){
    if(s.slotIndex<s.config.steps.size())return std::clamp(s.config.steps[s.slotIndex].delayMs,300,10000);
    return 1500;
}

void ArmRunPhase(State& s,RunPhase phase,UINT firstProbeDelay=40){
    const ULONGLONG now=GetTickCount64();s.runPhase=phase;s.nextProbeTick=now+firstProbeDelay;s.phaseDeadlineTick=now+static_cast<ULONGLONG>(SlotTimeoutMs(s));
}

void FinishRun(State& s,const std::wstring& status){
    KillTimer(s.hwnd,kRunTimer);s.runningChain=false;s.runPhase=RunPhase::Idle;EnableWindow(GetDlgItem(s.hwnd,IDC_TEST),TRUE);SetStatus(s.hwnd,status);
}

bool ClickCurrentSlot(State& s,std::wstring& error){
    if(s.slotIndex>=s.config.steps.size()){error=L"slot index vượt cấu hình";return false;}
    int cw=0,ch=0,x=0,y=0;if(!CurrentClientSize(s.target.gameWindow,cw,ch)||!ResolveStepPoint(s.config.steps[s.slotIndex],cw,ch,x,y)){error=L"không resolve được tọa CLICK";return false;}
    return RawClick(s,x,y,cw,ch,error);
}

bool ClickAfterDiscard(State& s,std::wstring& error){
    int cw=0,ch=0,x=0,y=0;if(!CurrentClientSize(s.target.gameWindow,cw,ch)||!ResolveStepPoint(s.config.afterDiscard,cw,ch,x,y)){error=L"không resolve được CLICK SAU VỨT";return false;}
    return RawClick(s,x,y,cw,ch,error);
}

bool ProbeTimedOut(const State& s,ULONGLONG now){return now>=s.phaseDeadlineTick;}
void ScheduleNextProbe(State& s,ULONGLONG now){s.nextProbeTick=now+kProbeIntervalMs;}

void ProcessRunTick(State& s){
    if(!s.runningChain)return;
    if((GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0){FinishRun(s,L"ĐÃ DỪNG BẰNG ESC tại CLICK "+std::to_wstring(s.slotIndex+1));return;}
    if(!s.target.gameWindow||!IsWindow(s.target.gameWindow)){FinishRun(s,L"FAIL • cửa sổ game đã mất");return;}
    const ULONGLONG now=GetTickCount64();if(now<s.nextProbeTick)return;

    // V4: MỖI PROBE CHỈ CHỤP HWND ĐÚNG 1 LẦN. Mọi scan cần thiết dùng chung frame này.
    Image frame{};std::wstring backend,error;
    if(!CaptureClient(s.target.gameWindow,frame,backend,error)){FinishRun(s,L"CAPTURE FAIL • "+error);return;}

    if(s.runPhase==RunPhase::WaitItemReady){
        Match good{};error.clear();
        if(!ScanGoodOnFrame(s,frame,good,error)){FinishRun(s,L"ROI ẢNH 1 FAIL • "+error);return;}
        if(good.found){
            Match close{};error.clear();
            if(!ScanCloseOnFrame(s,frame,close,error)){FinishRun(s,L"ROI DẤU X FAIL • "+error);return;}
            if(close.found){
                const int cx=close.x+s.closeTpl.width/2,cy=close.y+s.closeTpl.height/2;
                if(!RawClick(s,cx,cy,frame.width,frame.height,error)){FinishRun(s,L"CLICK TÂM X FAIL • "+error);return;}
                SetStatus(s.hwnd,L"CLICK "+std::to_wstring(s.slotIndex+1)+L" • ẢNH 1 PASS "+ScoreText(good.score)+L" • X PASS "+ScoreText(close.score)+L" • đợi X biến mất");
                ArmRunPhase(s,RunPhase::WaitCloseGone,40);return;
            }
            if(ProbeTimedOut(s,now)){FinishRun(s,L"TIMEOUT • ẢNH 1 đã PASS nhưng DẤU X chưa xuất hiện tại CLICK "+std::to_wstring(s.slotIndex+1));return;}
            ScheduleNextProbe(s,now);return;
        }

        Match discard{};error.clear();
        if(!ScanDiscardOnFrame(s,frame,discard,error)){FinishRun(s,L"ROI VỨT FAIL • "+error);return;}
        if(discard.found){
            const int dx=discard.x+s.discardTpl.width/2,dy=discard.y+s.discardTpl.height/2;
            if(!RawClick(s,dx,dy,frame.width,frame.height,error)){FinishRun(s,L"CLICK TÂM VỨT FAIL • "+error);return;}
            SetStatus(s.hwnd,L"CLICK "+std::to_wstring(s.slotIndex+1)+L" • ẢNH 1 SAI • VỨT PASS "+ScoreText(discard.score)+L" • đợi nút VỨT biến mất");
            ArmRunPhase(s,RunPhase::WaitDiscardGone,40);return;
        }
        if(ProbeTimedOut(s,now)){FinishRun(s,L"TIMEOUT • chưa thấy ẢNH 1 hoặc NÚT VỨT tại CLICK "+std::to_wstring(s.slotIndex+1));return;}
        ScheduleNextProbe(s,now);return;
    }

    if(s.runPhase==RunPhase::WaitDiscardGone){
        Match discard{};error.clear();
        if(!ScanDiscardOnFrame(s,frame,discard,error)){FinishRun(s,L"ROI VỨT FAIL • "+error);return;}
        if(!discard.found){
            if(!ClickAfterDiscard(s,error)){FinishRun(s,L"CLICK SAU VỨT FAIL • "+error);return;}
            SetStatus(s.hwnd,L"CLICK "+std::to_wstring(s.slotIndex+1)+L" • VỨT đã biến mất → click SAU VỨT → đợi popup đóng");
            ArmRunPhase(s,RunPhase::WaitPopupGoneAfterConfirm,40);return;
        }
        if(ProbeTimedOut(s,now)){FinishRun(s,L"TIMEOUT • nút VỨT không biến mất tại CLICK "+std::to_wstring(s.slotIndex+1));return;}
        ScheduleNextProbe(s,now);return;
    }

    if(s.runPhase==RunPhase::WaitPopupGoneAfterConfirm){
        Match good{},discard{},close{};std::wstring e1,e2,e3;
        const bool ok1=ScanGoodOnFrame(s,frame,good,e1);const bool ok2=ScanDiscardOnFrame(s,frame,discard,e2);const bool ok3=ScanCloseOnFrame(s,frame,close,e3);
        if(!ok1||!ok2||!ok3){FinishRun(s,L"SCAN trạng thái sau VỨT FAIL • "+(!ok1?e1:(!ok2?e2:e3)));return;}
        if(!good.found&&!discard.found&&!close.found){
            ++s.discardCount;
            if(!ClickCurrentSlot(s,error)){FinishRun(s,L"LẶP CLICK "+std::to_wstring(s.slotIndex+1)+L" FAIL • "+error);return;}
            SetStatus(s.hwnd,L"CLICK "+std::to_wstring(s.slotIndex+1)+L" • popup đã đóng • đã vứt "+std::to_wstring(s.discardCount)+L" món → LẶP LẠI CÙNG Ô");
            ArmRunPhase(s,RunPhase::WaitItemReady,40);return;
        }
        if(ProbeTimedOut(s,now)){FinishRun(s,L"TIMEOUT • popup sau VỨT chưa đóng tại CLICK "+std::to_wstring(s.slotIndex+1));return;}
        ScheduleNextProbe(s,now);return;
    }

    if(s.runPhase==RunPhase::WaitCloseGone){
        Match close{};error.clear();
        if(!ScanCloseOnFrame(s,frame,close,error)){FinishRun(s,L"ROI DẤU X FAIL • "+error);return;}
        if(!close.found){
            ++s.slotIndex;s.discardCount=0;
            if(s.slotIndex>=kMaxClickSteps){FinishRun(s,L"LỌC 20 CLICK HOÀN TẤT • 1 capture/probe • 3 ROI riêng • không Sleep khóa UI");return;}
            if(!ClickCurrentSlot(s,error)){FinishRun(s,L"CLICK "+std::to_wstring(s.slotIndex+1)+L" FAIL • "+error);return;}
            SetStatus(s.hwnd,L"DẤU X đã biến mất → SANG CLICK "+std::to_wstring(s.slotIndex+1));
            ArmRunPhase(s,RunPhase::WaitItemReady,40);return;
        }
        if(ProbeTimedOut(s,now)){FinishRun(s,L"TIMEOUT • DẤU X không biến mất tại CLICK "+std::to_wstring(s.slotIndex+1));return;}
        ScheduleNextProbe(s,now);return;
    }
}

void RunTest(State& s) {
    if(s.runningChain)return;SyncConfig(s);
    if(!s.target.gameWindow||!IsWindow(s.target.gameWindow)){SetStatus(s.hwnd,L"FAIL • cửa sổ game đã mất");return;}
    std::wstring error;if(!ValidateFilter(s,error)){SetStatus(s.hwnd,L"CHƯA THỂ CHẠY • "+error);return;}
    if(!LoadImageWic(s.config.templatePath,s.goodTpl,error)){SetStatus(s.hwnd,L"ẢNH 1 FAIL • "+error);return;}
    if(!LoadImageWic(s.config.discardTemplatePath,s.discardTpl,error)){SetStatus(s.hwnd,L"ẢNH VỨT FAIL • "+error);return;}
    if(!LoadImageWic(s.config.closeTemplatePath,s.closeTpl,error)){SetStatus(s.hwnd,L"ẢNH X FAIL • "+error);return;}

    s.runningChain=true;s.slotIndex=0;s.discardCount=0;s.runPhase=RunPhase::Idle;EnableWindow(GetDlgItem(s.hwnd,IDC_TEST),FALSE);
    if(!ClickCurrentSlot(s,error)){FinishRun(s,L"CLICK 1 FAIL • "+error);return;}
    ArmRunPhase(s,RunPhase::WaitItemReady,40);SetTimer(s.hwnd,kRunTimer,25,nullptr);
    SetStatus(s.hwnd,L"V4 START • CLICK 1 → probe theo trạng thái • 1 PrintWindow/probe • ROI ẢNH1/VỨT/X riêng • Timeout fail-closed");
}'''
text = replace_span(text, 'void ResolveScanRoi', 'void BuildControls(State& s)', runtime_block)

controls_block = r'''void AddRoiEditors(State& s,int y,int xId,int yId,int wId,int hId,int pickId,int previewId,int fullId,
                   int x,int yy,int w,int h,const wchar_t* label){
    Add(s.hwnd,L"STATIC",label,SS_LEFT|SS_CENTERIMAGE,x,y,80,24,0);
    Add(s.hwnd,L"STATIC",L"X",SS_CENTER|SS_CENTERIMAGE,x+82,y,16,24,0);Add(s.hwnd,L"EDIT",std::to_wstring(xx).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,x+98,y,54,24,xId);
    Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,x+154,y,16,24,0);Add(s.hwnd,L"EDIT",std::to_wstring(yy).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,x+170,y,54,24,yId);
    Add(s.hwnd,L"STATIC",L"W",SS_CENTER|SS_CENTERIMAGE,x+226,y,16,24,0);Add(s.hwnd,L"EDIT",std::to_wstring(w).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,x+242,y,58,24,wId);
    Add(s.hwnd,L"STATIC",L"H",SS_CENTER|SS_CENTERIMAGE,x+302,y,16,24,0);Add(s.hwnd,L"EDIT",std::to_wstring(h).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,x+318,y,58,24,hId);
    Add(s.hwnd,L"BUTTON",L"CHỌN VÙNG",BS_PUSHBUTTON,x+386,y-2,112,28,pickId);
    Add(s.hwnd,L"BUTTON",L"XEM",BS_PUSHBUTTON,x+504,y-2,68,28,previewId);
    Add(s.hwnd,L"BUTTON",L"FULL",BS_PUSHBUTTON,x+578,y-2,68,28,fullId);
}

void BuildControls(State& s) {
    Add(s.hwnd,L"STATIC",(L"ACC TEST: "+s.target.accountLabel).c_str(),SS_LEFT|SS_CENTERIMAGE,18,9,990,23,0);

    Add(s.hwnd,L"STATIC",L"ẢNH 1 • ĐỒ ĐÚNG (chỉ scan):",SS_LEFT|SS_CENTERIMAGE,18,38,205,25,0);
    Add(s.hwnd,L"EDIT",s.config.templatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,225,38,555,25,IDC_TEMPLATE);
    Add(s.hwnd,L"BUTTON",L"CHỌN ẢNH 1",BS_PUSHBUTTON,790,37,110,27,IDC_PICK);
    AddRoiEditors(s,69,IDC_X,IDC_Y,IDC_W,IDC_H,IDC_PICK_REGION,IDC_PREVIEW_REGION,IDC_FULL_REGION,s.config.x,s.config.y,s.config.w,s.config.h,L"ROI ẢNH 1");
    Add(s.hwnd,L"STATIC",L"Ngưỡng %",SS_LEFT|SS_CENTERIMAGE,835,69,65,24,0);Add(s.hwnd,L"EDIT",std::to_wstring(s.config.thresholdPercent).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,902,69,58,24,IDC_THRESHOLD);

    Add(s.hwnd,L"STATIC",L"NÚT VỨT • scan rồi click tâm:",SS_LEFT|SS_CENTERIMAGE,18,105,205,25,0);
    Add(s.hwnd,L"EDIT",s.config.discardTemplatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,225,105,555,25,IDC_TEMPLATE_DISCARD);
    Add(s.hwnd,L"BUTTON",L"CHỌN VỨT",BS_PUSHBUTTON,790,104,110,27,IDC_PICK_DISCARD);
    AddRoiEditors(s,136,IDC_DISCARD_X,IDC_DISCARD_Y,IDC_DISCARD_W,IDC_DISCARD_H,IDC_DISCARD_REGION,IDC_DISCARD_PREVIEW,IDC_DISCARD_FULL,s.config.discardRoi.x,s.config.discardRoi.y,s.config.discardRoi.w,s.config.discardRoi.h,L"ROI VỨT");

    Add(s.hwnd,L"STATIC",L"DẤU X • scan rồi click tâm:",SS_LEFT|SS_CENTERIMAGE,18,172,205,25,0);
    Add(s.hwnd,L"EDIT",s.config.closeTemplatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,225,172,555,25,IDC_TEMPLATE_CLOSE);
    Add(s.hwnd,L"BUTTON",L"CHỌN DẤU X",BS_PUSHBUTTON,790,171,110,27,IDC_PICK_CLOSE);
    AddRoiEditors(s,203,IDC_CLOSE_X,IDC_CLOSE_Y,IDC_CLOSE_W,IDC_CLOSE_H,IDC_CLOSE_REGION,IDC_CLOSE_PREVIEW,IDC_CLOSE_FULL,s.config.closeRoi.x,s.config.closeRoi.y,s.config.closeRoi.w,s.config.closeRoi.h,L"ROI DẤU X");
    Add(s.hwnd,L"STATIC",L"V4: 3 ROI khác nhau nhưng mọi scan trong cùng một probe dùng CHUNG 1 frame PrintWindow.",SS_LEFT|SS_CENTERIMAGE,675,203,330,24,0);

    Add(s.hwnd,L"BUTTON",L"20 TỌA CLICK Ô ĐỒ • Timeout là thời gian tối đa chờ trạng thái, KHÔNG phải Sleep",BS_GROUPBOX,18,238,987,330,0);
    s.stepList=Add(s.hwnd,WC_LISTVIEWW,L"",LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS|WS_BORDER,32,263,959,205,IDC_STEP_LIST);
    ListView_SetExtendedListViewStyle(s.stepList,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);
    AddColumn(s.stepList,0,45,L"#");AddColumn(s.stepList,1,80,L"X");AddColumn(s.stepList,2,80,L"Y");AddColumn(s.stepList,3,110,L"Base size");AddColumn(s.stepList,4,100,L"Timeout ms");AddColumn(s.stepList,5,515,L"Logic");

    Add(s.hwnd,L"STATIC",L"X:",SS_LEFT|SS_CENTERIMAGE,32,480,20,27,0);Add(s.hwnd,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,54,480,64,27,IDC_STEP_X);
    Add(s.hwnd,L"STATIC",L"Y:",SS_LEFT|SS_CENTERIMAGE,126,480,20,27,0);Add(s.hwnd,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,148,480,64,27,IDC_STEP_Y);
    Add(s.hwnd,L"STATIC",L"Timeout:",SS_LEFT|SS_CENTERIMAGE,222,480,55,27,0);Add(s.hwnd,L"EDIT",L"1500",WS_BORDER|ES_NUMBER|ES_CENTER,279,480,70,27,IDC_STEP_DELAY);
    Add(s.hwnd,L"BUTTON",L"LƯU CLICK",BS_PUSHBUTTON,360,480,105,27,IDC_STEP_SAVE);Add(s.hwnd,L"BUTTON",L"LẤY TỌA F8",BS_PUSHBUTTON,474,480,130,27,IDC_STEP_CAPTURE);Add(s.hwnd,L"BUTTON",L"TEST CLICK ẨN",BS_PUSHBUTTON,613,480,130,27,IDC_STEP_TEST);
    Add(s.hwnd,L"STATIC",L"Mặc định timeout 1500ms; máy nhanh thấy UI sớm thì xử lý ngay, không chờ đủ.",SS_LEFT|SS_CENTERIMAGE,752,480,235,27,0);

    Add(s.hwnd,L"STATIC",L"CLICK SAU VỨT:",SS_LEFT|SS_CENTERIMAGE,32,522,112,27,0);Add(s.hwnd,L"STATIC",L"X",SS_CENTER|SS_CENTERIMAGE,146,522,16,27,0);
    Add(s.hwnd,L"EDIT",s.config.afterDiscard.valid?std::to_wstring(s.config.afterDiscard.x).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,164,522,65,27,IDC_AFTER_X);
    Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,235,522,16,27,0);Add(s.hwnd,L"EDIT",s.config.afterDiscard.valid?std::to_wstring(s.config.afterDiscard.y).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,253,522,65,27,IDC_AFTER_Y);
    Add(s.hwnd,L"BUTTON",L"LẤY F8 SAU VỨT",BS_PUSHBUTTON,329,522,155,27,IDC_AFTER_CAPTURE);Add(s.hwnd,L"BUTTON",L"TEST SAU VỨT",BS_PUSHBUTTON,493,522,135,27,IDC_AFTER_TEST);
    Add(s.hwnd,L"STATIC",L"Sau VỨT: tool chờ ảnh VỨT biến mất rồi mới click tọa này.",SS_LEFT|SS_CENTERIMAGE,640,522,345,27,0);

    Add(s.hwnd,L"STATIC",L"STATE: CLICK N → probe 1 frame → GOOD? {scan X cùng frame} : {scan VỨT cùng frame} → chờ dấu hiệu UI biến đổi → bước tiếp.",SS_LEFT|SS_CENTERIMAGE,32,548,955,20,0);
    Add(s.hwnd,L"BUTTON",L"CHẠY TEST LỌC 20 CLICK ẨN • V4 OPTIMIZED",BS_DEFPUSHBUTTON,18,582,987,39,IDC_TEST);
    Add(s.hwnd,L"STATIC",L"Sẵn sàng. ESC = dừng khẩn. Không Sleep khóa UI; timer chỉ probe khi đến lượt.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,632,987,90,IDC_STATUS);
    Add(s.hwnd,L"BUTTON",L"ĐÓNG",BS_PUSHBUTTON,885,735,120,30,IDCANCEL);
    RefreshStepList(s);
}'''
text = replace_span(text, 'void BuildControls(State& s)', 'LRESULT CALLBACK WndProc', controls_block)

# WndProc: timer runtime + ba ROI riêng.
text = text.replace('case WM_TIMER:if(wp==kF8PollTimer){PollF8(*s);return 0;}break;',
                    'case WM_TIMER:if(wp==kF8PollTimer){PollF8(*s);return 0;}if(wp==kRunTimer){ProcessRunTick(*s);return 0;}break;')
text = text.replace('case IDC_FULL_REGION:ResetFullRegion(*s);return 0;\n', '''case IDC_FULL_REGION:ResetFullRegion(*s);return 0;
                case IDC_DISCARD_REGION:SelectDiscardRegion(*s);return 0;
                case IDC_DISCARD_PREVIEW:PreviewDiscardRegion(*s);return 0;
                case IDC_DISCARD_FULL:ResetDiscardRegion(*s);return 0;
                case IDC_CLOSE_REGION:SelectCloseRegion(*s);return 0;
                case IDC_CLOSE_PREVIEW:PreviewCloseRegion(*s);return 0;
                case IDC_CLOSE_FULL:ResetCloseRegion(*s);return 0;
''')
text = text.replace('KillTimer(hwnd,kF8PollTimer);s->hwnd=nullptr;', 'KillTimer(hwnd,kF8PollTimer);KillTimer(hwnd,kRunTimer);s->hwnd=nullptr;')

# Dialog title/size.
text = text.replace('L"TEST FILTER ẢNH ẨN • 20 CLICK • GIỮ/VỨT v3"', 'L"TEST FILTER ẢNH ẨN • 20 CLICK • V4 OPTIMIZED"')
text = text.replace('CW_USEDEFAULT,CW_USEDEFAULT,990,760', 'CW_USEDEFAULT,CW_USEDEFAULT,1040,815')

# Source must not retain blocking Sleep from v3 runtime.
if 'Sleep(' in text:
    raise SystemExit('FILTER v4 still contains blocking Sleep()')

required = [
    'kRunTimer = 92',
    'ScanRoi discardRoi',
    'ScanRoi closeRoi',
    'RunPhase::WaitItemReady',
    'MỖI PROBE CHỈ CHỤP HWND ĐÚNG 1 LẦN',
    'ScanGoodOnFrame',
    'ScanDiscardOnFrame',
    'ScanCloseOnFrame',
    'ROI VỨT',
    'ROI DẤU X',
    'Timeout ms',
    'ProcessRunTick',
]
for needle in required:
    if needle not in text:
        raise SystemExit(f'FILTER v4 missing contract: {needle}')

Path('src/image_scan_filter_v4.inl').write_text(text, encoding='utf-8')
print('TEST FILTER v4 generator PASS • 1 capture/probe • 3 ROI • timer/state • no Sleep')
