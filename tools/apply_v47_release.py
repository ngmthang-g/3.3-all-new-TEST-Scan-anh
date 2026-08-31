from pathlib import Path
import re

ROOT = Path('.')

def read(path):
    return (ROOT / path).read_text(encoding='utf-8-sig')

def write(path, text):
    (ROOT / path).write_text(text, encoding='utf-8')

def replace_once(path, old, new):
    text = read(path)
    if new in text and old not in text:
        return False
    if old not in text:
        raise SystemExit(f'{path}: missing expected text: {old[:160]!r}')
    if text.count(old) != 1:
        raise SystemExit(f'{path}: expected one match, found {text.count(old)}: {old[:160]!r}')
    write(path, text.replace(old, new, 1))
    return True

def regex_once(path, pattern, replacement, flags=re.S):
    text = read(path)
    if re.search(pattern, text, flags) is None:
        raise SystemExit(f'{path}: regex did not match: {pattern[:180]!r}')
    out, count = re.subn(pattern, replacement, text, count=1, flags=flags)
    if count != 1:
        raise SystemExit(f'{path}: regex expected one match, got {count}: {pattern[:180]!r}')
    write(path, out)

version = read('VERSION.txt').strip()
if version == '4.7':
    print('v4.7 patch already applied; nothing to do')
    raise SystemExit(0)
if version != '4.6':
    raise SystemExit(f'Expected VERSION 4.6 before migration, got {version!r}')

write('VERSION.txt', '4.7\n')
replace_once('CMakeLists.txt',
             'project(ThanLongItemConsolidator VERSION 4.6',
             'project(ThanLongItemConsolidator VERSION 4.7')
replace_once('src/protocol.h',
             'constexpr std::uint32_t kProtocolVersion = 0x00040600u;',
             'constexpr std::uint32_t kProtocolVersion = 0x00040700u;')
replace_once('src/controller.cpp',
             'constexpr wchar_t kTitle[] = L"AUTO Thần Long đa tính năng Pro v4.6";',
             'constexpr wchar_t kTitle[] = L"AUTO Thần Long đa tính năng Pro v4.7";')

rc = read('resources/app.rc')
for old, new in [
    ('FILEVERSION 4,6,0,0', 'FILEVERSION 4,7,0,0'),
    ('PRODUCTVERSION 4,6,0,0', 'PRODUCTVERSION 4,7,0,0'),
    ('"FileVersion", "4.6\\0"', '"FileVersion", "4.7\\0"'),
    ('AUTO_Than_Long_da_tinh_nang_Pro_v4.6.exe', 'AUTO_Than_Long_da_tinh_nang_Pro_v4.7.exe'),
    ('"ProductVersion", "4.6\\0"', '"ProductVersion", "4.7\\0"'),
]:
    if old not in rc:
        raise SystemExit(f'resources/app.rc missing {old!r}')
    rc = rc.replace(old, new, 1)
write('resources/app.rc', rc)

replace_once(
    'src/controller.cpp',
    '        if (!a.profile.enableRevive) return false;\n',
    '        if (!a.profile.enableRevive && !a.dungeonOwned) return false;\n'
)

replace_once(
    'src/controller.cpp',
    '    std::set<std::uint32_t> postSellDone{};\n'
    '    // v4.5 persistent plan identity; queue runtime indices reset at START.\n',
    '    std::set<std::uint32_t> postSellDone{};\n'
    '    // v4.7: absolute next STEP index per participant-mask inside one active Parallel Group.\n'
    '    // Equal masks form one sequential lane; different non-overlapping masks run concurrently.\n'
    '    std::map<std::uint32_t, int> parallelLaneCursor{};\n'
    '    // v4.5 persistent plan identity; queue runtime indices reset at START.\n'
)

regex_once(
    'src/dungeon_v45_logic.h',
    r'inline bool ValidateParallelGroups\(const std::vector<cleanroute_dungeon::Step>& steps,\s*std::wstring& error\) \{.*?\n\}\n\ninline bool ValidateInheritedCoordinates',
    r'''inline bool ValidateParallelGroups(const std::vector<cleanroute_dungeon::Step>& steps,
                                   std::wstring& error) {
    int activeGroup = 0;
    std::vector<int> closed;
    std::vector<std::uint32_t> laneMasks;
    for (std::size_t i = 0; i < steps.size(); ++i) {
        const auto& step = steps[i];
        if (step.parallelGroup <= 0) {
            if (activeGroup > 0) closed.push_back(activeGroup);
            activeGroup = 0;
            laneMasks.clear();
            continue;
        }
        if (step.kind != cleanroute_dungeon::StepKind::Move) {
            error = L"STEP " + std::to_wstring(i + 1) + L" • Parallel chỉ hỗ trợ TỌA ĐỘ";
            return false;
        }
        if (step.parallelGroup != activeGroup) {
            if (activeGroup > 0) closed.push_back(activeGroup);
            if (std::find(closed.begin(), closed.end(), step.parallelGroup) != closed.end()) {
                error = L"Parallel Group " + std::to_wstring(step.parallelGroup) + L" phải nằm liền nhau";
                return false;
            }
            activeGroup = step.parallelGroup;
            laneMasks.clear();
        }
        const std::uint32_t mask = step.participantMask;
        bool sameLane = false;
        for (std::uint32_t existing : laneMasks) {
            if (existing == mask) { sameLane = true; break; }
            if ((existing & mask) != 0) {
                error = L"Parallel Group " + std::to_wstring(step.parallelGroup) +
                        L" có hai lane khác nhau dùng trùng Slot ACC";
                return false;
            }
        }
        if (!sameLane) laneMasks.push_back(mask);
    }
    return true;
}

inline bool ValidateInheritedCoordinates'''
)

replace_once(
    'src/dungeon_app_methods.inl',
    '        team.lastTaskDetail = L"TASK chưa đọc";\n'
    '        team.phaseTick = now;\n'
    '    }\n',
    '        team.lastTaskDetail = L"TASK chưa đọc";\n'
    '        team.parallelLaneCursor.clear();\n'
    '        team.phaseTick = now;\n'
    '    }\n'
)

replace_once(
    'src/dungeon_app_methods.inl',
    '''        if (!transitionPhase && !DungeonAllStable(team, error)) {
            if (DungeonTimeout(team, now, 30)) FailDungeonTeam(team, error);
            else team.status = L"CHỜ STATE • " + error;
            return;
        }
''',
    '''        if (!transitionPhase && !DungeonAllStable(team, error)) {
            bool lifeRecovery = false;
            for (std::uint32_t pid : team.config.pids) {
                Account* account = AccountByPid(pid);
                if (!account) continue;
                if ((account->snapshotValid && (account->snapshot.validMask & ValidLifeState) && account->snapshot.dead) ||
                    account->deathSessionLatched || account->runtime.revivePhase != 0) {
                    lifeRecovery = true;
                    break;
                }
            }
            if (lifeRecovery) {
                team.phaseTick = now;
                team.status = L"CHỜ ĐẦU THAI • giữ nguyên STEP/lane hiện tại";
                return;
            }
            if (DungeonTimeout(team, now, 30)) FailDungeonTeam(team, error);
            else team.status = L"CHỜ STATE • " + error;
            return;
        }
'''
)

regex_once(
    'src/dungeon_app_methods.inl',
    r'''        if \(team\.phase == cleanroute_dungeon::TeamPhase::WaitEnter\) \{\n            bool all = true;\n            for \(std::uint32_t pid : team\.config\.pids\) \{\n                Account& account = \*AccountByPid\(pid\);\n                if \(account\.snapshot\.mapID != preset->dungeonMap \|\| !account\.snapshot\.mapReady \|\|\n                    account\.snapshot\.waitingChangeMap\) all = false;\n            \}\n            if \(all\) \{\n                team\.phase = cleanroute_dungeon::TeamPhase::Steps;\n                team\.stepIndex = std::clamp\(team\.stepIndex, 0, static_cast<int>\(preset->steps\.size\(\)\) - 1\);\n                ResetDungeonStepRuntime\(team, now\);\n                team\.status = L"VÀO PB PASS • STEP " \+ std::to_wstring\(team\.stepIndex \+ 1\);\n            \} else if \(DungeonTimeout\(team, now, 60\)\) \{\n                FailDungeonTeam\(team, L"không chứng minh đủ acc vào dungeon map"\);\n            \}\n            return;\n        \}''',
    r'''        if (team.phase == cleanroute_dungeon::TeamPhase::WaitEnter) {
            bool all = true;
            int inside = 0;
            for (std::uint32_t pid : team.config.pids) {
                Account& account = *AccountByPid(pid);
                const bool entered = account.snapshot.mapID == preset->dungeonMap && account.snapshot.mapReady &&
                                     !account.snapshot.waitingChangeMap;
                if (entered) ++inside;
                else all = false;
            }
            if (all) {
                team.phase = cleanroute_dungeon::TeamPhase::Steps;
                team.stepIndex = std::clamp(team.stepIndex, 0, static_cast<int>(preset->steps.size()) - 1);
                ResetDungeonStepRuntime(team, now);
                team.status = L"VÀO PB PASS • STEP " + std::to_wstring(team.stepIndex + 1);
            } else {
                team.phaseTick = now;
                team.status = L"WAIT ENTER • " + std::to_wstring(inside) + L"/" +
                              std::to_wstring(team.config.pids.size()) + L" acc đã vào M" +
                              std::to_wstring(preset->dungeonMap);
            }
            return;
        }'''
)

regex_once(
    'src/dungeon_app_methods.inl',
    r'''        if \(team\.phase == cleanroute_dungeon::TeamPhase::WaitExit\) \{\n            bool allOut = true;\n            for \(std::uint32_t pid : team\.config\.pids\) \{\n                Account& account = \*AccountByPid\(pid\);\n                if \(account\.snapshot\.mapID == preset->dungeonMap\) allOut = false;\n            \}\n            if \(allOut\) \{\n                RecordDungeonDailyRun\(team, \*preset\);\n                DungeonV45NotifyRunComplete\(team, \*preset\);\n                team\.phase = cleanroute_dungeon::TeamPhase::PostSell;\n                team\.phaseTick = now;\n                team\.postSellDone\.clear\(\);\n                team\.status = L"ĐÃ RA MAP • check túi / Auto Sell từng acc";\n            \} else if \(DungeonTimeout\(team, now, 180\)\) \{\n                FailDungeonTeam\(team, L"đã hết step nhưng chưa chứng minh rời dungeon map"\);\n            \}\n            return;\n        \}''',
    r'''        if (team.phase == cleanroute_dungeon::TeamPhase::WaitExit) {
            bool allOut = true;
            int outside = 0;
            for (std::uint32_t pid : team.config.pids) {
                Account& account = *AccountByPid(pid);
                if (account.snapshot.mapID == preset->dungeonMap) allOut = false;
                else ++outside;
            }
            if (allOut) {
                RecordDungeonDailyRun(team, *preset);
                DungeonV45NotifyRunComplete(team, *preset);
                team.phase = cleanroute_dungeon::TeamPhase::PostSell;
                team.phaseTick = now;
                team.postSellDone.clear();
                team.status = L"ĐÃ RA MAP • check túi / Auto Sell từng acc";
            } else {
                team.phaseTick = now;
                team.status = L"WAIT EXIT • " + std::to_wstring(outside) + L"/" +
                              std::to_wstring(team.config.pids.size()) + L" acc đã rời M" +
                              std::to_wstring(preset->dungeonMap);
            }
            return;
        }'''
)

regex_once(
    'src/dungeon_app_methods.inl',
    r'''            if \(step\.parallelGroup > 0\) \{.*?\n            \}\n\n            if \(step\.kind == cleanroute_dungeon::StepKind::Move \|\|''',
    r'''            if (step.parallelGroup > 0) {
                const int group = step.parallelGroup;
                const int groupStart = team.stepIndex;
                int groupEnd = groupStart;
                while (groupEnd < static_cast<int>(preset->steps.size()) &&
                       preset->steps[static_cast<std::size_t>(groupEnd)].parallelGroup == group) {
                    ++groupEnd;
                }

                std::vector<std::uint32_t> laneMasks;
                for (int i = groupStart; i < groupEnd; ++i) {
                    const auto mask = preset->steps[static_cast<std::size_t>(i)].participantMask;
                    if (std::find(laneMasks.begin(), laneMasks.end(), mask) == laneMasks.end())
                        laneMasks.push_back(mask);
                }

                bool groupDone = true;
                for (std::uint32_t laneMask : laneMasks) {
                    int& cursor = team.parallelLaneCursor[laneMask];
                    auto firstForMask = [&]() {
                        for (int i = groupStart; i < groupEnd; ++i)
                            if (preset->steps[static_cast<std::size_t>(i)].participantMask == laneMask) return i;
                        return groupEnd;
                    };
                    if (cursor < groupStart || cursor >= groupEnd ||
                        preset->steps[static_cast<std::size_t>(cursor)].participantMask != laneMask) {
                        cursor = firstForMask();
                    }
                    if (cursor >= groupEnd) continue;

                    const cleanroute_dungeon::Step& lane = preset->steps[static_cast<std::size_t>(cursor)];
                    std::vector<Account*> laneMembers = DungeonStepMembers(team, lane);
                    if (laneMembers.empty()) {
                        FailDungeonTeam(team, L"Parallel lane không có participant hợp lệ");
                        return;
                    }

                    TargetProfile target{};
                    target.name = lane.label; target.mapID = lane.mapID;
                    target.x = lane.x; target.y = lane.y; target.valid = true;
                    bool laneArrived = true;
                    for (Account* account : laneMembers) {
                        bool arrived = false;
                        HandleRobustTravel(*account, now, target, lane.label.c_str(), arrived, lane.tolerance);
                        if (!arrived) laneArrived = false;
                    }
                    if (!laneArrived) {
                        groupDone = false;
                        continue;
                    }
                    if (lane.autoFightOnArrival && !DungeonEnsureAutoFightOn(team, laneMembers, lane.label)) {
                        groupDone = false;
                        continue;
                    }

                    int next = groupEnd;
                    for (int i = cursor + 1; i < groupEnd; ++i) {
                        if (preset->steps[static_cast<std::size_t>(i)].participantMask == laneMask) {
                            next = i;
                            break;
                        }
                    }
                    cursor = next;
                    if (cursor < groupEnd) groupDone = false;
                }

                team.status = L"SONG SONG P" + std::to_wstring(group) +
                              L" • mỗi lane chạy tuần tự waypoint riêng";
                if (!groupDone) return;
                team.stepIndex = groupEnd;
                ResetDungeonStepRuntime(team, now);
                team.status = L"PASS SONG SONG P" + std::to_wstring(group) + L" • STEP " +
                              std::to_wstring(team.stepIndex + 1);
                return;
            }

            if (step.kind == cleanroute_dungeon::StepKind::Move ||'''
)

replace_once(
    'src/dungeon_activity_bridge.inl',
    '''            if (c < 0 || t <= 0 || c > 100000000 || t > 100000000) continue;
            std::wstring prefix = text.substr(0, left);
            while (!prefix.empty() && (iswspace(prefix.back()) || prefix.back() == L':' || prefix.back() == L'-')) prefix.pop_back();
            const bool hasAlpha = std::any_of(prefix.begin(), prefix.end(), [](wchar_t ch) { return iswalpha(ch) != 0; });
            if (!hasAlpha || prefix.size() < 2) continue;
            label = prefix; current = static_cast<int>(c); target = static_cast<int>(t); return true;
''',
    '''            if (c < 0 || t <= 0 || c > t || c > 100000000 || t > 100000000) continue;
            std::wstring prefix = text.substr(0, left);
            while (!prefix.empty() && (iswspace(prefix.back()) || prefix.back() == L':' || prefix.back() == L'-')) prefix.pop_back();
            const bool hasAlpha = std::any_of(prefix.begin(), prefix.end(), [](wchar_t ch) { return iswalpha(ch) != 0; });
            if (!hasAlpha || prefix.size() < 2) continue;
            const std::wstring key = background_ui_logic::Key(prefix);
            const wchar_t* banned[] = {L"text",L"recttransform",L"transform",L"color",L"anchoredposition",
                                       L"sizedelta",L"font",L"material",L"alpha",L"pivot",L"rotation",L"scale"};
            bool propertyRow = false;
            for (const wchar_t* item : banned) {
                if (key == item || key.rfind(item, 0) == 0) { propertyRow = true; break; }
            }
            if (propertyRow) continue;
            label = prefix; current = static_cast<int>(c); target = static_cast<int>(t); return true;
'''
)

regex_once(
    'src/dungeon_v45_methods.inl',
    r'''    void RefreshDungeonActivityBoard\(bool force = false, DWORD now = GetTickCount\(\)\) \{.*?\n    \}\n\n    static LRESULT CALLBACK DungeonManagerWndProc''',
    r'''    void RefreshDungeonActivityBoard(bool force = false, DWORD now = GetTickCount()) {
        if (!dungeonActivityWindow_ || !IsWindow(dungeonActivityWindow_) || !IsWindowVisible(dungeonActivityWindow_)) return;
        if (!force && !Elapsed(now, dungeonActivityLastTick_, 1000)) return;
        dungeonActivityLastTick_ = now;

        const int teamIndex = SelectedDungeonTeamIndex();
        if (teamIndex < 0 || teamIndex >= static_cast<int>(dungeonTeams_.size())) {
            if (dungeonActivityList_) ListView_DeleteAllItems(dungeonActivityList_);
            SetText(dungeonActivityTitle_, L"CHƯA ĐỒNG BỘ • chọn một tổ đội");
            SetText(dungeonActivityStatus_, L"Không có authority client để đọc bảng hoạt động");
            return;
        }
        DungeonTeamRuntime& team = dungeonTeams_[static_cast<std::size_t>(teamIndex)];
        cleanroute_dungeon::Preset* preset = DungeonPresetForTeam(team);
        Account* authority = AccountByPid(team.config.leaderPid);
        if (!preset || !authority) {
            SetText(dungeonActivityTitle_, L"CHƯA ĐỒNG BỘ • thiếu preset/KEY");
            return;
        }

        struct CachedActivity {
            DungeonActivitySnapshot board{};
            bool valid = false;
        };
        static std::map<int, CachedActivity> lastGood;

        auto render = [&](const DungeonActivitySnapshot& board, bool cached, const std::wstring& extra) {
            if (dungeonActivityList_) ListView_DeleteAllItems(dungeonActivityList_);
            std::wstring title = board.activityName[0] ? board.activityName : preset->name;
            if (board.remainingSeconds >= 0) {
                const int mm = board.remainingSeconds / 60, ss = board.remainingSeconds % 60;
                wchar_t timeText[32]{};
                swprintf_s(timeText, L" • Thời gian còn %02d:%02d", mm, ss);
                title += timeText;
            }
            SetText(dungeonActivityTitle_, title + (cached ? L" • CACHED GẦN NHẤT" : L" • ĐÃ ĐỒNG BỘ"));
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
            SetText(dungeonActivityStatus_,
                    std::wstring(cached ? L"CACHED • giữ snapshot LIVE gần nhất • " : L"LIVE • ") +
                    L"nguồn=" + std::wstring(board.source) +
                    L" • Map " + std::to_wstring(board.mapID) +
                    L" • objectives=" + std::to_wstring(board.objectiveCount) +
                    (extra.empty() ? L"" : L" • " + extra));
        };

        Response response{}; std::wstring error;
        const bool ok = preset->dungeonMap > 0 &&
            authority->bridge.Call(Command::ReadDungeonActivityBoard,
                                   preset->dungeonMap, 0, 0, response, error, 1000);
        if (ok && response.dungeonActivity.synchronized && response.dungeonActivity.objectiveCount > 0) {
            lastGood[team.config.id].board = response.dungeonActivity;
            lastGood[team.config.id].valid = true;
            render(response.dungeonActivity, false, L"");
            return;
        }

        const auto it = lastGood.find(team.config.id);
        if (it != lastGood.end() && it->second.valid) {
            render(it->second.board, true, error.empty() ? std::wstring(response.detail) : error);
            return;
        }

        if (dungeonActivityList_) ListView_DeleteAllItems(dungeonActivityList_);
        SetText(dungeonActivityTitle_, L"CHƯA ĐỒNG BỘ ĐƯỢC BẢNG HOẠT ĐỘNG");
        SetText(dungeonActivityStatus_, L"FAIL-CLOSED • không dùng scanner/STEP thay thế • " +
                                         (error.empty() ? std::wstring(response.detail) : error));
    }

    static LRESULT CALLBACK DungeonManagerWndProc'''
)

replace_once(
    'src/dungeon_v45_methods.inl',
    '''        message += L"Tiếp theo: kiểm tra túi / dùng chung Auto Sell nếu cần\\nThời gian: " + LocalDateTimeText();
''',
    '''        std::wstring nextText = L"HOÀN THÀNH TOÀN BỘ KẾ HOẠCH";
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
        message += L"Tiếp theo: " + nextText + L"\\nThời gian: " + LocalDateTimeText();
'''
)

presets = read('src/dungeon_presets.h')

def replace_preset(pattern, replacement):
    global presets
    out, count = re.subn(pattern, replacement, presets, count=1, flags=re.S)
    if count != 1:
        raise SystemExit(f'dungeon_presets.h preset replace failed: {pattern[:120]}')
    presets = out

replace_preset(
    r' \{auto p=Base\(L"Q2_ToChau",L"Trúc Lâm",94,4,707,11300,9890,L"Trừ hại"\);.*?a\.push_back\(std::move\(p\)\);\}',
    r''' {auto p=Base(L"Q2_ToChau",L"Trúc Lâm",94,4,707,11300,9890,L"Trừ hại");
  const int x[]={1130,1793,3567,3349,2168,822}; const int y[]={659,1128,1384,2099,2005,1882};
  for(int i=0;i<6;++i){p.steps.push_back(M((L"TRÚC LÂM • KHU "+std::to_wstring(i+1)).c_str(),94,x[i],y[i]));
    auto f=F((L"QUÉT SẠCH KHU "+std::to_wstring(i+1)).c_str(),L"",900);f.mapID=94;f.x=x[i];f.y=y[i];f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  p.steps.push_back(M(L"TRÚC LÂM • MOVE ONLY",94,3279,2774));
  p.steps.push_back(M(L"TRÚC LÂM • KHU CUỐI",94,1255,2791));
  {auto f=F(L"QUÉT SẠCH KHU CUỐI",L"",900);f.mapID=94;f.x=1255;f.y=2791;f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  a.push_back(std::move(p));}'''
)

replace_preset(
    r' \{auto p=Base\(L"Q3_ToChau",L"Dã Ngoại Trại Phỉ",95,4,674,4500,8190,L"Doanh trại của phỉ"\);.*?a\.push_back\(std::move\(p\)\);\}',
    r''' {auto p=Base(L"Q3_ToChau",L"Dã Ngoại Trại Phỉ",95,4,674,4500,8190,L"Doanh trại của phỉ");
  constexpr std::uint32_t S12=(1u<<0)|(1u<<1); constexpr std::uint32_t S3456=(1u<<2)|(1u<<3)|(1u<<4)|(1u<<5);
  p.steps.push_back(M(L"DÃ NGOẠI P1 • 1-2",95,1035,2145,120,S12,11));
  p.steps.push_back(M(L"DÃ NGOẠI P1 • 3-6",95,2112,3040,120,S3456,11));
  p.steps.push_back(M(L"DÃ NGOẠI P2 • 1-2",95,1444,1393,120,S12,12));
  p.steps.push_back(M(L"DÃ NGOẠI P2 • 3-6",95,2843,2493,120,S3456,12));
  p.steps.push_back(M(L"DÃ NGOẠI • HỘI QUÂN CUỐI",95,2039,2099));
  {auto f=F(L"DÃ NGOẠI • QUÉT SẠCH CUỐI",L"",1000);f.mapID=95;f.x=2039;f.y=2099;f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  a.push_back(std::move(p));}'''
)

replace_preset(
    r' \{auto p=Base\(L"Q1_LauLan",L"Hoàng Kim Chi Liên",108,5,385,9400,8000,L"Hoàng Kim Chi Liên"\);.*?a\.push_back\(std::move\(p\)\);\}',
    r''' {auto p=Base(L"Q1_LauLan",L"Hoàng Kim Chi Liên",108,5,385,9400,8000,L"Hoàng Kim Chi Liên");
  constexpr std::uint32_t S12=(1u<<0)|(1u<<1); constexpr std::uint32_t S3456=(1u<<2)|(1u<<3)|(1u<<4)|(1u<<5);
  p.steps.push_back(M(L"HKCL P1 • 1-2 • A1",108,9394,3009,120,S12,21));
  p.steps.push_back(M(L"HKCL P1 • 1-2 • A2",108,4598,3028,120,S12,21));
  p.steps.push_back(M(L"HKCL P1 • 3-6 • B1",108,4029,1447,120,S3456,21));
  p.steps.push_back(M(L"HKCL P1 • 3-6 • B2",108,4691,1476,120,S3456,21));
  p.steps.push_back(M(L"HKCL P1 • 3-6 • B3",108,5129,2263,120,S3456,21));
  p.steps.push_back(M(L"HKCL • HỘI QUÂN 1",108,4316,2283));
  {auto f=F(L"HKCL • QUÉT SẠCH 1",L"",1000);f.mapID=108;f.x=4316;f.y=2283;f.matchAnyVerified=true;p.steps.push_back(std::move(f));}

  p.steps.push_back(M(L"HKCL P2 • 1-2 • A1",108,6513,6102,120,S12,22));
  p.steps.push_back(M(L"HKCL P2 • 1-2 • A2",108,5603,6612,120,S12,22));
  p.steps.push_back(M(L"HKCL P2 • 3-6 • B1",108,5316,5373,120,S3456,22));
  p.steps.push_back(M(L"HKCL P2 • 3-6 • B2",108,4935,6028,120,S3456,22));
  p.steps.push_back(M(L"HKCL • HỘI QUÂN 2",108,5712,6114));
  {auto f=F(L"HKCL • QUÉT SẠCH 2",L"",1000);f.mapID=108;f.x=5712;f.y=6114;f.matchAnyVerified=true;p.steps.push_back(std::move(f));}

  p.steps.push_back(M(L"HKCL P3 • 1-2 • A1",108,3363,4978,120,S12,23));
  p.steps.push_back(M(L"HKCL P3 • 1-2 • A2",108,1888,4954,120,S12,23));
  p.steps.push_back(M(L"HKCL P3 • 3-6 • B1",108,3499,6425,120,S3456,23));
  p.steps.push_back(M(L"HKCL P3 • 3-6 • B2",108,1755,6531,120,S3456,23));
  const int hx[]={2601,2120,5344,6330}; const int hy[]={5796,6858,2880,6655};
  for(int i=0;i<4;++i){p.steps.push_back(M((L"HKCL • HỘI/ĐÁNH "+std::to_wstring(i+3)).c_str(),108,hx[i],hy[i]));
    auto f=F((L"HKCL • QUÉT SẠCH "+std::to_wstring(i+3)).c_str(),L"",1000);f.mapID=108;f.x=hx[i];f.y=hy[i];f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  a.push_back(std::move(p));}'''
)

replace_preset(
    r' \{auto p=Base\(L"Q2_LauLan",L"Huyền Phật Châu",109,5,385,9400,8000,L"Huyền Phật Châu"\);.*?a\.push_back\(std::move\(p\)\);\}',
    r''' {auto p=Base(L"Q2_LauLan",L"Huyền Phật Châu",109,5,385,9400,8000,L"Huyền Phật Châu");
  constexpr std::uint32_t S12=(1u<<0)|(1u<<1); constexpr std::uint32_t S3456=(1u<<2)|(1u<<3)|(1u<<4)|(1u<<5);
  p.steps.push_back(M(L"HPC P1 • 1-2",109,5784,2927,120,S12,31));
  p.steps.push_back(M(L"HPC P1 • 3-6",109,4377,1923,120,S3456,31));
  p.steps.push_back(M(L"HPC • HỘI/ĐÁNH 1",109,5262,2414));
  {auto f=F(L"HPC • QUÉT SẠCH 1",L"",1000);f.mapID=109;f.x=5262;f.y=2414;f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  p.steps.push_back(M(L"HPC P2 • 1-2",109,2654,5796,120,S12,32));
  p.steps.push_back(M(L"HPC P2 • 3-6",109,2309,3825,120,S3456,32));
  const int x[]={2332,5788,5881,4310,2322,2601,4040}; const int y[]={4713,5942,2901,1906,3831,5789,4579};
  for(int i=0;i<7;++i){p.steps.push_back(M((L"HPC • HỘI/ĐÁNH "+std::to_wstring(i+2)).c_str(),109,x[i],y[i]));
    auto f=F((L"HPC • QUÉT SẠCH "+std::to_wstring(i+2)).c_str(),L"",1000);f.mapID=109;f.x=x[i];f.y=y[i];f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  a.push_back(std::move(p));}'''
)

replace_preset(
    r' \{auto p=Base\(L"Q3_LauLan",L"Dung Nham Chi Địa",110,5,386,4000,8500,L"Dung Nham Chi Địa"\);.*?a\.push_back\(std::move\(p\)\);\}',
    r''' {auto p=Base(L"Q3_LauLan",L"Dung Nham Chi Địa",110,5,386,4000,8500,L"Dung Nham Chi Địa");
  const int x[]={6637,6724,5849,5803,4909,4760,3391,2270,1474,2308,2586,1741,1730,2691,2790,1689,1649,2852,4979,4831,5170,6402,6537,6569,5273,4645,1729};
  const int y[]={1830,2788,2887,1948,1861,2931,1385,1525,1625,2980,4088,4184,5159,5134,6164,6176,6904,6920,5858,5076,4451,4584,5724,6917,6670,6327,6868};
  for(int i=0;i<27;++i){p.steps.push_back(M((L"DUNG NHAM • ĐIỂM "+std::to_wstring(i+1)).c_str(),110,x[i],y[i]));
    auto f=F((L"DUNG NHAM • QUÉT SẠCH "+std::to_wstring(i+1)).c_str(),L"",950);f.mapID=110;f.x=x[i];f.y=y[i];f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  a.push_back(std::move(p));}'''
)

insert_before = ' {auto p=Base(L"SatTinh",L"Thập Nhị Sát Tinh",111,2,41,4170,7840,L"Khiêu chiến Thập Nhị Sát Tinh");'
if insert_before not in presets:
    raise SystemExit('dungeon_presets.h: SatTinh insertion anchor missing')
placeholder = r''' {auto p=Base(L"AcTacTaoPhan",L"Ác Tặc Tạo Phản • CHỜ ENTRY",0,0,0,0,0,L"");
  const int x[]={3127,3318,2243,1065,1345,708}; const int y[]={1548,3018,3399,3276,1808,1032};
  for(int i=0;i<6;++i){p.steps.push_back(M((L"ÁC TẶC • STEP "+std::to_wstring(i+1)).c_str(),0,x[i],y[i]));
    auto f=F((L"ÁC TẶC • QUÉT SẠCH "+std::to_wstring(i+1)).c_str(),L"",900);f.mapID=0;f.x=x[i];f.y=y[i];f.matchAnyVerified=true;p.steps.push_back(std::move(f));}
  a.push_back(std::move(p));}
'''
presets = presets.replace(insert_before, placeholder + insert_before, 1)
write('src/dungeon_presets.h', presets)

changelog = read('CHANGELOG.md')
header = '''# v4.7 — approved DOCX dungeon routes and runtime barriers

- Hoàng Kim Chi Liên: approved `9394,3009`; multi-waypoint parallel lanes for Slot 1-2 vs 3-6.
- Huyền Phật Châu: approved `5881,2901`.
- Trúc Lâm: `3279,2774` is MOVE ONLY.
- Dã Ngoại Trại Phỉ: ALL -> `2039,2099` -> AutoFight -> verified GMonster area-clear.
- Dung Nham Chi Địa: full 27-point move/clear route.
- Ác Tặc Tạo Phản: STEP coordinates are present as a non-startable placeholder; NPC/entry/map remain intentionally blank.
- WAIT ENTER / WAIT EXIT no longer auto-STOP on time alone.
- Dungeon death is a local revive/resume pause and does not consume STEP timeout.
- Activity board filters Unity property rows and keeps the last good LIVE counters as CACHED.
- Telegram per-run completion names the actual next run/dungeon.
- Parallel Group now supports sequential waypoints inside each participant lane.

'''
if not changelog.startswith('# v4.7'):
    write('CHANGELOG.md', header + changelog)

print('v4.7 migration applied')
