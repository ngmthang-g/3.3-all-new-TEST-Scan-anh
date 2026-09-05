            if(s.slotIndex>=s.config.steps.size()){FinishRun(s,L"LỌC "+std::to_wstring(s.config.steps.size())+L" CLICK HOÀN TẤT • 1 capture/probe • 3 ROI riêng • Sleep sau mỗi thao tác");return;}
            if(!ClickCurrentSlot(s,error)){FinishRun(s,L"CLICK "+std::to_wstring(s.slotIndex+1)+L" FAIL • "+error);return;}
            s.delayKind=DelayKind::Slot;
            Sleep(static_cast<DWORD>(StepDelayMs(s)));
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
    s.delayKind=DelayKind::Slot;
    Sleep(static_cast<DWORD>(StepDelayMs(s)));
    ArmRunPhase(s,RunPhase::WaitItemReady,40);SetTimer(s.hwnd,kRunTimer,25,nullptr);
    SetStatus(s.hwnd,L"V4 SLEEP START • CLICK 1 → Sleep Delay → probe 1 frame • ROI ẢNH1/VỨT/X riêng");
}

void AddRoiEditors(State& s,int y,int xId,int yId,int wId,int hId,int pickId,int previewId,int fullId,
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
