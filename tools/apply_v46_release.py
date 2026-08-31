from pathlib import Path
import re
import sys

ROOT = Path('.')


def read(path):
    return (ROOT / path).read_text(encoding='utf-8-sig')


def write(path, text):
    (ROOT / path).write_text(text, encoding='utf-8')


def replace_once(path, old, new):
    text = read(path)
    if old not in text:
        if new in text:
            return False
        raise SystemExit(f'{path}: missing expected text: {old[:120]!r}')
    if text.count(old) != 1:
        raise SystemExit(f'{path}: expected one match, found {text.count(old)} for {old[:120]!r}')
    write(path, text.replace(old, new, 1))
    return True


def replace_all(path, old, new, minimum=1):
    text = read(path)
    count = text.count(old)
    if count < minimum:
        if new in text:
            return False
        raise SystemExit(f'{path}: expected >= {minimum} matches for {old!r}, found {count}')
    write(path, text.replace(old, new))
    return True


def regex_once(path, pattern, replacement, flags=re.S):
    text = read(path)
    out, count = re.subn(pattern, replacement, text, count=1, flags=flags)
    if count != 1:
        raise SystemExit(f'{path}: regex expected one match, found {count}: {pattern[:120]!r}')
    write(path, out)


version = read('VERSION.txt').strip()
if version == '4.6':
    required = {
        'src/dungeon_logic.h': ['parallelGroup', 'autoFightOnArrival'],
        'src/dungeon_app_methods.inl': ['SONG SONG P', 'StartDungeonSelectedFromStep', 'OpenDungeonMonsterCatalog'],
        'src/dungeon_activity_bridge.inl': ['ratioSlash'],
        'src/dungeon_presets.h': ['TÁCH 1-2 • điểm A', 'HỘI QUÂN CUỐI'],
    }
    for p, needles in required.items():
        t = read(p)
        for needle in needles:
            if needle not in t:
                raise SystemExit(f'v4.6 marker missing: {p}: {needle}')
    print('v4.6 patch already applied; nothing to do')
    raise SystemExit(0)
if version != '4.5':
    raise SystemExit(f'Expected VERSION 4.5 before migration, got {version!r}')

write('VERSION.txt', '4.6\n')
replace_once('CMakeLists.txt', 'project(ThanLongItemConsolidator VERSION 4.5',
             'project(ThanLongItemConsolidator VERSION 4.6')
rc = read('resources/app.rc')
for old, new in [
    ('FILEVERSION 4,5,0,0', 'FILEVERSION 4,6,0,0'),
    ('PRODUCTVERSION 4,5,0,0', 'PRODUCTVERSION 4,6,0,0'),
    ('"FileVersion", "4.5\\0"', '"FileVersion", "4.6\\0"'),
    ('AUTO_Than_Long_da_tinh_nang_Pro_v4.5.exe', 'AUTO_Than_Long_da_tinh_nang_Pro_v4.6.exe'),
    ('"ProductVersion", "4.5\\0"', '"ProductVersion", "4.6\\0"'),
]:
    if old not in rc:
        raise SystemExit(f'app.rc missing {old}')
    rc = rc.replace(old, new, 1)
write('resources/app.rc', rc)
replace_once('src/protocol.h', 'constexpr std::uint32_t kProtocolVersion = 0x00040500u;',
             'constexpr std::uint32_t kProtocolVersion = 0x00040600u;')
replace_once('src/controller.cpp', 'constexpr wchar_t kTitle[] = L"AUTO Thần Long đa tính năng Pro v4.5";',
             'constexpr wchar_t kTitle[] = L"AUTO Thần Long đa tính năng Pro v4.6";')

replace_once('src/dungeon_logic.h',
'''    // Bit 0..5 correspond to the six ordered slots inside the team.\n    std::uint32_t participantMask = kAllParticipantsMask;\n};''',
'''    // Bit 0..5 correspond to the six ordered slots inside the team. Slot 1 is always KEY.\n    std::uint32_t participantMask = kAllParticipantsMask;\n    // v4.6: consecutive MOVE steps with the same positive group run concurrently.\n    int parallelGroup = 0;\n    // v4.6: after this MOVE has arrived, keep the lane there and verify AutoFight ON.\n    bool autoFightOnArrival = false;\n};''')
replace_once('src/dungeon_logic.h',
'''    if (step.kind == StepKind::Fight && step.monsterResID <= 0 &&\n        step.monsterName.empty() && !step.matchAnyVerified) {\n        error = L"STEP đánh quái phải có Monster/ResID hoặc bật Match ANY GMonster";\n        return false;\n    }\n    if (step.participantMask == 0) {''',
'''    // v4.6: FIGHT may intentionally leave Monster/ResID blank. Runtime completion is\n    // authoritative area-clear (verified live GMonster presence), not a name filter.\n    if (step.parallelGroup < 0 || step.parallelGroup > 255) {\n        error = L"Parallel Group phải 0–255";\n        return false;\n    }\n    if (step.parallelGroup > 0 && step.kind != StepKind::Move) {\n        error = L"Parallel Group v4.6 chỉ áp dụng cho STEP TỌA ĐỘ";\n        return false;\n    }\n    if (step.autoFightOnArrival && step.kind != StepKind::Move) {\n        error = L"AutoFight khi đến chỉ áp dụng cho STEP TỌA ĐỘ";\n        return false;\n    }\n    if (step.participantMask == 0) {''')

replace_once('src/dungeon_v45_logic.h',
'''inline bool ValidateInheritedCoordinates(const std::vector<cleanroute_dungeon::Step>& steps,\n                                         std::wstring& error) {''',
'''inline bool ValidateParallelGroups(const std::vector<cleanroute_dungeon::Step>& steps,\n                                   std::wstring& error) {\n    int activeGroup = 0;\n    std::uint32_t usedMask = 0;\n    std::vector<int> closed;\n    for (std::size_t i = 0; i < steps.size(); ++i) {\n        const auto& step = steps[i];\n        if (step.parallelGroup <= 0) {\n            if (activeGroup > 0) closed.push_back(activeGroup);\n            activeGroup = 0; usedMask = 0;\n            continue;\n        }\n        if (step.kind != cleanroute_dungeon::StepKind::Move) {\n            error = L"STEP " + std::to_wstring(i + 1) + L" • Parallel chỉ hỗ trợ TỌA ĐỘ";\n            return false;\n        }\n        if (step.parallelGroup != activeGroup) {\n            if (activeGroup > 0) closed.push_back(activeGroup);\n            if (std::find(closed.begin(), closed.end(), step.parallelGroup) != closed.end()) {\n                error = L"Parallel Group " + std::to_wstring(step.parallelGroup) + L" phải nằm liền nhau";\n                return false;\n            }\n            activeGroup = step.parallelGroup; usedMask = 0;\n        }\n        if ((usedMask & step.participantMask) != 0) {\n            error = L"Parallel Group " + std::to_wstring(step.parallelGroup) +\n                    L" có cùng slot ACC ở hai tọa khác nhau";\n            return false;\n        }\n        usedMask |= step.participantMask;\n    }\n    return true;\n}\n\ninline bool ValidateInheritedCoordinates(const std::vector<cleanroute_dungeon::Step>& steps,\n                                         std::wstring& error) {''')

replace_once('src/dungeon_presets.h',
'''inline Step M(const wchar_t*l,int map,int x,int y,int tol=120){Step s{};s.kind=StepKind::Move;s.label=l;s.mapID=map;s.x=x;s.y=y;s.tolerance=tol;s.timeoutSec=120;return s;}''',
'''inline Step M(const wchar_t*l,int map,int x,int y,int tol=120,\n              std::uint32_t mask=kAllParticipantsMask,int parallel=0,bool fightOnArrival=false){\n    Step s{};s.kind=StepKind::Move;s.label=l;s.mapID=map;s.x=x;s.y=y;s.tolerance=tol;s.timeoutSec=120;\n    s.participantMask=mask;s.parallelGroup=parallel;s.autoFightOnArrival=fightOnArrival;return s;\n}''')
regex_once('src/dungeon_presets.h',
    r' \{auto p=Base\(L"Q1_ToChau",L"Biên Giới Tống Liêu",93,4,674,4400,8090,L"Một tên cũng không thể thoát"\);.*?a\.push_back\(std::move\(p\)\);\}',
''' {auto p=Base(L"Q1_ToChau",L"Biên Giới Tống Liêu",93,4,674,4400,8090,L"Một tên cũng không thể thoát");\n  constexpr std::uint32_t S12=(1u<<0)|(1u<<1);\n  constexpr std::uint32_t S3456=(1u<<2)|(1u<<3)|(1u<<4)|(1u<<5);\n  p.steps.push_back(M(L"HỘI QUÂN ĐẦU",93,1765,2665));\n  p.steps.push_back(M(L"TÁCH 1-2 • điểm A",93,1110,2456,120,S12,1));\n  p.steps.push_back(M(L"TÁCH 3-6 • điểm A",93,3072,2314,120,S3456,1));\n  p.steps.push_back(M(L"TÁCH 1-2 • điểm B",93,1163,1759,120,S12,2));\n  p.steps.push_back(M(L"TÁCH 3-6 • điểm B",93,2977,1656,120,S3456,2));\n  p.steps.push_back(M(L"HỘI QUÂN ĐÁNH 1",93,2213,1665));\n  {auto f=F(L"QUÉT SẠCH KHU HỘI QUÂN 1",L"",900);f.mapID=93;f.x=2213;f.y=1665;f.matchAnyVerified=true;p.steps.push_back(std::move(f));}\n  p.steps.push_back(M(L"TÁCH 1-2 • đánh 10s",93,1279,1627,120,S12,3,true));\n  p.steps.push_back(M(L"TÁCH 3-6 • đánh 10s",93,2503,1425,120,S3456,3,true));\n  p.steps.push_back(W(L"ĐÁNH TÁCH 10 GIÂY",10));\n  p.steps.push_back(M(L"HỘI QUÂN CUỐI",93,2071,1746));\n  {auto f=F(L"QUÉT SẠCH KHU CUỐI",L"",900);f.mapID=93;f.x=2071;f.y=1746;f.matchAnyVerified=true;p.steps.push_back(std::move(f));}\n  a.push_back(std::move(p));}''')

regex_once('src/dungeon_activity_bridge.inl',
    r'void DungeonActivityAppendTokens\(const std::wstring& raw, std::vector<std::wstring>& tokens\) \{.*?\n\}',
'''void DungeonActivityAppendTokens(const std::wstring& raw, std::vector<std::wstring>& tokens) {\n    if (raw.empty()) return;\n    std::wstring part;\n    auto flush = [&]() {\n        std::wstring token = DungeonActivityStripMarkup(part);\n        if (!token.empty()) tokens.push_back(std::move(token));\n        part.clear();\n    };\n    for (std::size_t i = 0; i < raw.size(); ++i) {\n        const wchar_t ch = raw[i];\n        const bool ratioSlash = ch == L'/' && i > 0 && i + 1 < raw.size() &&\n                                iswdigit(raw[i - 1]) && iswdigit(raw[i + 1]);\n        if (ch == L'/' && !ratioSlash) { flush(); continue; }\n        part.push_back(ch);\n    }\n    flush();\n}''')
replace_once('src/dungeon_activity_bridge.inl',
'''            const std::wstring key = background_ui_logic::Key(prefix);\n            const bool semantic = key.find(L"tieudiet") != std::wstring::npos ||\n                                  key.find(L"danhbai") != std::wstring::npos ||\n                                  key.find(L"hoanthanh") != std::wstring::npos ||\n                                  key.find(L"thuthap") != std::wstring::npos ||\n                                  key.find(L"baove") != std::wstring::npos ||\n                                  key.find(L"diet") != std::wstring::npos;\n            if (!semantic || prefix.size() < 3) continue;''',
'''            const bool hasAlpha = std::any_of(prefix.begin(), prefix.end(), [](wchar_t ch) { return iswalpha(ch) != 0; });\n            if (!hasAlpha || prefix.size() < 2) continue;''')

replace_once('src/controller.cpp',
'''constexpr int IDC_DG_IMPORT_ALL=726;\n// Dungeon preset editor window (740-779 reserved).''',
'''constexpr int IDC_DG_IMPORT_ALL=726;\nconstexpr int IDC_DG_START_FROM_STEP=727;\nconstexpr int IDC_DG_MONSTER_CATALOG=728;\n// Dungeon preset editor window (740-779 reserved).''')
replace_once('src/controller.cpp',
'''constexpr int IDC_DGE_SAVE_HEADER=777;\n\nconstexpr int IDC_PK_START = 400;''',
'''constexpr int IDC_DGE_SAVE_HEADER=777;\nconstexpr int IDC_DGE_PARALLEL=778;\nconstexpr int IDC_DGE_FIGHT_ON_ARRIVAL=779;\n\nconstexpr int IDC_PK_START = 400;''')
replace_once('src/controller.cpp',
'''constexpr int IDC_DGM_QUEUE_DOWN=791;\n\nconstexpr int IDC_DGE_LIST=740;''',
'''constexpr int IDC_DGM_QUEUE_DOWN=791;\nconstexpr int IDC_DGC_DELETE=792;\nconstexpr int IDC_DGC_CLOSE=793;\n\nconstexpr int IDC_DGE_LIST=740;''')
replace_once('src/controller.cpp',
'''    HWND dungeonMonsterStatus_=nullptr;\n    std::vector<MonsterRecord> dungeonLastScan_{};''',
'''    HWND dungeonMonsterStatus_=nullptr;\n    HWND dungeonMonsterCatalogWindow_=nullptr;\n    HWND dungeonMonsterCatalogList_=nullptr;\n    HWND dungeonMonsterCatalogStatus_=nullptr;\n    std::vector<MonsterRecord> dungeonLastScan_{};''')
replace_once('src/controller.cpp',
'''    HWND dungeonEditorBoss_=nullptr;\n    HWND dungeonEditorAny_=nullptr;\n    std::array<HWND,6> dungeonEditorSlots_{};''',
'''    HWND dungeonEditorBoss_=nullptr;\n    HWND dungeonEditorAny_=nullptr;\n    HWND dungeonEditorParallel_=nullptr;\n    HWND dungeonEditorFightOnArrival_=nullptr;\n    std::array<HWND,6> dungeonEditorSlots_{};''')
replace_once('src/controller.cpp',
'''                    case IDC_DG_START: StartDungeonSelected(); break;\n                    case IDC_DG_PAUSE: PauseResumeDungeonSelected(); break;''',
'''                    case IDC_DG_START: StartDungeonSelected(); break;\n                    case IDC_DG_START_FROM_STEP: StartDungeonSelectedFromStep(); break;\n                    case IDC_DG_MONSTER_CATALOG: OpenDungeonMonsterCatalog(); break;\n                    case IDC_DG_PAUSE: PauseResumeDungeonSelected(); break;''')

replace_once('src/dungeon_app_methods.inl',
'''                step.timeoutSec = ClampDungeonSetting(ReadIniInt(section, prefix + L"Timeout", 180), 1, 86400, 180);\n                step.monsterName = ReadIniText(section, prefix + L"Monster");''',
'''                step.timeoutSec = ClampDungeonSetting(ReadIniInt(section, prefix + L"Timeout", 180), 1, 86400, 180);\n                step.parallelGroup = ClampDungeonSetting(ReadIniInt(section, prefix + L"Parallel", 0), 0, 255, 0);\n                step.autoFightOnArrival = ReadIniInt(section, prefix + L"FightOnArrival", 0) != 0;\n                step.monsterName = ReadIniText(section, prefix + L"Monster");''')
replace_once('src/dungeon_app_methods.inl',
'''            WriteIniInt(section, prefix + L"Timeout", step.timeoutSec);\n            WriteIniText(section, prefix + L"Monster", step.monsterName);''',
'''            WriteIniInt(section, prefix + L"Timeout", step.timeoutSec);\n            WriteIniInt(section, prefix + L"Parallel", step.parallelGroup);\n            WriteIniInt(section, prefix + L"FightOnArrival", step.autoFightOnArrival ? 1 : 0);\n            WriteIniText(section, prefix + L"Monster", step.monsterName);''')
replace_once('src/dungeon_app_methods.inl',
'''            if (!invalid && !loaded.empty()) candidate.steps = std::move(loaded);\n            if (!invalid && candidate.dungeonMap > 0 && candidate.gatherMap > 0 && candidate.npcResID > 0) {''',
'''            if (!invalid && !loaded.empty()) {\n                std::wstring parallelError;\n                if (!dungeon_v45::ValidateParallelGroups(loaded, parallelError)) invalid = true;\n                else candidate.steps = std::move(loaded);\n            }\n            if (!invalid && candidate.dungeonMap > 0 && candidate.gatherMap > 0 && candidate.npcResID > 0) {''')
replace_once('src/dungeon_app_methods.inl',
'''            for (int m = 0; m < members; ++m) {\n                const int roleID = ReadIniInt(section, L"MemberRole_" + std::to_wstring(m), 0);\n                if (roleID > 0 && std::find(team.memberRoleIDs.begin(), team.memberRoleIDs.end(), roleID) == team.memberRoleIDs.end())\n                    team.memberRoleIDs.push_back(roleID);\n            }\n            const int queueCount =''',
'''            for (int m = 0; m < members; ++m) {\n                const int roleID = ReadIniInt(section, L"MemberRole_" + std::to_wstring(m), 0);\n                if (roleID > 0 && std::find(team.memberRoleIDs.begin(), team.memberRoleIDs.end(), roleID) == team.memberRoleIDs.end())\n                    team.memberRoleIDs.push_back(roleID);\n            }\n            const auto savedLeader = std::find(team.memberRoleIDs.begin(), team.memberRoleIDs.end(), team.leaderRoleID);\n            if (savedLeader != team.memberRoleIDs.end() && savedLeader != team.memberRoleIDs.begin())\n                std::rotate(team.memberRoleIDs.begin(), savedLeader, savedLeader + 1);\n            const int queueCount =''')
replace_once('src/dungeon_app_methods.inl',
'''            std::wstring label = DungeonV45TeamLabel(team) + L" • KEY";''',
'''            std::wstring label = DungeonV45TeamLabel(team) + L" • Slot 1 / KEY";''')
replace_once('src/dungeon_app_methods.inl',
'''                std::wstring memberLabel = L"    ↳ Thành viên";''',
'''                std::wstring memberLabel = L"    ↳ Slot " + std::to_wstring(m + 1);''')
replace_once('src/dungeon_app_methods.inl',
'''        if (leaderIndex != CB_ERR)\n            config.leaderPid = static_cast<DWORD>(SendMessageW(dungeonLeaderCombo_, CB_GETITEMDATA, leaderIndex, 0));\n        std::wstring error;\n        if (!cleanroute_dungeon::ValidateTeam(config, error)) {''',
'''        if (leaderIndex != CB_ERR)\n            config.leaderPid = static_cast<DWORD>(SendMessageW(dungeonLeaderCombo_, CB_GETITEMDATA, leaderIndex, 0));\n        const auto leaderPidIt = std::find(config.pids.begin(), config.pids.end(), config.leaderPid);\n        if (leaderPidIt != config.pids.end() && leaderPidIt != config.pids.begin()) {\n            const std::size_t leaderPos = static_cast<std::size_t>(std::distance(config.pids.begin(), leaderPidIt));\n            std::rotate(config.pids.begin(), config.pids.begin() + static_cast<std::ptrdiff_t>(leaderPos),\n                        config.pids.begin() + static_cast<std::ptrdiff_t>(leaderPos + 1));\n            std::rotate(roles.begin(), roles.begin() + static_cast<std::ptrdiff_t>(leaderPos),\n                        roles.begin() + static_cast<std::ptrdiff_t>(leaderPos + 1));\n        }\n        std::wstring error;\n        if (!cleanroute_dungeon::ValidateTeam(config, error)) {''')
replace_once('src/dungeon_app_methods.inl',
'''        if (!dungeon_v45::ValidateInheritedCoordinates(preset->steps, error)) return false;\n        if (team.config.pids.size() <''',
'''        if (!dungeon_v45::ValidateInheritedCoordinates(preset->steps, error)) return false;\n        if (!dungeon_v45::ValidateParallelGroups(preset->steps, error)) return false;\n        if (team.config.pids.size() <''')
regex_once('src/dungeon_app_methods.inl',
    r'    void StartDungeonSelected\(\) \{.*?\n    \}\n\n    void PauseResumeDungeonSelected\(\) \{',
'''    void StartDungeonAtStep(int requestedStep, bool allowDirectInside) {\n        const int index = SelectedDungeonTeamIndex();\n        if (index >= 0 && index < static_cast<int>(dungeonTeams_.size())) {\n            DungeonTeamRuntime& reset = dungeonTeams_[static_cast<std::size_t>(index)];\n            if (!cleanroute_dungeon::ActiveState(reset.state)) {\n                reset.queueIndex = 0; reset.queueRunIndex = 1; reset.completionNotifyKeys.clear();\n                DungeonV45SyncLegacyConfig(reset);\n            }\n        }\n        std::wstring error;\n        if (!ValidateDungeonStart(index, error)) {\n            if (dungeonStatus_) SetText(dungeonStatus_, L"KHÔNG START • " + error);\n            return;\n        }\n        DungeonTeamRuntime& team = dungeonTeams_[static_cast<std::size_t>(index)];\n        cleanroute_dungeon::Preset* preset = DungeonConfiguredPresetForTeam(team);\n        if (!preset || preset->steps.empty()) {\n            if (dungeonStatus_) SetText(dungeonStatus_, L"KHÔNG START • preset không có STEP");\n            return;\n        }\n        requestedStep = std::clamp(requestedStep, 0, static_cast<int>(preset->steps.size()) - 1);\n        if (preset->steps[static_cast<std::size_t>(requestedStep)].parallelGroup > 0) {\n            const int group = preset->steps[static_cast<std::size_t>(requestedStep)].parallelGroup;\n            while (requestedStep > 0 && preset->steps[static_cast<std::size_t>(requestedStep - 1)].parallelGroup == group)\n                --requestedStep;\n        }\n        team.activePreset = *preset;\n        team.activePresetValid = true;\n        bool allAlreadyInside = allowDirectInside;\n        for (std::uint32_t pid : team.config.pids) {\n            Account& account = *AccountByPid(pid);\n            account.runtime.running = false;\n            account.pk = {};\n            account.dungeonOwned = true;\n            ResetRuntime(account.runtime);\n            account.runtime.running = false;\n            account.runtime.status = L"AUTO PHÓ BẢN • chuẩn bị";\n            if (!account.snapshotValid || account.snapshot.mapID != preset->dungeonMap ||\n                !account.snapshot.mapReady || account.snapshot.waitingChangeMap) allAlreadyInside = false;\n        }\n        team.state = cleanroute_dungeon::TeamState::Running;\n        team.phase = allAlreadyInside ? cleanroute_dungeon::TeamPhase::Steps : cleanroute_dungeon::TeamPhase::Precheck;\n        team.runIndex = team.queueRunIndex;\n        team.stepIndex = requestedStep;\n        team.npcAttempts = 0;\n        team.dialogAttempts = 0;\n        team.postSellDone.clear();\n        ResetDungeonStepRuntime(team, GetTickCount());\n        team.status = allAlreadyInside\n            ? L"RUN TRỰC TIẾP • STEP " + std::to_wstring(requestedStep + 1)\n            : L"RUN • PRECHECK • sẽ vào STEP " + std::to_wstring(requestedStep + 1);\n        RefreshDungeonAccountList();\n        RefreshDungeonTeamList(index);\n        RefreshDungeonStepList();\n        Log(L"AUTO PHÓ BẢN START • ĐỘI " + std::to_wstring(team.config.id) + L" • " + preset->name +\n            L" • STEP " + std::to_wstring(requestedStep + 1));\n    }\n\n    void StartDungeonSelected() {\n        StartDungeonAtStep(0, false);\n    }\n\n    void StartDungeonSelectedFromStep() {\n        if (!dungeonStepList_) return;\n        const int row = ListView_GetNextItem(dungeonStepList_, -1, LVNI_SELECTED);\n        if (row < 0) {\n            if (dungeonStatus_) SetText(dungeonStatus_, L"Chọn một STEP rồi bấm BẮT ĐẦU TỪ STEP ĐÃ CHỌN");\n            return;\n        }\n        StartDungeonAtStep(row, true);\n    }\n\n    void PauseResumeDungeonSelected() {''')
replace_once('src/dungeon_app_methods.inl',
'''                team.phase = cleanroute_dungeon::TeamPhase::Steps;\n                team.stepIndex = 0;\n                ResetDungeonStepRuntime(team, now);\n                team.status = L"VÀO PB PASS • STEP 1";''',
'''                team.phase = cleanroute_dungeon::TeamPhase::Steps;\n                team.stepIndex = std::clamp(team.stepIndex, 0, static_cast<int>(preset->steps.size()) - 1);\n                ResetDungeonStepRuntime(team, now);\n                team.status = L"VÀO PB PASS • STEP " + std::to_wstring(team.stepIndex + 1);''')
replace_once('src/dungeon_app_methods.inl',
'''    bool TickDungeonFight(DungeonTeamRuntime& team, const cleanroute_dungeon::Step& step,\n                          DWORD now, std::wstring& error) {''',
'''    bool DungeonEnsureAutoFightOn(DungeonTeamRuntime& team, const std::vector<Account*>& members,\n                                  const std::wstring& label) {\n        bool allOn = true;\n        for (Account* account : members) {\n            if (!account || !account->snapshotValid || (account->snapshot.validMask & ValidAutoFight) == 0 ||\n                !account->snapshot.autoFight) allOn = false;\n        }\n        if (allOn) return true;\n        for (Account* account : members) {\n            if (!account || (account->snapshot.validMask & ValidAutoFight) == 0 || account->snapshot.autoFight) continue;\n            bool clickOk = false; DWORD clickedAt = 0;\n            if (ConsumePriorityAutoResult(*account, ClickSlot::Attack, PriorityAutoOwner::Dungeon, clickOk, clickedAt)) {\n                if (!clickOk) team.status = L"AUTO-ĐÁNH • InputSync fail • sẽ thử lại";\n            } else {\n                (void)QueuePriorityAutoClick(*account, ClickSlot::Attack, PriorityAutoOwner::Dungeon,\n                                             L"AUTO PHÓ BẢN: MOVE tới đích → bật ĐÁNH QUÁI");\n            }\n        }\n        team.status = L"AUTO-ĐÁNH • " + label + L" • chờ verify AutoFight ON";\n        return false;\n    }\n\n    bool TickDungeonFight(DungeonTeamRuntime& team, const cleanroute_dungeon::Step& step,\n                          DWORD now, std::wstring& error) {''')
old_move = '''            if (step.kind == cleanroute_dungeon::StepKind::Move ||\n                step.kind == cleanroute_dungeon::StepKind::Portal) {\n                bool all = true;\n                TargetProfile target{};\n                target.name = step.label;\n                target.mapID = step.mapID;\n                target.x = step.x;\n                target.y = step.y;\n                target.valid = true;\n                for (Account* account : stepMembers) {\n                    bool arrived = false;\n                    HandleRobustTravel(*account, now, target, step.label.c_str(), arrived, step.tolerance);\n                    if (!arrived) all = false;\n                }\n                team.status = (step.kind == cleanroute_dungeon::StepKind::Portal ? L"CỔNG • " : L"DI CHUYỂN • ") +\n                              step.label + L" • slot " + DungeonMaskText(step.participantMask);\n                if (!all) {\n                    team.dueTick = 0;\n                    return;\n                }\n                if (step.kind == cleanroute_dungeon::StepKind::Portal) {\n                    // DATA marks these as UsePortal. Current v3.3 core has no separate blind portal click;\n                    // arrival at the live portal coordinate is the semantic trigger. Give the client a\n                    // short settle window so the next action cannot race the teleport/scene reposition.\n                    if (!team.dueTick) team.dueTick = now + 1800;\n                    if (now < team.dueTick) return;\n                }\n                AdvanceDungeonStep(team, now);\n                return;\n            }\n'''
new_move = '''            if (step.parallelGroup > 0) {\n                const int group = step.parallelGroup;\n                bool groupDone = true;\n                int nextIndex = team.stepIndex;\n                for (; nextIndex < static_cast<int>(preset->steps.size()); ++nextIndex) {\n                    const cleanroute_dungeon::Step& lane = preset->steps[static_cast<std::size_t>(nextIndex)];\n                    if (lane.parallelGroup != group) break;\n                    std::vector<Account*> laneMembers = DungeonStepMembers(team, lane);\n                    if (laneMembers.empty()) {\n                        FailDungeonTeam(team, L"Parallel Group không có participant hợp lệ");\n                        return;\n                    }\n                    TargetProfile target{}; target.name = lane.label; target.mapID = lane.mapID;\n                    target.x = lane.x; target.y = lane.y; target.valid = true;\n                    bool laneArrived = true;\n                    for (Account* account : laneMembers) {\n                        bool arrived = false;\n                        HandleRobustTravel(*account, now, target, lane.label.c_str(), arrived, lane.tolerance);\n                        if (!arrived) laneArrived = false;\n                    }\n                    if (!laneArrived) { groupDone = false; continue; }\n                    if (lane.autoFightOnArrival && !DungeonEnsureAutoFightOn(team, laneMembers, lane.label))\n                        groupDone = false;\n                }\n                team.status = L"SONG SONG P" + std::to_wstring(group) + L" • chạy đồng thời các slot đã gán";\n                if (!groupDone) return;\n                team.stepIndex = nextIndex;\n                ResetDungeonStepRuntime(team, now);\n                team.status = L"PASS SONG SONG P" + std::to_wstring(group) + L" • STEP " +\n                              std::to_wstring(team.stepIndex + 1);\n                return;\n            }\n\n            if (step.kind == cleanroute_dungeon::StepKind::Move ||\n                step.kind == cleanroute_dungeon::StepKind::Portal) {\n                bool all = true;\n                TargetProfile target{};\n                target.name = step.label;\n                target.mapID = step.mapID;\n                target.x = step.x;\n                target.y = step.y;\n                target.valid = true;\n                for (Account* account : stepMembers) {\n                    bool arrived = false;\n                    HandleRobustTravel(*account, now, target, step.label.c_str(), arrived, step.tolerance);\n                    if (!arrived) all = false;\n                }\n                team.status = (step.kind == cleanroute_dungeon::StepKind::Portal ? L"CỔNG • " : L"DI CHUYỂN • ") +\n                              step.label + L" • slot " + DungeonMaskText(step.participantMask);\n                if (!all) {\n                    team.dueTick = 0;\n                    return;\n                }\n                if (step.autoFightOnArrival && !DungeonEnsureAutoFightOn(team, stepMembers, step.label)) return;\n                if (step.kind == cleanroute_dungeon::StepKind::Portal) {\n                    if (!team.dueTick) team.dueTick = now + 1800;\n                    if (now < team.dueTick) return;\n                }\n                AdvanceDungeonStep(team, now);\n                return;\n            }\n'''
replace_once('src/dungeon_app_methods.inl', old_move, new_move)
replace_once('src/dungeon_app_methods.inl',
'''            ListView_SetItemText(dungeonStepList_, static_cast<int>(i), 1,\n                                 const_cast<wchar_t*>(cleanroute_dungeon::StepKindLabel(step.kind)));''',
'''            std::wstring action = cleanroute_dungeon::StepKindLabel(step.kind);\n            if (step.parallelGroup > 0) action += L" P" + std::to_wstring(step.parallelGroup);\n            if (step.autoFightOnArrival) action += L" +FIGHT";\n            ListView_SetItemText(dungeonStepList_, static_cast<int>(i), 1, action.data());''')
replace_once('src/dungeon_app_methods.inl',
'''        SendMessageW(dungeonEditorAny_, BM_SETCHECK, step.matchAnyVerified ? BST_CHECKED : BST_UNCHECKED, 0);\n        for (std::size_t i = 0; i < dungeonEditorSlots_.size(); ++i) {''',
'''        SendMessageW(dungeonEditorAny_, BM_SETCHECK, step.matchAnyVerified ? BST_CHECKED : BST_UNCHECKED, 0);\n        SetText(dungeonEditorParallel_, std::to_wstring(step.parallelGroup));\n        SendMessageW(dungeonEditorFightOnArrival_, BM_SETCHECK, step.autoFightOnArrival ? BST_CHECKED : BST_UNCHECKED, 0);\n        for (std::size_t i = 0; i < dungeonEditorSlots_.size(); ++i) {''')
replace_once('src/dungeon_app_methods.inl',
'''        step.matchAnyVerified = SendMessageW(dungeonEditorAny_, BM_GETCHECK, 0, 0) == BST_CHECKED;\n        step.participantMask = 0;''',
'''        step.matchAnyVerified = SendMessageW(dungeonEditorAny_, BM_GETCHECK, 0, 0) == BST_CHECKED;\n        step.parallelGroup = ParseEditInt(dungeonEditorParallel_, 0, 0, 255);\n        step.autoFightOnArrival = SendMessageW(dungeonEditorFightOnArrival_, BM_GETCHECK, 0, 0) == BST_CHECKED;\n        step.participantMask = 0;''')
replace_once('src/dungeon_app_methods.inl',
'''        const std::wstring title = L"CẤU HÌNH AUTO PHÓ BẢN v4.4 — " +''',
'''        const std::wstring title = L"CẤU HÌNH AUTO PHÓ BẢN v4.6 — " +''')
replace_all('src/dungeon_app_methods.inl', 'AUTO PHÓ BẢN v4.4', 'AUTO PHÓ BẢN v4.6', minimum=1)
replace_once('src/dungeon_app_methods.inl',
'''            18, 796, 360, 126, IDC_DG_TASK_LIST);''',
'''            18, 796, 360, 90, IDC_DG_TASK_LIST);''')
replace_once('src/dungeon_app_methods.inl',
'''            386, 796, 637, 126, IDC_DG_SCAN_LIST);''',
'''            386, 796, 637, 90, IDC_DG_SCAN_LIST);''')
replace_once('src/dungeon_app_methods.inl',
'''            DungeonEditorMake(hwnd, L"STATIC", L"Nhóm ACC (slot theo thứ tự trong bảng tổ đội):", SS_LEFT | SS_CENTERIMAGE, x, 340, 330, 24, 0);\n            for (int i = 0; i < 6; ++i) {\n                const std::wstring label = std::to_wstring(i + 1);\n                dungeonEditorSlots_[static_cast<std::size_t>(i)] = DungeonEditorMake(\n                    hwnd, L"BUTTON", label.c_str(), BS_AUTOCHECKBOX,\n                    x + i * 48, 366, 45, 24, IDC_DGE_SLOT1 + i);\n            }\n\n            DungeonEditorMake(hwnd, L"BUTTON", L"LƯU STEP", BS_DEFPUSHBUTTON, x, 402, 145, 28, IDC_DGE_SAVE);''',
'''            DungeonEditorMake(hwnd, L"STATIC", L"Parallel Group (0 = tuần tự)", SS_LEFT | SS_CENTERIMAGE, x, 340, 190, 24, 0);\n            dungeonEditorParallel_ = DungeonEditorMake(hwnd, L"EDIT", L"0", WS_BORDER | ES_NUMBER | ES_CENTER, x + 194, 340, 55, 24, IDC_DGE_PARALLEL);\n            dungeonEditorFightOnArrival_ = DungeonEditorMake(hwnd, L"BUTTON", L"Bật AutoFight khi đến tọa", BS_AUTOCHECKBOX, x, 368, 250, 24, IDC_DGE_FIGHT_ON_ARRIVAL);\n            DungeonEditorMake(hwnd, L"STATIC", L"Nhóm ACC (Slot 1 luôn là KEY):", SS_LEFT | SS_CENTERIMAGE, x, 396, 330, 24, 0);\n            for (int i = 0; i < 6; ++i) {\n                const std::wstring label = std::to_wstring(i + 1);\n                dungeonEditorSlots_[static_cast<std::size_t>(i)] = DungeonEditorMake(\n                    hwnd, L"BUTTON", label.c_str(), BS_AUTOCHECKBOX,\n                    x + i * 48, 422, 45, 24, IDC_DGE_SLOT1 + i);\n            }\n\n            DungeonEditorMake(hwnd, L"BUTTON", L"LƯU STEP", BS_DEFPUSHBUTTON, x, 458, 145, 28, IDC_DGE_SAVE);''')
replace_once('src/dungeon_app_methods.inl',
'''            dungeonEditorAny_ = nullptr;\n            dungeonEditorSlots_.fill(nullptr);''',
'''            dungeonEditorAny_ = nullptr;\n            dungeonEditorParallel_ = nullptr;\n            dungeonEditorFightOnArrival_ = nullptr;\n            dungeonEditorSlots_.fill(nullptr);''')
replace_once('src/dungeon_app_methods.inl',
'''    void ApplySavedDungeonMonsterSelection() {''',
'''    void RefreshDungeonMonsterCatalogWindow() {\n        if (!dungeonMonsterCatalogList_) return;\n        ListView_DeleteAllItems(dungeonMonsterCatalogList_);\n        for (std::size_t i = 0; i < dungeonSavedMonsters_.size(); ++i) {\n            const auto& rule = dungeonSavedMonsters_[i];\n            std::wstring index = std::to_wstring(i + 1);\n            LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = static_cast<int>(i); item.pszText = index.data();\n            ListView_InsertItem(dungeonMonsterCatalogList_, &item);\n            std::wstring name = rule.name.empty() ? L"(không tên)" : rule.name;\n            std::wstring res = rule.resID > 0 ? std::to_wstring(rule.resID) : L"—";\n            ListView_SetItemText(dungeonMonsterCatalogList_, static_cast<int>(i), 1, name.data());\n            ListView_SetItemText(dungeonMonsterCatalogList_, static_cast<int>(i), 2, res.data());\n        }\n        if (dungeonMonsterCatalogStatus_)\n            SetText(dungeonMonsterCatalogStatus_, L"Đã lưu " + std::to_wstring(dungeonSavedMonsters_.size()) + L" Monster • chọn dòng để xóa");\n    }\n\n    void DeleteSelectedDungeonMonsterCatalog() {\n        if (!dungeonMonsterCatalogList_) return;\n        const int row = ListView_GetNextItem(dungeonMonsterCatalogList_, -1, LVNI_SELECTED);\n        if (row < 0 || row >= static_cast<int>(dungeonSavedMonsters_.size())) {\n            SetText(dungeonMonsterCatalogStatus_, L"Chọn một Monster đã lưu trước khi XÓA"); return;\n        }\n        dungeonSavedMonsters_.erase(dungeonSavedMonsters_.begin() + row);\n        SaveDungeonMonsterCatalog();\n        PopulateDungeonMonsterCombo();\n        RefreshDungeonMonsterCatalogWindow();\n    }\n\n    static LRESULT CALLBACK DungeonMonsterCatalogWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {\n        App* self = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));\n        if (msg == WM_NCCREATE) {\n            const auto* cs = reinterpret_cast<const CREATESTRUCTW*>(lp);\n            self = reinterpret_cast<App*>(cs->lpCreateParams);\n            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));\n        }\n        if (!self) return DefWindowProcW(hwnd, msg, wp, lp);\n        if (msg == WM_CLOSE) { ShowWindow(hwnd, SW_HIDE); return 0; }\n        if (msg == WM_COMMAND) {\n            if (LOWORD(wp) == IDC_DGC_DELETE) { self->DeleteSelectedDungeonMonsterCatalog(); return 0; }\n            if (LOWORD(wp) == IDC_DGC_CLOSE) { ShowWindow(hwnd, SW_HIDE); return 0; }\n        }\n        return DefWindowProcW(hwnd, msg, wp, lp);\n    }\n\n    void EnsureDungeonMonsterCatalogWindow() {\n        if (dungeonMonsterCatalogWindow_ && IsWindow(dungeonMonsterCatalogWindow_)) return;\n        static bool registered = false;\n        if (!registered) {\n            WNDCLASSEXW wc{}; wc.cbSize = sizeof(wc); wc.hInstance = instance_;\n            wc.lpfnWndProc = DungeonMonsterCatalogWndProc; wc.lpszClassName = L"ThanLongDungeonMonsterCatalogV46";\n            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW); wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);\n            registered = RegisterClassExW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;\n        }\n        if (!registered) return;\n        dungeonMonsterCatalogWindow_ = CreateWindowExW(WS_EX_TOOLWINDOW, L"ThanLongDungeonMonsterCatalogV46",\n            L"MONSTER ĐÃ LƯU — AUTO PHÓ BẢN", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,\n            CW_USEDEFAULT, CW_USEDEFAULT, 560, 470, hwnd_, nullptr, instance_, this);\n        if (!dungeonMonsterCatalogWindow_) return;\n        dungeonMonsterCatalogList_ = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",\n            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,\n            12, 12, 520, 340, dungeonMonsterCatalogWindow_, nullptr, instance_, nullptr);\n        ListView_SetExtendedListViewStyle(dungeonMonsterCatalogList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);\n        DungeonListColumn(dungeonMonsterCatalogList_, 0, 50, L"#");\n        DungeonListColumn(dungeonMonsterCatalogList_, 1, 330, L"Tên Monster");\n        DungeonListColumn(dungeonMonsterCatalogList_, 2, 110, L"ResID");\n        CreateWindowExW(0, L"BUTTON", L"XÓA DÒNG ĐÃ CHỌN", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,\n                        12, 362, 180, 30, dungeonMonsterCatalogWindow_, reinterpret_cast<HMENU>(IDC_DGC_DELETE), instance_, nullptr);\n        CreateWindowExW(0, L"BUTTON", L"ĐÓNG", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,\n                        202, 362, 90, 30, dungeonMonsterCatalogWindow_, reinterpret_cast<HMENU>(IDC_DGC_CLOSE), instance_, nullptr);\n        dungeonMonsterCatalogStatus_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE | WS_BORDER,\n                        12, 398, 520, 28, dungeonMonsterCatalogWindow_, nullptr, instance_, nullptr);\n        HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));\n        for (HWND h : {dungeonMonsterCatalogList_, dungeonMonsterCatalogStatus_}) if (h) SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);\n    }\n\n    void OpenDungeonMonsterCatalog() {\n        EnsureDungeonMonsterCatalogWindow();\n        if (!dungeonMonsterCatalogWindow_) return;\n        RefreshDungeonMonsterCatalogWindow();\n        ShowWindow(dungeonMonsterCatalogWindow_, SW_SHOW);\n        SetForegroundWindow(dungeonMonsterCatalogWindow_);\n    }\n\n    void ApplySavedDungeonMonsterSelection() {''')

replace_once('src/dungeon_v45_methods.inl',
'''        mk(L"STATIC", L"Thành viên (tick = thuộc đội)", SS_LEFT | SS_CENTERIMAGE, 14, 48, 300, 24, 0);''',
'''        mk(L"STATIC", L"Thành viên: tick = THÊM • bỏ tick = XÓA • KEY luôn Slot 1", SS_LEFT | SS_CENTERIMAGE, 14, 48, 385, 24, 0);''')
replace_once('src/dungeon_v45_methods.inl',
'''        const auto leaderIt = std::find(roles.begin(), roles.end(), leaderRole);\n        if (leaderIt == roles.end()) { SetText(dungeonManagerStatus_, L"LỖI • KEY phải được tick trong đội"); return; }\n        for (std::size_t other = 0; other < dungeonTeams_.size(); ++other) {''',
'''        auto leaderIt = std::find(roles.begin(), roles.end(), leaderRole);\n        if (leaderIt == roles.end()) { SetText(dungeonManagerStatus_, L"LỖI • KEY phải được tick trong đội"); return; }\n        const std::size_t leaderPos = static_cast<std::size_t>(std::distance(roles.begin(), leaderIt));\n        if (leaderPos != 0) {\n            std::rotate(roles.begin(), roles.begin() + static_cast<std::ptrdiff_t>(leaderPos),\n                        roles.begin() + static_cast<std::ptrdiff_t>(leaderPos + 1));\n            std::rotate(pids.begin(), pids.begin() + static_cast<std::ptrdiff_t>(leaderPos),\n                        pids.begin() + static_cast<std::ptrdiff_t>(leaderPos + 1));\n        }\n        for (std::size_t other = 0; other < dungeonTeams_.size(); ++other) {''')
replace_once('src/dungeon_v45_methods.inl',
'''        team.displayName = GetText(dungeonManagerName_);\n        team.memberRoleIDs = roles; team.config.pids = pids; team.leaderRoleID = leaderRole;\n        team.config.leaderPid = pids[static_cast<std::size_t>(std::distance(roles.begin(), leaderIt))];''',
'''        team.displayName = GetText(dungeonManagerName_);\n        team.memberRoleIDs = roles; team.config.pids = pids; team.leaderRoleID = leaderRole;\n        team.config.leaderPid = pids.front();''')
replace_once('src/dungeon_v45_methods.inl',
'''                step.timeoutSec = GetPrivateProfileIntW(sec.c_str(), (p + L"Timeout").c_str(), 180, path.c_str());\n                step.participantMask =''',
'''                step.timeoutSec = GetPrivateProfileIntW(sec.c_str(), (p + L"Timeout").c_str(), 180, path.c_str());\n                step.parallelGroup = GetPrivateProfileIntW(sec.c_str(), (p + L"Parallel").c_str(), 0, path.c_str());\n                step.autoFightOnArrival = GetPrivateProfileIntW(sec.c_str(), (p + L"FightOnArrival").c_str(), 0, path.c_str()) != 0;\n                step.participantMask =''')
replace_once('src/dungeon_v45_methods.inl',
'''            std::wstring inheritError; if (!dungeon_v45::ValidateInheritedCoordinates(steps, inheritError)) { error = inheritError; return false; }''',
'''            std::wstring inheritError; if (!dungeon_v45::ValidateInheritedCoordinates(steps, inheritError)) { error = inheritError; return false; }\n            if (!dungeon_v45::ValidateParallelGroups(steps, inheritError)) { error = inheritError; return false; }''')
replace_once('src/dungeon_v45_methods.inl',
'''            if (dungeonManagerWindow_ && IsWindow(dungeonManagerWindow_)) ShowWindow(dungeonManagerWindow_, SW_HIDE);''',
'''            if (dungeonManagerWindow_ && IsWindow(dungeonManagerWindow_)) ShowWindow(dungeonManagerWindow_, SW_HIDE);\n            if (dungeonMonsterCatalogWindow_ && IsWindow(dungeonMonsterCatalogWindow_)) ShowWindow(dungeonMonsterCatalogWindow_, SW_HIDE);''')
replace_once('src/dungeon_v45_methods.inl',
'''        DungeonMake(L"BUTTON", L"BẢNG HOẠT ĐỘNG PB", BS_PUSHBUTTON,\n                    18, 930, 180, 30, IDC_DG_ACTIVITY_BOARD);''',
'''        DungeonMake(L"BUTTON", L"BẮT ĐẦU TỪ STEP ĐÃ CHỌN", BS_DEFPUSHBUTTON,\n                    18, 894, 260, 30, IDC_DG_START_FROM_STEP);\n        DungeonMake(L"BUTTON", L"MONSTER ĐÃ LƯU", BS_PUSHBUTTON,\n                    286, 894, 180, 30, IDC_DG_MONSTER_CATALOG);\n        DungeonMake(L"STATIC", L"Slot 1 = KEY • Parallel P# chạy đồng thời • Monster có thể để trống",\n                    SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 474, 894, 549, 30, 0);\n        DungeonMake(L"BUTTON", L"BẢNG HOẠT ĐỘNG PB", BS_PUSHBUTTON,\n                    18, 930, 180, 30, IDC_DG_ACTIVITY_BOARD);''')

replace_once('src/dungeon_v45_logic_test.cpp',
'''    assert(ValidateInheritedCoordinates(steps, error));\n\n    steps[1].mapID = 92;''',
'''    assert(ValidateInheritedCoordinates(steps, error));\n\n    Step laneA{}; laneA.kind = StepKind::Move; laneA.mapID = 93; laneA.x = 1110; laneA.y = 2456; laneA.parallelGroup = 7; laneA.participantMask = 0x03;\n    Step laneB{}; laneB.kind = StepKind::Move; laneB.mapID = 93; laneB.x = 3072; laneB.y = 2314; laneB.parallelGroup = 7; laneB.participantMask = 0x3c;\n    std::vector<Step> parallel{laneA, laneB};\n    assert(ValidateParallelGroups(parallel, error));\n    parallel[1].participantMask = 0x02;\n    assert(!ValidateParallelGroups(parallel, error));\n    parallel[1].participantMask = 0x3c;\n    Step blankFight{}; blankFight.kind = StepKind::Fight; blankFight.participantMask = kAllParticipantsMask;\n    assert(ValidateStep(blankFight, error));\n\n    steps[1].mapID = 92;''')
replace_once('src/dungeon_v45_logic_test.cpp',
             'std::wcout << L"dungeon v4.5 logic tests PASS\\n";',
             'std::wcout << L"dungeon v4.6 logic tests PASS\\n";')

changelog = read('CHANGELOG.md')
v46 = '''## v4.6 — Parallel Stage + START từ STEP + Activity/Monster UX\n\n- Sửa Activity Board: không còn tách mất dấu `/` trong tỷ lệ `current/target`; parser chấp nhận dòng mục tiêu có chữ dù client không hiện đúng một nhóm động từ hardcode.\n- Slot tổ đội được chuẩn hóa bền vững: **Slot 1 luôn là KEY**; khi tạo/sửa/nạp đội, RoleID/PID của KEY được đưa lên đầu. UI ghi rõ tick = thêm, bỏ tick = xóa và hiển thị Slot 2–6.\n- Thêm **BẮT ĐẦU TỪ STEP ĐÃ CHỌN**. Nếu toàn đội đã ở đúng DungeonMap thì chạy trực tiếp; nếu chưa ở trong PB vẫn đi qua entry pipeline rồi giữ đúng STEP đã chọn. Chọn giữa một Parallel Group tự chuẩn hóa về đầu group.\n- Thêm `Parallel Group`: các STEP TỌA ĐỘ liên tiếp cùng P# được dispatch đồng thời cho các mask slot không chồng nhau. Thêm `AutoFight khi đến tọa` để mỗi lane có thể tự bật đánh khi đến đích.\n- Điền lại canonical **Biên Giới Tống Liêu** theo kế hoạch đã duyệt: hội quân 1765,2665; tách 1–2 / 3–6 song song qua hai cặp điểm; hội quân 2213,1665 đánh sạch; tách 1279,1627 / 2503,1425 bật AutoFight; sau 10 giây hội quân 2071,1746 đánh sạch rồi chờ game đưa ra map.\n- FIGHT cho phép để trống Monster/ResID; completion vẫn fail-closed theo verified GMonster presence trong radius.\n- Thêm cửa sổ **MONSTER ĐÃ LƯU** hiển thị Name/ResID và cho xóa từng dòng; catalog vẫn dùng chung với combobox editor.\n- Editor STEP có `Parallel Group (0=tuần tự)` và `Bật AutoFight khi đến tọa`; INI export/import/preset override lưu đầy đủ hai trường mới.\n- Protocol EXE↔Bridge `0x00040600`; product/EXE/source/dist đồng bộ v4.6.\n\n'''
if not changelog.startswith('## v4.6'):
    write('CHANGELOG.md', v46 + changelog)

readme = read('README.md')
if readme.startswith('# AUTO Thần Long đa tính năng Pro v4.4'):
    readme = readme.replace('# AUTO Thần Long đa tính năng Pro v4.4', '# AUTO Thần Long đa tính năng Pro v4.6', 1)
intro_anchor = 'Tool Windows x64 quản lý nhiều client Thần Long Mobile. EXE và DLL phải luôn được thay cùng nhau vì giao thức bridge được khóa theo phiên bản.\n\n'
if '## Mới trong v4.6' not in readme:
    note = '''## Mới trong v4.6 — Parallel Stage + START từ STEP + Activity/Monster UX\n\n- **Biên Giới Tống Liêu** có sẵn chu trình tách `1,2` và `3,4,5,6` chạy song song đúng slot; Slot 1 luôn là KEY.\n- Có nút **BẮT ĐẦU TỪ STEP ĐÃ CHỌN** để test/chạy tiếp từ một STEP cụ thể.\n- Activity Board giữ nguyên tỷ lệ `current/target`; Monster của FIGHT được phép để trống; có cửa sổ **MONSTER ĐÃ LƯU** để xem/xóa Name/ResID.\n- Editor hỗ trợ `Parallel Group` và `Bật AutoFight khi đến tọa`; các trường này đi cùng export/import cấu hình.\n- Protocol v4.6: `0x00040600`; luôn thay EXE và Bridge DLL cùng bộ.\n\n'''
    if intro_anchor not in readme:
        raise SystemExit('README intro anchor missing')
    readme = readme.replace(intro_anchor, intro_anchor + note, 1)
write('README.md', readme)

checks = {
    'src/dungeon_logic.h': ['parallelGroup', 'autoFightOnArrival', 'Slot 1 is always KEY'],
    'src/dungeon_app_methods.inl': ['SONG SONG P', 'StartDungeonSelectedFromStep', 'OpenDungeonMonsterCatalog'],
    'src/dungeon_activity_bridge.inl': ['ratioSlash', 'hasAlpha'],
    'src/dungeon_presets.h': ['1110,2456', '3072,2314', '2071,1746'],
    'src/protocol.h': ['0x00040600u'],
}
for path, needles in checks.items():
    text = read(path)
    for needle in needles:
        if needle not in text:
            raise SystemExit(f'final contract missing {path}: {needle}')

print('Applied approved AUTO PHÓ BẢN v4.6 source migration')
