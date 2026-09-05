                              CW_USEDEFAULT,CW_USEDEFAULT,winW,winH,owner,nullptr,GetModuleHandleW(nullptr),&state);
    if(!hwnd){EnableWindow(owner,TRUE);return;}
    ShowWindow(hwnd,SW_SHOW);UpdateWindow(hwnd);SetFocus(hwnd);
    MSG msg{};bool sawQuit=false;int quitCode=0;
    while(IsWindow(hwnd)){
        const BOOL gm=GetMessageW(&msg,nullptr,0,0);
        if(gm<=0){if(gm==0){sawQuit=true;quitCode=static_cast<int>(msg.wParam);}break;}
        TranslateMessage(&msg);DispatchMessageW(&msg);
    }
    EnableWindow(owner,TRUE);SetActiveWindow(owner);if(sawQuit)PostQuitMessage(quitCode);
}

void SelectRegion(State& s) {
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
void ResetCloseRegion(State& s){ResetNamedRegion(s,s.config.closeRoi,IDC_CLOSE_X,IDC_CLOSE_Y,IDC_CLOSE_W,IDC_CLOSE_H,L"ROI DẤU X");}

void AddStep(State& s) {
    SaveStepEditor(s);
    ClickStep step{};int cw=0,ch=0;if(CurrentClientSize(s.target.gameWindow,cw,ch)){step.baseW=cw;step.baseH=ch;}
    s.config.steps.push_back(step);g_lastConfig=s.config;RefreshStepList(s,static_cast<int>(s.config.steps.size()-1));
    SetStatus(s.hwnd,L"Đã thêm CLICK "+std::to_wstring(s.config.steps.size())+L" • không có giới hạn cứng");
}

void DeleteStep(State& s) {
    const int row=SelectedStep(s);if(row<0||row>=static_cast<int>(s.config.steps.size()))return;
    s.config.steps.erase(s.config.steps.begin()+row);g_lastConfig=s.config;
    RefreshStepList(s,std::min(row,static_cast<int>(s.config.steps.size())-1));
}

void MoveStep(State& s,int delta){
    SaveStepEditor(s);const int row=SelectedStep(s),next=row+delta;
    if(row<0||next<0||row>=static_cast<int>(s.config.steps.size())||next>=static_cast<int>(s.config.steps.size()))return;
    std::swap(s.config.steps[static_cast<std::size_t>(row)],s.config.steps[static_cast<std::size_t>(next)]);
    g_lastConfig=s.config;RefreshStepList(s,next);
}

void SaveSelectedStep(State& s){
    const int row=SelectedStep(s);if(row<0){SetStatus(s.hwnd,L"Chọn một dòng click trước");return;}
    SaveStepEditor(s);RefreshStepList(s,row);SetStatus(s.hwnd,L"Đã lưu dòng click "+std::to_wstring(row+1));
}

void ArmF8(State& s){
    const int row=SelectedStep(s);if(row<0||row>=static_cast<int>(s.config.steps.size())){SetStatus(s.hwnd,L"F8: chọn một dòng CLICK trước");return;}
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
