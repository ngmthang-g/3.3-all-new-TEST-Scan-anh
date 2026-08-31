from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(rel):
    return (ROOT / rel).read_text(encoding='utf-8')


def write(rel, text):
    (ROOT / rel).write_text(text, encoding='utf-8')


def replace_exact(rel, old, new, expected=1):
    text = read(rel)
    count = text.count(old)
    if count != expected:
        raise RuntimeError(f'{rel}: expected {expected} exact matches, got {count}: {old[:120]!r}')
    write(rel, text.replace(old, new))


def replace_regex(rel, pattern, replacement, expected=1, flags=re.S):
    text = read(rel)
    new_text, count = re.subn(pattern, replacement, text, flags=flags)
    if count != expected:
        raise RuntimeError(f'{rel}: expected {expected} regex matches, got {count}: {pattern[:160]!r}')
    write(rel, new_text)


# ---------------- controller.cpp ----------------
replace_exact('src/controller.cpp',
              'constexpr wchar_t kTitle[] = L"AUTO Thần Long đa tính năng Pro v4.2";',
              'constexpr wchar_t kTitle[] = L"AUTO Thần Long đa tính năng Pro v4.3";')
replace_exact('src/controller.cpp',
              'constexpr int IDC_DG_SAVE_THRESHOLD=717;',
              'constexpr int IDC_DG_SAVE_THRESHOLD=717;\nconstexpr int IDC_DG_SCAN_CLIENT=718;')
replace_exact('src/controller.cpp',
              '    Train,\n    MountRecovery,\n};',
              '    Train,\n    MountRecovery,\n    Dungeon,\n};')
replace_exact('src/controller.cpp',
              '    int runIndex = 1;\n    int stepIndex = 0;\n    int kills = 0;',
              '    int runIndex = 1;\n    int stepIndex = 0;\n    bool fightStopPending = false;\n    bool bindingError = false;\n    int kills = 0;')
replace_exact('src/controller.cpp',
              'if ((!rt.running && !a.pk.active) || rt.clientFreezeActive || !a.snapshotValid || !IsWindow(a.game.window)) return false;',
              'if ((!rt.running && !a.pk.active && !a.dungeonOwned) || rt.clientFreezeActive || !a.snapshotValid || !IsWindow(a.game.window)) return false;',
              expected=2)
replace_exact('src/controller.cpp',
              '    void SwitchMainTab(int index) {',
              '''    void EnterDungeonTabSafetyStop() {
        if (recorderMode_ != RecorderMode::None) StopRecorder(true);
        if (tradeTxn_.phase != TradePhase::Idle) AbortTrade(L"chuyển sang tab AUTO PHÓ BẢN", GetTickCount());
        StopAutoPk(L"vào tab AUTO PHÓ BẢN");
        for (auto& p : accounts_) {
            if (p && p->runtime.running && !p->dungeonOwned) StopAccount(*p);
        }
        ReleaseTradeHolds();
        for (std::size_t i = 0; i < accounts_.size(); ++i) UpdateAccountRow(static_cast<int>(i), *accounts_[i]);
        Log(L"AUTO PHÓ BẢN • TAB OWNERSHIP: đã STOP toàn bộ AUTO + AUTO PK trước khi điều phối đội.");
    }

    void SwitchMainTab(int index) {''')
replace_exact('src/controller.cpp',
              '''        } else if (index == 2) {
            ShowDungeonControls(true); RefreshDungeonAccountList(); RefreshDungeonTeamList(); RefreshDungeonStepList();
''',
              '''        } else if (index == 2) {
            EnterDungeonTabSafetyStop();
            ShowDungeonControls(true); RefreshDungeonAccountList(); RefreshDungeonTeamList(); RefreshDungeonStepList();
''')
replace_regex('src/controller.cpp',
              r'''        if \(hdr->hwndFrom == dungeonTeamList_ && hdr->code == LVN_ITEMCHANGED\) \{.*?            return;\n        \}\n        if \(hdr->hwndFrom == dungeonAccountList_''',
              '''        if (hdr->hwndFrom == dungeonTeamList_ && hdr->code == LVN_ITEMCHANGED) {
            const auto* n = reinterpret_cast<const NMLISTVIEW*>(hdr);
            if ((n->uChanged & LVIF_STATE) != 0 && (n->uNewState & LVIS_SELECTED) != 0) {
                RefreshDungeonStepList();
                const int teamIndex = SelectedDungeonTeamIndex();
                if (teamIndex >= 0 && teamIndex < static_cast<int>(dungeonTeams_.size()) && dungeonStatus_)
                    SetText(dungeonStatus_, L"ĐỘI " + std::to_wstring(dungeonTeams_[static_cast<std::size_t>(teamIndex)].config.id) +
                                            L" • " + dungeonTeams_[static_cast<std::size_t>(teamIndex)].status);
            }
            return;
        }
        if (hdr->hwndFrom == dungeonAccountList_''')
replace_exact('src/controller.cpp',
              '''            case WM_NOTIFY:
                OnListNotification(reinterpret_cast<const NMHDR*>(lp));
                return 0;
''',
              '''            case WM_NOTIFY: {
                const NMHDR* hdr = reinterpret_cast<const NMHDR*>(lp);
                if (hdr && hdr->hwndFrom == dungeonTeamList_ && hdr->code == NM_CUSTOMDRAW)
                    return DungeonTeamCustomDraw(reinterpret_cast<const NMLVCUSTOMDRAW*>(lp));
                OnListNotification(hdr);
                return 0;
            }
''')
replace_exact('src/controller.cpp',
              '                    case IDC_DG_REFRESH: RefreshDungeonAccountList(); break;\n',
              '                    case IDC_DG_REFRESH: RefreshDungeonAccountList(); break;\n                    case IDC_DG_SCAN_CLIENT: DungeonScanClients(); break;\n')

# ---------------- dungeon_app_methods.inl ----------------
replace_regex('src/dungeon_app_methods.inl',
              r'''    void ResolveDungeonTeamBindings\(\) \{.*?\n    \}\n\n    void BuildDungeonUi\(\) \{''',
              '''    void ResolveDungeonTeamBindings() {
        for (DungeonTeamRuntime& team : dungeonTeams_) {
            if (cleanroute_dungeon::ActiveState(team.state)) continue;
            team.config.pids.clear();
            team.config.leaderPid = 0;
            for (std::int32_t wantedRole : team.memberRoleIDs) {
                for (auto& item : accounts_) {
                    Account& account = *item;
                    if (!account.snapshotValid || (account.snapshot.validMask & ValidIdentity) == 0) continue;
                    if (account.snapshot.roleID == wantedRole) {
                        team.config.pids.push_back(account.game.pid);
                        if (wantedRole == team.leaderRoleID) team.config.leaderPid = account.game.pid;
                        break;
                    }
                }
            }
            const bool complete = team.config.pids.size() == team.memberRoleIDs.size() && team.config.leaderPid != 0;
            if (complete) {
                if (team.state == cleanroute_dungeon::TeamState::Error && team.bindingError) {
                    team.state = cleanroute_dungeon::TeamState::Stopped;
                    team.bindingError = false;
                    team.status = L"Đã bind lại đủ client • STOP";
                } else if (team.state != cleanroute_dungeon::TeamState::Error) {
                    team.status = L"Đã bind đủ client • STOP";
                }
            } else {
                team.status = L"Thiếu client: " + std::to_wstring(team.config.pids.size()) + L"/" +
                              std::to_wstring(team.memberRoleIDs.size());
            }
        }
    }

    void DungeonScanClients() {
        if (AnyDungeonActive()) {
            if (dungeonStatus_) SetText(dungeonStatus_, L"KHÔNG QUÉT • đang có đội RUN/PAUSE; STOP đội trước");
            Log(L"AUTO PHÓ BẢN • QUÉT CLIENT bị chặn vì có đội RUN/PAUSE.");
            return;
        }
        ScanClients();
        ResolveDungeonTeamBindings();
        RefreshDungeonAccountList();
        RefreshDungeonTeamList();
        RefreshDungeonStepList();
        if (dungeonStatus_) SetText(dungeonStatus_, L"QUÉT CLIENT xong • đã bind lại tổ đội theo RoleID");
    }

    void BuildDungeonUi() {''')
replace_exact('src/dungeon_app_methods.inl',
              'L"AUTO PHÓ BẢN v4.0 • mỗi tổ đội 1–6 acc • nhiều tổ đội chạy độc lập"',
              'L"AUTO PHÓ BẢN v4.3 • mỗi tổ đội 1–6 acc • nhiều tổ đội chạy độc lập"')
replace_regex('src/dungeon_app_methods.inl',
              r'''        DungeonMake\(L"BUTTON", L"LÀM MỚI", BS_PUSHBUTTON, 18, 234, 105, 28, IDC_DG_REFRESH\);.*?        DungeonMake\(L"BUTTON", L"TẠO TỔ ĐỘI", BS_DEFPUSHBUTTON, 880, 234, 143, 28, IDC_DG_CREATE_TEAM\);''',
              '''        DungeonMake(L"BUTTON", L"LÀM MỚI", BS_PUSHBUTTON, 18, 234, 105, 28, IDC_DG_REFRESH);
        DungeonMake(L"BUTTON", L"QUÉT CLIENT", BS_PUSHBUTTON, 128, 234, 110, 28, IDC_DG_SCAN_CLIENT);
        DungeonMake(L"STATIC", L"Bán <", SS_LEFT | SS_CENTERIMAGE, 245, 234, 42, 28, 0);
        dungeonSellThresholdEdit_ = DungeonMake(L"EDIT", L"40", WS_BORDER | ES_NUMBER | ES_CENTER,
                                                289, 234, 43, 28, IDC_DG_SELL_THRESHOLD);
        DungeonMake(L"BUTTON", L"LƯU", BS_PUSHBUTTON, 336, 234, 50, 28, IDC_DG_SAVE_THRESHOLD);
        DungeonMake(L"STATIC", L"Đội trưởng:", SS_LEFT | SS_CENTERIMAGE, 394, 234, 70, 28, 0);
        dungeonLeaderCombo_ = DungeonMake(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL,
                                          466, 234, 140, 300, IDC_DG_LEADER);
        DungeonMake(L"STATIC", L"Phó bản:", SS_LEFT | SS_CENTERIMAGE, 612, 234, 58, 28, 0);
        dungeonPresetCombo_ = DungeonMake(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL,
                                          672, 234, 150, 400, IDC_DG_PRESET);
        DungeonMake(L"STATIC", L"Lượt:", SS_LEFT | SS_CENTERIMAGE, 828, 234, 38, 28, 0);
        dungeonRunsEdit_ = DungeonMake(L"EDIT", L"1", WS_BORDER | ES_NUMBER | ES_CENTER,
                                       868, 234, 42, 28, IDC_DG_RUNS);
        DungeonMake(L"BUTTON", L"TẠO TỔ ĐỘI", BS_DEFPUSHBUTTON, 918, 234, 105, 28, IDC_DG_CREATE_TEAM);''')
replace_regex('src/dungeon_app_methods.inl',
              r'''        DungeonListColumn\(dungeonTeamList_, 0, 70, L"Tổ đội"\);.*?        DungeonListColumn\(dungeonTeamList_, 6, 165, L"Tiến độ"\);''',
              '''        DungeonListColumn(dungeonTeamList_, 0, 120, L"Tổ đội / vai trò");
        DungeonListColumn(dungeonTeamList_, 1, 250, L"Nhân vật / RoleID");
        DungeonListColumn(dungeonTeamList_, 2, 75, L"PID");
        DungeonListColumn(dungeonTeamList_, 3, 155, L"Phó bản");
        DungeonListColumn(dungeonTeamList_, 4, 60, L"Lượt");
        DungeonListColumn(dungeonTeamList_, 5, 80, L"State");
        DungeonListColumn(dungeonTeamList_, 6, 265, L"Tiến độ");''')
replace_regex('src/dungeon_app_methods.inl',
              r'''    int SelectedDungeonTeamIndex\(\) const \{.*?\n    \}\n\n    cleanroute_dungeon::Preset\* DungeonConfiguredPresetForTeam''',
              '''    int SelectedDungeonTeamIndex() const {
        if (!dungeonTeamList_) return -1;
        const int row = ListView_GetNextItem(dungeonTeamList_, -1, LVNI_SELECTED);
        if (row < 0) return -1;
        LVITEMW item{};
        item.mask = LVIF_PARAM;
        item.iItem = row;
        if (!ListView_GetItem(dungeonTeamList_, &item) || item.lParam <= 0) return -1;
        return static_cast<int>((static_cast<std::uintptr_t>(item.lParam) >> 1) - 1u);
    }

    cleanroute_dungeon::Preset* DungeonConfiguredPresetForTeam''')
replace_regex('src/dungeon_app_methods.inl',
              r'''    void RefreshDungeonTeamList\(int select = -1\) \{.*?\n    \}\n\n    int DungeonDisplayPresetIndex\(\) const \{''',
              '''    void RefreshDungeonTeamList(int select = -1) {
        if (!dungeonTeamList_) return;
        if (select < 0) select = SelectedDungeonTeamIndex();
        ListView_DeleteAllItems(dungeonTeamList_);
        int visibleRow = 0;
        for (std::size_t i = 0; i < dungeonTeams_.size(); ++i) {
            DungeonTeamRuntime& team = dungeonTeams_[i];
            const cleanroute_dungeon::Preset* preset = DungeonConfiguredPresetForTeam(team);
            Account* leaderAccount = team.config.leaderPid ? AccountByPid(team.config.leaderPid) : nullptr;
            std::wstring leader = leaderAccount ? AccountTag(*leaderAccount)
                : (team.leaderRoleID > 0 ? L"Role " + std::to_wstring(team.leaderRoleID) : L"?");
            std::wstring pid = team.config.leaderPid ? std::to_wstring(team.config.leaderPid) : L"—";
            std::wstring label = L"ĐỘI " + std::to_wstring(team.config.id) + L" • KEY";
            LVITEMW keyItem{};
            keyItem.mask = LVIF_TEXT | LVIF_PARAM;
            keyItem.iItem = visibleRow;
            keyItem.pszText = label.data();
            keyItem.lParam = static_cast<LPARAM>((static_cast<std::uintptr_t>(i + 1) << 1));
            ListView_InsertItem(dungeonTeamList_, &keyItem);
            ListView_SetItemText(dungeonTeamList_, visibleRow, 1, leader.data());
            ListView_SetItemText(dungeonTeamList_, visibleRow, 2, pid.data());
            std::wstring presetName = preset ? preset->name : L"?";
            ListView_SetItemText(dungeonTeamList_, visibleRow, 3, presetName.data());
            std::wstring runs = std::to_wstring(team.runIndex) + L"/" + std::to_wstring(team.config.runs);
            ListView_SetItemText(dungeonTeamList_, visibleRow, 4, runs.data());
            std::wstring state = cleanroute_dungeon::TeamStateLabel(team.state);
            ListView_SetItemText(dungeonTeamList_, visibleRow, 5, state.data());
            ListView_SetItemText(dungeonTeamList_, visibleRow, 6, team.status.data());
            ++visibleRow;

            for (std::size_t m = 0; m < team.memberRoleIDs.size(); ++m) {
                const std::int32_t roleID = team.memberRoleIDs[m];
                if (roleID == team.leaderRoleID) continue;
                Account* found = nullptr;
                for (auto& candidate : accounts_) {
                    if (candidate->snapshotValid && (candidate->snapshot.validMask & ValidIdentity) &&
                        candidate->snapshot.roleID == roleID) {
                        found = candidate.get();
                        break;
                    }
                }
                std::wstring memberLabel = L"    ↳ Thành viên";
                std::wstring memberName = found ? AccountTag(*found) : L"Role " + std::to_wstring(roleID);
                std::wstring memberPid = found ? std::to_wstring(found->game.pid) : L"—";
                LVITEMW memberItem{};
                memberItem.mask = LVIF_TEXT | LVIF_PARAM;
                memberItem.iItem = visibleRow;
                memberItem.pszText = memberLabel.data();
                memberItem.lParam = static_cast<LPARAM>((static_cast<std::uintptr_t>(i + 1) << 1) | 1u);
                ListView_InsertItem(dungeonTeamList_, &memberItem);
                ListView_SetItemText(dungeonTeamList_, visibleRow, 1, memberName.data());
                ListView_SetItemText(dungeonTeamList_, visibleRow, 2, memberPid.data());
                ++visibleRow;
            }
        }

        if (select < 0 && !dungeonTeams_.empty()) select = 0;
        if (select >= 0 && select < static_cast<int>(dungeonTeams_.size())) {
            const LPARAM wanted = static_cast<LPARAM>((static_cast<std::uintptr_t>(select + 1) << 1));
            const int rows = ListView_GetItemCount(dungeonTeamList_);
            for (int row = 0; row < rows; ++row) {
                LVITEMW item{};
                item.mask = LVIF_PARAM;
                item.iItem = row;
                if (ListView_GetItem(dungeonTeamList_, &item) && item.lParam == wanted) {
                    ListView_SetItemState(dungeonTeamList_, row, LVIS_SELECTED | LVIS_FOCUSED,
                                          LVIS_SELECTED | LVIS_FOCUSED);
                    ListView_EnsureVisible(dungeonTeamList_, row, FALSE);
                    break;
                }
            }
        }
    }

    LRESULT DungeonTeamCustomDraw(const NMLVCUSTOMDRAW* draw) const {
        if (!draw) return CDRF_DODEFAULT;
        if (draw->nmcd.dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
        if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
            const int row = static_cast<int>(draw->nmcd.dwItemSpec);
            LVITEMW item{};
            item.mask = LVIF_PARAM;
            item.iItem = row;
            if (ListView_GetItem(dungeonTeamList_, &item) && item.lParam > 0 &&
                (static_cast<std::uintptr_t>(item.lParam) & 1u) == 0u) {
                auto* mutableDraw = const_cast<NMLVCUSTOMDRAW*>(draw);
                mutableDraw->clrTextBk = RGB(238, 238, 238);
            }
        }
        return CDRF_DODEFAULT;
    }

    int DungeonDisplayPresetIndex() const {''')
replace_regex('src/dungeon_app_methods.inl',
              r'''    void FailDungeonTeam\(DungeonTeamRuntime& team, const std::wstring& reason\) \{.*?\n    \}\n\n    bool ValidateDungeonStart''',
              '''    void FailDungeonTeam(DungeonTeamRuntime& team, const std::wstring& reason) {
        ReleaseDungeonTeamMembers(team);
        team.state = cleanroute_dungeon::TeamState::Error;
        team.bindingError = reason.find(L"mất PID") != std::wstring::npos ||
                            reason.find(L"client") != std::wstring::npos ||
                            reason.find(L"bind") != std::wstring::npos;
        team.status = L"FAIL • " + reason;
        Log(L"AUTO PHÓ BẢN ĐỘI " + std::to_wstring(team.config.id) + L" FAIL-CLOSED • " + reason);
        RefreshDungeonAccountList();
        RefreshDungeonTeamList();
    }

    bool ValidateDungeonStart''')
replace_exact('src/dungeon_app_methods.inl',
              '    void ResetDungeonStepRuntime(DungeonTeamRuntime& team, DWORD now) {\n        team.kills = 0;',
              '    void ResetDungeonStepRuntime(DungeonTeamRuntime& team, DWORD now) {\n        team.fightStopPending = false;\n        team.kills = 0;')
replace_regex('src/dungeon_app_methods.inl',
              r'''        if \(!allOn\) \{.*?            return false;\n        \}\n        if \(team.lastScanTick''',
              '''        if (!allOn) {
            for (Account* account : members) {
                if (account->snapshot.autoFight) continue;
                bool clickOk = false;
                DWORD clickedAt = 0;
                if (ConsumePriorityAutoResult(*account, ClickSlot::Attack, PriorityAutoOwner::Dungeon,
                                              clickOk, clickedAt)) {
                    if (!clickOk) team.status = L"AUTO-ĐÁNH • InputSync AUTO→ĐÁNH QUÁI fail • sẽ thử lại";
                } else {
                    (void)QueuePriorityAutoClick(*account, ClickSlot::Attack, PriorityAutoOwner::Dungeon,
                                                 L"AUTO PHÓ BẢN: dùng core AUTO→ĐÁNH QUÁI của Tab AUTO");
                }
            }
            team.status = taskSnapshotFresh
                ? L"AUTO-ĐÁNH • TASK snapshot OK • InputSync AUTO→ĐÁNH QUÁI • chờ verify ON"
                : L"AUTO-ĐÁNH • TASK fallback GMonster • InputSync AUTO→ĐÁNH QUÁI • chờ verify ON";
            return false;
        }
        if (team.lastScanTick''')
replace_regex('src/dungeon_app_methods.inl',
              r'''            if \(team.stepIndex >= static_cast<int>\(preset->steps.size\(\)\)\) \{.*?                return;\n            \}\n            const cleanroute_dungeon::Step& step''',
              '''            if (team.stepIndex >= static_cast<int>(preset->steps.size())) {
                bool allOff = true;
                for (std::uint32_t pid : team.config.pids) {
                    if (Account* account = AccountByPid(pid)) {
                        if (!EnsureAutoFightOffForTravel(*account, now, L"PB hết STEP")) allOff = false;
                    }
                }
                if (!allOff) {
                    team.status = L"HẾT STEP • dùng core DỪNG AUTO/Travel Guard • chờ verify OFF";
                    return;
                }
                team.phase = cleanroute_dungeon::TeamPhase::WaitExit;
                team.phaseTick = now;
                team.status = L"HẾT STEP • AutoFight OFF • chờ server rời Map " + std::to_wstring(preset->dungeonMap);
                return;
            }
            const cleanroute_dungeon::Step& step''')
replace_regex('src/dungeon_app_methods.inl',
              r'''            if \(step.kind == cleanroute_dungeon::StepKind::Fight\) \{.*?                return;\n            \}\n\n            if \(step.kind == cleanroute_dungeon::StepKind::StopFight\) \{.*?                return;\n            \}''',
              '''            if (step.kind == cleanroute_dungeon::StepKind::Fight) {
                if (!team.fightStopPending && TickDungeonFight(team, step, now, error))
                    team.fightStopPending = true;
                if (team.fightStopPending) {
                    bool allOff = true;
                    for (Account* account : stepMembers) {
                        if (!EnsureAutoFightOffForTravel(*account, now, L"PB hoàn tất mục tiêu")) allOff = false;
                    }
                    if (allOff) AdvanceDungeonStep(team, now);
                    else team.status = L"AUTO-DỪNG • dùng core DỪNG/Travel Guard của Tab AUTO • chờ verify OFF";
                }
                return;
            }

            if (step.kind == cleanroute_dungeon::StepKind::StopFight) {
                bool off = true;
                for (Account* account : stepMembers)
                    if (!EnsureAutoFightOffForTravel(*account, now, L"PB STOP-FIGHT")) off = false;
                if (off) AdvanceDungeonStep(team, now);
                else team.status = L"AUTO-DỪNG • STOP-FIGHT • chờ authoritative AutoFight OFF";
                return;
            }''')
replace_exact('src/dungeon_app_methods.inl', 'L"CẤU HÌNH AUTO PHÓ BẢN v4.0 — "', 'L"CẤU HÌNH AUTO PHÓ BẢN v4.3 — "')

# ---------------- bridge.cpp ----------------
replace_exact('src/bridge.cpp',
              'InvokeScalar(moveNext, enumerator, moved, ignored, _countof(ignored))',
              'InvokeScalar(moveNext, ManagedThis(enumerator), moved, ignored, _countof(ignored))',
              expected=3)
replace_exact('src/bridge.cpp',
              'InvokeObject(getCurrent, enumerator, current, ignored, _countof(ignored))',
              'InvokeObject(getCurrent, ManagedThis(enumerator), current, ignored, _countof(ignored))',
              expected=3)

# ---------------- Telegram account filter ----------------
replace_exact('src/telegram_account_filter.cpp',
              'constexpr int kTelegramTabIndex = 2;',
              'constexpr int kTelegramTabIndex = 3;')

# ---------------- version + docs ----------------
replace_exact('VERSION.txt', '4.2\n', '4.3\n')
replace_exact('CMakeLists.txt',
              'project(ThanLongItemConsolidator VERSION 4.2 LANGUAGES CXX RC)',
              'project(ThanLongItemConsolidator VERSION 4.3 LANGUAGES CXX RC)')
rc = read('resources/app.rc')
for old, new in [
    ('FILEVERSION 4,2,0,0', 'FILEVERSION 4,3,0,0'),
    ('PRODUCTVERSION 4,2,0,0', 'PRODUCTVERSION 4,3,0,0'),
    ('VALUE "FileVersion", "4.2\\0"', 'VALUE "FileVersion", "4.3\\0"'),
    ('AUTO_Than_Long_da_tinh_nang_Pro_v4.2.exe', 'AUTO_Than_Long_da_tinh_nang_Pro_v4.3.exe'),
    ('VALUE "ProductVersion", "4.2\\0"', 'VALUE "ProductVersion", "4.3\\0"'),
]:
    if rc.count(old) != 1:
        raise RuntimeError(f'resources/app.rc missing unique token {old!r}')
    rc = rc.replace(old, new)
write('resources/app.rc', rc)
readme = read('README.md')
readme = readme.replace('# AUTO Thần Long đa tính năng Pro v4.2', '# AUTO Thần Long đa tính năng Pro v4.3', 1)
insert = '''## Mới trong v4.3 — ổn định AUTO PHÓ BẢN + giao diện tổ đội

- Vào tab **AUTO PHÓ BẢN** lập tức STOP toàn bộ AUTO và AUTO PK trước khi trao ownership cho dungeon.
- FIGHT/STOP-FIGHT dùng lại đúng core InputSync `AUTO → ĐÁNH QUÁI` và `DỪNG/Travel Guard` đã chạy ổn ở tab AUTO; luôn verify `Snapshot.AutoFight`.
- Thêm nút **QUÉT CLIENT** thật; tổ đội vẫn persist theo RoleID và bind lại PID sau scan. Lỗi mất client/bind có thể tự về STOP khi bind đủ, lỗi runtime khác vẫn giữ ERROR.
- Bảng tổ đội hiển thị KEY ở dòng đầu nền xám nhẹ, từng thành viên một dòng thụt vào; mọi thao tác START/PAUSE/STOP/XÓA map theo team-id thay vì số dòng UI.
- Sửa TASK `GetDoingTasks()` với boxed value-type enumerator (`ManagedThis`) để Dictionary/List runtime không còn fail chỉ vì gọi `MoveNext/get_Current` trên boxed object.
- Sửa **ACC BÁO CÁO**: tab Telegram thực tế là index 3, không còn render đè lên tab AUTO PHÓ BẢN; sidebar báo cáo chỉ xuất hiện trong Telegram.

'''
anchor = '## Mới trong v4.2 — Mạng lưới đường đi Xa Truyền\n'
if anchor not in readme:
    raise RuntimeError('README anchor missing')
readme = readme.replace(anchor, insert + anchor, 1)
readme = readme.replace('dist/AUTO_Than_Long_da_tinh_nang_Pro_v4.2.exe', 'dist/AUTO_Than_Long_da_tinh_nang_Pro_v4.3.exe')
readme = readme.replace('dist/AUTO-Than-Long-da-tinh-nang-Pro-v4.2-win-x64.zip', 'dist/AUTO-Than-Long-da-tinh-nang-Pro-v4.3-win-x64.zip')
readme = readme.replace('dist/AUTO-Than-Long-da-tinh-nang-Pro-v4.2-source.zip', 'dist/AUTO-Than-Long-da-tinh-nang-Pro-v4.3-source.zip')
write('README.md', readme)
changelog = read('CHANGELOG.md')
entry = '''## 4.3

- AUTO PHÓ BẢN nhận ownership khi vào tab: dừng AUTO + AUTO PK ngay, không để ba scheduler cạnh tranh cùng PID.
- FIGHT/STOP-FIGHT tái dùng InputSync AUTO→ĐÁNH QUÁI và DỪNG/Travel Guard của AUTO, có authoritative AutoFight verify trước khi tiến step.
- Thêm QUÉT CLIENT thật + RoleID→PID rebind; chỉ lỗi binding/client được tự phục hồi ERROR→STOP.
- Giao diện tổ đội dạng cây: KEY dòng đầu nền xám, member mỗi người một dòng; row mapping dùng lParam/team-id tránh START/STOP nhầm đội.
- Sửa boxed IEnumerator trong ReadDungeonProgress/GetDoingTasks và Parameters bằng ManagedThis.
- Sửa ACC BÁO CÁO dùng đúng Telegram tab index 3, không còn chồng lên AUTO PHÓ BẢN.

'''
if '## 4.3' not in changelog:
    changelog = changelog.replace('# Changelog\n\n', '# Changelog\n\n' + entry, 1)
write('CHANGELOG.md', changelog)
plan = read('AUTO_DUNGEON_RUNTIME_TEST_PLAN.md')
plan = plan.replace('- Start AUTO PHÓ BẢN while AUTO TRAIN runs; choose No, then Yes. Confirm one mode only.',
                    '- While AUTO/AUTO PK is running, switch to AUTO PHÓ BẢN. Confirm both prior workflows STOP immediately before dungeon ownership is granted.')
plan += '''

## F. v4.3 regressions

- Close/reopen three of six clients, press **QUÉT CLIENT**, confirm `3/6 -> 6/6` and a binding-only ERROR returns to STOP; an NPC/timeout ERROR must not auto-clear.
- Select both a KEY row and a member row, then START/PAUSE/STOP; both must resolve to the same team and never another visible row.
- Verify KEY rows have a light grey background and members render one-per-line indented below KEY.
- Open AUTO PHÓ BẢN and Telegram repeatedly: **ACC BÁO CÁO** must appear only in Telegram and never overlap dungeon STEP.
- With TASK API returning a Dictionary/value-type enumerator, verify `GetDoingTasks` can enumerate without boxed-this exception; if TASK still fails, GMonster fallback must continue and must not block AutoFight start.
'''
write('AUTO_DUNGEON_RUNTIME_TEST_PLAN.md', plan)

final_workflow = r'''# Canonical clean rebuild after v4.3 dungeon runtime/UI integration.
name: Build v4.3 AUTO Thần Long đa tính năng Pro

on:
  workflow_dispatch:
  pull_request:
    branches:
      - main
  push:
    branches:
      - main
    paths:
      - '.github/workflows/build-v41.yml'
      - 'src/**'
      - 'tools/**'
      - 'resources/**'
      - 'CMakeLists.txt'
      - 'VERSION.txt'
      - 'README.md'
      - 'CHANGELOG.md'

permissions:
  contents: write

jobs:
  build:
    runs-on: windows-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4
        with:
          fetch-depth: 0

      - name: Verify source contracts
        shell: powershell
        run: |
          git diff --check
          if ($LASTEXITCODE -ne 0) { throw 'git diff check failed' }
          python tools/verify_v152_clean.py
          if ($LASTEXITCODE -ne 0) { throw 'core verifier failed' }
          python tools/test_tlmaster_contract.py
          if ($LASTEXITCODE -ne 0) { throw 'TLMASTER verifier failed' }
          if ((Get-Content VERSION.txt -Raw).Trim() -ne '4.3') { throw 'VERSION.txt is not 4.3' }
          if (-not (Select-String -Path CMakeLists.txt -SimpleMatch 'project(ThanLongItemConsolidator VERSION 4.3' -Quiet)) { throw 'CMake project version is not 4.3' }
          if (-not (Select-String -Path resources/app.rc -SimpleMatch 'FILEVERSION 4,3,0,0' -Quiet)) { throw 'EXE VERSIONINFO is not 4.3' }
          if (Select-String -Path src/controller.cpp -Pattern 'DisplayCoordToAutoPath' -Quiet) { throw 'coordinate scaling token still exists' }
          if (-not (Select-String -Path src/controller.cpp -Pattern 'kPreciseWorldTolerance = 20' -Quiet)) { throw 'precise world tolerance missing' }
          if (-not (Select-String -Path src/controller.cpp -Pattern 'PORTAL SPECIAL' -Quiet)) { throw 'interserver portal special routing missing' }
          if (-not (Select-String -Path src/controller.cpp -SimpleMatch 'V3.0 SHORTCUT FIRST' -Quiet)) { throw 'v3.3 initial train shortcut routing missing' }
          if (-not (Select-String -Path src/controller.cpp -SimpleMatch 'runtime NEVER reads these; ID387 uses sellNpcPositions_' -Quiet)) { throw 'Xa Truyen single coordinate source marker missing' }
          if (-not (Select-String -Path src/bridge.cpp -SimpleMatch 'SEMANTIC_SCAN CALLBACK PASS' -Quiet)) { throw 'v0.8-style semantic scanner missing' }
          if (-not (Select-String -Path src/controller.cpp -SimpleMatch 'arrived,kPreciseWorldTolerance); if(!arrived)return false;' -Quiet)) { throw 'treatment NPC precise tolerance missing' }
          if (-not (Select-String -Path src/telegram_account_filter.cpp -SimpleMatch 'kTelegramTabIndex = 3' -Quiet)) { throw 'Telegram report panel tab ownership missing' }
          if (-not (Select-String -Path src/bridge.cpp -SimpleMatch 'InvokeScalar(moveNext, ManagedThis(enumerator)' -Quiet)) { throw 'boxed dungeon enumerator fix missing' }

      - name: Configure x64 Release
        shell: powershell
        run: |
          cmake -S . -B build -A x64
          if ($LASTEXITCODE -ne 0) { throw 'configure failed' }

      - name: Build x64 Release
        shell: powershell
        run: |
          cmake --build build --config Release --parallel
          if ($LASTEXITCODE -ne 0) { throw 'build failed' }

      - name: Run 15 logic tests
        shell: powershell
        run: |
          $tests=@(
            'route_logic_tests','rotation_logic_tests','trade_coordinator_logic_tests',
            'background_ui_logic_tests','fixed_slot_sell_logic_tests','unity_geometry_logic_tests',
            'internal_ui_click_logic_tests','travel_fight_guard_logic_tests','auto_fight_retry_logic_tests',
            'auto_pk_logic_tests','telegram_logic_tests','inventory_filter_logic_tests','dungeon_logic_tests','dungeon_progress_logic_tests','travel_network_logic_tests'
          )
          foreach($t in $tests){
            $p=Join-Path 'build/Release' ($t+'.exe')
            if(!(Test-Path $p)){throw "missing $p"}
            & $p
            if($LASTEXITCODE -ne 0){throw "$t failed"}
          }

      - name: Stage v4.3 binaries and source
        shell: powershell
        run: |
          Remove-Item dist -Recurse -Force -ErrorAction SilentlyContinue
          New-Item -ItemType Directory -Force dist | Out-Null
          Copy-Item build/Release/ThanLongItemConsolidator.exe dist/AUTO_Than_Long_da_tinh_nang_Pro_v4.3.exe -Force
          Copy-Item build/Release/ThanLongCleanRouteBridge.dll dist/ThanLongCleanRouteBridge.dll -Force
          Compress-Archive -Path dist/AUTO_Than_Long_da_tinh_nang_Pro_v4.3.exe,dist/ThanLongCleanRouteBridge.dll -DestinationPath dist/AUTO-Than-Long-da-tinh-nang-Pro-v4.3-win-x64.zip -CompressionLevel Optimal -Force
          git archive --format=zip --output=dist/AUTO-Than-Long-da-tinh-nang-Pro-v4.3-source.zip HEAD -- . ':(exclude)dist'
          if ($LASTEXITCODE -ne 0) { throw 'source archive failed' }
          $items=@('dist/AUTO_Than_Long_da_tinh_nang_Pro_v4.3.exe','dist/ThanLongCleanRouteBridge.dll','dist/AUTO-Than-Long-da-tinh-nang-Pro-v4.3-win-x64.zip','dist/AUTO-Than-Long-da-tinh-nang-Pro-v4.3-source.zip')
          $lines=foreach($p in $items){$h=(Get-FileHash $p -Algorithm SHA256).Hash.ToLower();"$h  $([IO.Path]::GetFileName($p))"}
          $lines | Set-Content dist/SHA256SUMS.txt
          Get-Content dist/SHA256SUMS.txt

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: AUTO-Than-Long-da-tinh-nang-Pro-v4.3
          path: dist/*
          if-no-files-found: error

      - name: Publish dist to main
        if: github.event_name == 'push' || github.event_name == 'workflow_dispatch'
        shell: powershell
        run: |
          git config user.name "github-actions[bot]"
          git config user.email "41898282+github-actions[bot]@users.noreply.github.com"
          Remove-Item build -Recurse -Force -ErrorAction SilentlyContinue
          git add -A -f dist
          git diff --cached --quiet
          if ($LASTEXITCODE -ne 0) {
            git commit -m "build: publish AUTO Thần Long đa tính năng Pro v4.3 [skip ci]"
            git push origin HEAD:main
          }
'''
write('.github/workflows/build-v41.yml', final_workflow)
print('v4.3 approved dungeon fixes applied successfully')
