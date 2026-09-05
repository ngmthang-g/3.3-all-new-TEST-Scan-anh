    Add(s.hwnd,L"EDIT",s.config.closeTemplatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,225,172,555,25,IDC_TEMPLATE_CLOSE);
    Add(s.hwnd,L"BUTTON",L"CHỌN DẤU X",BS_PUSHBUTTON,790,171,110,27,IDC_PICK_CLOSE);
    AddRoiEditors(s,203,IDC_CLOSE_X,IDC_CLOSE_Y,IDC_CLOSE_W,IDC_CLOSE_H,IDC_CLOSE_REGION,IDC_CLOSE_PREVIEW,IDC_CLOSE_FULL,s.config.closeRoi.x,s.config.closeRoi.y,s.config.closeRoi.w,s.config.closeRoi.h,L"ROI DẤU X");
    Add(s.hwnd,L"STATIC",L"V4: 3 ROI khác nhau nhưng mọi scan trong cùng một probe dùng CHUNG 1 frame PrintWindow.",SS_LEFT|SS_CENTERIMAGE,675,203,330,24,0);

    Add(s.hwnd,L"BUTTON",L"DANH SÁCH CLICK Ô ĐỒ • mặc định 20 • Delay riêng từng CLICK N",BS_GROUPBOX,18,238,987,330,0);
    Add(s.hwnd,L"BUTTON",L"+ THÊM",BS_PUSHBUTTON,650,236,78,26,IDC_STEP_ADD);
    Add(s.hwnd,L"BUTTON",L"- XÓA",BS_PUSHBUTTON,733,236,78,26,IDC_STEP_DELETE);
    Add(s.hwnd,L"BUTTON",L"LÊN",BS_PUSHBUTTON,816,236,70,26,IDC_STEP_UP);
    Add(s.hwnd,L"BUTTON",L"XUỐNG",BS_PUSHBUTTON,891,236,90,26,IDC_STEP_DOWN);
    s.stepList=Add(s.hwnd,WC_LISTVIEWW,L"",LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS|WS_BORDER,32,263,959,205,IDC_STEP_LIST);
    ListView_SetExtendedListViewStyle(s.stepList,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);
    AddColumn(s.stepList,0,45,L"#");AddColumn(s.stepList,1,80,L"X");AddColumn(s.stepList,2,80,L"Y");AddColumn(s.stepList,3,110,L"Base size");AddColumn(s.stepList,4,100,L"Delay ms");AddColumn(s.stepList,5,515,L"Logic");

    Add(s.hwnd,L"STATIC",L"X:",SS_LEFT|SS_CENTERIMAGE,32,480,20,27,0);Add(s.hwnd,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,54,480,64,27,IDC_STEP_X);
    Add(s.hwnd,L"STATIC",L"Y:",SS_LEFT|SS_CENTERIMAGE,126,480,20,27,0);Add(s.hwnd,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,148,480,64,27,IDC_STEP_Y);
    Add(s.hwnd,L"STATIC",L"Delay:",SS_LEFT|SS_CENTERIMAGE,222,480,55,27,0);Add(s.hwnd,L"EDIT",L"500",WS_BORDER|ES_NUMBER|ES_CENTER,279,480,70,27,IDC_STEP_DELAY);
    Add(s.hwnd,L"BUTTON",L"LƯU CLICK",BS_PUSHBUTTON,360,480,105,27,IDC_STEP_SAVE);Add(s.hwnd,L"BUTTON",L"LẤY TỌA F8",BS_PUSHBUTTON,474,480,130,27,IDC_STEP_CAPTURE);Add(s.hwnd,L"BUTTON",L"TEST CLICK ẨN",BS_PUSHBUTTON,613,480,130,27,IDC_STEP_TEST);
    Add(s.hwnd,L"STATIC",L"Delay N chỉ áp dụng CLICK N; VỨT / SAU VỨT / X có delay riêng bên dưới.",SS_LEFT|SS_CENTERIMAGE,752,480,235,27,0);

    Add(s.hwnd,L"STATIC",L"CLICK SAU VỨT:",SS_LEFT|SS_CENTERIMAGE,32,522,112,27,0);Add(s.hwnd,L"STATIC",L"X",SS_CENTER|SS_CENTERIMAGE,146,522,16,27,0);
    Add(s.hwnd,L"EDIT",s.config.afterDiscard.valid?std::to_wstring(s.config.afterDiscard.x).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,164,522,65,27,IDC_AFTER_X);
    Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,235,522,16,27,0);Add(s.hwnd,L"EDIT",s.config.afterDiscard.valid?std::to_wstring(s.config.afterDiscard.y).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,253,522,65,27,IDC_AFTER_Y);
    Add(s.hwnd,L"BUTTON",L"LẤY F8 SAU VỨT",BS_PUSHBUTTON,329,522,155,27,IDC_AFTER_CAPTURE);Add(s.hwnd,L"BUTTON",L"TEST SAU VỨT",BS_PUSHBUTTON,493,522,135,27,IDC_AFTER_TEST);
    Add(s.hwnd,L"STATIC",L"VỨT ms",SS_LEFT|SS_CENTERIMAGE,640,522,48,27,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.discardClickDelayMs).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,689,522,55,27,IDC_DELAY_DISCARD);
    Add(s.hwnd,L"STATIC",L"SAU ms",SS_LEFT|SS_CENTERIMAGE,750,522,50,27,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.afterDiscardClickDelayMs).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,801,522,55,27,IDC_DELAY_AFTER_DISCARD);
    Add(s.hwnd,L"STATIC",L"X ms",SS_LEFT|SS_CENTERIMAGE,862,522,38,27,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.closeClickDelayMs).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,901,522,55,27,IDC_DELAY_CLOSE);

    Add(s.hwnd,L"STATIC",L"STATE: CLICK N → probe 1 frame → GOOD? {scan X cùng frame} : {scan VỨT cùng frame} → chờ dấu hiệu UI biến đổi → bước tiếp.",SS_LEFT|SS_CENTERIMAGE,32,548,955,20,0);
    Add(s.hwnd,L"BUTTON",L"CHẠY TEST LỌC CLICK ẨN • V4 SLEEP",BS_DEFPUSHBUTTON,18,582,987,39,IDC_TEST);
    Add(s.hwnd,L"STATIC",L"Sẵn sàng. V4 SLEEP: CLICK N, VỨT, SAU VỨT và X đều có Delay riêng; 50-10000 ms.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,632,987,90,IDC_STATUS);
    Add(s.hwnd,L"BUTTON",L"ĐÓNG",BS_PUSHBUTTON,885,735,120,30,IDCANCEL);
    RefreshStepList(s);
}

LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    State* s=reinterpret_cast<State*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE){auto* cs=reinterpret_cast<CREATESTRUCTW*>(lp);s=static_cast<State*>(cs->lpCreateParams);SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));if(s)s->hwnd=hwnd;}
    if(!s)return DefWindowProcW(hwnd,msg,wp,lp);
    switch(msg){
        case WM_CREATE:BuildControls(*s);return 0;
        case WM_TIMER:if(wp==kF8PollTimer){PollF8(*s);return 0;}if(wp==kRunTimer){ProcessRunTick(*s);return 0;}break;
        case WM_NOTIFY:{
            auto* hdr=reinterpret_cast<NMHDR*>(lp);
            if(hdr&&hdr->idFrom==IDC_STEP_LIST&&hdr->code==LVN_ITEMCHANGED){
                const auto* n=reinterpret_cast<NMLISTVIEW*>(hdr);
                if((n->uChanged&LVIF_STATE)!=0&&(n->uNewState&LVIS_SELECTED)!=0)LoadStepEditor(*s,n->iItem);
            }
            return 0;
        }
        case WM_COMMAND:
            if(s->runningChain && LOWORD(wp)!=IDCANCEL){SetStatus(s->hwnd,L"Đang chạy FILTER v4 • ESC để dừng trước khi sửa cấu hình");return 0;}
