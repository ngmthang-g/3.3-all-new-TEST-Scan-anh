    // ---------------- v4.5 dungeon management / activity board ----------------
    static std::wstring DungeonV45TeamLabel(const DungeonTeamRuntime& team) {
        return team.displayName.empty() ? (L"ĐỘI " + std::to_wstring(team.config.id)) : team.displayName;
    }

    const dungeon_v45::QueueEntry* DungeonV45CurrentQueueEntry(const DungeonTeamRuntime& team) const {
        if (team.queue.empty()) return nullptr;
        const int index = std::clamp(team.queueIndex, 0, static_cast<int>(team.queue.size()) - 1);
        return &team.queue[static_cast<std::size_t>(index)];
    }

    int DungeonV45PresetIndex(const DungeonTeamRuntime& team) const {
        const auto* entry = DungeonV45CurrentQueueEntry(team);
        if (!entry) return team.config.presetIndex;
        const int resolved = FindDungeonPresetById(entry->presetId);
        return resolved >= 0 ? resolved : team.config.presetIndex;
    }

    void DungeonV45SyncLegacyConfig(DungeonTeamRuntime& team) {
        if (team.queue.empty()) {
            if (team.config.presetIndex >= 0 && team.config.presetIndex < static_cast<int>(dungeonPresets_.size())) {
                team.queue.push_back({dungeonPresets_[static_cast<std::size_t>(team.config.presetIndex)].id,
                                      std::clamp(team.config.runs, 1, dungeon_v45::kMaxRunsPerEntry)});
            }
        }
        if (team.queue.empty()) return;
        team.queueIndex = std::clamp(team.queueIndex, 0, static_cast<int>(team.queue.size()) - 1);
        const auto& entry = team.queue[static_cast<std::size_t>(team.queueIndex)];
        const int presetIndex = FindDungeonPresetById(entry.presetId);
        if (presetIndex >= 0) team.config.presetIndex = presetIndex;
        team.config.runs = std::clamp(entry.runs, 1, dungeon_v45::kMaxRunsPerEntry);
        team.runIndex = std::clamp(team.queueRunIndex, 1, team.config.runs);
    }

    std::wstring DungeonV45QueueSummary(const DungeonTeamRuntime& team) const {
        if (team.queue.empty()) {
            const auto* preset = DungeonConfiguredPresetForTeam(team);
            return preset ? preset->name : L"?";
        }
        std::wstring out;
        for (std::size_t i = 0; i < team.queue.size(); ++i) {
            const int p = FindDungeonPresetById(team.queue[i].presetId);
            const std::wstring name = p >= 0 ? dungeonPresets_[static_cast<std::size_t>(p)].name : team.queue[i].presetId;
            if (!out.empty()) out += L" → ";
            out += name + L" x" + std::to_wstring(team.queue[i].runs);
            if (out.size() > 120) { out += L"…"; break; }
        }
        return out;
    }

    void DungeonV45HideForeignControlsForDungeon() {
        if (!hwnd_) return;
        for (HWND h = GetWindow(hwnd_, GW_CHILD); h; h = GetWindow(h, GW_HWNDNEXT)) {
            if (h == mainTab_) continue;
            if (IsDungeonControl(h)) continue;
            ShowWindow(h, SW_HIDE);
        }
        ShowWindow(mainTab_, SW_SHOW);
        ShowDungeonControls(true);
    }

    void DungeonV45OnMainTabChanged(int index) {
        if (index == 2) DungeonV45HideForeignControlsForDungeon();
        if (index != 2) {
            if (dungeonActivityWindow_ && IsWindow(dungeonActivityWindow_)) ShowWindow(dungeonActivityWindow_, SW_HIDE);
            if (dungeonManagerWindow_ && IsWindow(dungeonManagerWindow_)) ShowWindow(dungeonManagerWindow_, SW_HIDE);
            if (dungeonMonsterCatalogWindow_ && IsWindow(dungeonMonsterCatalogWindow_)) ShowWindow(dungeonMonsterCatalogWindow_, SW_HIDE);
        }
    }

    void DungeonV45NotifyRunComplete(DungeonTeamRuntime& team, const cleanroute_dungeon::Preset& preset) {
        const std::wstring key = dungeon_v45::CompletionDedupKey(
            team.config.id, preset.id, team.queueIndex, team.queueRunIndex);
        if (std::find(team.completionNotifyKeys.begin(), team.completionNotifyKeys.end(), key) != team.completionNotifyKeys.end()) return;
        team.completionNotifyKeys.push_back(key);

        std::wstring message = L"✅ HOÀN THÀNH 1 LƯỢT PHÓ BẢN\n";
        message += L"Đội: " + DungeonV45TeamLabel(team) + L" [ID " + std::to_wstring(team.config.id) + L"]\n";
        message += L"Phó bản: " + preset.name + L"\n";
        message += L"Kế hoạch: " + std::to_wstring(team.queueIndex + 1) + L"/" +
                   std::to_wstring(std::max<std::size_t>(1, team.queue.size())) + L" • lượt " +
                   std::to_wstring(team.queueRunIndex) + L"/" + std::to_wstring(team.config.runs) + L"\n";
        message += L"Đã chứng minh rời Map " + std::to_wstring(preset.dungeonMap) + L"\n";
        message += L"Thành viên:\n";
        for (std::int32_t roleID : team.memberRoleIDs) {
            std::wstring name = L"RoleID " + std::to_wstring(roleID);
            for (const auto& accountPtr : accounts_) {
                if (!accountPtr || !accountPtr->snapshotValid || (accountPtr->snapshot.validMask & ValidIdentity) == 0) continue;
                if (accountPtr->snapshot.roleID == roleID) { name = accountPtr->snapshot.characterName; break; }
            }
            message += L"- " + name + L"\n";
        }
        std::wstring nextText = L"HOÀN THÀNH TOÀN BỘ KẾ HOẠCH";
        if (team.queueRunIndex < team.config.runs) {
            nextText = preset.name + L" • lượt " + std::to_wstring(team.queueRunIndex + 1) + L"/" +
                       std::to_wstring(team.config.runs);
        } else if (team.queueIndex + 1 < static_cast<int>(team.queue.size())) {
            const auto& nextEntry = team.queue[static_cast<std::size_t>(team.queueIndex + 1)];
            const int nextPreset = FindDungeonPresetById(nextEntry.presetId);
            const std::wstring nextName = nextPreset >= 0
                ? dungeonPresets_[static_cast<std::size_t>(nextPreset)].name : nextEntry.presetId;
            nextText = nextName + L" • lượt 1/" + std::to_wstring(nextEntry.runs);
        }
        message += L"Tiếp theo: " + nextText + L"\nThời gian: " + LocalDateTimeText();
        (void)QueueTelegramRequest(telegram_notify::TaskKind::SendMessage,
                                   message, L"DUNGEON SUMMARY", L"-");
    }

    static LRESULT CALLBACK DungeonActivityWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        App* self = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            const auto* cs = reinterpret_cast<const CREATESTRUCTW*>(lp);
            self = reinterpret_cast<App*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (msg == WM_CLOSE) { ShowWindow(hwnd, SW_HIDE); return 0; }
        if (msg == WM_SIZE && self && self->dungeonActivityList_) {
            RECT rc{}; GetClientRect(hwnd, &rc);
            MoveWindow(self->dungeonActivityTitle_, 12, 10, std::max(100, static_cast<int>(rc.right) - 24), 36, TRUE);
            MoveWindow(self->dungeonActivityList_, 12, 52, std::max(100, static_cast<int>(rc.right) - 24), std::max(100, static_cast<int>(rc.bottom) - 104), TRUE);
            MoveWindow(self->dungeonActivityStatus_, 12, std::max(58, static_cast<int>(rc.bottom) - 44), std::max(100, static_cast<int>(rc.right) - 24), 30, TRUE);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    void EnsureDungeonActivityWindow() {
        if (dungeonActivityWindow_ && IsWindow(dungeonActivityWindow_)) return;
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{}; wc.cbSize = sizeof(wc); wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpfnWndProc = DungeonActivityWndProc; wc.lpszClassName = L"ThanLongDungeonActivityV45";
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW); wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            registered = RegisterClassExW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
        }
        if (!registered) return;
        dungeonActivityWindow_ = CreateWindowExW(WS_EX_TOOLWINDOW, L"ThanLongDungeonActivityV45",
            L"BẢNG HOẠT ĐỘNG PHÓ BẢN — LIVE SYNC", WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 620, 480, hwnd_, nullptr, GetModuleHandleW(nullptr), this);
        if (!dungeonActivityWindow_) return;
        dungeonActivityTitle_ = CreateWindowExW(0, L"STATIC", L"CHƯA ĐỒNG BỘ ĐƯỢC BẢNG HOẠT ĐỘNG",
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, 12, 10, 580, 36,
            dungeonActivityWindow_, nullptr, GetModuleHandleW(nullptr), nullptr);
        dungeonActivityList_ = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            12, 52, 580, 330, dungeonActivityWindow_, nullptr, GetModuleHandleW(nullptr), nullptr);
        ListView_SetExtendedListViewStyle(dungeonActivityList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        DungeonListColumn(dungeonActivityList_, 0, 350, L"Mục tiêu hoạt động");
        DungeonListColumn(dungeonActivityList_, 1, 80, L"Đã");
        DungeonListColumn(dungeonActivityList_, 2, 80, L"Tổng");
        dungeonActivityStatus_ = CreateWindowExW(0, L"STATIC",
            L"Nguồn bắt buộc: LIVE Activity/FuBen UI/server • không dùng GMonster scan để giả số",
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 12, 390, 580, 30,
            dungeonActivityWindow_, nullptr, GetModuleHandleW(nullptr), nullptr);
        HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        SendMessageW(dungeonActivityTitle_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(dungeonActivityList_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(dungeonActivityStatus_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }

    void OpenDungeonActivityBoard() {
        EnsureDungeonActivityWindow();
        if (!dungeonActivityWindow_) return;
        ShowWindow(dungeonActivityWindow_, SW_SHOW);
        SetForegroundWindow(dungeonActivityWindow_);
        dungeonActivityLastTick_ = 0;
        RefreshDungeonActivityBoard(true);
    }

    void RefreshDungeonActivityBoard(bool force = false, DWORD now = GetTickCount()) {
        if (!dungeonActivityWindow_ || !IsWindow(dungeonActivityWindow_) || !IsWindowVisible(dungeonActivityWindow_)) return;
        if (!force && !Elapsed(now, dungeonActivityLastTick_, 1000)) return;
        dungeonActivityLastTick_ = now;
        if (dungeonActivityList_) ListView_DeleteAllItems(dungeonActivityList_);

        const int teamIndex = SelectedDungeonTeamIndex();
        if (teamIndex < 0 || teamIndex >= static_cast<int>(dungeonTeams_.size())) {
            SetText(dungeonActivityTitle_, L"CHƯA ĐỒNG BỘ • chọn một tổ đội");
            SetText(dungeonActivityStatus_, L"Không có authority client để đọc bảng hoạt động");
            return;
        }
        ResolveDungeonTeamBindings();
        DungeonTeamRuntime& team = dungeonTeams_[static_cast<std::size_t>(teamIndex)];
        cleanroute_dungeon::Preset* preset = DungeonPresetForTeam(team);
        Account* authority = AccountByPid(team.config.leaderPid);
        if ((!authority || !authority->snapshotValid ||
             (authority->snapshot.validMask & ValidIdentity) == 0 ||
             authority->snapshot.roleID != team.leaderRoleID) && team.leaderRoleID > 0) {
            for (auto& item : accounts_) {
                if (!item || !item->snapshotValid || (item->snapshot.validMask & ValidIdentity) == 0) continue;
                if (item->snapshot.roleID == team.leaderRoleID) {
                    authority = item.get();
                    team.config.leaderPid = authority->game.pid;
                    break;
                }
            }
        }
        if (!preset || !authority) {
            SetText(dungeonActivityTitle_, L"CHƯA ĐỒNG BỘ • thiếu preset/KEY");
            return;
        }
        Response response{}; std::wstring error;
        const bool ok = authority->bridge.Call(Command::ReadDungeonActivityBoard,
                                                preset->dungeonMap, 0, 0, response, error, 1000);
        if (!ok || !response.dungeonActivity.synchronized) {
            SetText(dungeonActivityTitle_, L"CHƯA ĐỒNG BỘ ĐƯỢC BẢNG HOẠT ĐỘNG");
            SetText(dungeonActivityStatus_, L"FAIL-CLOSED • không dùng scanner/STEP thay thế • " +
                                             (error.empty() ? std::wstring(response.detail) : error));
            return;
        }
        const DungeonActivitySnapshot& board = response.dungeonActivity;
        std::wstring title = board.activityName[0] ? board.activityName : preset->name;
        if (board.remainingSeconds >= 0) {
            const int mm = board.remainingSeconds / 60, ss = board.remainingSeconds % 60;
            wchar_t timeText[32]{}; swprintf_s(timeText, L" • Thời gian còn %02d:%02d", mm, ss);
            title += timeText;
        }
        SetText(dungeonActivityTitle_, title + L" • ĐÃ ĐỒNG BỘ");
        for (std::uint32_t i = 0; i < board.objectiveCount && i < kMaxDungeonActivityObjectives; ++i) {
            const auto& objective = board.objectives[i];
            LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = static_cast<int>(i);
            item.pszText = const_cast<wchar_t*>(objective.name);
            const int row = ListView_InsertItem(dungeonActivityList_, &item);
            std::wstring current = std::to_wstring(objective.current);
            std::wstring target = std::to_wstring(objective.target);
            ListView_SetItemText(dungeonActivityList_, row, 1, current.data());
            ListView_SetItemText(dungeonActivityList_, row, 2, target.data());
        }
        SetText(dungeonActivityStatus_, L"ĐÃ ĐỒNG BỘ • nguồn=" + std::wstring(board.source) +
                                         L" • Map " + std::to_wstring(board.mapID) +
                                         L" • objectives=" + std::to_wstring(board.objectiveCount));
    }

    static LRESULT CALLBACK DungeonManagerWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        App* self = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            const auto* cs = reinterpret_cast<const CREATESTRUCTW*>(lp);
            self = reinterpret_cast<App*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (!self) return DefWindowProcW(hwnd, msg, wp, lp);
        if (msg == WM_CLOSE) { ShowWindow(hwnd, SW_HIDE); return 0; }
        if (msg == WM_COMMAND) {
            switch (LOWORD(wp)) {
                case IDC_DGM_UPDATE_TEAM: self->DungeonV45UpdateTeamFromManager(); return 0;
                case IDC_DGM_ADD_MEMBER: self->DungeonV47AddMemberFromManager(); return 0;
                case IDC_DGM_REMOVE_MEMBER: self->DungeonV47RemoveMemberFromManager(); return 0;
                case IDC_DGM_QUEUE_ADD: self->DungeonV45QueueAdd(false); return 0;
                case IDC_DGM_QUEUE_UPDATE: self->DungeonV45QueueAdd(true); return 0;
                case IDC_DGM_QUEUE_DELETE: self->DungeonV45QueueDelete(); return 0;
                case IDC_DGM_QUEUE_UP: self->DungeonV45QueueMove(-1); return 0;
                case IDC_DGM_QUEUE_DOWN: self->DungeonV45QueueMove(1); return 0;
            }
        }
        if (msg == WM_NOTIFY && reinterpret_cast<NMHDR*>(lp)->hwndFrom == self->dungeonManagerQueue_ &&
            reinterpret_cast<NMHDR*>(lp)->code == LVN_ITEMCHANGED) {
            const auto* n = reinterpret_cast<const NMLISTVIEW*>(lp);
            if ((n->uChanged & LVIF_STATE) && (n->uNewState & LVIS_SELECTED)) self->DungeonV45LoadQueueRow(n->iItem);
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    void EnsureDungeonManagerWindow() {
        if (dungeonManagerWindow_ && IsWindow(dungeonManagerWindow_)) return;
        static bool registered = false;
        if (!registered) {
            WNDCLASSEXW wc{}; wc.cbSize = sizeof(wc); wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpfnWndProc = DungeonManagerWndProc; wc.lpszClassName = L"ThanLongDungeonManagerV47";
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW); wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            registered = RegisterClassExW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
        }
        if (!registered) return;
        dungeonManagerWindow_ = CreateWindowExW(WS_EX_TOOLWINDOW, L"ThanLongDungeonManagerV47",
            L"QUẢN LÝ TỔ ĐỘI / KẾ HOẠCH PHÓ BẢN v4.7", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, 820, 680, hwnd_, nullptr, GetModuleHandleW(nullptr), this);
        if (!dungeonManagerWindow_) return;
        HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        auto mk = [&](const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int w, int h, int id) {
            HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h,
                                     dungeonManagerWindow_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
            if (c) SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            return c;
        };
        mk(L"STATIC", L"Tên đội hiển thị:", SS_LEFT | SS_CENTERIMAGE, 14, 12, 110, 28, 0);
        dungeonManagerName_ = mk(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, 128, 12, 290, 28, IDC_DGM_NAME);
        dungeonManagerStatus_ = mk(L"STATIC", L"Chọn đội ở bảng chính", SS_LEFT | SS_CENTERIMAGE | WS_BORDER,
                                   430, 12, 350, 28, 0);
        mk(L"STATIC", L"THÀNH VIÊN ĐỘI ĐANG CHỌN • KEY luôn Slot 1", SS_LEFT | SS_CENTERIMAGE, 14, 48, 385, 24, 0);
        dungeonManagerMembers_ = mk(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER,
                                    14, 74, 385, 250, IDC_DGM_MEMBERS);
        ListView_SetExtendedListViewStyle(dungeonManagerMembers_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        DungeonListColumn(dungeonManagerMembers_, 0, 270, L"Slot / Nhân vật / RoleID");
        DungeonListColumn(dungeonManagerMembers_, 1, 80, L"PID");
        mk(L"STATIC", L"KEY:", SS_LEFT | SS_CENTERIMAGE, 14, 332, 45, 28, 0);
        dungeonManagerLeader_ = mk(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL, 62, 332, 337, 280, IDC_DGM_LEADER);
        mk(L"BUTTON", L"+ THÊM THÀNH VIÊN", BS_PUSHBUTTON, 14, 370, 150, 34, IDC_DGM_ADD_MEMBER);
        mk(L"BUTTON", L"XÓA THÀNH VIÊN", BS_PUSHBUTTON, 170, 370, 125, 34, IDC_DGM_REMOVE_MEMBER);
        mk(L"BUTTON", L"LƯU TÊN / KEY", BS_DEFPUSHBUTTON, 301, 370, 98, 34, IDC_DGM_UPDATE_TEAM);

        mk(L"STATIC", L"DANH SÁCH PHÓ BẢN / SỐ LƯỢT", SS_LEFT | SS_CENTERIMAGE, 418, 48, 362, 24, 0);
        dungeonManagerQueue_ = mk(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER,
                                  418, 74, 362, 250, IDC_DGM_QUEUE);
        ListView_SetExtendedListViewStyle(dungeonManagerQueue_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        DungeonListColumn(dungeonManagerQueue_, 0, 45, L"#");
        DungeonListColumn(dungeonManagerQueue_, 1, 230, L"Phó bản");
        DungeonListColumn(dungeonManagerQueue_, 2, 65, L"Lượt");
        mk(L"STATIC", L"Phó bản:", SS_LEFT | SS_CENTERIMAGE, 418, 332, 60, 28, 0);
        dungeonManagerPreset_ = mk(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL, 480, 332, 210, 300, IDC_DGM_PRESET);
        mk(L"STATIC", L"Lượt:", SS_LEFT | SS_CENTERIMAGE, 696, 332, 36, 28, 0);
        dungeonManagerRuns_ = mk(L"EDIT", L"1", WS_BORDER | ES_NUMBER | ES_CENTER, 734, 332, 46, 28, IDC_DGM_RUNS);
        mk(L"BUTTON", L"+ THÊM", BS_PUSHBUTTON, 418, 370, 100, 32, IDC_DGM_QUEUE_ADD);
        mk(L"BUTTON", L"SỬA DÒNG", BS_PUSHBUTTON, 524, 370, 105, 32, IDC_DGM_QUEUE_UPDATE);
        mk(L"BUTTON", L"XÓA", BS_PUSHBUTTON, 635, 370, 70, 32, IDC_DGM_QUEUE_DELETE);
        mk(L"BUTTON", L"↑", BS_PUSHBUTTON, 711, 370, 32, 32, IDC_DGM_QUEUE_UP);
        mk(L"BUTTON", L"↓", BS_PUSHBUTTON, 748, 370, 32, 32, IDC_DGM_QUEUE_DOWN);
        mk(L"STATIC",
           L"Quy tắc: bảng thành viên chỉ hiện người của đội đang chọn. Dùng + THÊM / XÓA THÀNH VIÊN; KEY luôn là Slot 1 sau khi lưu.\r\n"
           L"Mỗi đội có thể xếp nhiều phó bản, mỗi dòng 1–10 lượt. Sau mỗi lượt: rời map → báo Telegram → dùng chung Auto Sell → mới chạy lượt/kế hoạch tiếp.\r\n"
           L"AUTO ĐÁNH / DỪNG AUTO / WAIT không nhập tọa độ riêng sẽ kế thừa tọa độ hợp lệ gần nhất phía trên cùng Map.",
           SS_LEFT | WS_BORDER, 14, 420, 766, 150, 0);
    }

    void OpenDungeonTeamManager() {
        EnsureDungeonManagerWindow();
        if (!dungeonManagerWindow_) return;
        RefreshDungeonManagerFromSelection();
        ShowWindow(dungeonManagerWindow_, SW_SHOW);
        SetForegroundWindow(dungeonManagerWindow_);
    }

    void RefreshDungeonManagerFromSelection() {
        if (!dungeonManagerWindow_) return;
        const int index = SelectedDungeonTeamIndex();
        if (index < 0 || index >= static_cast<int>(dungeonTeams_.size())) {
            SetText(dungeonManagerStatus_, L"Chọn một đội ở bảng chính"); return;
        }
        DungeonTeamRuntime& team = dungeonTeams_[static_cast<std::size_t>(index)];
        DungeonV45SyncLegacyConfig(team);
        SetText(dungeonManagerName_, team.displayName);
        SetText(dungeonManagerStatus_, DungeonV45TeamLabel(team) + L" • " + cleanroute_dungeon::TeamStateLabel(team.state));

        ListView_DeleteAllItems(dungeonManagerMembers_);
        SendMessageW(dungeonManagerLeader_, CB_RESETCONTENT, 0, 0);
        int row = 0, leaderSel = -1;
        for (std::size_t slot = 0; slot < team.memberRoleIDs.size(); ++slot) {
            const std::int32_t roleID = team.memberRoleIDs[slot];
            Account* found = nullptr;
            for (const auto& ptr : accounts_) {
                if (ptr && ptr->snapshotValid && (ptr->snapshot.validMask & ValidIdentity) && ptr->snapshot.roleID == roleID) {
                    found = ptr.get();
                    break;
                }
            }
            std::wstring name = found ? std::wstring(found->snapshot.characterName) : L"RoleID " + std::to_wstring(roleID);
            std::wstring label = L"Slot " + std::to_wstring(slot + 1) +
                                 (slot == 0 ? L" / KEY • " : L" • ") + name +
                                 L" • " + std::to_wstring(roleID);
            LVITEMW item{}; item.mask = LVIF_TEXT | LVIF_PARAM; item.iItem = row;
            item.lParam = static_cast<LPARAM>(roleID); item.pszText = label.data();
            ListView_InsertItem(dungeonManagerMembers_, &item);
            std::wstring pid = (slot < team.config.pids.size() && team.config.pids[slot] > 0)
                ? std::to_wstring(team.config.pids[slot]) : L"—";
            ListView_SetItemText(dungeonManagerMembers_, row, 1, pid.data());
            const LRESULT cb = SendMessageW(dungeonManagerLeader_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
            if (cb >= 0) {
                SendMessageW(dungeonManagerLeader_, CB_SETITEMDATA, cb, static_cast<LPARAM>(roleID));
                if (roleID == team.leaderRoleID) leaderSel = static_cast<int>(cb);
            }
            ++row;
        }
        if (leaderSel >= 0) SendMessageW(dungeonManagerLeader_, CB_SETCURSEL, leaderSel, 0);
        SendMessageW(dungeonManagerPreset_, CB_RESETCONTENT, 0, 0);
        for (const auto& preset : dungeonPresets_) SendMessageW(dungeonManagerPreset_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(preset.name.c_str()));
        if (!dungeonPresets_.empty()) SendMessageW(dungeonManagerPreset_, CB_SETCURSEL, 0, 0);
        DungeonV45RefreshQueueList(team);
    }

    void DungeonV45RefreshQueueList(const DungeonTeamRuntime& team) {
        if (!dungeonManagerQueue_) return;
        ListView_DeleteAllItems(dungeonManagerQueue_);
        for (std::size_t i = 0; i < team.queue.size(); ++i) {
            std::wstring number = std::to_wstring(i + 1);
            LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = static_cast<int>(i); item.pszText = number.data();
            const int row = ListView_InsertItem(dungeonManagerQueue_, &item);
            const int p = FindDungeonPresetById(team.queue[i].presetId);
            std::wstring name = p >= 0 ? dungeonPresets_[static_cast<std::size_t>(p)].name : team.queue[i].presetId;
            std::wstring runs = std::to_wstring(team.queue[i].runs);
            ListView_SetItemText(dungeonManagerQueue_, row, 1, name.data());
            ListView_SetItemText(dungeonManagerQueue_, row, 2, runs.data());
        }
    }

    void DungeonV45LoadQueueRow(int row) {
        const int index = SelectedDungeonTeamIndex();
        if (index < 0 || index >= static_cast<int>(dungeonTeams_.size())) return;
        auto& team = dungeonTeams_[static_cast<std::size_t>(index)];
        if (row < 0 || row >= static_cast<int>(team.queue.size())) return;
        const auto& entry = team.queue[static_cast<std::size_t>(row)];
        const int preset = FindDungeonPresetById(entry.presetId);
        if (preset >= 0) SendMessageW(dungeonManagerPreset_, CB_SETCURSEL, preset, 0);
        SetText(dungeonManagerRuns_, std::to_wstring(entry.runs));
    }

    bool DungeonV47RoleInOtherTeam(int currentIndex, std::int32_t roleID) const {
        for (std::size_t i = 0; i < dungeonTeams_.size(); ++i) {
            if (static_cast<int>(i) == currentIndex) continue;
            const auto& members = dungeonTeams_[i].memberRoleIDs;
            if (std::find(members.begin(), members.end(), roleID) != members.end()) return true;
        }
        return false;
    }

    void DungeonV47AddMemberFromManager() {
        const int index = SelectedDungeonTeamIndex();
        if (index < 0 || index >= static_cast<int>(dungeonTeams_.size())) return;
        DungeonTeamRuntime& team = dungeonTeams_[static_cast<std::size_t>(index)];
        if (!dungeon_v45::EditableTeam(team.state)) {
            SetText(dungeonManagerStatus_, L"KHÔNG THÊM • STOP đội trước"); return;
        }
        if (team.memberRoleIDs.size() >= cleanroute_dungeon::kMaxTeamMembers) {
            SetText(dungeonManagerStatus_, L"Đội đã đủ 6 thành viên"); return;
        }

        struct Candidate { std::int32_t roleID; std::uint32_t pid; std::wstring label; };
        std::vector<Candidate> candidates;
        for (const auto& ptr : accounts_) {
            if (!ptr || !ptr->snapshotValid || (ptr->snapshot.validMask & ValidIdentity) == 0) continue;
            const std::int32_t roleID = ptr->snapshot.roleID;
            if (std::find(team.memberRoleIDs.begin(), team.memberRoleIDs.end(), roleID) != team.memberRoleIDs.end()) continue;
            if (DungeonV47RoleInOtherTeam(index, roleID)) continue;
            candidates.push_back({roleID, ptr->game.pid,
                std::wstring(ptr->snapshot.characterName) + L" • RoleID " + std::to_wstring(roleID)});
        }
        if (candidates.empty()) {
            SetText(dungeonManagerStatus_, L"Không có acc ngoài đội khả dụng để thêm"); return;
        }

        HMENU menu = CreatePopupMenu();
        if (!menu) return;
        constexpr UINT kFirst = 9100;
        for (std::size_t i = 0; i < candidates.size() && i < 100; ++i)
            AppendMenuW(menu, MF_STRING, kFirst + static_cast<UINT>(i), candidates[i].label.c_str());
        POINT pt{}; GetCursorPos(&pt);
        const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                            pt.x, pt.y, 0, dungeonManagerWindow_, nullptr);
        DestroyMenu(menu);
        if (command < kFirst || command >= kFirst + candidates.size()) return;
        const Candidate& chosen = candidates[static_cast<std::size_t>(command - kFirst)];
        team.memberRoleIDs.push_back(chosen.roleID);
        team.config.pids.push_back(chosen.pid);
        SaveDungeonTeams();
        ResolveDungeonTeamBindings();
        RefreshDungeonAccountList();
        RefreshDungeonTeamList(index);
        RefreshDungeonManagerFromSelection();
        SetText(dungeonManagerStatus_, L"ĐÃ THÊM • " + chosen.label);
    }

    void DungeonV47RemoveMemberFromManager() {
        const int index = SelectedDungeonTeamIndex();
        if (index < 0 || index >= static_cast<int>(dungeonTeams_.size())) return;
        DungeonTeamRuntime& team = dungeonTeams_[static_cast<std::size_t>(index)];
        if (!dungeon_v45::EditableTeam(team.state)) {
            SetText(dungeonManagerStatus_, L"KHÔNG XÓA • STOP đội trước"); return;
        }
        const int row = dungeonManagerMembers_ ? ListView_GetNextItem(dungeonManagerMembers_, -1, LVNI_SELECTED) : -1;
        if (row < 0) { SetText(dungeonManagerStatus_, L"Chọn thành viên cần xóa"); return; }
        LVITEMW item{}; item.mask = LVIF_PARAM; item.iItem = row;
        if (!ListView_GetItem(dungeonManagerMembers_, &item)) return;
        const std::int32_t roleID = static_cast<std::int32_t>(item.lParam);
        if (roleID == team.leaderRoleID) {
            SetText(dungeonManagerStatus_, L"Không xóa KEY • hãy chọn KEY khác rồi LƯU TÊN / KEY trước"); return;
        }
        auto it = std::find(team.memberRoleIDs.begin(), team.memberRoleIDs.end(), roleID);
        if (it == team.memberRoleIDs.end()) return;
        if (team.memberRoleIDs.size() <= 1) {
            SetText(dungeonManagerStatus_, L"Tổ đội phải còn tối thiểu 1 thành viên"); return;
        }
        const std::size_t pos = static_cast<std::size_t>(std::distance(team.memberRoleIDs.begin(), it));
        team.memberRoleIDs.erase(it);
        if (pos < team.config.pids.size()) team.config.pids.erase(team.config.pids.begin() + static_cast<std::ptrdiff_t>(pos));
        SaveDungeonTeams();
        ResolveDungeonTeamBindings();
        RefreshDungeonAccountList();
        RefreshDungeonTeamList(index);
        RefreshDungeonManagerFromSelection();
        SetText(dungeonManagerStatus_, L"ĐÃ XÓA RoleID " + std::to_wstring(roleID) + L" khỏi đội");
    }

    void DungeonV45UpdateTeamFromManager() {
        const int index = SelectedDungeonTeamIndex();
        if (index < 0 || index >= static_cast<int>(dungeonTeams_.size())) return;
        DungeonTeamRuntime& team = dungeonTeams_[static_cast<std::size_t>(index)];
        if (!dungeon_v45::EditableTeam(team.state)) {
            SetText(dungeonManagerStatus_, L"KHÔNG SỬA • STOP đội trước"); return;
        }
        std::vector<std::int32_t> roles = team.memberRoleIDs;
        std::vector<std::uint32_t> pids = team.config.pids;
        if (roles.empty() || roles.size() != pids.size() || roles.size() > cleanroute_dungeon::kMaxTeamMembers) {
            SetText(dungeonManagerStatus_, L"LỖI • đội phải có 1–6 acc"); return;
        }
        const int leaderSel = static_cast<int>(SendMessageW(dungeonManagerLeader_, CB_GETCURSEL, 0, 0));
        if (leaderSel < 0) { SetText(dungeonManagerStatus_, L"LỖI • chưa chọn KEY"); return; }
        const std::int32_t leaderRole = static_cast<std::int32_t>(SendMessageW(dungeonManagerLeader_, CB_GETITEMDATA, leaderSel, 0));
        auto leaderIt = std::find(roles.begin(), roles.end(), leaderRole);
        if (leaderIt == roles.end()) { SetText(dungeonManagerStatus_, L"LỖI • KEY phải được tick trong đội"); return; }
        const std::size_t leaderPos = static_cast<std::size_t>(std::distance(roles.begin(), leaderIt));
        if (leaderPos != 0) {
            std::rotate(roles.begin(), roles.begin() + static_cast<std::ptrdiff_t>(leaderPos),
                        roles.begin() + static_cast<std::ptrdiff_t>(leaderPos + 1));
            std::rotate(pids.begin(), pids.begin() + static_cast<std::ptrdiff_t>(leaderPos),
                        pids.begin() + static_cast<std::ptrdiff_t>(leaderPos + 1));
        }
        for (std::size_t other = 0; other < dungeonTeams_.size(); ++other) {
            if (static_cast<int>(other) == index || !cleanroute_dungeon::ActiveState(dungeonTeams_[other].state)) continue;
            for (std::int32_t role : roles) {
                if (std::find(dungeonTeams_[other].memberRoleIDs.begin(), dungeonTeams_[other].memberRoleIDs.end(), role) != dungeonTeams_[other].memberRoleIDs.end()) {
                    SetText(dungeonManagerStatus_, L"LỖI • RoleID đang thuộc đội RUN/PAUSE khác"); return;
                }
            }
        }
        std::wstring queueError;
        if (!dungeon_v45::ValidateQueue(team.queue, queueError)) { SetText(dungeonManagerStatus_, L"LỖI • " + queueError); return; }
        team.displayName = GetText(dungeonManagerName_);
        team.memberRoleIDs = roles; team.config.pids = pids; team.leaderRoleID = leaderRole;
        team.config.leaderPid = pids.front();
        team.queueIndex = 0; team.queueRunIndex = 1; DungeonV45SyncLegacyConfig(team);
        SaveDungeonTeams(); ResolveDungeonTeamBindings(); RefreshDungeonAccountList(); RefreshDungeonTeamList(); RefreshDungeonStepList();
        SetText(dungeonManagerStatus_, L"ĐÃ CẬP NHẬT • " + DungeonV45TeamLabel(team));
    }

    void DungeonV45QueueAdd(bool update) {
        const int index = SelectedDungeonTeamIndex();
        if (index < 0 || index >= static_cast<int>(dungeonTeams_.size())) return;
        auto& team = dungeonTeams_[static_cast<std::size_t>(index)];
        if (!dungeon_v45::EditableTeam(team.state)) { SetText(dungeonManagerStatus_, L"KHÔNG SỬA • STOP đội trước"); return; }
        const int preset = static_cast<int>(SendMessageW(dungeonManagerPreset_, CB_GETCURSEL, 0, 0));
        const int runs = ParseEditInt(dungeonManagerRuns_, 1, 1, dungeon_v45::kMaxRunsPerEntry);
        if (preset < 0 || preset >= static_cast<int>(dungeonPresets_.size()) || runs < 1 || runs > dungeon_v45::kMaxRunsPerEntry) {
            SetText(dungeonManagerStatus_, L"LỖI • phó bản/lượt không hợp lệ (1–10)"); return;
        }
        dungeon_v45::QueueEntry entry{dungeonPresets_[static_cast<std::size_t>(preset)].id, runs};
        if (update) {
            const int row = ListView_GetNextItem(dungeonManagerQueue_, -1, LVNI_SELECTED);
            if (row < 0 || row >= static_cast<int>(team.queue.size())) { SetText(dungeonManagerStatus_, L"Chọn dòng kế hoạch cần sửa"); return; }
            team.queue[static_cast<std::size_t>(row)] = entry;
        } else {
            if (team.queue.size() >= static_cast<std::size_t>(dungeon_v45::kMaxQueueEntries)) { SetText(dungeonManagerStatus_, L"Tối đa 32 dòng kế hoạch"); return; }
            team.queue.push_back(entry);
        }
        team.queueIndex = 0; team.queueRunIndex = 1; DungeonV45SyncLegacyConfig(team); SaveDungeonTeams(); DungeonV45RefreshQueueList(team); RefreshDungeonTeamList(); RefreshDungeonStepList();
    }

    void DungeonV45QueueDelete() {
        const int index = SelectedDungeonTeamIndex(); if (index < 0 || index >= static_cast<int>(dungeonTeams_.size())) return;
        auto& team = dungeonTeams_[static_cast<std::size_t>(index)];
        if (!dungeon_v45::EditableTeam(team.state)) return;
        const int row = ListView_GetNextItem(dungeonManagerQueue_, -1, LVNI_SELECTED);
        if (row < 0 || row >= static_cast<int>(team.queue.size())) return;
        if (team.queue.size() <= 1) { SetText(dungeonManagerStatus_, L"Phải giữ ít nhất 1 phó bản"); return; }
        team.queue.erase(team.queue.begin() + row); team.queueIndex = 0; team.queueRunIndex = 1; DungeonV45SyncLegacyConfig(team); SaveDungeonTeams(); DungeonV45RefreshQueueList(team); RefreshDungeonTeamList();
    }

    void DungeonV45QueueMove(int delta) {
        const int index = SelectedDungeonTeamIndex(); if (index < 0 || index >= static_cast<int>(dungeonTeams_.size())) return;
        auto& team = dungeonTeams_[static_cast<std::size_t>(index)]; if (!dungeon_v45::EditableTeam(team.state)) return;
        const int row = ListView_GetNextItem(dungeonManagerQueue_, -1, LVNI_SELECTED); const int to = row + delta;
        if (row < 0 || to < 0 || to >= static_cast<int>(team.queue.size())) return;
        std::swap(team.queue[static_cast<std::size_t>(row)], team.queue[static_cast<std::size_t>(to)]);
        team.queueIndex = 0; team.queueRunIndex = 1; DungeonV45SyncLegacyConfig(team); SaveDungeonTeams(); DungeonV45RefreshQueueList(team);
        ListView_SetItemState(dungeonManagerQueue_, to, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    static bool DungeonV45SectionPrefix(const std::wstring& section) {
        return section.rfind(L"Dungeon", 0) == 0;
    }

    static std::vector<std::wstring> DungeonV45IniSections(const std::wstring& path) {
        std::vector<wchar_t> buffer(65536, L'\0');
        const DWORD n = GetPrivateProfileSectionNamesW(buffer.data(), static_cast<DWORD>(buffer.size()), path.c_str());
        std::vector<std::wstring> out; if (n == 0) return out;
        const wchar_t* p = buffer.data(); while (*p) { out.emplace_back(p); p += wcslen(p) + 1; }
        return out;
    }

    static bool DungeonV45CopyIniSection(const std::wstring& source, const std::wstring& dest, const std::wstring& section) {
        std::vector<wchar_t> data(65536, L'\0');
        const DWORD n = GetPrivateProfileSectionW(section.c_str(), data.data(), static_cast<DWORD>(data.size()), source.c_str());
        if (n == 0) return WritePrivateProfileStringW(section.c_str(), nullptr, nullptr, dest.c_str()) != FALSE;
        return WritePrivateProfileSectionW(section.c_str(), data.data(), dest.c_str()) != FALSE;
    }

    bool DungeonV45ValidateImport(const std::wstring& path, std::wstring& error) {
        if (GetPrivateProfileIntW(L"Meta", L"FormatVersion", 0, path.c_str()) != dungeon_v45::kConfigFormatVersion) {
            error = L"FormatVersion không hỗ trợ"; return false;
        }
        const auto canonical = cleanroute_dungeon::CanonicalPresets();
        auto validPreset = [&](const std::wstring& id) {
            return std::any_of(canonical.begin(), canonical.end(), [&](const auto& p) { return p.id == id; });
        };
        const int teamCount = std::clamp(static_cast<int>(GetPrivateProfileIntW(L"DungeonTeams", L"Count", 0, path.c_str())), 0, 64);
        for (int i = 0; i < teamCount; ++i) {
            const std::wstring sec = L"DungeonTeam_" + std::to_wstring(i);
            const int members = GetPrivateProfileIntW(sec.c_str(), L"MemberCount", 0, path.c_str());
            if (members < 1 || members > 6) { error = L"Team import có MemberCount lỗi"; return false; }
            const int leader = GetPrivateProfileIntW(sec.c_str(), L"LeaderRoleID", 0, path.c_str());
            bool leaderFound = false;
            for (int m = 0; m < members; ++m) {
                const int role = GetPrivateProfileIntW(sec.c_str(), (L"MemberRole_" + std::to_wstring(m)).c_str(), 0, path.c_str());
                if (role <= 0) { error = L"Team import có RoleID lỗi"; return false; }
                if (role == leader) leaderFound = true;
            }
            if (!leaderFound) { error = L"KEY import không nằm trong thành viên"; return false; }
            const int qCount = GetPrivateProfileIntW(sec.c_str(), L"QueueCount", 0, path.c_str());
            if (qCount < 1 || qCount > dungeon_v45::kMaxQueueEntries) { error = L"QueueCount import lỗi"; return false; }
            for (int q = 0; q < qCount; ++q) {
                wchar_t id[128]{}; GetPrivateProfileStringW(sec.c_str(), (L"QueuePreset_" + std::to_wstring(q)).c_str(), L"", id, _countof(id), path.c_str());
                const int runs = GetPrivateProfileIntW(sec.c_str(), (L"QueueRuns_" + std::to_wstring(q)).c_str(), 0, path.c_str());
                if (!validPreset(id) || runs < 1 || runs > dungeon_v45::kMaxRunsPerEntry) { error = L"Queue import có preset/lượt lỗi"; return false; }
            }
        }
        for (const auto& preset : canonical) {
            const std::wstring sec = L"DungeonPreset_" + preset.id;
            if (GetPrivateProfileIntW(sec.c_str(), L"Override", 0, path.c_str()) == 0) continue;
            const int count = GetPrivateProfileIntW(sec.c_str(), L"StepCount", 0, path.c_str());
            if (count < 1 || count > 256) { error = L"StepCount import lỗi: " + preset.name; return false; }
            std::vector<cleanroute_dungeon::Step> steps; steps.reserve(count);
            for (int s = 0; s < count; ++s) {
                const std::wstring p = L"S" + std::to_wstring(s) + L"_"; cleanroute_dungeon::Step step{};
                const int kind = GetPrivateProfileIntW(sec.c_str(), (p + L"Kind").c_str(), -1, path.c_str());
                if (kind < static_cast<int>(cleanroute_dungeon::StepKind::Move) || kind > static_cast<int>(cleanroute_dungeon::StepKind::Portal)) { error = L"Step Kind import lỗi"; return false; }
                step.kind = static_cast<cleanroute_dungeon::StepKind>(kind);
                step.mapID = GetPrivateProfileIntW(sec.c_str(), (p + L"Map").c_str(), 0, path.c_str());
                step.x = GetPrivateProfileIntW(sec.c_str(), (p + L"X").c_str(), 0, path.c_str());
                step.y = GetPrivateProfileIntW(sec.c_str(), (p + L"Y").c_str(), 0, path.c_str());
                step.timeoutSec = GetPrivateProfileIntW(sec.c_str(), (p + L"Timeout").c_str(), 180, path.c_str());
                step.parallelGroup = GetPrivateProfileIntW(sec.c_str(), (p + L"Parallel").c_str(), 0, path.c_str());
                step.autoFightOnArrival = GetPrivateProfileIntW(sec.c_str(), (p + L"FightOnArrival").c_str(), 0, path.c_str()) != 0;
                step.participantMask = static_cast<std::uint32_t>(GetPrivateProfileIntW(sec.c_str(), (p + L"Mask").c_str(), static_cast<int>(cleanroute_dungeon::kAllParticipantsMask), path.c_str()));
                wchar_t monster[256]{}; GetPrivateProfileStringW(sec.c_str(), (p + L"Monster").c_str(), L"", monster, _countof(monster), path.c_str()); step.monsterName = monster;
                step.monsterResID = GetPrivateProfileIntW(sec.c_str(), (p + L"ResID").c_str(), 0, path.c_str());
                step.matchAnyVerified = GetPrivateProfileIntW(sec.c_str(), (p + L"MatchAny").c_str(), 0, path.c_str()) != 0;
                std::wstring stepError; if (!cleanroute_dungeon::ValidateStep(step, stepError)) { error = L"STEP import lỗi • " + stepError; return false; }
                steps.push_back(step);
            }
            std::wstring inheritError; if (!dungeon_v45::ValidateInheritedCoordinates(steps, inheritError)) { error = inheritError; return false; }
            if (!dungeon_v45::ValidateParallelGroups(steps, inheritError)) { error = inheritError; return false; }
        }
        return true;
    }

    void ExportDungeonAllConfig() {
        SaveDungeonTeams(); SaveDungeonMonsterCatalog();
        OPENFILENAMEW ofn{}; wchar_t path[MAX_PATH] = L"ThanLong_AutoPhoBan_v45.tlpb";
        ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd_; ofn.lpstrFile = path; ofn.nMaxFile = _countof(path);
        ofn.lpstrFilter = L"Thần Long Phó Bản Backup (*.tlpb)\0*.tlpb\0All files\0*.*\0";
        ofn.lpstrDefExt = L"tlpb"; ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        if (!GetSaveFileNameW(&ofn)) return;
        HANDLE f = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (f == INVALID_HANDLE_VALUE) { SetText(dungeonStatus_, L"XUẤT CẤU HÌNH FAIL • không tạo được file"); return; }
        const wchar_t bom = 0xFEFF; DWORD written = 0; WriteFile(f, &bom, sizeof(bom), &written, nullptr); CloseHandle(f);
        WritePrivateProfileStringW(L"Meta", L"FormatVersion", L"1", path);
        WritePrivateProfileStringW(L"Meta", L"ToolVersion", L"4.5", path);
        WritePrivateProfileStringW(L"Meta", L"DungeonConfigVersion", L"1", path);
        WritePrivateProfileStringW(L"Meta", L"RuntimeExcluded", L"PID,HWND,snapshot,timer,RUN-state", path);
        const auto sections = DungeonV45IniSections(ConfigPath()); int copied = 0;
        for (const auto& section : sections) if (DungeonV45SectionPrefix(section) && DungeonV45CopyIniSection(ConfigPath(), path, section)) ++copied;
        SetText(dungeonStatus_, L"ĐÃ XUẤT TOÀN BỘ CẤU HÌNH PB • sections=" + std::to_wstring(copied));
    }

    void ImportDungeonAllConfig() {
        if (AnyDungeonActive()) { SetText(dungeonStatus_, L"KHÔNG NHẬP • STOP toàn bộ đội trước"); return; }
        OPENFILENAMEW ofn{}; wchar_t path[MAX_PATH]{};
        ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd_; ofn.lpstrFile = path; ofn.nMaxFile = _countof(path);
        ofn.lpstrFilter = L"Thần Long Phó Bản Backup (*.tlpb)\0*.tlpb\0All files\0*.*\0";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (!GetOpenFileNameW(&ofn)) return;
        std::wstring error; if (!DungeonV45ValidateImport(path, error)) { SetText(dungeonStatus_, L"NHẬP FAIL-CLOSED • " + error); return; }
        const std::wstring backup = ConfigPath() + L".pre_v45_import.bak"; CopyFileW(ConfigPath().c_str(), backup.c_str(), FALSE);
        for (const auto& section : DungeonV45IniSections(ConfigPath())) if (DungeonV45SectionPrefix(section)) WritePrivateProfileStringW(section.c_str(), nullptr, nullptr, ConfigPath().c_str());
        for (const auto& section : DungeonV45IniSections(path)) if (DungeonV45SectionPrefix(section)) DungeonV45CopyIniSection(path, ConfigPath(), section);
        FlushIni();
        dungeonPresets_ = cleanroute_dungeon::CanonicalPresets(); LoadDungeonPresetOverrides(); LoadDungeonTeams(); LoadDungeonMonsterCatalog();
        ResolveDungeonTeamBindings(); RefreshDungeonAccountList(); RefreshDungeonPresetCombo(); RefreshDungeonTeamList(); RefreshDungeonStepList(); RefreshDungeonProgressPanel();
        SetText(dungeonStatus_, L"ĐÃ NHẬP TOÀN BỘ CẤU HÌNH PB • backup=" + backup);
    }

    void DungeonV45CreateMainControls() {
        // Reserve the last two rows for v4.5 controls. Move the old safety note down instead of
        // painting over it; this is the same no-overlap rule used for ACC BÁO CÁO.
        for (HWND h : dungeonControls_) {
            if (!h) continue;
            wchar_t text[256]{};
            GetWindowTextW(h, text, _countof(text));
            if (wcsncmp(text, L"An toàn:", 7) == 0) {
                MoveWindow(h, 18, 966, 1005, 30, TRUE);
                break;
            }
        }
        DungeonMake(L"BUTTON", L"BẮT ĐẦU TỪ STEP ĐÃ CHỌN", BS_DEFPUSHBUTTON,
                    18, 894, 260, 30, IDC_DG_START_FROM_STEP);
        DungeonMake(L"BUTTON", L"MONSTER ĐÃ LƯU", BS_PUSHBUTTON,
                    286, 894, 180, 30, IDC_DG_MONSTER_CATALOG);
        DungeonMake(L"STATIC", L"Slot 1 = KEY • Parallel P# chạy đồng thời • Monster có thể để trống",
                    SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 474, 894, 549, 30, 0);
        DungeonMake(L"BUTTON", L"BẢNG HOẠT ĐỘNG PB", BS_PUSHBUTTON,
                    18, 930, 180, 30, IDC_DG_ACTIVITY_BOARD);
        DungeonMake(L"BUTTON", L"QUẢN LÝ ĐỘI / KẾ HOẠCH", BS_PUSHBUTTON,
                    206, 930, 245, 30, IDC_DG_TEAM_MANAGER);
        DungeonMake(L"BUTTON", L"XUẤT TOÀN BỘ CẤU HÌNH", BS_PUSHBUTTON,
                    459, 930, 220, 30, IDC_DG_EXPORT_ALL);
        DungeonMake(L"BUTTON", L"NHẬP TOÀN BỘ CẤU HÌNH", BS_PUSHBUTTON,
                    687, 930, 220, 30, IDC_DG_IMPORT_ALL);
    }
