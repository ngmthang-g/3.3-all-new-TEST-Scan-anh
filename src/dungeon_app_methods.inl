    bool IsDungeonControl(HWND h) const {
        return std::find(dungeonControls_.begin(), dungeonControls_.end(), h) != dungeonControls_.end();
    }

    void ShowDungeonControls(bool show) {
        for (HWND h : dungeonControls_) if (h) ShowWindow(h, show ? SW_SHOW : SW_HIDE);
    }

    void DungeonListColumn(HWND list, int index, int width, const wchar_t* text) {
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        column.pszText = const_cast<wchar_t*>(text);
        column.cx = width;
        column.iSubItem = index;
        ListView_InsertColumn(list, index, &column);
    }

    HWND DungeonMake(const wchar_t* cls, const wchar_t* text, DWORD style,
                     int x, int y, int w, int h, int id) {
        HWND control = Make(cls, text, style, x, y, w, h, id);
        if (control) {
            SendMessageW(control, WM_SETFONT,
                         reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
            dungeonControls_.push_back(control);
            ShowWindow(control, SW_HIDE);
        }
        return control;
    }

    static std::wstring DungeonPresetSection(const cleanroute_dungeon::Preset& preset) {
        return L"DungeonPreset_" + preset.id;
    }

    static std::wstring DungeonTodayKey() {
        SYSTEMTIME st{};
        GetLocalTime(&st);
        wchar_t text[16]{};
        wsprintfW(text, L"%04u%02u%02u", static_cast<unsigned>(st.wYear),
                  static_cast<unsigned>(st.wMonth), static_cast<unsigned>(st.wDay));
        return text;
    }

    static int ClampDungeonSetting(int value, int low, int high, int fallback) {
        if (value < low || value > high) return fallback;
        return value;
    }

    void LoadDungeonPresetOverrides() {
        for (cleanroute_dungeon::Preset& preset : dungeonPresets_) {
            const std::wstring section = DungeonPresetSection(preset);
            if (ReadIniInt(section, L"Override", 0) == 0) continue;

            cleanroute_dungeon::Preset candidate = preset;
            candidate.gatherMap = ReadIniInt(section, L"GatherMap", candidate.gatherMap);
            candidate.npcResID = ReadIniInt(section, L"NpcResID", candidate.npcResID);
            candidate.gatherX = ReadIniInt(section, L"GatherX", candidate.gatherX);
            candidate.gatherY = ReadIniInt(section, L"GatherY", candidate.gatherY);
            candidate.dungeonMap = ReadIniInt(section, L"DungeonMap", candidate.dungeonMap);
            candidate.minPlayers = ClampDungeonSetting(ReadIniInt(section, L"MinPlayers", candidate.minPlayers), 1, 6, candidate.minPlayers);
            const std::wstring dialog = ReadIniText(section, L"DialogText");
            if (!dialog.empty()) candidate.dialogText = dialog;

            const int count = std::clamp(ReadIniInt(section, L"StepCount", 0), 0, 256);
            std::vector<cleanroute_dungeon::Step> loaded;
            loaded.reserve(static_cast<std::size_t>(count));
            bool invalid = false;
            for (int i = 0; i < count; ++i) {
                const std::wstring prefix = L"S" + std::to_wstring(i) + L"_";
                cleanroute_dungeon::Step step{};
                const int kind = ReadIniInt(section, prefix + L"Kind", 0);
                if (kind < static_cast<int>(cleanroute_dungeon::StepKind::Move) ||
                    kind > static_cast<int>(cleanroute_dungeon::StepKind::Portal)) {
                    invalid = true;
                    break;
                }
                step.kind = static_cast<cleanroute_dungeon::StepKind>(kind);
                step.label = ReadIniText(section, prefix + L"Label");
                step.mapID = ReadIniInt(section, prefix + L"Map", candidate.dungeonMap);
                step.x = ReadIniInt(section, prefix + L"X", 0);
                step.y = ReadIniInt(section, prefix + L"Y", 0);
                step.tolerance = ClampDungeonSetting(ReadIniInt(section, prefix + L"Tolerance", 120), 1, 10000, 120);
                step.requiredKills = ClampDungeonSetting(ReadIniInt(section, prefix + L"Kills", 0), 0, 100000, 0);
                step.radius = ClampDungeonSetting(ReadIniInt(section, prefix + L"Radius", 800), 0, 100000, 800);
                step.delayMs = ClampDungeonSetting(ReadIniInt(section, prefix + L"Delay", 0), 0, 3600000, 0);
                step.timeoutSec = ClampDungeonSetting(ReadIniInt(section, prefix + L"Timeout", 180), 1, 86400, 180);
                step.monsterName = ReadIniText(section, prefix + L"Monster");
                step.monsterResID = ReadIniInt(section, prefix + L"ResID", 0);
                step.group = ReadIniText(section, prefix + L"Group");
                if (step.group.empty()) step.group = L"THUONG";
                step.boss = ReadIniInt(section, prefix + L"Boss", 0) != 0;
                step.matchAnyVerified = ReadIniInt(section, prefix + L"MatchAny", 0) != 0;
                step.participantMask = static_cast<std::uint32_t>(
                    ReadIniInt(section, prefix + L"Mask", static_cast<int>(cleanroute_dungeon::kAllParticipantsMask)));
                std::wstring error;
                if (!cleanroute_dungeon::ValidateStep(step, error)) {
                    invalid = true;
                    break;
                }
                loaded.push_back(std::move(step));
            }
            if (!invalid && !loaded.empty()) candidate.steps = std::move(loaded);
            if (!invalid && candidate.dungeonMap > 0 && candidate.gatherMap > 0 && candidate.npcResID > 0) {
                preset = std::move(candidate);
            } else {
                Log(L"AUTO PHÓ BẢN • bỏ override lỗi của " + preset.name + L" • dùng canonical");
            }
        }
    }

    void SaveDungeonPresetOverride(int index) {
        if (index < 0 || index >= static_cast<int>(dungeonPresets_.size())) return;
        EnsureUnicodeIni();
        const cleanroute_dungeon::Preset& preset = dungeonPresets_[static_cast<std::size_t>(index)];
        const std::wstring section = DungeonPresetSection(preset);
        WriteIniInt(section, L"Override", 1);
        WriteIniInt(section, L"GatherMap", preset.gatherMap);
        WriteIniInt(section, L"NpcResID", preset.npcResID);
        WriteIniInt(section, L"GatherX", preset.gatherX);
        WriteIniInt(section, L"GatherY", preset.gatherY);
        WriteIniInt(section, L"DungeonMap", preset.dungeonMap);
        WriteIniInt(section, L"MinPlayers", preset.minPlayers);
        WriteIniText(section, L"DialogText", preset.dialogText);
        WriteIniInt(section, L"StepCount", static_cast<int>(preset.steps.size()));
        for (std::size_t i = 0; i < preset.steps.size(); ++i) {
            const cleanroute_dungeon::Step& step = preset.steps[i];
            const std::wstring prefix = L"S" + std::to_wstring(i) + L"_";
            WriteIniInt(section, prefix + L"Kind", static_cast<int>(step.kind));
            WriteIniText(section, prefix + L"Label", step.label);
            WriteIniInt(section, prefix + L"Map", step.mapID);
            WriteIniInt(section, prefix + L"X", step.x);
            WriteIniInt(section, prefix + L"Y", step.y);
            WriteIniInt(section, prefix + L"Tolerance", step.tolerance);
            WriteIniInt(section, prefix + L"Kills", step.requiredKills);
            WriteIniInt(section, prefix + L"Radius", step.radius);
            WriteIniInt(section, prefix + L"Delay", step.delayMs);
            WriteIniInt(section, prefix + L"Timeout", step.timeoutSec);
            WriteIniText(section, prefix + L"Monster", step.monsterName);
            WriteIniInt(section, prefix + L"ResID", step.monsterResID);
            WriteIniText(section, prefix + L"Group", step.group);
            WriteIniInt(section, prefix + L"Boss", step.boss ? 1 : 0);
            WriteIniInt(section, prefix + L"MatchAny", step.matchAnyVerified ? 1 : 0);
            WriteIniInt(section, prefix + L"Mask", static_cast<int>(step.participantMask));
        }
        FlushIni();
    }

    void ResetDungeonPresetOverride(int index) {
        if (index < 0 || index >= static_cast<int>(dungeonPresets_.size())) return;
        const auto canonical = cleanroute_dungeon::CanonicalPresets();
        if (index >= static_cast<int>(canonical.size())) return;
        const std::wstring section = DungeonPresetSection(dungeonPresets_[static_cast<std::size_t>(index)]);
        WritePrivateProfileStringW(section.c_str(), nullptr, nullptr, ConfigPath().c_str());
        dungeonPresets_[static_cast<std::size_t>(index)] = canonical[static_cast<std::size_t>(index)];
        FlushIni();
    }

    std::wstring DungeonDailySection(std::int32_t roleID, const std::wstring& presetId) const {
        return L"DungeonDaily_" + std::to_wstring(roleID) + L"_" + presetId;
    }

    int DungeonDailyCount(std::int32_t roleID, const std::wstring& presetId, bool normalizeDate = true) {
        if (roleID <= 0 || presetId.empty()) return 0;
        const std::wstring section = DungeonDailySection(roleID, presetId);
        const std::wstring today = DungeonTodayKey();
        if (ReadIniText(section, L"Date") != today) {
            if (normalizeDate) {
                WriteIniText(section, L"Date", today);
                WriteIniInt(section, L"Count", 0);
                FlushIni();
            }
            return 0;
        }
        return std::clamp(ReadIniInt(section, L"Count", 0), 0, 99);
    }

    void RecordDungeonDailyRun(const DungeonTeamRuntime& team, const cleanroute_dungeon::Preset& preset) {
        for (std::int32_t roleID : team.memberRoleIDs) {
            if (roleID <= 0) continue;
            const std::wstring section = DungeonDailySection(roleID, preset.id);
            const int next = DungeonDailyCount(roleID, preset.id) + 1;
            WriteIniText(section, L"Date", DungeonTodayKey());
            WriteIniInt(section, L"Count", next);
        }
        FlushIni();
    }

    int DungeonSellThresholdForRole(std::int32_t roleID) {
        if (roleID <= 0) return 40;
        return ClampDungeonSetting(ReadIniInt(L"DungeonAccount_" + std::to_wstring(roleID),
                                               L"SellThreshold", 40), 0, 200, 40);
    }

    void SaveDungeonSellThresholdForRole(std::int32_t roleID, int threshold) {
        if (roleID <= 0) return;
        WriteIniInt(L"DungeonAccount_" + std::to_wstring(roleID), L"SellThreshold",
                    std::clamp(threshold, 0, 200));
        FlushIni();
    }

    int FindDungeonPresetById(const std::wstring& id) const {
        for (std::size_t i = 0; i < dungeonPresets_.size(); ++i) {
            if (dungeonPresets_[i].id == id) return static_cast<int>(i);
        }
        return -1;
    }

    void SaveDungeonTeams() {
        EnsureUnicodeIni();
        const int oldCount = std::clamp(ReadIniInt(L"DungeonTeams", L"Count", 0), 0, 64);
        const int clearCount = std::max(oldCount, static_cast<int>(dungeonTeams_.size()));
        for (int i = 0; i < clearCount; ++i) {
            const std::wstring section = L"DungeonTeam_" + std::to_wstring(i);
            WritePrivateProfileStringW(section.c_str(), nullptr, nullptr, ConfigPath().c_str());
        }
        WriteIniInt(L"DungeonTeams", L"Count", static_cast<int>(dungeonTeams_.size()));
        WriteIniInt(L"DungeonTeams", L"NextId", dungeonNextTeamId_);
        for (std::size_t i = 0; i < dungeonTeams_.size(); ++i) {
            DungeonTeamRuntime& team = dungeonTeams_[i];
            const std::wstring section = L"DungeonTeam_" + std::to_wstring(i);
            if (team.memberRoleIDs.empty()) {
                for (std::uint32_t pid : team.config.pids) {
                    Account* account = AccountByPid(pid);
                    if (account && account->snapshotValid && (account->snapshot.validMask & ValidIdentity))
                        team.memberRoleIDs.push_back(account->snapshot.roleID);
                }
            }
            if (team.leaderRoleID <= 0) {
                Account* leader = AccountByPid(team.config.leaderPid);
                if (leader && leader->snapshotValid && (leader->snapshot.validMask & ValidIdentity))
                    team.leaderRoleID = leader->snapshot.roleID;
            }
            WriteIniInt(section, L"Id", team.config.id);
            WriteIniInt(section, L"Runs", team.config.runs);
            const int presetIndex = std::clamp(team.config.presetIndex, 0,
                                               std::max(0, static_cast<int>(dungeonPresets_.size()) - 1));
            if (!dungeonPresets_.empty()) WriteIniText(section, L"PresetId", dungeonPresets_[static_cast<std::size_t>(presetIndex)].id);
            WriteIniInt(section, L"LeaderRoleID", team.leaderRoleID);
            WriteIniInt(section, L"MemberCount", static_cast<int>(team.memberRoleIDs.size()));
            for (std::size_t n = 0; n < team.memberRoleIDs.size() && n < cleanroute_dungeon::kMaxTeamMembers; ++n)
                WriteIniInt(section, L"MemberRole_" + std::to_wstring(n), team.memberRoleIDs[n]);
        }
        FlushIni();
    }

    void LoadDungeonTeams() {
        dungeonTeams_.clear();
        dungeonNextTeamId_ = std::max(1, ReadIniInt(L"DungeonTeams", L"NextId", 1));
        const int count = std::clamp(ReadIniInt(L"DungeonTeams", L"Count", 0), 0, 32);
        for (int i = 0; i < count; ++i) {
            const std::wstring section = L"DungeonTeam_" + std::to_wstring(i);
            DungeonTeamRuntime team{};
            team.config.id = std::max(1, ReadIniInt(section, L"Id", i + 1));
            team.config.runs = std::clamp(ReadIniInt(section, L"Runs", 1), 1, 3);
            const std::wstring presetId = ReadIniText(section, L"PresetId");
            team.config.presetIndex = FindDungeonPresetById(presetId);
            if (team.config.presetIndex < 0) team.config.presetIndex = 0;
            team.leaderRoleID = ReadIniInt(section, L"LeaderRoleID", 0);
            const int members = std::clamp(ReadIniInt(section, L"MemberCount", 0), 0,
                                           static_cast<int>(cleanroute_dungeon::kMaxTeamMembers));
            for (int n = 0; n < members; ++n) {
                const int roleID = ReadIniInt(section, L"MemberRole_" + std::to_wstring(n), 0);
                if (roleID > 0) team.memberRoleIDs.push_back(roleID);
            }
            if (team.memberRoleIDs.empty()) continue;
            team.state = cleanroute_dungeon::TeamState::Stopped;
            team.status = L"Đã nạp cấu hình • chờ quét/bind client";
            dungeonNextTeamId_ = std::max(dungeonNextTeamId_, team.config.id + 1);
            dungeonTeams_.push_back(std::move(team));
        }
    }

    void ResolveDungeonTeamBindings() {
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

    void BuildDungeonUi() {
        dungeonPresets_ = cleanroute_dungeon::CanonicalPresets();
        LoadDungeonPresetOverrides();
        LoadDungeonTeams();

        DungeonMake(L"STATIC", L"AUTO PHÓ BẢN v4.3 • mỗi tổ đội 1–6 acc • nhiều tổ đội chạy độc lập",
                    SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 18, 45, 1005, 30, 0);
        dungeonAccountList_ = DungeonMake(WC_LISTVIEWW, L"",
            LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER, 18, 82, 1005, 145, IDC_DG_ACCOUNT_LIST);
        ListView_SetExtendedListViewStyle(dungeonAccountList_,
            LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);
        DungeonListColumn(dungeonAccountList_, 0, 245, L"Nhân vật / RoleID");
        DungeonListColumn(dungeonAccountList_, 1, 80, L"PID");
        DungeonListColumn(dungeonAccountList_, 2, 140, L"Map / X,Y");
        DungeonListColumn(dungeonAccountList_, 3, 160, L"Workflow");
        DungeonListColumn(dungeonAccountList_, 4, 360, L"Ghi chú");

        DungeonMake(L"BUTTON", L"LÀM MỚI", BS_PUSHBUTTON, 18, 234, 105, 28, IDC_DG_REFRESH);
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
        DungeonMake(L"BUTTON", L"TẠO TỔ ĐỘI", BS_DEFPUSHBUTTON, 918, 234, 105, 28, IDC_DG_CREATE_TEAM);

        dungeonTeamList_ = DungeonMake(WC_LISTVIEWW, L"",
            LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER,
            18, 270, 1005, 160, IDC_DG_TEAM_LIST);
        ListView_SetExtendedListViewStyle(dungeonTeamList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        DungeonListColumn(dungeonTeamList_, 0, 120, L"Tổ đội / vai trò");
        DungeonListColumn(dungeonTeamList_, 1, 250, L"Nhân vật / RoleID");
        DungeonListColumn(dungeonTeamList_, 2, 75, L"PID");
        DungeonListColumn(dungeonTeamList_, 3, 155, L"Phó bản");
        DungeonListColumn(dungeonTeamList_, 4, 60, L"Lượt");
        DungeonListColumn(dungeonTeamList_, 5, 80, L"State");
        DungeonListColumn(dungeonTeamList_, 6, 265, L"Tiến độ");

        DungeonMake(L"BUTTON", L"BẮT ĐẦU", BS_DEFPUSHBUTTON, 18, 437, 110, 30, IDC_DG_START);
        DungeonMake(L"BUTTON", L"TẠM DỪNG / TIẾP", BS_PUSHBUTTON, 136, 437, 150, 30, IDC_DG_PAUSE);
        DungeonMake(L"BUTTON", L"STOP", BS_PUSHBUTTON, 294, 437, 90, 30, IDC_DG_STOP);
        DungeonMake(L"BUTTON", L"XÓA BẢNG", BS_PUSHBUTTON, 392, 437, 105, 30, IDC_DG_DELETE_TEAM);
        DungeonMake(L"BUTTON", L"CẤU HÌNH PHÓ BẢN", BS_PUSHBUTTON, 505, 437, 170, 30, IDC_DG_CONFIG);
        dungeonStatus_ = DungeonMake(L"STATIC", L"AUTO PHÓ BẢN • READY",
                                     SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 683, 437, 340, 30, IDC_DG_STATUS);

        DungeonMake(L"STATIC",
            L"KẾ HOẠCH / STEP — MOVE tự Stop Auto → Travel Guard → barrier; FIGHT verify AutoFight ON rồi mới đếm.",
            SS_LEFT | SS_CENTERIMAGE, 18, 476, 1005, 26, 0);
        dungeonStepList_ = DungeonMake(WC_LISTVIEWW, L"",
            LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER,
            18, 506, 1005, 330, IDC_DG_STEP_LIST);
        ListView_SetExtendedListViewStyle(dungeonStepList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        DungeonListColumn(dungeonStepList_, 0, 42, L"#");
        DungeonListColumn(dungeonStepList_, 1, 140, L"Thao tác");
        DungeonListColumn(dungeonStepList_, 2, 225, L"Mục tiêu");
        DungeonListColumn(dungeonStepList_, 3, 65, L"Map");
        DungeonListColumn(dungeonStepList_, 4, 105, L"X,Y");
        DungeonListColumn(dungeonStepList_, 5, 70, L"Kill");
        DungeonListColumn(dungeonStepList_, 6, 90, L"Radius/Tol");
        DungeonListColumn(dungeonStepList_, 7, 85, L"Timeout");
        DungeonListColumn(dungeonStepList_, 8, 150, L"Nhóm ACC");

        DungeonMake(L"BUTTON", L"TEST TASK + QUÁI", BS_PUSHBUTTON,
                    18, 844, 205, 30, IDC_DG_SCAN_MONSTER);
        dungeonMonsterStatus_ = DungeonMake(L"STATIC",
            L"Tiến độ: TASK server (GetDoingTasks/Parameters) + strict GMonster • không OCR/scan người",
            SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 232, 844, 791, 30, IDC_DG_MONSTER_STATUS);
        DungeonMake(L"STATIC",
            L"An toàn: 1 PID không thuộc 2 đội RUN/PAUSE. Timeout chỉ FAIL. Preset đang RUN được đóng băng; sửa preset không làm đổi workflow đang chạy.",
            SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 18, 884, 1005, 38, 0);

        RefreshDungeonAccountList();
        RefreshDungeonPresetCombo();
        RefreshDungeonTeamList();
        RefreshDungeonStepList();
    }

    void RefreshDungeonAccountList() {
        if (!dungeonAccountList_) return;
        ResolveDungeonTeamBindings();
        ListView_DeleteAllItems(dungeonAccountList_);
        SendMessageW(dungeonLeaderCombo_, CB_RESETCONTENT, 0, 0);
        for (std::size_t i = 0; i < accounts_.size(); ++i) {
            Account& account = *accounts_[i];
            std::wstring display = account.displayName;
            if (account.snapshotValid && (account.snapshot.validMask & ValidIdentity))
                display += L" / " + std::to_wstring(account.snapshot.roleID);
            LVITEMW item{};
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = static_cast<int>(i);
            item.pszText = display.data();
            item.lParam = static_cast<LPARAM>(account.game.pid);
            ListView_InsertItem(dungeonAccountList_, &item);

            std::wstring pid = std::to_wstring(account.game.pid);
            std::wstring pos = account.snapshotValid
                ? L"M" + std::to_wstring(account.snapshot.mapID) + L" • " +
                  std::to_wstring(account.snapshot.x) + L"," + std::to_wstring(account.snapshot.y)
                : L"NO STATE";
            ListView_SetItemText(dungeonAccountList_, static_cast<int>(i), 1, pid.data());
            ListView_SetItemText(dungeonAccountList_, static_cast<int>(i), 2, pos.data());
            std::wstring workflow = account.dungeonOwned ? L"PHÓ BẢN" :
                                    account.pk.active ? L"AUTO PK" :
                                    account.runtime.running ? L"AUTO" : L"IDLE";
            ListView_SetItemText(dungeonAccountList_, static_cast<int>(i), 3, workflow.data());
            const int threshold = (account.snapshot.validMask & ValidIdentity)
                ? DungeonSellThresholdForRole(account.snapshot.roleID) : 40;
            std::wstring note = account.dungeonOwned ? L"Đang bị một tổ đội giữ quyền action" : L"Có thể chọn vào tổ đội";
            note += L" • Auto Sell=" + std::wstring(account.profile.enableSell ? L"ON" : L"OFF") +
                    L" • ngưỡng PB <" + std::to_wstring(threshold);
            ListView_SetItemText(dungeonAccountList_, static_cast<int>(i), 4, note.data());

            LRESULT row = SendMessageW(dungeonLeaderCombo_, CB_ADDSTRING, 0,
                                       reinterpret_cast<LPARAM>(display.c_str()));
            SendMessageW(dungeonLeaderCombo_, CB_SETITEMDATA, row, static_cast<LPARAM>(account.game.pid));
        }
        if (!accounts_.empty() && SendMessageW(dungeonLeaderCombo_, CB_GETCURSEL, 0, 0) == CB_ERR)
            SendMessageW(dungeonLeaderCombo_, CB_SETCURSEL, 0, 0);
        RefreshDungeonTeamList();
    }

    void RefreshDungeonSellThresholdEditor() {
        if (!dungeonSellThresholdEdit_ || !dungeonAccountList_) return;
        const int row = ListView_GetNextItem(dungeonAccountList_, -1, LVNI_SELECTED);
        if (row < 0) return;
        LVITEMW item{};
        item.mask = LVIF_PARAM;
        item.iItem = row;
        if (!ListView_GetItem(dungeonAccountList_, &item)) return;
        Account* account = AccountByPid(static_cast<DWORD>(item.lParam));
        if (!account || !account->snapshotValid || (account->snapshot.validMask & ValidIdentity) == 0) return;
        SetText(dungeonSellThresholdEdit_, std::to_wstring(DungeonSellThresholdForRole(account->snapshot.roleID)));
    }

    void SaveDungeonSellThresholdSelected() {
        const int row = dungeonAccountList_ ? ListView_GetNextItem(dungeonAccountList_, -1, LVNI_SELECTED) : -1;
        if (row < 0) {
            if (dungeonStatus_) SetText(dungeonStatus_, L"Chọn một acc trước khi lưu ngưỡng bán");
            return;
        }
        LVITEMW item{};
        item.mask = LVIF_PARAM;
        item.iItem = row;
        if (!ListView_GetItem(dungeonAccountList_, &item)) return;
        Account* account = AccountByPid(static_cast<DWORD>(item.lParam));
        if (!account || !account->snapshotValid || (account->snapshot.validMask & ValidIdentity) == 0) {
            if (dungeonStatus_) SetText(dungeonStatus_, L"Acc chưa có RoleID authoritative");
            return;
        }
        const int threshold = ParseEditInt(dungeonSellThresholdEdit_, 40, 0, 200);
        SaveDungeonSellThresholdForRole(account->snapshot.roleID, threshold);
        RefreshDungeonAccountList();
        if (dungeonStatus_) SetText(dungeonStatus_, L"Đã lưu ngưỡng bán PB cho " + AccountTag(*account));
    }

    void RefreshDungeonPresetCombo() {
        if (!dungeonPresetCombo_) return;
        int keep = static_cast<int>(SendMessageW(dungeonPresetCombo_, CB_GETCURSEL, 0, 0));
        SendMessageW(dungeonPresetCombo_, CB_RESETCONTENT, 0, 0);
        for (const auto& preset : dungeonPresets_)
            SendMessageW(dungeonPresetCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(preset.name.c_str()));
        if (!dungeonPresets_.empty()) {
            const int selected = std::clamp(keep < 0 ? 0 : keep, 0, static_cast<int>(dungeonPresets_.size()) - 1);
            SendMessageW(dungeonPresetCombo_, CB_SETCURSEL, selected, 0);
        }
    }

    int SelectedDungeonTeamIndex() const {
        if (!dungeonTeamList_) return -1;
        const int row = ListView_GetNextItem(dungeonTeamList_, -1, LVNI_SELECTED);
        if (row < 0) return -1;
        LVITEMW item{};
        item.mask = LVIF_PARAM;
        item.iItem = row;
        if (!ListView_GetItem(dungeonTeamList_, &item) || item.lParam <= 0) return -1;
        return static_cast<int>((static_cast<std::uintptr_t>(item.lParam) >> 1) - 1u);
    }

    cleanroute_dungeon::Preset* DungeonConfiguredPresetForTeam(DungeonTeamRuntime& team) {
        if (team.config.presetIndex < 0 || team.config.presetIndex >= static_cast<int>(dungeonPresets_.size())) return nullptr;
        return &dungeonPresets_[static_cast<std::size_t>(team.config.presetIndex)];
    }

    const cleanroute_dungeon::Preset* DungeonConfiguredPresetForTeam(const DungeonTeamRuntime& team) const {
        if (team.config.presetIndex < 0 || team.config.presetIndex >= static_cast<int>(dungeonPresets_.size())) return nullptr;
        return &dungeonPresets_[static_cast<std::size_t>(team.config.presetIndex)];
    }

    cleanroute_dungeon::Preset* DungeonPresetForTeam(DungeonTeamRuntime& team) {
        if (team.activePresetValid) return &team.activePreset;
        return DungeonConfiguredPresetForTeam(team);
    }

    std::wstring DungeonTeamMembers(const DungeonTeamRuntime& team) const {
        std::wstring out;
        if (!team.memberRoleIDs.empty()) {
            for (std::size_t i = 0; i < team.memberRoleIDs.size(); ++i) {
                if (!out.empty()) out += L", ";
                Account* found = nullptr;
                for (const auto& item : accounts_) {
                    if (item->snapshotValid && (item->snapshot.validMask & ValidIdentity) &&
                        item->snapshot.roleID == team.memberRoleIDs[i]) {
                        found = item.get();
                        break;
                    }
                }
                out += found ? AccountTag(*found) : L"Role " + std::to_wstring(team.memberRoleIDs[i]);
            }
            return out;
        }
        for (std::uint32_t pid : team.config.pids) {
            if (!out.empty()) out += L", ";
            Account* account = const_cast<App*>(this)->AccountByPid(pid);
            out += account ? AccountTag(*account) : L"PID " + std::to_wstring(pid);
        }
        return out;
    }

    void RefreshDungeonTeamList(int select = -1) {
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

    int DungeonDisplayPresetIndex() const {
        const int teamIndex = SelectedDungeonTeamIndex();
        if (teamIndex >= 0 && teamIndex < static_cast<int>(dungeonTeams_.size()))
            return dungeonTeams_[static_cast<std::size_t>(teamIndex)].config.presetIndex;
        const int selected = dungeonPresetCombo_
            ? static_cast<int>(SendMessageW(dungeonPresetCombo_, CB_GETCURSEL, 0, 0)) : 0;
        return std::clamp(selected, 0, std::max(0, static_cast<int>(dungeonPresets_.size()) - 1));
    }

    static std::wstring DungeonMaskText(std::uint32_t mask) {
        mask &= cleanroute_dungeon::kAllParticipantsMask;
        if (mask == 0 || mask == cleanroute_dungeon::kAllParticipantsMask) return L"ALL";
        std::wstring text;
        for (std::size_t i = 0; i < cleanroute_dungeon::kMaxTeamMembers; ++i) {
            if ((mask & (1u << static_cast<unsigned>(i))) == 0) continue;
            if (!text.empty()) text += L",";
            text += std::to_wstring(i + 1);
        }
        return text.empty() ? L"ALL" : text;
    }

    void RefreshDungeonStepList() {
        if (!dungeonStepList_ || dungeonPresets_.empty()) return;
        ListView_DeleteAllItems(dungeonStepList_);
        const int presetIndex = DungeonDisplayPresetIndex();
        if (presetIndex < 0 || presetIndex >= static_cast<int>(dungeonPresets_.size())) return;
        const cleanroute_dungeon::Preset& preset = dungeonPresets_[static_cast<std::size_t>(presetIndex)];
        for (std::size_t i = 0; i < preset.steps.size(); ++i) {
            const cleanroute_dungeon::Step& step = preset.steps[i];
            std::wstring index = std::to_wstring(i + 1);
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = static_cast<int>(i);
            item.pszText = index.data();
            ListView_InsertItem(dungeonStepList_, &item);
            ListView_SetItemText(dungeonStepList_, static_cast<int>(i), 1,
                                 const_cast<wchar_t*>(cleanroute_dungeon::StepKindLabel(step.kind)));
            ListView_SetItemText(dungeonStepList_, static_cast<int>(i), 2,
                                 const_cast<wchar_t*>(step.label.c_str()));
            std::wstring map = step.mapID ? std::to_wstring(step.mapID) : L"—";
            std::wstring pos = (step.x || step.y) ? std::to_wstring(step.x) + L"," + std::to_wstring(step.y) : L"—";
            std::wstring kills = step.requiredKills ? std::to_wstring(step.requiredKills) : L"—";
            std::wstring radius = std::to_wstring(step.kind == cleanroute_dungeon::StepKind::Fight ? step.radius : step.tolerance);
            std::wstring timeout = std::to_wstring(step.timeoutSec) + L"s";
            std::wstring mask = DungeonMaskText(step.participantMask);
            ListView_SetItemText(dungeonStepList_, static_cast<int>(i), 3, map.data());
            ListView_SetItemText(dungeonStepList_, static_cast<int>(i), 4, pos.data());
            ListView_SetItemText(dungeonStepList_, static_cast<int>(i), 5, kills.data());
            ListView_SetItemText(dungeonStepList_, static_cast<int>(i), 6, radius.data());
            ListView_SetItemText(dungeonStepList_, static_cast<int>(i), 7, timeout.data());
            ListView_SetItemText(dungeonStepList_, static_cast<int>(i), 8, mask.data());
        }
    }

    bool AnyDungeonActive() const {
        for (const auto& team : dungeonTeams_) if (cleanroute_dungeon::ActiveState(team.state)) return true;
        return false;
    }

    void CreateDungeonTeam() {
        cleanroute_dungeon::TeamConfig config{};
        config.id = dungeonNextTeamId_++;
        config.runs = ParseEditInt(dungeonRunsEdit_, 1, 1, 3);
        config.presetIndex = dungeonPresetCombo_
            ? static_cast<int>(SendMessageW(dungeonPresetCombo_, CB_GETCURSEL, 0, 0)) : 0;
        const int rows = ListView_GetItemCount(dungeonAccountList_);
        std::vector<std::int32_t> roles;
        for (int i = 0; i < rows; ++i) {
            if (!ListView_GetCheckState(dungeonAccountList_, i)) continue;
            LVITEMW item{};
            item.mask = LVIF_PARAM;
            item.iItem = i;
            if (!ListView_GetItem(dungeonAccountList_, &item)) continue;
            const DWORD pid = static_cast<DWORD>(item.lParam);
            Account* account = AccountByPid(pid);
            if (!account || !account->snapshotValid || (account->snapshot.validMask & ValidIdentity) == 0) {
                MessageBoxW(hwnd_, L"Có acc chưa đọc được RoleID authoritative.", L"Auto phó bản", MB_OK | MB_ICONWARNING);
                return;
            }
            config.pids.push_back(pid);
            roles.push_back(account->snapshot.roleID);
        }
        if (config.pids.size() > cleanroute_dungeon::kMaxTeamMembers) {
            MessageBoxW(hwnd_, L"Mỗi bảng tổ đội tối đa 6 acc.", L"Auto phó bản", MB_OK | MB_ICONWARNING);
            return;
        }
        LRESULT leaderIndex = SendMessageW(dungeonLeaderCombo_, CB_GETCURSEL, 0, 0);
        if (leaderIndex != CB_ERR)
            config.leaderPid = static_cast<DWORD>(SendMessageW(dungeonLeaderCombo_, CB_GETITEMDATA, leaderIndex, 0));
        std::wstring error;
        if (!cleanroute_dungeon::ValidateTeam(config, error)) {
            MessageBoxW(hwnd_, error.c_str(), L"Auto phó bản", MB_OK | MB_ICONWARNING);
            return;
        }
        Account* leader = AccountByPid(config.leaderPid);
        if (!leader || !leader->snapshotValid || (leader->snapshot.validMask & ValidIdentity) == 0) {
            MessageBoxW(hwnd_, L"Đội trưởng chưa có RoleID authoritative.", L"Auto phó bản", MB_OK | MB_ICONWARNING);
            return;
        }

        DungeonTeamRuntime team{};
        team.config = config;
        team.memberRoleIDs = std::move(roles);
        team.leaderRoleID = leader->snapshot.roleID;
        team.status = L"Đã tạo • STOP";
        dungeonTeams_.push_back(std::move(team));
        SaveDungeonTeams();
        RefreshDungeonTeamList(static_cast<int>(dungeonTeams_.size()) - 1);
        RefreshDungeonStepList();
        Log(L"AUTO PHÓ BẢN • tạo ĐỘI " + std::to_wstring(config.id) + L" • " +
            std::to_wstring(config.pids.size()) + L" acc • leader RoleID " +
            std::to_wstring(leader->snapshot.roleID));
    }

    void DungeonSafeStopMember(Account& account) {
        Response response{};
        std::wstring error;
        (void)account.bridge.Call(Command::StopPath, 0, 0, 0, response, error, 700);
        (void)account.bridge.Call(Command::StopAutoFight, 0, 0, 0, response, error, 900);
        ResetRobustTravel(account.runtime);
        ResetTravelFightGuard(account.runtime);
    }

    void ReleaseDungeonTeamMembers(DungeonTeamRuntime& team) {
        for (std::uint32_t pid : team.config.pids) {
            if (Account* account = AccountByPid(pid)) {
                DungeonSafeStopMember(*account);
                if (account->runtime.sellPhase != 0) {
                    Response closeResponse{};
                    std::wstring closeError;
                    (void)account->bridge.Call(Command::CloseBackgroundSell, 0, 0, 0,
                                               closeResponse, closeError, 900);
                    account->runtime.sellPhase = 0;
                }
                account->dungeonOwned = false;
                account->runtime.status = L"Đã dừng phó bản";
            }
        }
        team.activePresetValid = false;
        team.postSellDone.clear();
    }

    void FailDungeonTeam(DungeonTeamRuntime& team, const std::wstring& reason) {
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

    bool ValidateDungeonStart(int index, std::wstring& error) {
        if (index < 0 || index >= static_cast<int>(dungeonTeams_.size())) {
            error = L"Chưa chọn bảng tổ đội";
            return false;
        }
        ResolveDungeonTeamBindings();
        DungeonTeamRuntime& team = dungeonTeams_[static_cast<std::size_t>(index)];
        if (!cleanroute_dungeon::ValidateTeam(team.config, error)) return false;
        cleanroute_dungeon::Preset* preset = DungeonConfiguredPresetForTeam(team);
        if (!preset) {
            error = L"Preset phó bản không hợp lệ";
            return false;
        }
        if (team.config.pids.size() < static_cast<std::size_t>(std::max(1, preset->minPlayers))) {
            error = L"Không đủ số acc tối thiểu của preset";
            return false;
        }
        if (preset->steps.empty()) {
            error = L"Preset không có STEP";
            return false;
        }
        for (const auto& step : preset->steps) {
            if (!cleanroute_dungeon::ValidateStep(step, error)) return false;
        }
        for (std::size_t j = 0; j < dungeonTeams_.size(); ++j) {
            if (static_cast<int>(j) == index) continue;
            if (cleanroute_dungeon::TeamsOverlap(team.config, cleanroute_dungeon::TeamState::Running,
                                                 dungeonTeams_[j].config, dungeonTeams_[j].state)) {
                error = L"Có acc đang thuộc tổ đội RUN/PAUSE khác";
                return false;
            }
        }
        if (team.memberRoleIDs.size() != team.config.pids.size()) {
            error = L"Chưa bind đủ RoleID → PID của tổ đội";
            return false;
        }
        for (std::size_t n = 0; n < team.config.pids.size(); ++n) {
            const std::uint32_t pid = team.config.pids[n];
            Account* account = AccountByPid(pid);
            if (!account || !account->snapshotValid) {
                error = L"Mất client/state PID " + std::to_wstring(pid);
                return false;
            }
            if (account->tradeHeld || tradeTxn_.childPid == pid || tradeTxn_.mainPid == pid) {
                error = L"PID " + std::to_wstring(pid) + L" đang thuộc workflow giao dịch";
                return false;
            }
            if (account->pk.active) {
                error = L"PID " + std::to_wstring(pid) + L" đang chạy AUTO PK";
                return false;
            }
            const int used = DungeonDailyCount(team.memberRoleIDs[n], preset->id);
            if (used + team.config.runs > 3) {
                error = L"RoleID " + std::to_wstring(team.memberRoleIDs[n]) + L" đã dùng " +
                        std::to_wstring(used) + L"/3 lượt hôm nay của " + preset->name;
                return false;
            }
        }
        return true;
    }

    void ResetDungeonStepRuntime(DungeonTeamRuntime& team, DWORD now) {
        team.fightStopPending = false;
        team.kills = 0;
        team.dueTick = 0;
        team.lastScanTick = 0;
        team.lastProgressTick = 0;
        team.lastProgressAttemptTick = 0;
        team.serverSyncWaitTick = 0;
        team.progressReadOk = false;
        team.progressSnapshot = DungeonProgressSnapshot{};
        team.boundTaskID = 0;
        team.boundTaskName.clear();
        team.boundTaskParameterKeys.clear();
        team.taskParameterBaselines.clear();
        team.taskStepProgress = 0;
        team.seenMatchingAlive = false;
        team.deathTracker.Reset(team.activePresetValid ? team.activePreset.dungeonMap : 0, team.stepIndex);
        team.loggedDiagnostics.clear();
        team.phaseTick = now;
    }

    void StartDungeonSelected() {
        const int index = SelectedDungeonTeamIndex();
        std::wstring error;
        if (!ValidateDungeonStart(index, error)) {
            if (dungeonStatus_) SetText(dungeonStatus_, L"KHÔNG START • " + error);
            return;
        }
        DungeonTeamRuntime& team = dungeonTeams_[static_cast<std::size_t>(index)];
        cleanroute_dungeon::Preset* preset = DungeonConfiguredPresetForTeam(team);
        team.activePreset = *preset;
        team.activePresetValid = true;
        for (std::uint32_t pid : team.config.pids) {
            Account& account = *AccountByPid(pid);
            account.runtime.running = false;
            account.pk = {};
            account.dungeonOwned = true;
            ResetRuntime(account.runtime);
            account.runtime.running = false;
            account.runtime.status = L"AUTO PHÓ BẢN • chuẩn bị";
        }
        team.state = cleanroute_dungeon::TeamState::Running;
        team.phase = cleanroute_dungeon::TeamPhase::Precheck;
        team.runIndex = 1;
        team.stepIndex = 0;
        team.npcAttempts = 0;
        team.dialogAttempts = 0;
        team.postSellDone.clear();
        ResetDungeonStepRuntime(team, GetTickCount());
        team.status = L"RUN • PRECHECK";
        RefreshDungeonAccountList();
        RefreshDungeonTeamList(index);
        Log(L"AUTO PHÓ BẢN START • ĐỘI " + std::to_wstring(team.config.id) + L" • preset snapshot " + preset->name);
    }

    void PauseResumeDungeonSelected() {
        const int index = SelectedDungeonTeamIndex();
        if (index < 0 || index >= static_cast<int>(dungeonTeams_.size())) return;
        DungeonTeamRuntime& team = dungeonTeams_[static_cast<std::size_t>(index)];
        if (team.state == cleanroute_dungeon::TeamState::Running) {
            for (std::uint32_t pid : team.config.pids)
                if (Account* account = AccountByPid(pid)) DungeonSafeStopMember(*account);
            team.state = cleanroute_dungeon::TeamState::Paused;
            team.status = L"PAUSE • giữ nguyên bước " + std::to_wstring(team.stepIndex + 1);
        } else if (team.state == cleanroute_dungeon::TeamState::Paused) {
            std::wstring error;
            // Resume validates live ownership/state but preserves the frozen preset snapshot.
            if (!cleanroute_dungeon::ValidateTeam(team.config, error)) {
                FailDungeonTeam(team, error);
                return;
            }
            for (std::uint32_t pid : team.config.pids) {
                Account* account = AccountByPid(pid);
                if (!account || !account->snapshotValid || account->pk.active || account->tradeHeld) {
                    FailDungeonTeam(team, L"RESUME không chứng minh được ownership/state của PID " + std::to_wstring(pid));
                    return;
                }
                account->dungeonOwned = true;
            }
            team.state = cleanroute_dungeon::TeamState::Running;
            team.phaseTick = GetTickCount();
            team.dueTick = 0;
            team.status = L"RESUME • chạy lại proof bước hiện tại";
        }
        RefreshDungeonTeamList(index);
    }

    void StopDungeonSelected(const std::wstring& why = L"người dùng STOP") {
        const int index = SelectedDungeonTeamIndex();
        if (index < 0 || index >= static_cast<int>(dungeonTeams_.size())) return;
        DungeonTeamRuntime& team = dungeonTeams_[static_cast<std::size_t>(index)];
        ReleaseDungeonTeamMembers(team);
        team.state = cleanroute_dungeon::TeamState::Stopped;
        team.status = L"STOP • " + why;
        RefreshDungeonAccountList();
        RefreshDungeonTeamList(index);
    }

    void DeleteDungeonSelected() {
        const int index = SelectedDungeonTeamIndex();
        if (index < 0 || index >= static_cast<int>(dungeonTeams_.size())) return;
        if (cleanroute_dungeon::ActiveState(dungeonTeams_[static_cast<std::size_t>(index)].state)) {
            MessageBoxW(hwnd_, L"STOP tổ đội trước khi xóa.", L"Auto phó bản", MB_OK | MB_ICONWARNING);
            return;
        }
        dungeonTeams_.erase(dungeonTeams_.begin() + index);
        SaveDungeonTeams();
        RefreshDungeonTeamList();
        RefreshDungeonStepList();
    }

    bool DungeonStable(Account& account) {
        const Snapshot& snapshot = account.snapshot;
        const unsigned need = ValidMap | ValidPosition | ValidAutoPath | ValidAutoFight;
        return account.snapshotValid && IsWindow(account.game.window) && !account.runtime.clientFreezeActive &&
               (snapshot.validMask & need) == need && snapshot.mapReady && !snapshot.waitingChangeMap;
    }

    std::vector<Account*> DungeonMembers(DungeonTeamRuntime& team) {
        std::vector<Account*> out;
        for (std::uint32_t pid : team.config.pids) if (Account* account = AccountByPid(pid)) out.push_back(account);
        return out;
    }

    std::vector<Account*> DungeonStepMembers(DungeonTeamRuntime& team, const cleanroute_dungeon::Step& step) {
        std::vector<Account*> out;
        const std::uint32_t mask = cleanroute_dungeon::NormalizeParticipantMask(step.participantMask, team.config.pids.size());
        for (std::size_t i = 0; i < team.config.pids.size(); ++i) {
            if ((mask & (1u << static_cast<unsigned>(i))) == 0) continue;
            if (Account* account = AccountByPid(team.config.pids[i])) out.push_back(account);
        }
        return out;
    }

    bool DungeonAllStable(DungeonTeamRuntime& team, std::wstring& error) {
        for (std::uint32_t pid : team.config.pids) {
            Account* account = AccountByPid(pid);
            if (!account) {
                error = L"mất PID " + std::to_wstring(pid);
                return false;
            }
            if (!DungeonStable(*account)) {
                error = L"state chưa ổn định PID " + std::to_wstring(pid);
                return false;
            }
        }
        return true;
    }

    void AdvanceDungeonStep(DungeonTeamRuntime& team, DWORD now) {
        ++team.stepIndex;
        ResetDungeonStepRuntime(team, now);
        team.status = L"STEP " + std::to_wstring(team.stepIndex + 1);
    }

    bool DungeonTimeout(DungeonTeamRuntime& team, DWORD now, int sec) {
        return team.phaseTick && Elapsed(now, team.phaseTick, static_cast<DWORD>(std::max(1, sec)) * 1000u);
    }


    const DungeonTaskRecord* DungeonTaskById(const DungeonProgressSnapshot& snapshot, int taskID) const {
        if (taskID <= 0) return nullptr;
        for (std::size_t i = 0; i < snapshot.taskCount && i < kMaxDungeonTasks; ++i) {
            const DungeonTaskRecord& task = snapshot.tasks[i];
            if ((task.validMask & DungeonTaskValidIdentity) && task.taskID == taskID) return &task;
        }
        return nullptr;
    }

    bool DungeonTaskParameterValue(const DungeonTaskRecord& task, int key, int& value) const {
        if ((task.validMask & DungeonTaskValidParameters) == 0 || key <= 0) return false;
        for (std::size_t i = 0; i < task.parameterCount && i < kMaxDungeonTaskParameters; ++i) {
            if (task.parameters[i].key == key) {
                value = task.parameters[i].value;
                return true;
            }
        }
        return false;
    }

    bool RefreshDungeonProgressSnapshot(DungeonTeamRuntime& team, Account& authority,
                                        DWORD now, std::wstring& detail) {
        detail.clear();
        if (team.lastProgressAttemptTick && !Elapsed(now, team.lastProgressAttemptTick, 700)) {
            return team.progressReadOk && team.lastProgressTick && !Elapsed(now, team.lastProgressTick, 2500);
        }
        team.lastProgressAttemptTick = now;
        Response response{};
        std::wstring callError;
        if (authority.bridge.Call(Command::ReadDungeonProgress, 0, 0, 0, response, callError, 1600)) {
            team.progressSnapshot = response.dungeonProgress;
            team.progressReadOk = (response.dungeonProgress.validMask & 1u) != 0;
            team.lastProgressTick = now;
            detail = response.detail;
            return team.progressReadOk;
        }
        detail = callError;
        if (!team.lastProgressTick || Elapsed(now, team.lastProgressTick, 2500)) team.progressReadOk = false;
        return team.progressReadOk && !Elapsed(now, team.lastProgressTick, 2500);
    }

    int DungeonPriorObjectiveTarget(const cleanroute_dungeon::Preset& preset, int stepIndex,
                                    const cleanroute_dungeon::Step& step) const {
        int prior = 0;
        if (stepIndex <= 0) return prior;
        for (int i = 0; i < stepIndex && i < static_cast<int>(preset.steps.size()); ++i) {
            const cleanroute_dungeon::Step& candidate = preset.steps[static_cast<std::size_t>(i)];
            if (candidate.kind != cleanroute_dungeon::StepKind::Fight) continue;
            const bool sameNamedObjective = !step.monsterName.empty() &&
                cleanroute_dungeon::EqualFolded(candidate.monsterName, step.monsterName) &&
                cleanroute_dungeon::EqualFolded(candidate.group, step.group);
            const bool sameExplicitRes = step.monsterResID > 0 && candidate.monsterResID == step.monsterResID;
            if (sameNamedObjective || sameExplicitRes) prior += std::max(0, candidate.requiredKills);
        }
        return prior;
    }


    std::wstring DungeonTaskLearnSection(const cleanroute_dungeon::Preset& preset, int stepIndex) const {
        return L"DungeonTaskLearn_" + preset.id + L"_S" + std::to_wstring(stepIndex);
    }

    std::set<int> ParseDungeonTaskKeys(const std::wstring& text) const {
        std::set<int> keys;
        std::size_t start = 0;
        while (start < text.size()) {
            const std::size_t comma = text.find(L',', start);
            const std::wstring token = text.substr(start, comma == std::wstring::npos ? std::wstring::npos : comma - start);
            if (!token.empty()) {
                wchar_t* end = nullptr;
                const long value = std::wcstol(token.c_str(), &end, 10);
                if (end && *end == 0 && value > 0 && value <= INT_MAX) keys.insert(static_cast<int>(value));
            }
            if (comma == std::wstring::npos) break;
            start = comma + 1;
        }
        return keys;
    }

    void RestoreDungeonTaskLearn(DungeonTeamRuntime& team, const cleanroute_dungeon::Preset& preset) {
        if (!team.progressReadOk || team.boundTaskID > 0) return;
        const std::wstring section = DungeonTaskLearnSection(preset, team.stepIndex);
        const int taskID = ReadIniInt(section, L"TaskID", 0);
        if (taskID <= 0) return;
        const DungeonTaskRecord* task = DungeonTaskById(team.progressSnapshot, taskID);
        if (!task) return;
        const std::set<int> saved = ParseDungeonTaskKeys(ReadIniText(section, L"ParamKeys"));
        for (int key : saved) {
            int ignored = 0;
            if (DungeonTaskParameterValue(*task, key, ignored)) team.boundTaskParameterKeys.insert(key);
        }
        if (team.boundTaskParameterKeys.empty()) return;
        team.boundTaskID = taskID;
        if ((task->validMask & DungeonTaskValidName) && task->name[0]) team.boundTaskName = task->name;
    }

    void SaveDungeonTaskLearn(const DungeonTeamRuntime& team, const cleanroute_dungeon::Preset& preset) {
        if (team.boundTaskID <= 0 || team.boundTaskParameterKeys.empty()) return;
        const std::wstring section = DungeonTaskLearnSection(preset, team.stepIndex);
        std::set<int> merged = ParseDungeonTaskKeys(ReadIniText(section, L"ParamKeys"));
        merged.insert(team.boundTaskParameterKeys.begin(), team.boundTaskParameterKeys.end());
        std::wstring csv;
        for (int key : merged) {
            if (!csv.empty()) csv += L',';
            csv += std::to_wstring(key);
        }
        WriteIniInt(section, L"TaskID", team.boundTaskID);
        WriteIniText(section, L"ParamKeys", csv);
        if (!team.boundTaskName.empty()) WriteIniText(section, L"TaskName", team.boundTaskName);
        FlushIni();
    }

    void BindDungeonTaskObjective(DungeonTeamRuntime& team,
                                  const std::vector<int>& observedResIDs) {
        if (!team.progressReadOk) return;

        const DungeonTaskRecord* bound = DungeonTaskById(team.progressSnapshot, team.boundTaskID);
        if (!bound) {
            int bestScore = 0;
            int bestTaskID = 0;
            bool tie = false;
            for (std::size_t i = 0; i < team.progressSnapshot.taskCount && i < kMaxDungeonTasks; ++i) {
                const DungeonTaskRecord& task = team.progressSnapshot.tasks[i];
                if ((task.validMask & DungeonTaskValidParameters) == 0) continue;
                int score = 0;
                for (int resID : observedResIDs) {
                    int ignored = 0;
                    if (DungeonTaskParameterValue(task, resID, ignored)) ++score;
                }
                if (score > bestScore) {
                    bestScore = score;
                    bestTaskID = task.taskID;
                    tie = false;
                } else if (score > 0 && score == bestScore && task.taskID != bestTaskID) {
                    tie = true;
                }
            }
            if (bestScore > 0 && !tie) {
                team.boundTaskID = bestTaskID;
                bound = DungeonTaskById(team.progressSnapshot, team.boundTaskID);
                if (bound && (bound->validMask & DungeonTaskValidName)) team.boundTaskName = bound->name;
            }
        }
        if (!bound) return;

        for (int resID : observedResIDs) {
            int ignored = 0;
            if (resID > 0 && DungeonTaskParameterValue(*bound, resID, ignored))
                team.boundTaskParameterKeys.insert(resID);
        }
    }

    bool ReadBoundDungeonTaskProgress(DungeonTeamRuntime& team, int& sum) const {
        sum = 0;
        if (!team.progressReadOk || team.boundTaskID <= 0 || team.boundTaskParameterKeys.empty()) return false;
        const DungeonTaskRecord* task = DungeonTaskById(team.progressSnapshot, team.boundTaskID);
        if (!task) return false;
        bool any = false;
        for (int key : team.boundTaskParameterKeys) {
            int value = 0;
            if (!DungeonTaskParameterValue(*task, key, value)) continue;
            sum += std::max(0, value);
            any = true;
        }
        return any;
    }

    bool TickDungeonFight(DungeonTeamRuntime& team, const cleanroute_dungeon::Step& step,
                          DWORD now, std::wstring& error) {
        cleanroute_dungeon::Preset* preset = DungeonPresetForTeam(team);
        if (!preset) {
            error = L"mất preset runtime";
            return false;
        }
        std::vector<Account*> members = DungeonStepMembers(team, step);
        if (members.empty()) {
            error = L"STEP không có participant hợp lệ";
            return false;
        }
        bool allOn = true;
        for (Account* account : members) {
            if (!DungeonStable(*account)) {
                error = L"state acc không ổn định khi đánh";
                return false;
            }
            if (account->snapshot.mapID != preset->dungeonMap) {
                error = L"acc lệch MapID khi đánh";
                return false;
            }
            if (!account->snapshot.autoFight) allOn = false;
        }

        Account* authority = nullptr;
        for (Account* account : members) {
            if (account->game.pid == team.config.leaderPid) {
                authority = account;
                break;
            }
        }
        if (!authority) authority = members.front(); // one authoritative observer per split group.

        // Read the game's own server-backed task board first. This is observer-only. A failure does
        // not fake PASS and does not break old dungeons: the strict GMonster scanner remains fallback.
        std::wstring taskReadDetail;
        const bool taskSnapshotFresh = RefreshDungeonProgressSnapshot(team, *authority, now, taskReadDetail);

        if (!allOn) {
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
        if (team.lastScanTick && !Elapsed(now, team.lastScanTick, 500)) return false;
        team.lastScanTick = now;

        Response response{};
        std::wstring scanError;
        if (!authority->bridge.Call(Command::ScanNearbyMonsters, 0, 0, 0, response, scanError, 1600)) {
            team.status = L"scanner chưa pass • " + scanError;
            return false;
        }

        std::vector<cleanroute_dungeon::MonsterObservation> scan;
        scan.reserve(response.monsterCount);
        std::vector<cleanroute_dungeon::MonsterRule> rules;
        cleanroute_dungeon::MonsterRule rule{};
        rule.name = step.monsterName;
        rule.resID = step.monsterResID;
        rule.group = step.group;
        rule.boss = step.boss;
        rules.push_back(rule);
        int aliveMatch = 0;
        std::vector<int> observedResIDs;
        observedResIDs.reserve(16);
        for (std::size_t i = 0; i < response.monsterCount && i < kMaxMonsterRecords; ++i) {
            const MonsterRecord& record = response.monsters[i];
            cleanroute_dungeon::MonsterObservation observation{};
            observation.roleID = record.roleID;
            observation.resID = record.resID;
            observation.hp = record.hp;
            observation.maxHP = record.maxHP;
            observation.x = record.x;
            observation.y = record.y;
            observation.dead = record.dead != 0;
            observation.positionValid = (record.validMask & MonsterValidPosition) != 0;
            observation.verifiedMonster = (record.validMask & MonsterValidClassProof) != 0;
            observation.liveVitalsValid = (record.validMask & MonsterValidLiveVitals) != 0;
            observation.name = record.name;
            scan.push_back(observation);
            const bool matched = step.matchAnyVerified || cleanroute_dungeon::MatchesRule(observation, rule, step.group);
            if (matched && cleanroute_dungeon::InRadius(observation, step.x, step.y, step.radius)) {
                if (observation.resID > 0 &&
                    std::find(observedResIDs.begin(), observedResIDs.end(), observation.resID) == observedResIDs.end()) {
                    observedResIDs.push_back(observation.resID);
                }
                if (!observation.dead && observation.hp > 0) {
                    ++aliveMatch;
                    team.seenMatchingAlive = true;
                }
            }
        }
        if (step.monsterResID > 0 &&
            std::find(observedResIDs.begin(), observedResIDs.end(), step.monsterResID) == observedResIDs.end()) {
            observedResIDs.push_back(step.monsterResID);
        }

        std::vector<cleanroute_dungeon::CountDiagnostic> diagnostics;
        const auto events = team.deathTracker.Observe(scan, rules, step.group, step.x, step.y,
                                                      step.radius, step.matchAnyVerified, &diagnostics);
        team.kills += static_cast<int>(events.size());

        for (const auto& diagnostic : diagnostics) {
            const std::wstring key = std::to_wstring(diagnostic.roleID) + L":" +
                                     std::to_wstring(static_cast<int>(diagnostic.reason));
            if (team.loggedDiagnostics.insert(key).second) {
                LogAccount(*authority, L"PB KHÔNG TÍNH • " +
                           std::wstring(cleanroute_dungeon::CountSkipReasonText(diagnostic.reason)) +
                           L" • RoleID " + std::to_wstring(diagnostic.roleID) +
                           L" • ResID " + std::to_wstring(diagnostic.resID) + L" • " + diagnostic.name);
            }
        }

        // TASK + MONSTER LEARN (in-memory): Parameters is keyed by MonsterID according to the
        // shipped quest engine. Intersect it with live strict GMonster ResID values before binding;
        // ambiguous tasks stay unbound and therefore cannot become false authority.
        if (taskSnapshotFresh) {
            RestoreDungeonTaskLearn(team, *preset);
            const int beforeTaskID = team.boundTaskID;
            const std::size_t beforeKeys = team.boundTaskParameterKeys.size();
            BindDungeonTaskObjective(team, observedResIDs);
            if (team.boundTaskID != beforeTaskID || team.boundTaskParameterKeys.size() != beforeKeys)
                SaveDungeonTaskLearn(team, *preset);
        }

        int serverAbsolute = 0;
        const bool taskBound = taskSnapshotFresh && ReadBoundDungeonTaskProgress(team, serverAbsolute);
        const int priorTarget = DungeonPriorObjectiveTarget(*preset, team.stepIndex, step);
        const int serverRelative = taskBound ? std::max(0, serverAbsolute - priorTarget) : 0;
        team.taskStepProgress = serverRelative;

        const bool scannerComplete = step.requiredKills > 0 && team.kills >= step.requiredKills;
        if (taskBound && scannerComplete && serverRelative < step.requiredKills) {
            if (!team.serverSyncWaitTick) team.serverSyncWaitTick = now;
        } else {
            team.serverSyncWaitTick = 0;
        }

        cleanroute_dungeon_progress::Input proof{};
        proof.required = step.requiredKills;
        proof.taskAvailable = team.progressReadOk;
        proof.taskFresh = taskSnapshotFresh;
        proof.taskBound = taskBound;
        proof.taskDelta = serverRelative;
        // FuBen packet IDs 200168..200174 are known in DATA, but no inbound response store/direction
        // has been proven in this bridge yet. Keep this source explicitly unavailable rather than
        // manufacturing server proof from a packet ID.
        proof.fubenAvailable = false;
        proof.scannerAvailable = true;
        proof.scannerKills = team.kills;
        proof.serverSyncGraceExpired = team.serverSyncWaitTick && Elapsed(now, team.serverSyncWaitTick, 2500);
        const cleanroute_dungeon_progress::Result decision = cleanroute_dungeon_progress::Decide(proof);

        std::wstring source = L"SCAN";
        if (decision.sources & cleanroute_dungeon_progress::ProofTask) source = L"TASK+SCAN";
        if (taskBound) {
            team.status = L"ĐÁNH • " + step.label + L" • " + source + L" • TaskID " +
                          std::to_wstring(team.boundTaskID) +
                          (team.boundTaskName.empty() ? L"" : L" " + team.boundTaskName) +
                          L" • server " + std::to_wstring(serverRelative) + L"/" +
                          std::to_wstring(step.requiredKills) + L" • scan " + std::to_wstring(team.kills) + L"/" +
                          std::to_wstring(step.requiredKills) + L" • sống " + std::to_wstring(aliveMatch);
        } else {
            team.status = L"ĐÁNH • " + step.label + L" • SCAN fallback " + std::to_wstring(team.kills) + L"/" +
                          std::to_wstring(step.requiredKills) + L" • TASK chưa bind • sống " + std::to_wstring(aliveMatch);
        }

        if (decision.decision == cleanroute_dungeon_progress::Decision::WaitForServerSync) {
            team.status += L" • chờ UpdateTask";
            return false;
        }
        if (decision.decision == cleanroute_dungeon_progress::Decision::Conflict) {
            error = L"TASK/FuBen progress xung đột; fail-closed";
            return false;
        }
        if (decision.decision == cleanroute_dungeon_progress::Decision::FailClosed) {
            error = L"không có nguồn proof tiến độ hợp lệ";
            return false;
        }
        // Donor contract remains intact: disappearance/empty AOI is never a kill proof.
        return decision.decision == cleanroute_dungeon_progress::Decision::Advance;
    }

    bool DungeonPostSellNeeded(Account& account, bool& valid, std::wstring& reason) {
        valid = true;
        reason.clear();
        if (!account.profile.enableSell) return false;
        if (!account.snapshotValid || (account.snapshot.validMask & ValidIdentity) == 0) {
            valid = false;
            reason = L"không đọc được RoleID authoritative khi check bán sau phó bản";
            return false;
        }
        if ((account.snapshot.validMask & ValidBagSpace) == 0) {
            valid = false;
            reason = L"không đọc được FreeBagSpace khi check bán sau phó bản";
            return false;
        }
        const int threshold = DungeonSellThresholdForRole(account.snapshot.roleID);
        if (threshold <= 0) return false;
        return account.snapshot.freeBagSpace < threshold;
    }

    bool TickDungeonPostSell(DungeonTeamRuntime& team, DWORD now, std::wstring& error) {
        bool allDone = true;
        for (std::uint32_t pid : team.config.pids) {
            Account* account = AccountByPid(pid);
            if (!account) {
                error = L"mất PID trong post-sell";
                return false;
            }
            if (team.postSellDone.count(pid)) continue;
            allDone = false;

            if (account->runtime.sellPhase == 8) {
                // Existing Auto Sell has already proven a stable non-full bag. Dungeon owns the
                // next destination, so skip its normal return-to-train phase and continue PB.
                account->runtime.sellPhase = 0;
                ResetRobustTravel(account->runtime);
                team.postSellDone.insert(pid);
                account->runtime.status = L"PB • bán xong, chờ đội";
                continue;
            }
            if (account->runtime.sellPhase == 10) {
                error = L"Auto Sell fail-closed PID " + std::to_wstring(pid);
                return false;
            }
            if (account->runtime.sellPhase != 0) {
                (void)HandleAutoSell(*account, now);
                continue;
            }
            bool bagProofValid = false;
            std::wstring bagReason;
            const bool sellNeeded = DungeonPostSellNeeded(*account, bagProofValid, bagReason);
            if (!bagProofValid) {
                error = L"PID " + std::to_wstring(pid) + L" • " + bagReason;
                return false;
            }
            if (!sellNeeded) {
                team.postSellDone.insert(pid);
                continue;
            }
            const TargetProfile npcTarget = SellNpcTarget(*account);
            std::wstring sellReason;
            if (!npcTarget.valid) {
                error = L"PID " + std::to_wstring(pid) + L" cần bán nhưng NPC chưa có tọa độ";
                return false;
            }
            if (!SemanticSellRulesActive() && !SellMacroConfigured(*account, sellReason)) {
                error = L"PID " + std::to_wstring(pid) + L" cần bán nhưng " + sellReason;
                return false;
            }
            BeginAutoSell(*account, now);
        }
        if (!allDone) {
            allDone = team.postSellDone.size() == team.config.pids.size();
        }
        return allDone;
    }

    void BeginDungeonNextRunOrComplete(DungeonTeamRuntime& team, DWORD now) {
        if (team.runIndex < team.config.runs) {
            ++team.runIndex;
            team.phase = cleanroute_dungeon::TeamPhase::Gather;
            team.stepIndex = 0;
            team.npcAttempts = 0;
            team.dialogAttempts = 0;
            team.postSellDone.clear();
            ResetDungeonStepRuntime(team, now);
            team.status = L"LƯỢT " + std::to_wstring(team.runIndex) + L" • quay NPC";
            return;
        }
        const int completedRuns = team.config.runs;
        ReleaseDungeonTeamMembers(team);
        team.state = cleanroute_dungeon::TeamState::Completed;
        team.phase = cleanroute_dungeon::TeamPhase::Complete;
        team.status = L"HOÀN THÀNH " + std::to_wstring(completedRuns) + L" lượt";
        Log(L"AUTO PHÓ BẢN ĐỘI " + std::to_wstring(team.config.id) + L" COMPLETE");
        RefreshDungeonAccountList();
    }

    void TickDungeonTeam(DungeonTeamRuntime& team, DWORD now) {
        if (team.state != cleanroute_dungeon::TeamState::Running) return;
        cleanroute_dungeon::Preset* preset = DungeonPresetForTeam(team);
        if (!preset) {
            FailDungeonTeam(team, L"mất preset runtime");
            return;
        }
        std::wstring error;
        const bool transitionPhase = team.phase == cleanroute_dungeon::TeamPhase::WaitEnter ||
                                     team.phase == cleanroute_dungeon::TeamPhase::WaitExit ||
                                     team.phase == cleanroute_dungeon::TeamPhase::PostSell ||
                                     team.phase == cleanroute_dungeon::TeamPhase::Dialog ||
                                     team.phase == cleanroute_dungeon::TeamPhase::Npc;
        if (!transitionPhase && !DungeonAllStable(team, error)) {
            if (DungeonTimeout(team, now, 30)) FailDungeonTeam(team, error);
            else team.status = L"CHỜ STATE • " + error;
            return;
        }

        if (team.phase == cleanroute_dungeon::TeamPhase::Precheck) {
            for (std::uint32_t pid : team.config.pids) {
                Account* account = AccountByPid(pid);
                if (!account || account->snapshot.dead) {
                    FailDungeonTeam(team, L"có acc chết ở PRECHECK");
                    return;
                }
            }
            team.phase = cleanroute_dungeon::TeamPhase::Gather;
            team.phaseTick = now;
            team.status = L"TẬP KẾT NPC";
            return;
        }

        if (team.phase == cleanroute_dungeon::TeamPhase::Gather) {
            bool all = true;
            TargetProfile target{};
            target.name = L"NPC vào phó bản";
            target.mapID = preset->gatherMap;
            target.x = preset->gatherX;
            target.y = preset->gatherY;
            target.valid = true;
            for (std::uint32_t pid : team.config.pids) {
                Account& account = *AccountByPid(pid);
                bool arrived = false;
                HandleRobustTravel(account, now, target, L"NPC vào phó bản", arrived, 160);
                if (!arrived) all = false;
            }
            if (all) {
                team.phase = cleanroute_dungeon::TeamPhase::Npc;
                team.phaseTick = now;
                team.status = L"BARRIER PASS • leader mở NPC";
            } else if (DungeonTimeout(team, now, 180)) {
                FailDungeonTeam(team, L"timeout tập kết NPC");
            }
            return;
        }

        if (team.phase == cleanroute_dungeon::TeamPhase::Npc) {
            if (team.dueTick && now < team.dueTick) return;
            Account* leader = AccountByPid(team.config.leaderPid);
            Response response{};
            std::wstring callError;
            if (!leader || !leader->bridge.Call(Command::ClickNpc, preset->npcResID, 0, 0,
                                                response, callError, 1500)) {
                if (++team.npcAttempts >= 8) FailDungeonTeam(team, L"ClickNPC fail: " + callError);
                else {
                    team.status = L"ClickNPC retry";
                    team.dueTick = now + 1000;
                }
                return;
            }
            team.phase = cleanroute_dungeon::TeamPhase::Dialog;
            team.phaseTick = now;
            team.dueTick = now + 900;
            team.status = L"NPC PASS • chờ dialog exact";
            return;
        }

        if (team.phase == cleanroute_dungeon::TeamPhase::Dialog) {
            if (now < team.dueTick) return;
            Account* leader = AccountByPid(team.config.leaderPid);
            Response response{};
            std::wstring callError;
            if (!leader || !leader->bridge.CallText(Command::ClickDialogText, preset->dialogText,
                                                    response, callError, 1800)) {
                if (++team.dialogAttempts >= 12) FailDungeonTeam(team, L"dialog exact fail: " + callError);
                else {
                    team.status = L"chờ dialog exact • " + preset->dialogText;
                    team.dueTick = now + 1000;
                }
                return;
            }
            team.phase = cleanroute_dungeon::TeamPhase::WaitEnter;
            team.phaseTick = now;
            team.status = L"DIALOG PASS • chờ toàn đội vào M" + std::to_wstring(preset->dungeonMap);
            return;
        }

        if (team.phase == cleanroute_dungeon::TeamPhase::WaitEnter) {
            bool all = true;
            for (std::uint32_t pid : team.config.pids) {
                Account& account = *AccountByPid(pid);
                if (account.snapshot.mapID != preset->dungeonMap || !account.snapshot.mapReady ||
                    account.snapshot.waitingChangeMap) all = false;
            }
            if (all) {
                team.phase = cleanroute_dungeon::TeamPhase::Steps;
                team.stepIndex = 0;
                ResetDungeonStepRuntime(team, now);
                team.status = L"VÀO PB PASS • STEP 1";
            } else if (DungeonTimeout(team, now, 60)) {
                FailDungeonTeam(team, L"không chứng minh đủ acc vào dungeon map");
            }
            return;
        }

        if (team.phase == cleanroute_dungeon::TeamPhase::Steps) {
            if (team.stepIndex >= static_cast<int>(preset->steps.size())) {
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
            const cleanroute_dungeon::Step& step = preset->steps[static_cast<std::size_t>(team.stepIndex)];
            if (DungeonTimeout(team, now, step.timeoutSec)) {
                FailDungeonTeam(team, L"timeout STEP " + std::to_wstring(team.stepIndex + 1) + L" • " + step.label);
                return;
            }
            std::vector<Account*> stepMembers = DungeonStepMembers(team, step);
            if (stepMembers.empty()) {
                FailDungeonTeam(team, L"STEP không có participant hợp lệ");
                return;
            }

            if (step.kind == cleanroute_dungeon::StepKind::Move ||
                step.kind == cleanroute_dungeon::StepKind::Portal) {
                bool all = true;
                TargetProfile target{};
                target.name = step.label;
                target.mapID = step.mapID;
                target.x = step.x;
                target.y = step.y;
                target.valid = true;
                for (Account* account : stepMembers) {
                    bool arrived = false;
                    HandleRobustTravel(*account, now, target, step.label.c_str(), arrived, step.tolerance);
                    if (!arrived) all = false;
                }
                team.status = (step.kind == cleanroute_dungeon::StepKind::Portal ? L"CỔNG • " : L"DI CHUYỂN • ") +
                              step.label + L" • slot " + DungeonMaskText(step.participantMask);
                if (!all) {
                    team.dueTick = 0;
                    return;
                }
                if (step.kind == cleanroute_dungeon::StepKind::Portal) {
                    // DATA marks these as UsePortal. Current v3.3 core has no separate blind portal click;
                    // arrival at the live portal coordinate is the semantic trigger. Give the client a
                    // short settle window so the next action cannot race the teleport/scene reposition.
                    if (!team.dueTick) team.dueTick = now + 1800;
                    if (now < team.dueTick) return;
                }
                AdvanceDungeonStep(team, now);
                return;
            }

            if (step.kind == cleanroute_dungeon::StepKind::Fight) {
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
            }

            if (step.kind == cleanroute_dungeon::StepKind::Wait) {
                if (!team.dueTick) team.dueTick = now + static_cast<DWORD>(std::max(0, step.delayMs));
                team.status = L"ĐỢI • " + step.label;
                if (now >= team.dueTick) AdvanceDungeonStep(team, now);
                return;
            }

            if (step.kind == cleanroute_dungeon::StepKind::WaitMap) {
                bool all = true;
                for (Account* account : stepMembers) if (account->snapshot.mapID != step.mapID) all = false;
                if (all) AdvanceDungeonStep(team, now);
                return;
            }
        }

        if (team.phase == cleanroute_dungeon::TeamPhase::WaitExit) {
            bool allOut = true;
            for (std::uint32_t pid : team.config.pids) {
                Account& account = *AccountByPid(pid);
                if (account.snapshot.mapID == preset->dungeonMap) allOut = false;
            }
            if (allOut) {
                RecordDungeonDailyRun(team, *preset);
                team.phase = cleanroute_dungeon::TeamPhase::PostSell;
                team.phaseTick = now;
                team.postSellDone.clear();
                team.status = L"ĐÃ RA MAP • check túi / Auto Sell từng acc";
            } else if (DungeonTimeout(team, now, 180)) {
                FailDungeonTeam(team, L"đã hết step nhưng chưa chứng minh rời dungeon map");
            }
            return;
        }

        if (team.phase == cleanroute_dungeon::TeamPhase::PostSell) {
            if (DungeonTimeout(team, now, 600)) {
                FailDungeonTeam(team, L"timeout post-run Auto Sell");
                return;
            }
            if (TickDungeonPostSell(team, now, error)) BeginDungeonNextRunOrComplete(team, now);
            else if (!error.empty()) FailDungeonTeam(team, error);
            else team.status = L"POST-RUN • đang bán/đợi túi từng acc";
            return;
        }
    }

    void TickDungeon(DWORD now) {
        bool changed = false;
        for (DungeonTeamRuntime& team : dungeonTeams_) {
            const std::wstring before = team.status;
            TickDungeonTeam(team, now);
            if (before != team.status) changed = true;
        }
        if (changed) {
            RefreshDungeonTeamList();
            const int index = SelectedDungeonTeamIndex();
            if (index >= 0 && index < static_cast<int>(dungeonTeams_.size()) && dungeonStatus_)
                SetText(dungeonStatus_, L"ĐỘI " + std::to_wstring(dungeonTeams_[static_cast<std::size_t>(index)].config.id) +
                                        L" • " + dungeonTeams_[static_cast<std::size_t>(index)].status);
        }
    }

    void DungeonManualScan() {
        const int index = SelectedDungeonTeamIndex();
        if (index < 0 || index >= static_cast<int>(dungeonTeams_.size())) {
            SetText(dungeonMonsterStatus_, L"Chọn một bảng tổ đội trước");
            return;
        }
        DungeonTeamRuntime& team = dungeonTeams_[static_cast<std::size_t>(index)];
        Account* leader = AccountByPid(team.config.leaderPid);
        if (!leader) {
            SetText(dungeonMonsterStatus_, L"Mất đội trưởng / chưa bind client");
            return;
        }

        Response taskResponse{};
        std::wstring taskError;
        const bool taskOk = leader->bridge.Call(Command::ReadDungeonProgress, 0, 0, 0,
                                                taskResponse, taskError, 1800);
        Response scanResponse{};
        std::wstring scanError;
        const bool scanOk = leader->bridge.Call(Command::ScanNearbyMonsters, 0, 0, 0,
                                                scanResponse, scanError, 1800);
        std::wstring text;
        if (taskOk) {
            text = L"TASK PASS • doing=" + std::to_wstring(taskResponse.dungeonProgress.taskCount);
            if (taskResponse.dungeonProgress.taskCount > 0) {
                const DungeonTaskRecord& task = taskResponse.dungeonProgress.tasks[0];
                text += L" • TaskID=" + std::to_wstring(task.taskID);
                if ((task.validMask & DungeonTaskValidName) && task.name[0]) text += L" " + std::wstring(task.name);
                text += L" • params=" + std::to_wstring(task.parameterCount);
            }
        } else {
            text = L"TASK unavailable • " + taskError;
        }
        if (scanOk) {
            text += L" | SCAN PASS • GMonster=" + std::to_wstring(scanResponse.monsterCount) +
                    L" • loại GRole=" + std::to_wstring(scanResponse.excludedPlayerRoles);
        } else {
            text += L" | SCAN FAIL • " + scanError;
        }
        SetText(dungeonMonsterStatus_, text);
    }

    // ---------------- Dungeon preset / step editor ----------------
    static LRESULT CALLBACK DungeonEditorWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        App* self = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
            self = reinterpret_cast<App*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            if (self) self->dungeonEditor_ = hwnd;
        }
        return self ? self->HandleDungeonEditor(hwnd, msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
    }

    HWND DungeonEditorMake(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style,
                           int x, int y, int w, int h, int id) {
        HWND control = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style,
                                       x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                       instance_, nullptr);
        if (control)
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
        return control;
    }

    void OpenDungeonEditor() {
        const int presetIndex = DungeonDisplayPresetIndex();
        if (presetIndex < 0 || presetIndex >= static_cast<int>(dungeonPresets_.size())) return;
        dungeonEditorPresetIndex_ = presetIndex;
        if (dungeonEditor_ && IsWindow(dungeonEditor_)) {
            ShowWindow(dungeonEditor_, SW_SHOW);
            SetForegroundWindow(dungeonEditor_);
            RefreshDungeonEditorList();
            LoadDungeonEditorHeader();
            return;
        }
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = DungeonEditorWndProc;
        wc.hInstance = instance_;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = L"ThanLongDungeonEditorV40";
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            Log(L"Không đăng ký được cửa sổ cấu hình phó bản.");
            return;
        }
        const std::wstring title = L"CẤU HÌNH AUTO PHÓ BẢN v4.3 — " +
                                   dungeonPresets_[static_cast<std::size_t>(presetIndex)].name;
        HWND window = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, title.c_str(),
                                      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                      CW_USEDEFAULT, CW_USEDEFAULT, 1120, 720, hwnd_, nullptr, instance_, this);
        if (!window) return;
        ShowWindow(window, SW_SHOW);
        UpdateWindow(window);
    }

    int DungeonEditorSelectedStep() const {
        return dungeonEditorList_ ? ListView_GetNextItem(dungeonEditorList_, -1, LVNI_SELECTED) : -1;
    }

    cleanroute_dungeon::Preset* DungeonEditorPreset() {
        if (dungeonEditorPresetIndex_ < 0 || dungeonEditorPresetIndex_ >= static_cast<int>(dungeonPresets_.size())) return nullptr;
        return &dungeonPresets_[static_cast<std::size_t>(dungeonEditorPresetIndex_)];
    }

    void RefreshDungeonEditorList(int select = -1) {
        if (!dungeonEditorList_) return;
        cleanroute_dungeon::Preset* preset = DungeonEditorPreset();
        if (!preset) return;
        if (select < 0) select = DungeonEditorSelectedStep();
        ListView_DeleteAllItems(dungeonEditorList_);
        for (std::size_t i = 0; i < preset->steps.size(); ++i) {
            const auto& step = preset->steps[i];
            std::wstring index = std::to_wstring(i + 1);
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = static_cast<int>(i);
            item.pszText = index.data();
            ListView_InsertItem(dungeonEditorList_, &item);
            ListView_SetItemText(dungeonEditorList_, static_cast<int>(i), 1,
                                 const_cast<wchar_t*>(cleanroute_dungeon::StepKindLabel(step.kind)));
            ListView_SetItemText(dungeonEditorList_, static_cast<int>(i), 2,
                                 const_cast<wchar_t*>(step.label.c_str()));
            std::wstring pos = L"M" + std::to_wstring(step.mapID) + L" • " +
                               std::to_wstring(step.x) + L"," + std::to_wstring(step.y);
            std::wstring proof = step.kind == cleanroute_dungeon::StepKind::Fight
                ? L"Kill " + std::to_wstring(step.requiredKills) + L" • R" + std::to_wstring(step.radius)
                : L"Tol " + std::to_wstring(step.tolerance);
            std::wstring mask = DungeonMaskText(step.participantMask);
            ListView_SetItemText(dungeonEditorList_, static_cast<int>(i), 3, pos.data());
            ListView_SetItemText(dungeonEditorList_, static_cast<int>(i), 4, proof.data());
            ListView_SetItemText(dungeonEditorList_, static_cast<int>(i), 5, mask.data());
        }
        if (select < 0 && !preset->steps.empty()) select = 0;
        if (select >= 0 && select < static_cast<int>(preset->steps.size())) {
            ListView_SetItemState(dungeonEditorList_, select, LVIS_SELECTED | LVIS_FOCUSED,
                                  LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(dungeonEditorList_, select, FALSE);
            LoadDungeonEditorStep(select);
        }
    }

    void LoadDungeonEditorHeader() {
        cleanroute_dungeon::Preset* preset = DungeonEditorPreset();
        if (!preset) return;
        SetText(dungeonEditorGatherMap_, std::to_wstring(preset->gatherMap));
        SetText(dungeonEditorNpc_, std::to_wstring(preset->npcResID));
        SetText(dungeonEditorGatherX_, std::to_wstring(preset->gatherX));
        SetText(dungeonEditorGatherY_, std::to_wstring(preset->gatherY));
        SetText(dungeonEditorDungeonMap_, std::to_wstring(preset->dungeonMap));
        SetText(dungeonEditorDialog_, preset->dialogText);
    }

    void LoadDungeonEditorStep(int row = -1) {
        cleanroute_dungeon::Preset* preset = DungeonEditorPreset();
        if (!preset) return;
        if (row < 0) row = DungeonEditorSelectedStep();
        if (row < 0 || row >= static_cast<int>(preset->steps.size())) return;
        const cleanroute_dungeon::Step& step = preset->steps[static_cast<std::size_t>(row)];
        SendMessageW(dungeonEditorKind_, CB_SETCURSEL, static_cast<int>(step.kind), 0);
        SetText(dungeonEditorLabel_, step.label);
        SetText(dungeonEditorMap_, std::to_wstring(step.mapID));
        SetText(dungeonEditorX_, std::to_wstring(step.x));
        SetText(dungeonEditorY_, std::to_wstring(step.y));
        SetText(dungeonEditorTolerance_, std::to_wstring(step.tolerance));
        SetText(dungeonEditorKills_, std::to_wstring(step.requiredKills));
        SetText(dungeonEditorRadius_, std::to_wstring(step.radius));
        SetText(dungeonEditorDelay_, std::to_wstring(step.delayMs));
        SetText(dungeonEditorTimeout_, std::to_wstring(step.timeoutSec));
        SetText(dungeonEditorMonster_, step.monsterName);
        SetText(dungeonEditorResID_, std::to_wstring(step.monsterResID));
        SetText(dungeonEditorGroup_, step.group);
        SendMessageW(dungeonEditorBoss_, BM_SETCHECK, step.boss ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(dungeonEditorAny_, BM_SETCHECK, step.matchAnyVerified ? BST_CHECKED : BST_UNCHECKED, 0);
        for (std::size_t i = 0; i < dungeonEditorSlots_.size(); ++i) {
            const bool checked = (step.participantMask & (1u << static_cast<unsigned>(i))) != 0;
            SendMessageW(dungeonEditorSlots_[i], BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
        }
    }

    bool ReadDungeonEditorStep(cleanroute_dungeon::Step& step, std::wstring& error) {
        const LRESULT kind = SendMessageW(dungeonEditorKind_, CB_GETCURSEL, 0, 0);
        if (kind == CB_ERR || kind < 0 || kind > static_cast<int>(cleanroute_dungeon::StepKind::Portal)) {
            error = L"Chọn loại thao tác";
            return false;
        }
        step.kind = static_cast<cleanroute_dungeon::StepKind>(kind);
        step.label = TrimWs(GetText(dungeonEditorLabel_));
        if (step.label.empty()) step.label = cleanroute_dungeon::StepKindLabel(step.kind);
        step.mapID = ParseEditInt(dungeonEditorMap_, 0, 0, 100000);
        step.x = _wtoi(GetText(dungeonEditorX_).c_str());
        step.y = _wtoi(GetText(dungeonEditorY_).c_str());
        step.tolerance = ParseEditInt(dungeonEditorTolerance_, 120, 1, 10000);
        step.requiredKills = ParseEditInt(dungeonEditorKills_, 0, 0, 100000);
        step.radius = ParseEditInt(dungeonEditorRadius_, 800, 0, 100000);
        step.delayMs = ParseEditInt(dungeonEditorDelay_, 0, 0, 3600000);
        step.timeoutSec = ParseEditInt(dungeonEditorTimeout_, 180, 1, 86400);
        step.monsterName = TrimWs(GetText(dungeonEditorMonster_));
        step.monsterResID = ParseEditInt(dungeonEditorResID_, 0, 0, INT_MAX);
        step.group = TrimWs(GetText(dungeonEditorGroup_));
        if (step.group.empty()) step.group = L"THUONG";
        step.boss = SendMessageW(dungeonEditorBoss_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        step.matchAnyVerified = SendMessageW(dungeonEditorAny_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        step.participantMask = 0;
        for (std::size_t i = 0; i < dungeonEditorSlots_.size(); ++i) {
            if (SendMessageW(dungeonEditorSlots_[i], BM_GETCHECK, 0, 0) == BST_CHECKED)
                step.participantMask |= 1u << static_cast<unsigned>(i);
        }
        return cleanroute_dungeon::ValidateStep(step, error);
    }

    void SaveDungeonEditorHeader() {
        cleanroute_dungeon::Preset* preset = DungeonEditorPreset();
        if (!preset) return;
        const int gatherMap = ParseEditInt(dungeonEditorGatherMap_, preset->gatherMap, 1, 100000);
        const int npc = ParseEditInt(dungeonEditorNpc_, preset->npcResID, 1, INT_MAX);
        const int dungeonMap = ParseEditInt(dungeonEditorDungeonMap_, preset->dungeonMap, 1, 100000);
        const std::wstring dialog = TrimWs(GetText(dungeonEditorDialog_));
        if (dialog.empty()) {
            MessageBoxW(dungeonEditor_, L"Dialog text không được rỗng.", L"Auto phó bản", MB_OK | MB_ICONWARNING);
            return;
        }
        preset->gatherMap = gatherMap;
        preset->npcResID = npc;
        preset->gatherX = _wtoi(GetText(dungeonEditorGatherX_).c_str());
        preset->gatherY = _wtoi(GetText(dungeonEditorGatherY_).c_str());
        preset->dungeonMap = dungeonMap;
        preset->dialogText = dialog;
        SaveDungeonPresetOverride(dungeonEditorPresetIndex_);
        RefreshDungeonStepList();
        if (dungeonStatus_) SetText(dungeonStatus_, L"Đã lưu header preset " + preset->name);
    }

    void SaveDungeonEditorStep() {
        cleanroute_dungeon::Preset* preset = DungeonEditorPreset();
        if (!preset) return;
        const int row = DungeonEditorSelectedStep();
        if (row < 0 || row >= static_cast<int>(preset->steps.size())) return;
        cleanroute_dungeon::Step step{};
        std::wstring error;
        if (!ReadDungeonEditorStep(step, error)) {
            MessageBoxW(dungeonEditor_, error.c_str(), L"STEP không hợp lệ", MB_OK | MB_ICONWARNING);
            return;
        }
        preset->steps[static_cast<std::size_t>(row)] = std::move(step);
        SaveDungeonPresetOverride(dungeonEditorPresetIndex_);
        RefreshDungeonEditorList(row);
        RefreshDungeonStepList();
    }

    void AddDungeonEditorStep() {
        cleanroute_dungeon::Preset* preset = DungeonEditorPreset();
        if (!preset) return;
        cleanroute_dungeon::Step step{};
        step.kind = cleanroute_dungeon::StepKind::Move;
        step.label = L"TỌA MỚI";
        step.mapID = preset->dungeonMap;
        step.timeoutSec = 180;
        step.participantMask = cleanroute_dungeon::kAllParticipantsMask;
        int row = DungeonEditorSelectedStep();
        if (row < 0) row = static_cast<int>(preset->steps.size()) - 1;
        preset->steps.insert(preset->steps.begin() + std::min<std::size_t>(preset->steps.size(), static_cast<std::size_t>(row + 1)), step);
        SaveDungeonPresetOverride(dungeonEditorPresetIndex_);
        RefreshDungeonEditorList(std::min(row + 1, static_cast<int>(preset->steps.size()) - 1));
        RefreshDungeonStepList();
    }

    void DeleteDungeonEditorStep() {
        cleanroute_dungeon::Preset* preset = DungeonEditorPreset();
        if (!preset) return;
        const int row = DungeonEditorSelectedStep();
        if (row < 0 || row >= static_cast<int>(preset->steps.size())) return;
        preset->steps.erase(preset->steps.begin() + row);
        SaveDungeonPresetOverride(dungeonEditorPresetIndex_);
        RefreshDungeonEditorList(std::min(row, static_cast<int>(preset->steps.size()) - 1));
        RefreshDungeonStepList();
    }

    void MoveDungeonEditorStep(int delta) {
        cleanroute_dungeon::Preset* preset = DungeonEditorPreset();
        if (!preset) return;
        const int row = DungeonEditorSelectedStep();
        const int target = row + delta;
        if (row < 0 || target < 0 || target >= static_cast<int>(preset->steps.size())) return;
        std::swap(preset->steps[static_cast<std::size_t>(row)], preset->steps[static_cast<std::size_t>(target)]);
        SaveDungeonPresetOverride(dungeonEditorPresetIndex_);
        RefreshDungeonEditorList(target);
        RefreshDungeonStepList();
    }

    void DuplicateDungeonEditorStep() {
        cleanroute_dungeon::Preset* preset = DungeonEditorPreset();
        if (!preset) return;
        const int row = DungeonEditorSelectedStep();
        if (row < 0 || row >= static_cast<int>(preset->steps.size())) return;
        cleanroute_dungeon::Step copy = preset->steps[static_cast<std::size_t>(row)];
        copy.label += L" (copy)";
        preset->steps.insert(preset->steps.begin() + row + 1, std::move(copy));
        SaveDungeonPresetOverride(dungeonEditorPresetIndex_);
        RefreshDungeonEditorList(row + 1);
        RefreshDungeonStepList();
    }

    Account* DungeonEditorCoordinateAccount() {
        const int teamIndex = SelectedDungeonTeamIndex();
        if (teamIndex >= 0 && teamIndex < static_cast<int>(dungeonTeams_.size())) {
            Account* leader = AccountByPid(dungeonTeams_[static_cast<std::size_t>(teamIndex)].config.leaderPid);
            if (leader) return leader;
        }
        const LRESULT leaderIndex = dungeonLeaderCombo_ ? SendMessageW(dungeonLeaderCombo_, CB_GETCURSEL, 0, 0) : CB_ERR;
        if (leaderIndex != CB_ERR) {
            const DWORD pid = static_cast<DWORD>(SendMessageW(dungeonLeaderCombo_, CB_GETITEMDATA, leaderIndex, 0));
            return AccountByPid(pid);
        }
        return nullptr;
    }

    void CaptureDungeonEditorCoordinate() {
        Account* account = DungeonEditorCoordinateAccount();
        if (!account || !account->snapshotValid ||
            (account->snapshot.validMask & (ValidMap | ValidPosition)) != (ValidMap | ValidPosition)) {
            MessageBoxW(dungeonEditor_, L"Không có snapshot Map/X/Y authoritative của đội trưởng.",
                        L"Auto phó bản", MB_OK | MB_ICONWARNING);
            return;
        }
        SetText(dungeonEditorMap_, std::to_wstring(account->snapshot.mapID));
        SetText(dungeonEditorX_, std::to_wstring(account->snapshot.x));
        SetText(dungeonEditorY_, std::to_wstring(account->snapshot.y));
    }

    void ResetDungeonEditorPreset() {
        cleanroute_dungeon::Preset* preset = DungeonEditorPreset();
        if (!preset) return;
        if (MessageBoxW(dungeonEditor_, L"Khôi phục toàn bộ header + STEP preset về DATA canonical?",
                        L"Auto phó bản", MB_YESNO | MB_ICONWARNING) != IDYES) return;
        ResetDungeonPresetOverride(dungeonEditorPresetIndex_);
        RefreshDungeonEditorList(0);
        LoadDungeonEditorHeader();
        RefreshDungeonStepList();
    }

    LRESULT HandleDungeonEditor(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        if (msg == WM_CREATE) {
            cleanroute_dungeon::Preset* preset = DungeonEditorPreset();
            if (!preset) return -1;
            DungeonEditorMake(hwnd, L"STATIC", L"HEADER VÀO PHÓ BẢN", SS_LEFT | SS_CENTERIMAGE,
                              15, 12, 680, 24, 0);
            DungeonEditorMake(hwnd, L"STATIC", L"Gather Map", SS_LEFT | SS_CENTERIMAGE, 15, 40, 70, 24, 0);
            dungeonEditorGatherMap_ = DungeonEditorMake(hwnd, L"EDIT", L"", WS_BORDER | ES_NUMBER, 88, 40, 55, 24, IDC_DGE_GATHER_MAP);
            DungeonEditorMake(hwnd, L"STATIC", L"NPC ResID", SS_LEFT | SS_CENTERIMAGE, 150, 40, 65, 24, 0);
            dungeonEditorNpc_ = DungeonEditorMake(hwnd, L"EDIT", L"", WS_BORDER | ES_NUMBER, 218, 40, 60, 24, IDC_DGE_NPC);
            DungeonEditorMake(hwnd, L"STATIC", L"NPC X,Y", SS_LEFT | SS_CENTERIMAGE, 285, 40, 52, 24, 0);
            dungeonEditorGatherX_ = DungeonEditorMake(hwnd, L"EDIT", L"", WS_BORDER, 340, 40, 70, 24, IDC_DGE_GATHER_X);
            dungeonEditorGatherY_ = DungeonEditorMake(hwnd, L"EDIT", L"", WS_BORDER, 414, 40, 70, 24, IDC_DGE_GATHER_Y);
            DungeonEditorMake(hwnd, L"STATIC", L"Dungeon Map", SS_LEFT | SS_CENTERIMAGE, 491, 40, 80, 24, 0);
            dungeonEditorDungeonMap_ = DungeonEditorMake(hwnd, L"EDIT", L"", WS_BORDER | ES_NUMBER, 574, 40, 60, 24, IDC_DGE_DUNGEON_MAP);
            DungeonEditorMake(hwnd, L"BUTTON", L"LƯU HEADER", BS_PUSHBUTTON, 640, 40, 105, 24, IDC_DGE_SAVE_HEADER);
            DungeonEditorMake(hwnd, L"STATIC", L"Dialog exact", SS_LEFT | SS_CENTERIMAGE, 15, 70, 70, 24, 0);
            dungeonEditorDialog_ = DungeonEditorMake(hwnd, L"EDIT", L"", WS_BORDER, 88, 70, 657, 24, IDC_DGE_DIALOG);

            dungeonEditorList_ = DungeonEditorMake(hwnd, WC_LISTVIEWW, L"",
                LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER,
                15, 105, 720, 505, IDC_DGE_LIST);
            ListView_SetExtendedListViewStyle(dungeonEditorList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            DungeonListColumn(dungeonEditorList_, 0, 38, L"#");
            DungeonListColumn(dungeonEditorList_, 1, 128, L"Thao tác");
            DungeonListColumn(dungeonEditorList_, 2, 210, L"Mục tiêu");
            DungeonListColumn(dungeonEditorList_, 3, 150, L"Map / X,Y");
            DungeonListColumn(dungeonEditorList_, 4, 110, L"Proof");
            DungeonListColumn(dungeonEditorList_, 5, 70, L"ACC");

            int x = 755;
            DungeonEditorMake(hwnd, L"STATIC", L"SỬA STEP ĐANG CHỌN", SS_LEFT | SS_CENTERIMAGE, x, 12, 330, 24, 0);
            DungeonEditorMake(hwnd, L"STATIC", L"Loại", SS_LEFT | SS_CENTERIMAGE, x, 40, 55, 24, 0);
            dungeonEditorKind_ = DungeonEditorMake(hwnd, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL, x + 60, 40, 250, 200, IDC_DGE_KIND);
            for (int kind = static_cast<int>(cleanroute_dungeon::StepKind::Move);
                 kind <= static_cast<int>(cleanroute_dungeon::StepKind::Portal); ++kind)
                SendMessageW(dungeonEditorKind_, CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(cleanroute_dungeon::StepKindLabel(static_cast<cleanroute_dungeon::StepKind>(kind))));
            DungeonEditorMake(hwnd, L"STATIC", L"Tên", SS_LEFT | SS_CENTERIMAGE, x, 70, 55, 24, 0);
            dungeonEditorLabel_ = DungeonEditorMake(hwnd, L"EDIT", L"", WS_BORDER, x + 60, 70, 250, 24, IDC_DGE_LABEL);
            DungeonEditorMake(hwnd, L"STATIC", L"Map", SS_LEFT | SS_CENTERIMAGE, x, 100, 55, 24, 0);
            dungeonEditorMap_ = DungeonEditorMake(hwnd, L"EDIT", L"", WS_BORDER | ES_NUMBER, x + 60, 100, 65, 24, IDC_DGE_MAP);
            DungeonEditorMake(hwnd, L"STATIC", L"X", SS_LEFT | SS_CENTERIMAGE, x + 130, 100, 18, 24, 0);
            dungeonEditorX_ = DungeonEditorMake(hwnd, L"EDIT", L"", WS_BORDER, x + 150, 100, 72, 24, IDC_DGE_X);
            DungeonEditorMake(hwnd, L"STATIC", L"Y", SS_LEFT | SS_CENTERIMAGE, x + 226, 100, 18, 24, 0);
            dungeonEditorY_ = DungeonEditorMake(hwnd, L"EDIT", L"", WS_BORDER, x + 245, 100, 65, 24, IDC_DGE_Y);
            DungeonEditorMake(hwnd, L"BUTTON", L"GET TỌA LEADER", BS_PUSHBUTTON, x, 130, 310, 26, IDC_DGE_GET);

            DungeonEditorMake(hwnd, L"STATIC", L"Tolerance", SS_LEFT | SS_CENTERIMAGE, x, 164, 65, 24, 0);
            dungeonEditorTolerance_ = DungeonEditorMake(hwnd, L"EDIT", L"120", WS_BORDER | ES_NUMBER, x + 70, 164, 70, 24, IDC_DGE_TOL);
            DungeonEditorMake(hwnd, L"STATIC", L"Kill", SS_LEFT | SS_CENTERIMAGE, x + 150, 164, 35, 24, 0);
            dungeonEditorKills_ = DungeonEditorMake(hwnd, L"EDIT", L"0", WS_BORDER | ES_NUMBER, x + 188, 164, 55, 24, IDC_DGE_KILLS);
            DungeonEditorMake(hwnd, L"STATIC", L"Radius", SS_LEFT | SS_CENTERIMAGE, x + 248, 164, 48, 24, 0);
            dungeonEditorRadius_ = DungeonEditorMake(hwnd, L"EDIT", L"800", WS_BORDER | ES_NUMBER, x + 298, 164, 60, 24, IDC_DGE_RADIUS);

            DungeonEditorMake(hwnd, L"STATIC", L"Delay ms", SS_LEFT | SS_CENTERIMAGE, x, 194, 65, 24, 0);
            dungeonEditorDelay_ = DungeonEditorMake(hwnd, L"EDIT", L"0", WS_BORDER | ES_NUMBER, x + 70, 194, 70, 24, IDC_DGE_DELAY);
            DungeonEditorMake(hwnd, L"STATIC", L"Timeout s", SS_LEFT | SS_CENTERIMAGE, x + 150, 194, 70, 24, 0);
            dungeonEditorTimeout_ = DungeonEditorMake(hwnd, L"EDIT", L"180", WS_BORDER | ES_NUMBER, x + 225, 194, 70, 24, IDC_DGE_TIMEOUT);

            DungeonEditorMake(hwnd, L"STATIC", L"Monster", SS_LEFT | SS_CENTERIMAGE, x, 224, 65, 24, 0);
            dungeonEditorMonster_ = DungeonEditorMake(hwnd, L"EDIT", L"", WS_BORDER, x + 70, 224, 240, 24, IDC_DGE_MONSTER);
            DungeonEditorMake(hwnd, L"STATIC", L"ResID", SS_LEFT | SS_CENTERIMAGE, x, 254, 65, 24, 0);
            dungeonEditorResID_ = DungeonEditorMake(hwnd, L"EDIT", L"0", WS_BORDER | ES_NUMBER, x + 70, 254, 90, 24, IDC_DGE_RESID);
            DungeonEditorMake(hwnd, L"STATIC", L"Group", SS_LEFT | SS_CENTERIMAGE, x + 168, 254, 50, 24, 0);
            dungeonEditorGroup_ = DungeonEditorMake(hwnd, L"EDIT", L"THUONG", WS_BORDER, x + 220, 254, 90, 24, IDC_DGE_GROUP);
            dungeonEditorBoss_ = DungeonEditorMake(hwnd, L"BUTTON", L"BOSS", BS_AUTOCHECKBOX, x, 284, 90, 24, IDC_DGE_BOSS);
            dungeonEditorAny_ = DungeonEditorMake(hwnd, L"BUTTON", L"Match ANY GMonster (fallback)", BS_AUTOCHECKBOX, x + 95, 284, 220, 24, IDC_DGE_ANY);

            DungeonEditorMake(hwnd, L"STATIC", L"Nhóm ACC (slot theo thứ tự trong bảng tổ đội):", SS_LEFT | SS_CENTERIMAGE, x, 314, 330, 24, 0);
            for (int i = 0; i < 6; ++i) {
                const std::wstring label = std::to_wstring(i + 1);
                dungeonEditorSlots_[static_cast<std::size_t>(i)] = DungeonEditorMake(
                    hwnd, L"BUTTON", label.c_str(), BS_AUTOCHECKBOX,
                    x + i * 48, 342, 45, 24, IDC_DGE_SLOT1 + i);
            }

            DungeonEditorMake(hwnd, L"BUTTON", L"LƯU STEP", BS_DEFPUSHBUTTON, x, 378, 145, 28, IDC_DGE_SAVE);
            DungeonEditorMake(hwnd, L"BUTTON", L"+ THÊM", BS_PUSHBUTTON, x + 155, 378, 155, 28, IDC_DGE_ADD);
            DungeonEditorMake(hwnd, L"BUTTON", L"XÓA", BS_PUSHBUTTON, x, 414, 95, 28, IDC_DGE_DELETE);
            DungeonEditorMake(hwnd, L"BUTTON", L"↑", BS_PUSHBUTTON, x + 105, 414, 60, 28, IDC_DGE_UP);
            DungeonEditorMake(hwnd, L"BUTTON", L"↓", BS_PUSHBUTTON, x + 175, 414, 60, 28, IDC_DGE_DOWN);
            DungeonEditorMake(hwnd, L"BUTTON", L"NHÂN BẢN", BS_PUSHBUTTON, x + 245, 414, 110, 28, IDC_DGE_DUP);
            DungeonEditorMake(hwnd, L"BUTTON", L"KHÔI PHỤC DATA CANONICAL", BS_PUSHBUTTON, x, 454, 310, 28, IDC_DGE_RESET);
            DungeonEditorMake(hwnd, L"STATIC",
                L"Lưu ý: preset RUN được snapshot lúc START. Sửa ở đây chỉ áp dụng cho lần START kế tiếp.",
                SS_LEFT, x, 492, 330, 52, 0);
            DungeonEditorMake(hwnd, L"BUTTON", L"ĐÓNG", BS_PUSHBUTTON, x, 565, 310, 32, IDC_DGE_CLOSE);

            LoadDungeonEditorHeader();
            RefreshDungeonEditorList(0);
            return 0;
        }

        if (msg == WM_NOTIFY) {
            NMHDR* header = reinterpret_cast<NMHDR*>(lp);
            if (header && header->hwndFrom == dungeonEditorList_ && header->code == LVN_ITEMCHANGED) {
                auto* change = reinterpret_cast<NMLISTVIEW*>(lp);
                if ((change->uChanged & LVIF_STATE) && (change->uNewState & LVIS_SELECTED))
                    LoadDungeonEditorStep(change->iItem);
            }
        }

        if (msg == WM_COMMAND) {
            switch (LOWORD(wp)) {
                case IDC_DGE_SAVE_HEADER: SaveDungeonEditorHeader(); return 0;
                case IDC_DGE_SAVE: SaveDungeonEditorStep(); return 0;
                case IDC_DGE_ADD: AddDungeonEditorStep(); return 0;
                case IDC_DGE_DELETE: DeleteDungeonEditorStep(); return 0;
                case IDC_DGE_UP: MoveDungeonEditorStep(-1); return 0;
                case IDC_DGE_DOWN: MoveDungeonEditorStep(1); return 0;
                case IDC_DGE_DUP: DuplicateDungeonEditorStep(); return 0;
                case IDC_DGE_GET: CaptureDungeonEditorCoordinate(); return 0;
                case IDC_DGE_RESET: ResetDungeonEditorPreset(); return 0;
                case IDC_DGE_CLOSE: DestroyWindow(hwnd); return 0;
                default: break;
            }
        }

        if (msg == WM_CLOSE) {
            DestroyWindow(hwnd);
            return 0;
        }
        if (msg == WM_DESTROY) {
            dungeonEditor_ = nullptr;
            dungeonEditorList_ = nullptr;
            dungeonEditorKind_ = nullptr;
            dungeonEditorLabel_ = nullptr;
            dungeonEditorMap_ = nullptr;
            dungeonEditorX_ = nullptr;
            dungeonEditorY_ = nullptr;
            dungeonEditorTolerance_ = nullptr;
            dungeonEditorKills_ = nullptr;
            dungeonEditorRadius_ = nullptr;
            dungeonEditorDelay_ = nullptr;
            dungeonEditorTimeout_ = nullptr;
            dungeonEditorMonster_ = nullptr;
            dungeonEditorResID_ = nullptr;
            dungeonEditorGroup_ = nullptr;
            dungeonEditorBoss_ = nullptr;
            dungeonEditorAny_ = nullptr;
            dungeonEditorSlots_.fill(nullptr);
            dungeonEditorGatherMap_ = nullptr;
            dungeonEditorNpc_ = nullptr;
            dungeonEditorGatherX_ = nullptr;
            dungeonEditorGatherY_ = nullptr;
            dungeonEditorDungeonMap_ = nullptr;
            dungeonEditorDialog_ = nullptr;
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
