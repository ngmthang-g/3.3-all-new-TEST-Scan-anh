            switch(LOWORD(wp)){
                case IDC_PICK:PickTemplate(*s);return 0;
                case IDC_PICK_DISCARD:PickDiscardTemplate(*s);return 0;
                case IDC_PICK_CLOSE:PickCloseTemplate(*s);return 0;
                case IDC_PICK_REGION:SelectRegion(*s);return 0;
                case IDC_PREVIEW_REGION:PreviewRegion(*s);return 0;
                case IDC_FULL_REGION:ResetFullRegion(*s);return 0;
                case IDC_DISCARD_REGION:SelectDiscardRegion(*s);return 0;
                case IDC_DISCARD_PREVIEW:PreviewDiscardRegion(*s);return 0;
                case IDC_DISCARD_FULL:ResetDiscardRegion(*s);return 0;
                case IDC_CLOSE_REGION:SelectCloseRegion(*s);return 0;
                case IDC_CLOSE_PREVIEW:PreviewCloseRegion(*s);return 0;
                case IDC_CLOSE_FULL:ResetCloseRegion(*s);return 0;
                case IDC_STEP_ADD:AddStep(*s);return 0;
                case IDC_STEP_DELETE:DeleteStep(*s);return 0;
                case IDC_STEP_UP:MoveStep(*s,-1);return 0;
                case IDC_STEP_DOWN:MoveStep(*s,1);return 0;
                case IDC_STEP_SAVE:SaveSelectedStep(*s);return 0;
                case IDC_STEP_CAPTURE:ArmF8(*s);return 0;
                case IDC_STEP_TEST:TestSelectedStep(*s);return 0;
                case IDC_AFTER_CAPTURE:ArmAfterDiscardF8(*s);return 0;
                case IDC_AFTER_TEST:TestAfterDiscard(*s);return 0;
                case IDC_TEST:RunTest(*s);return 0;
                case IDCANCEL:if(!s->runningChain){SyncConfig(*s);DestroyWindow(hwnd);}return 0;
            }
            break;
        case WM_CLOSE:if(!s->runningChain){SyncConfig(*s);DestroyWindow(hwnd);}return 0;
        case WM_NCDESTROY:KillTimer(hwnd,kF8PollTimer);KillTimer(hwnd,kRunTimer);s->hwnd=nullptr;s->stepList=nullptr;return DefWindowProcW(hwnd,msg,wp,lp);
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}

} // namespace

void RunDialog(const Target& target){
    if(!target.owner||!target.gameWindow)return;
    State state{};state.target=target;state.config=g_lastConfig;RebaseForCurrentClient(state.config,target.gameWindow);if(state.config.steps.empty())state.config.steps.resize(kDefaultInitialSteps);
    WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.lpfnWndProc=WndProc;wc.hInstance=GetModuleHandleW(nullptr);wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);wc.lpszClassName=kClassName;
    if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return;
    EnableWindow(target.owner,FALSE);
    HWND hwnd=CreateWindowExW(WS_EX_TOOLWINDOW,kClassName,L"TEST LỌC ĐỒ ẢNH ẨN • V4 SLEEP • CLICK ĐỘNG",
                              WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
                              CW_USEDEFAULT,CW_USEDEFAULT,1000,755,target.owner,nullptr,GetModuleHandleW(nullptr),&state);
    if(!hwnd){EnableWindow(target.owner,TRUE);return;}
    ShowWindow(hwnd,SW_SHOW);UpdateWindow(hwnd);
    MSG msg{};bool sawQuit=false;int quitCode=0;
    while(IsWindow(hwnd)){
        const BOOL gm=GetMessageW(&msg,nullptr,0,0);
        if(gm<=0){if(gm==0){sawQuit=true;quitCode=static_cast<int>(msg.wParam);}break;}
        if(!IsDialogMessageW(hwnd,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    }
    g_lastConfig=state.config;EnableWindow(target.owner,TRUE);SetActiveWindow(target.owner);if(sawQuit)PostQuitMessage(quitCode);
}

} // namespace image_scan_test
