                const int cx=close.x+s.closeTpl.width/2,cy=close.y+s.closeTpl.height/2;
                if(!RawClick(s,cx,cy,frame.width,frame.height,error)){FinishRun(s,L"CLICK TÂM X FAIL • "+error);return;}
                s.delayKind=DelayKind::Close;
                Sleep(static_cast<DWORD>(StepDelayMs(s)));
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
            s.delayKind=DelayKind::Discard;
            Sleep(static_cast<DWORD>(StepDelayMs(s)));
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
            s.delayKind=DelayKind::AfterDiscard;
            Sleep(static_cast<DWORD>(StepDelayMs(s)));
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
            s.delayKind=DelayKind::Slot;
            Sleep(static_cast<DWORD>(StepDelayMs(s)));
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
