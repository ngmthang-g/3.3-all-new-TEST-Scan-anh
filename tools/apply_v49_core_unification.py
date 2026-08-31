from pathlib import Path

ROOT = Path('.')


def read(path):
    return (ROOT / path).read_text(encoding='utf-8-sig')


def write(path, text):
    (ROOT / path).write_text(text, encoding='utf-8')


def replace_once(path, old, new, *, required=True):
    text = read(path)
    if new in text and old not in text:
        return False
    count = text.count(old)
    if count != 1:
        if not required and count == 0:
            return False
        raise SystemExit(f'{path}: expected exactly one match, found {count}: {old[:180]!r}')
    write(path, text.replace(old, new, 1))
    return True


def function_span(text, marker):
    start = text.find(marker)
    if start < 0:
        raise SystemExit(f'function marker not found: {marker!r}')
    brace = text.find('{', start)
    if brace < 0:
        raise SystemExit(f'opening brace not found after: {marker!r}')
    depth = 0
    i = brace
    state = 'normal'
    while i < len(text):
        ch = text[i]
        nx = text[i + 1] if i + 1 < len(text) else ''
        if state == 'line_comment':
            if ch == '\n':
                state = 'normal'
        elif state == 'block_comment':
            if ch == '*' and nx == '/':
                state = 'normal'
                i += 1
        elif state == 'string':
            if ch == '\\':
                i += 1
            elif ch == '"':
                state = 'normal'
        elif state == 'char':
            if ch == '\\':
                i += 1
            elif ch == "'":
                state = 'normal'
        else:
            if ch == '/' and nx == '/':
                state = 'line_comment'; i += 1
            elif ch == '/' and nx == '*':
                state = 'block_comment'; i += 1
            elif ch == '"':
                state = 'string'
            elif ch == "'":
                state = 'char'
            elif ch == '{':
                depth += 1
            elif ch == '}':
                depth -= 1
                if depth == 0:
                    return start, i + 1
        i += 1
    raise SystemExit(f'unterminated function: {marker!r}')


def replace_function(path, marker, replacement):
    text = read(path)
    start, end = function_span(text, marker)
    write(path, text[:start] + replacement.rstrip() + text[end:])


version = read('VERSION.txt').strip()
if version == '4.9':
    checks = {
        'src/controller.cpp': [
            'AUTO Thần Long đa tính năng Pro v4.9',
            'a.runtime.running || a.pk.active || a.dungeonOwned',
            '(!a.runtime.running && !a.dungeonOwned)',
        ],
        'src/dungeon_app_methods.inl': [
            'AUTO PHÓ BẢN v4.9 • dùng trực tiếp CORE AUTO',
            'DungeonAccountByRoleID',
            'DungeonSlotAccount',
            'SCAN ALL GMonster VERIFIED',
            'PASS • mọi GMonster VERIFIED sống = 0',
        ],
    }
    for path, markers in checks.items():
        text = read(path)
        for marker in markers:
            if marker not in text:
                raise SystemExit(f'v4.9 incomplete: {path} missing {marker!r}')
    print('v4.9 core-unification patch already applied; nothing to do')
    raise SystemExit(0)

if version != '4.8':
    raise SystemExit(f'Expected VERSION 4.8 before v4.9 migration, got {version!r}')

# ---------------------------------------------------------------------------
# Product/version bump.
# ---------------------------------------------------------------------------
write('VERSION.txt', '4.9\n')
replace_once('CMakeLists.txt',
             'project(ThanLongItemConsolidator VERSION 4.8',
             'project(ThanLongItemConsolidator VERSION 4.9')
replace_once('src/protocol.h',
             'constexpr std::uint32_t kProtocolVersion = 0x00040800u;',
             'constexpr std::uint32_t kProtocolVersion = 0x00040900u;')
replace_once('src/controller.cpp',
             'constexpr wchar_t kTitle[] = L"AUTO Thần Long đa tính năng Pro v4.8";',
             'constexpr wchar_t kTitle[] = L"AUTO Thần Long đa tính năng Pro v4.9";')
replace_once('src/dungeon_app_methods.inl',
             'DungeonMake(L"STATIC", L"AUTO PHÓ BẢN v4.8 • core trạng thái/Đầu Thai theo Tab AUTO • FIGHT dùng scan ổn định v4.6",',
             'DungeonMake(L"STATIC", L"AUTO PHÓ BẢN v4.9 • dùng trực tiếp CORE AUTO • Phó Bản chỉ điều phối TEAM / STEP / FIGHT-CLEAR",')

rc = read('resources/app.rc')
for old, new in [
    ('FILEVERSION 4,8,0,0', 'FILEVERSION 4,9,0,0'),
    ('PRODUCTVERSION 4,8,0,0', 'PRODUCTVERSION 4,9,0,0'),
    ('"FileVersion", "4.8\\0"', '"FileVersion", "4.9\\0"'),
    ('AUTO_Than_Long_da_tinh_nang_Pro_v4.8.exe', 'AUTO_Than_Long_da_tinh_nang_Pro_v4.9.exe'),
    ('"ProductVersion", "4.8\\0"', '"ProductVersion", "4.9\\0"'),
]:
    if old not in rc:
        raise SystemExit(f'resources/app.rc missing {old!r}')
    rc = rc.replace(old, new, 1)
write('resources/app.rc', rc)

# ---------------------------------------------------------------------------
# RoleID is the durable team identity. PID is only a live cache owned by the
# same Account objects as Tab AUTO. No second client-health engine is created.
# ---------------------------------------------------------------------------
dungeon = read('src/dungeon_app_methods.inl')
start_marker = '    void ResolveDungeonTeamBindings() {'
end_marker = '    void DungeonScanClients() {'
start = dungeon.find(start_marker)
end = dungeon.find(end_marker, start)
if start < 0 or end < 0:
    raise SystemExit('cannot locate ResolveDungeonTeamBindings block')
role_binding = r'''    Account* DungeonAccountByRoleID(std::int32_t roleID) {
        if (roleID <= 0) return nullptr;
        for (auto& item : accounts_) {
            if (!item) continue;
            Account& account = *item;
            if (!account.snapshotValid || (account.snapshot.validMask & ValidIdentity) == 0) continue;
            if (account.snapshot.roleID == roleID) return &account;
        }
        return nullptr;
    }

    Account* DungeonSlotAccount(DungeonTeamRuntime& team, std::size_t slot) {
        if (slot >= team.memberRoleIDs.size()) return nullptr;
        if (team.config.pids.size() < team.memberRoleIDs.size())
            team.config.pids.resize(team.memberRoleIDs.size(), 0);

        const std::int32_t roleID = team.memberRoleIDs[slot];
        if (Account* live = DungeonAccountByRoleID(roleID)) {
            team.config.pids[slot] = live->game.pid;
            if (roleID == team.leaderRoleID || slot == 0) team.config.leaderPid = live->game.pid;
            return live;
        }

        // A transient ReadState failure must not detach a healthy Account. Reuse the
        // cached PID/window until Tab AUTO's freeze/recovery loop refreshes identity.
        const std::uint32_t cachedPid = team.config.pids[slot];
        Account* cached = cachedPid ? AccountByPid(cachedPid) : nullptr;
        if (cached && IsWindow(cached->game.window)) return cached;
        return nullptr;
    }

    std::size_t DungeonExpectedParticipants(const DungeonTeamRuntime& team,
                                            const cleanroute_dungeon::Step& step) const {
        const std::uint32_t mask = cleanroute_dungeon::NormalizeParticipantMask(
            step.participantMask, team.memberRoleIDs.size());
        std::size_t count = 0;
        for (std::size_t i = 0; i < team.memberRoleIDs.size() && i < cleanroute_dungeon::kMaxTeamMembers; ++i)
            if ((mask & (1u << static_cast<unsigned>(i))) != 0) ++count;
        return count;
    }

    void ResolveDungeonTeamBindings() {
        for (DungeonTeamRuntime& team : dungeonTeams_) {
            if (team.config.pids.size() < team.memberRoleIDs.size())
                team.config.pids.resize(team.memberRoleIDs.size(), 0);
            std::size_t resolved = 0;
            for (std::size_t slot = 0; slot < team.memberRoleIDs.size(); ++slot)
                if (DungeonSlotAccount(team, slot)) ++resolved;

            if (!cleanroute_dungeon::ActiveState(team.state)) {
                team.status = resolved == team.memberRoleIDs.size()
                    ? L"Đã nhận đủ RoleID từ CORE AUTO • STOP"
                    : L"Chờ CORE AUTO nhận client • " + std::to_wstring(resolved) + L"/" +
                      std::to_wstring(team.memberRoleIDs.size());
            }
        }
    }

'''
write('src/dungeon_app_methods.inl', dungeon[:start] + role_binding + dungeon[end:])

# While a team is active, QUÉT CLIENT becomes a harmless RoleID/PID refresh instead
# of a destructive account rebuild. The normal ScanClients implementation remains
# unchanged for STOP state and for the other tabs.
replace_function('src/dungeon_app_methods.inl', '    void DungeonScanClients() {', r'''    void DungeonScanClients() {
        if (AnyDungeonActive()) {
            ResolveDungeonTeamBindings();
            RefreshDungeonAccountList();
            RefreshDungeonTeamList();
            if (dungeonStatus_) SetText(dungeonStatus_, L"REFRESH ROLEID/PID • dùng Account CORE AUTO • không reset runtime");
            Log(L"AUTO PHÓ BẢN • refresh binding không phá runtime khi đội đang RUN/PAUSE.");
            return;
        }
        ScanClients();
        ResolveDungeonTeamBindings();
        RefreshDungeonAccountList();
        RefreshDungeonTeamList();
        if (dungeonStatus_) SetText(dungeonStatus_, L"QUÉT CLIENT • dùng ScanClients CORE AUTO • xong");
    }''')

# Resolve team members from RoleID first. This removes PID as the scheduler's
# authority and leaves client state/recovery to the existing AUTO Account engine.
replace_function('src/dungeon_app_methods.inl', '    std::vector<Account*> DungeonMembers(', r'''    std::vector<Account*> DungeonMembers(DungeonTeamRuntime& team) {
        std::vector<Account*> out;
        out.reserve(team.memberRoleIDs.size());
        for (std::size_t slot = 0; slot < team.memberRoleIDs.size(); ++slot)
            if (Account* account = DungeonSlotAccount(team, slot)) out.push_back(account);
        return out;
    }''')

replace_function('src/dungeon_app_methods.inl', '    std::vector<Account*> DungeonStepMembers(', r'''    std::vector<Account*> DungeonStepMembers(DungeonTeamRuntime& team,
                                                    const cleanroute_dungeon::Step& step) {
        std::vector<Account*> out;
        const std::uint32_t mask = cleanroute_dungeon::NormalizeParticipantMask(
            step.participantMask, team.memberRoleIDs.size());
        for (std::size_t slot = 0; slot < team.memberRoleIDs.size(); ++slot) {
            if ((mask & (1u << static_cast<unsigned>(slot))) == 0) continue;
            if (Account* account = DungeonSlotAccount(team, slot)) out.push_back(account);
        }
        return out;
    }''')

# ---------------------------------------------------------------------------
# FIGHT-CLEAR: use the strict GMonster scanner as a presence barrier. Every
# verified GMonster returned by the scanner counts, regardless of Name, ResID,
# Group, position or Radius. Unknown HP / scan failure / truncation all WAIT.
# TASK progress is diagnostic-only and no longer runs in the hot FIGHT loop.
# ---------------------------------------------------------------------------
replace_function('src/dungeon_app_methods.inl', '    bool TickDungeonFight(', r'''    bool TickDungeonFight(DungeonTeamRuntime& team, const cleanroute_dungeon::Step& step,
                          DWORD now, std::wstring& error) {
        cleanroute_dungeon::Preset* preset = DungeonPresetForTeam(team);
        if (!preset) {
            error = L"mất preset runtime";
            return false;
        }

        std::vector<Account*> members = DungeonStepMembers(team, step);
        const std::size_t expected = DungeonExpectedParticipants(team, step);
        if (members.size() != expected || members.empty()) {
            team.lastScanDecision = L"WAIT • CORE AUTO chưa đủ participant";
            team.status = L"FIGHT-CLEAR • chờ Account/RoleID từ CORE AUTO";
            error.clear();
            return false;
        }

        bool allOn = true;
        for (Account* account : members) {
            if (!account || !IsWindow(account->game.window) || account->runtime.clientFreezeActive ||
                !account->snapshotValid) {
                team.lastScanDecision = L"WAIT • client/state đang recovery";
                team.status = L"FIGHT-CLEAR • chờ CORE AUTO recovery client";
                error.clear();
                return false;
            }
            if ((account->snapshot.validMask & ValidLifeState) == 0 || account->snapshot.dead ||
                account->deathSessionLatched || account->runtime.revivePhase != 0) {
                team.lastScanDecision = L"WAIT • Đầu Thai/Life Guard";
                team.status = L"CHỜ ĐẦU THAI • CORE AUTO xử lý • giữ nguyên STEP";
                error.clear();
                return false;
            }
            if ((account->snapshot.validMask & ValidMap) == 0 || !account->snapshot.mapReady ||
                account->snapshot.waitingChangeMap || account->snapshot.mapID != preset->dungeonMap) {
                team.lastScanDecision = L"WAIT • Map state chưa sẵn sàng";
                team.status = L"FIGHT-CLEAR • chờ Map authoritative";
                error.clear();
                return false;
            }
            if ((account->snapshot.validMask & ValidAutoFight) == 0) {
                team.lastScanDecision = L"WAIT • AutoFight state chưa đọc được";
                team.status = L"FIGHT-CLEAR • chờ AutoFight state từ CORE AUTO";
                error.clear();
                return false;
            }
            if (!account->snapshot.autoFight) allOn = false;
        }

        Account* authority = DungeonAccountByRoleID(team.leaderRoleID);
        if (!authority || std::find(members.begin(), members.end(), authority) == members.end())
            authority = members.front();

        if (!allOn) {
            for (Account* account : members) {
                if (account->snapshot.autoFight) continue;
                bool clickOk = false;
                DWORD clickedAt = 0;
                if (ConsumePriorityAutoResult(*account, ClickSlot::Attack, PriorityAutoOwner::Dungeon,
                                              clickOk, clickedAt)) {
                    if (!clickOk) team.status = L"FIGHT-CLEAR • InputSync fail • CORE AUTO sẽ thử lại";
                } else {
                    (void)QueuePriorityAutoClick(*account, ClickSlot::Attack, PriorityAutoOwner::Dungeon,
                                                 L"AUTO PHÓ BẢN: bật ĐÁNH QUÁI bằng core Tab AUTO");
                }
            }
            team.lastScanDecision = L"WAIT • chờ AutoFight ON";
            team.status = L"FIGHT-CLEAR • " + step.label + L" • chờ verify AutoFight ON";
            return false;
        }

        if (!team.lastScanTick) {
            team.lastScanTick = now;
            team.lastScanDecision = L"WAIT • đánh 5s trước lần scan đầu";
            team.status = L"FIGHT-CLEAR • " + step.label + L" • scan mọi GMonster sau 5s";
            return false;
        }
        if (!Elapsed(now, team.lastScanTick, 5000)) {
            const DWORD remain = 5000u - std::min<DWORD>(5000u, now - team.lastScanTick);
            team.status = L"FIGHT-CLEAR • " + step.label + L" • scan lại sau " +
                          std::to_wstring((remain + 999u) / 1000u) + L"s";
            return false;
        }
        team.lastScanTick = now;

        Response response{};
        std::wstring scanError;
        if (!authority->bridge.Call(Command::ScanNearbyMonsters, 0, 0, 0, response, scanError, 1800)) {
            team.lastAliveMonsterCount = -1;
            team.lastUnknownHpMonsterCount = 0;
            team.lastScanDecision = L"WAIT • SCAN FAIL";
            team.status = L"FIGHT-CLEAR • SCAN FAIL • tiếp tục đánh, tuyệt đối không PASS";
            error.clear();
            return false;
        }

        dungeonLastScan_.assign(response.monsters, response.monsters +
            std::min<std::size_t>(response.monsterCount, kMaxMonsterRecords));
        RefreshDungeonScanList(response);

        int aliveAny = 0;
        int unknownHpAny = 0;
        for (std::size_t i = 0; i < response.monsterCount && i < kMaxMonsterRecords; ++i) {
            const MonsterRecord& record = response.monsters[i];
            if ((record.validMask & MonsterValidClassProof) == 0) continue;
            if ((record.validMask & MonsterValidLiveVitals) == 0 || record.hp < 0 || record.maxHP <= 0) {
                ++unknownHpAny;
                continue;
            }
            const bool knownDead = (record.validMask & MonsterValidDeath) != 0 && record.dead != 0;
            if (!knownDead && record.hp > 0) ++aliveAny;
        }

        team.lastAliveMonsterCount = aliveAny;
        team.lastUnknownHpMonsterCount = unknownHpAny;
        if (response.monsterTruncated) {
            team.lastScanDecision = L"WAIT • scanner bị TRUNCATED";
            team.status = L"FIGHT-CLEAR • scanner truncated • tiếp tục đánh, không được PASS";
            return false;
        }
        if (unknownHpAny > 0) {
            team.lastScanDecision = L"WAIT • có GMonster nhưng HP chưa chắc chắn";
            team.status = L"FIGHT-CLEAR • GMonster sống=" + std::to_wstring(aliveAny) +
                          L" • HP?=" + std::to_wstring(unknownHpAny) + L" • tiếp tục";
            return false;
        }
        if (aliveAny > 0) {
            team.lastScanDecision = L"WAIT • còn GMonster VERIFIED sống";
            team.status = L"FIGHT-CLEAR • còn " + std::to_wstring(aliveAny) +
                          L" GMonster sống • không phân biệt Name/ResID/X,Y • scan lại sau 5s";
            return false;
        }

        team.lastScanDecision = L"PASS • mọi GMonster VERIFIED sống = 0";
        team.status = L"PASS FIGHT-CLEAR • GMonster sống=0 • chuẩn bị sang STEP";
        return true;
    }''')

# ---------------------------------------------------------------------------
# Scheduler simplification: remove the global DungeonAllStable gate. The shared
# AUTO handlers already guard the exact state needed for movement, revive and
# AutoFight. Dungeon now only waits for the specific action it is scheduling.
# ---------------------------------------------------------------------------
dungeon = read('src/dungeon_app_methods.inl')
old_gate = '''        std::wstring error;
        const bool transitionPhase = team.phase == cleanroute_dungeon::TeamPhase::WaitEnter ||
                                     team.phase == cleanroute_dungeon::TeamPhase::WaitExit ||
                                     team.phase == cleanroute_dungeon::TeamPhase::PostSell ||
                                     team.phase == cleanroute_dungeon::TeamPhase::Dialog ||
                                     team.phase == cleanroute_dungeon::TeamPhase::Npc;
        if (!transitionPhase && !DungeonAllStable(team, error)) {
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
            team.phaseTick = now;
            if (lifeRecovery) {
                team.status = L"CHỜ ĐẦU THAI • giữ nguyên STEP/lane hiện tại";
            } else {
                team.status = L"WAIT CLIENT/STATE • giữ RUN/STEP • " + error;
            }
            return;
        }
'''
new_gate = '''        std::wstring error;
        ResolveDungeonTeamBindings();

        // Life Guard is owned by the same Account core as Tab AUTO. Dungeon merely
        // holds its STEP while any participant is dead/reviving; it never runs a
        // second revive state machine.
        for (Account* account : DungeonMembers(team)) {
            if (!account) continue;
            if ((account->snapshotValid && (account->snapshot.validMask & ValidLifeState) && account->snapshot.dead) ||
                account->deathSessionLatched || account->runtime.revivePhase != 0) {
                team.phaseTick = now;
                team.status = L"CHỜ ĐẦU THAI • CORE AUTO xử lý • giữ nguyên STEP/lane";
                return;
            }
        }
'''
if old_gate not in dungeon:
    raise SystemExit('TickDungeonTeam global stability gate not found')
write('src/dungeon_app_methods.inl', dungeon.replace(old_gate, new_gate, 1))

# PRECHECK must WAIT for AUTO core rather than failing merely because a character
# is dead or a snapshot is temporarily unavailable.
replace_once('src/dungeon_app_methods.inl',
'''        if (team.phase == cleanroute_dungeon::TeamPhase::Precheck) {
            for (std::uint32_t pid : team.config.pids) {
                Account* account = AccountByPid(pid);
                if (!account || account->snapshot.dead) {
                    FailDungeonTeam(team, L"có acc chết ở PRECHECK");
                    return;
                }
            }
            team.phase = cleanroute_dungeon::TeamPhase::Gather;
''',
'''        if (team.phase == cleanroute_dungeon::TeamPhase::Precheck) {
            std::vector<Account*> members = DungeonMembers(team);
            if (members.size() != team.memberRoleIDs.size()) {
                team.status = L"PRECHECK • chờ CORE AUTO nhận đủ RoleID/client";
                return;
            }
            for (Account* account : members) {
                if (!account->snapshotValid || account->runtime.clientFreezeActive ||
                    (account->snapshot.validMask & ValidLifeState) == 0) {
                    team.status = L"PRECHECK • chờ state/life từ CORE AUTO";
                    return;
                }
                if (account->snapshot.dead || account->deathSessionLatched || account->runtime.revivePhase != 0) {
                    team.status = L"PRECHECK • chờ CORE AUTO Đầu Thai";
                    return;
                }
            }
            team.phase = cleanroute_dungeon::TeamPhase::Gather;
''')

# Gather uses RobustTravel from AUTO and no longer turns a slow client/path into a
# dungeon-specific timeout failure.
replace_once('src/dungeon_app_methods.inl',
'''            for (std::uint32_t pid : team.config.pids) {
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
''',
'''            std::vector<Account*> members = DungeonMembers(team);
            if (members.size() != team.memberRoleIDs.size()) {
                team.status = L"TẬP KẾT • chờ CORE AUTO nhận đủ client";
                return;
            }
            for (Account* account : members) {
                bool arrived = false;
                HandleRobustTravel(*account, now, target, L"NPC vào phó bản", arrived, 160);
                if (!arrived) all = false;
            }
            if (all) {
                team.phase = cleanroute_dungeon::TeamPhase::Npc;
                team.phaseTick = now;
                team.status = L"BARRIER PASS • leader mở NPC";
            } else {
                team.phaseTick = now;
                team.status = L"TẬP KẾT • CORE AUTO đang di chuyển từng acc";
            }
''')

# KEY authority follows RoleID, not a stale PID cache. Replace every Dungeon
# leader lookup in this file; none of them should keep PID as authority.
text = read('src/dungeon_app_methods.inl')
needle = '            Account* leader = AccountByPid(team.config.leaderPid);\n'
count = text.count(needle)
if count < 2:
    raise SystemExit(f'expected at least two Dungeon leader PID lookups, found {count}')
write('src/dungeon_app_methods.inl', text.replace(needle,
    '            Account* leader = DungeonAccountByRoleID(team.leaderRoleID);\n'))

# WaitEnter/WaitExit use RoleID-resolved Account objects and simply wait while an
# Account is unavailable; they never dereference a missing PID.
replace_once('src/dungeon_app_methods.inl',
'''            for (std::uint32_t pid : team.config.pids) {
                Account& account = *AccountByPid(pid);
                const bool entered = account.snapshot.mapID == preset->dungeonMap && account.snapshot.mapReady &&
                                     !account.snapshot.waitingChangeMap;
                if (entered) ++inside;
                else all = false;
            }
''',
'''            std::vector<Account*> members = DungeonMembers(team);
            if (members.size() != team.memberRoleIDs.size()) all = false;
            for (Account* account : members) {
                const bool entered = account->snapshotValid && (account->snapshot.validMask & ValidMap) &&
                                     account->snapshot.mapID == preset->dungeonMap && account->snapshot.mapReady &&
                                     !account->snapshot.waitingChangeMap;
                if (entered) ++inside;
                else all = false;
            }
''')

replace_once('src/dungeon_app_methods.inl',
'''            for (std::uint32_t pid : team.config.pids) {
                Account& account = *AccountByPid(pid);
                if (account.snapshot.mapID == preset->dungeonMap) allOut = false;
                else ++outside;
            }
''',
'''            std::vector<Account*> members = DungeonMembers(team);
            if (members.size() != team.memberRoleIDs.size()) allOut = false;
            for (Account* account : members) {
                if (!account->snapshotValid || (account->snapshot.validMask & ValidMap) == 0) {
                    allOut = false;
                    continue;
                }
                if (account->snapshot.mapID == preset->dungeonMap) allOut = false;
                else ++outside;
            }
''')

# STEP timeouts were an extra dungeon-level failure gate. Keep the stored value
# for compatibility/diagnostics, but do not STOP a healthy AUTO core because a
# movement/fight STEP takes longer than expected.
replace_once('src/dungeon_app_methods.inl',
'''            if (DungeonTimeout(team, now, step.timeoutSec)) {
                FailDungeonTeam(team, L"timeout STEP " + std::to_wstring(team.stepIndex + 1) + L" • " + step.label);
                return;
            }
''',
'''            // v4.9: timeoutSec is diagnostic-only. CORE AUTO decides when an action is healthy;
            // Phó Bản keeps the STEP pending instead of creating a second timeout failure engine.
''')

# Missing participant in a running STEP is a WAIT, not a fatal team error.
replace_once('src/dungeon_app_methods.inl',
'''            std::vector<Account*> stepMembers = DungeonStepMembers(team, step);
            if (stepMembers.empty()) {
                FailDungeonTeam(team, L"STEP không có participant hợp lệ");
                return;
            }
''',
'''            std::vector<Account*> stepMembers = DungeonStepMembers(team, step);
            const std::size_t expectedMembers = DungeonExpectedParticipants(team, step);
            if (stepMembers.size() != expectedMembers || stepMembers.empty()) {
                team.status = L"STEP • chờ CORE AUTO nhận đủ participant theo RoleID";
                return;
            }
''')

replace_once('src/dungeon_app_methods.inl',
'''                    if (laneMembers.empty()) {
                        FailDungeonTeam(team, L"Parallel lane không có participant hợp lệ");
                        return;
                    }
''',
'''                    if (laneMembers.size() != DungeonExpectedParticipants(team, lane) || laneMembers.empty()) {
                        team.status = L"SONG SONG • chờ CORE AUTO nhận đủ participant theo RoleID";
                        return;
                    }
''')

# WaitMap checks only the state it needs: authoritative map.
replace_once('src/dungeon_app_methods.inl',
'''                bool all = true;
                for (Account* account : stepMembers) if (account->snapshot.mapID != step.mapID) all = false;
                if (all) AdvanceDungeonStep(team, now);
''',
'''                bool all = true;
                for (Account* account : stepMembers) {
                    if (!account->snapshotValid || (account->snapshot.validMask & ValidMap) == 0 ||
                        !account->snapshot.mapReady || account->snapshot.waitingChangeMap ||
                        account->snapshot.mapID != step.mapID) all = false;
                }
                if (all) AdvanceDungeonStep(team, now);
                else team.status = L"ĐỢI MAP • chỉ chờ Map state từ CORE AUTO";
''')

# ---------------------------------------------------------------------------
# UI wording: make the architecture and FIGHT semantics obvious. Diagnostic
# tables remain available manually but no longer participate in the hot loop.
# ---------------------------------------------------------------------------
replace_once('src/dungeon_app_methods.inl',
             'L"KẾ HOẠCH / STEP — FIGHT không đếm kill: scan presence 5s/lần; TASK chỉ quan sát/chẩn đoán.",',
             'L"KẾ HOẠCH / STEP — FIGHT-CLEAR: scan ALL GMonster VERIFIED 5s/lần; không Name/ResID/Radius.",')
replace_once('src/dungeon_app_methods.inl',
             'L"TASK observer + GMonster/Name/ResID/HP • chọn dòng monster rồi bấm LƯU để đưa tên vào catalog 0.6.2-style",',
             'L"CHẨN ĐOÁN THỦ CÔNG • TASK + GMonster chỉ để xem; runtime FIGHT không dùng Name/ResID/Radius",')
replace_once('src/dungeon_app_methods.inl',
             'L"An toàn: scan fail không được coi là hết quái. Timeout chỉ FAIL. Preset RUN được đóng băng tới lần START kế tiếp.",',
             'L"CORE AUTO quản lý client/state/Đầu Thai/di chuyển; Phó Bản chỉ TEAM/STEP. Scan fail/HP? tuyệt đối không PASS.",')
replace_once('src/dungeon_app_methods.inl',
             'std::wstring radius = std::to_wstring(step.kind == cleanroute_dungeon::StepKind::Fight ? step.radius : step.tolerance);',
             'std::wstring radius = step.kind == cleanroute_dungeon::StepKind::Fight ? L"SCAN ALL" : std::to_wstring(step.tolerance);')
replace_once('src/dungeon_app_methods.inl',
             'L"Proof STEP", L"SCAN GMonster presence • TASK observer-only"',
             'L"Proof STEP", L"SCAN ALL GMonster VERIFIED • TASK manual-only"')
replace_once('src/dungeon_app_methods.inl',
             'step.kind == cleanroute_dungeon::StepKind::Fight ? L" • R" + std::to_wstring(step.radius) : L" • Tol " + std::to_wstring(step.tolerance)',
             'step.kind == cleanroute_dungeon::StepKind::Fight ? L" • SCAN ALL" : L" • Tol " + std::to_wstring(step.tolerance)')
replace_once('src/dungeon_app_methods.inl',
             '? L"SCAN 5s • R" + std::to_wstring(step.radius)',
             '? L"SCAN ALL 5s"',
             required=False)

# Editor note: advanced monster fields are retained only for diagnostics/backward
# compatibility, not for FIGHT completion.
text = read('src/dungeon_app_methods.inl')
text = text.replace(
    'FIGHT: mỗi 5s scan Name/ResID/HP: còn quái thì đánh, hết quái thì sang STEP.',
    'FIGHT-CLEAR: mỗi 5s scan mọi GMonster VERIFIED; Name/ResID/Radius chỉ chẩn đoán, không quyết định PASS.')
write('src/dungeon_app_methods.inl', text)

# Changelog.
changelog = read('CHANGELOG.md')
entry = '''## v4.9 — Unified AUTO core + simplified dungeon scheduler\n\n- AUTO PHÓ BẢN không còn dùng DungeonAllStable như một engine trạng thái thứ hai; client/state/Đầu Thai/AutoPath/AutoFight tiếp tục do Account CORE AUTO quản lý.\n- Team dùng RoleID làm identity bền vững; PID chỉ là cache runtime và được refresh không phá STEP.\n- PRECHECK/GATHER/WAIT ENTER/WAIT EXIT/STEP thiếu state tạm thời đều WAIT thay vì tự STOP do rào riêng của Dungeon.\n- FIGHT-CLEAR scan toàn bộ GMonster VERIFIED mà scanner trả về; bỏ Name/ResID/Group/Radius/X,Y khỏi quyết định PASS. GMonster X,Y=? nhưng HP còn vẫn chặn STEP.\n- Scan fail, HP chưa chắc chắn hoặc scanner truncated đều WAIT; chỉ scan thành công và mọi GMonster VERIFIED sống = 0 mới PASS.\n- ReadDungeonProgress/TASK không còn chạy trong hot FIGHT loop; bảng TASK/Monster chỉ là chẩn đoán thủ công.\n- Giữ nguyên toàn bộ route/STEP/Parallel/Queue đã duyệt từ v4.7-v4.8.\n\n'''
if '## v4.9 — Unified AUTO core' not in changelog:
    write('CHANGELOG.md', entry + changelog)

print('v4.9 AUTO-core unification migration applied')
