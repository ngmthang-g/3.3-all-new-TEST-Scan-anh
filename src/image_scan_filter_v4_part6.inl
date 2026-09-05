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

bool ResolveStepPoint(const ClickStep& step,int currentW,int currentH,int& x,int& y){
    if(!step.valid||step.baseW<=0||step.baseH<=0||currentW<=0||currentH<=0)return false;
    x=ScaleCoord(step.x,step.baseW,currentW);y=ScaleCoord(step.y,step.baseH,currentH);
    x=std::clamp(x,0,currentW-1);y=std::clamp(y,0,currentH-1);return true;
}

void TestSelectedStep(State& s){
    SaveStepEditor(s);const int row=SelectedStep(s);
    if(row<0||row>=static_cast<int>(s.config.steps.size())){SetStatus(s.hwnd,L"TEST DÒNG: chưa chọn dòng");return;}
    int cw=0,ch=0,x=0,y=0;if(!CurrentClientSize(s.target.gameWindow,cw,ch)||!ResolveStepPoint(s.config.steps[static_cast<std::size_t>(row)],cw,ch,x,y)){
        SetStatus(s.hwnd,L"TEST DÒNG FAIL • dòng chưa có tọa F8 hợp lệ");return;
    }
    std::wstring detail;const bool ok=s.target.hiddenClick&&s.target.hiddenClick(s.target.context,x,y,cw,ch,detail);
    SetStatus(s.hwnd,L"TEST DÒNG "+std::to_wstring(row+1)+(ok?L" PASS • ":L" FAIL • ")+std::to_wstring(x)+L","+std::to_wstring(y)+L" • "+detail);
}

bool ValidateFilter(const State& s,std::wstring& error){
    if(s.config.templatePath.empty()){error=L"chưa chọn ẢNH 1 (đồ đúng)";return false;}
    if(s.config.discardTemplatePath.empty()){error=L"chưa chọn ảnh NÚT VỨT";return false;}
    if(s.config.closeTemplatePath.empty()){error=L"chưa chọn ảnh DẤU X";return false;}
    if(s.config.steps.empty()){error=L"danh sách CLICK đang rỗng";return false;}
    for(std::size_t i=0;i<s.config.steps.size();++i){
        const ClickStep& step=s.config.steps[i];
        if(!step.valid||step.baseW<=0||step.baseH<=0){error=L"CLICK "+std::to_wstring(i+1)+L" chưa gán tọa F8";return false;}
        if(step.delayMs<50||step.delayMs>10000){error=L"Delay CLICK "+std::to_wstring(i+1)+L" không hợp lệ (50-10000 ms)";return false;}
    }
    if(!s.config.afterDiscard.valid||s.config.afterDiscard.baseW<=0||s.config.afterDiscard.baseH<=0){
        error=L"chưa gán CLICK SAU VỨT bằng F8";return false;
    }
    return true;
}

void ResolveGoodRoi(const Config& c,int currentW,int currentH,int& rx,int& ry,int& rw,int& rh){
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

int StepDelayMs(const State& s){
    switch(s.delayKind){
        case DelayKind::Discard: return std::clamp(s.config.discardClickDelayMs,50,10000);
        case DelayKind::AfterDiscard: return std::clamp(s.config.afterDiscardClickDelayMs,50,10000);
        case DelayKind::Close: return std::clamp(s.config.closeClickDelayMs,50,10000);
        case DelayKind::Slot:
        default:
            if(s.slotIndex<s.config.steps.size())return std::clamp(s.config.steps[s.slotIndex].delayMs,50,10000);
            return 500;
    }
}

constexpr UINT kStateTimeoutMs = 5000;

void ArmRunPhase(State& s,RunPhase phase,UINT firstProbeDelay=40){
    const ULONGLONG now=GetTickCount64();s.runPhase=phase;s.nextProbeTick=now+firstProbeDelay;s.phaseDeadlineTick=now+static_cast<ULONGLONG>(kStateTimeoutMs);
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
