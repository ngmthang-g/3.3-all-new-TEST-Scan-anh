from pathlib import Path
import subprocess

ROOT = Path('.')
OLD_STABLE_COMMIT = '4650b57e563ae8211e8c6cd4253195eb29458dda'  # v4.6 release source


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


def git_show(commit, path):
    try:
        return subprocess.check_output(
            ['git', 'show', f'{commit}:{path}'],
            text=True,
            encoding='utf-8',
        )
    except subprocess.CalledProcessError as exc:
        raise SystemExit(f'git show failed for {commit}:{path}: {exc}')


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
                state = 'line_comment'
                i += 1
            elif ch == '/' and nx == '*':
                state = 'block_comment'
                i += 1
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


def restore_function_from_commit(path, marker, commit=OLD_STABLE_COMMIT):
    current = read(path)
    old = git_show(commit, path)
    cs, ce = function_span(current, marker)
    os, oe = function_span(old, marker)
    replacement = old[os:oe]
    write(path, current[:cs] + replacement + current[ce:])


version = read('VERSION.txt').strip()
if version == '4.8':
    required_markers = {
        'src/controller.cpp': [
            'constexpr wchar_t kTitle[] = L"AUTO Thần Long đa tính năng Pro v4.8";',
            '(!a.runtime.running && !a.dungeonOwned)',
            'a.runtime.running || a.pk.active || a.dungeonOwned',
            's.riding && !a.dungeonOwned',
        ],
        'src/dungeon_app_methods.inl': [
            'AUTO PHÓ BẢN v4.8',
            'WAIT CLIENT/STATE • giữ RUN/STEP',
            'previousPids',
        ],
        'src/dungeon_v45_methods.inl': [
            'ResolveDungeonTeamBindings();',
            'CHƯA ĐỒNG BỘ ĐƯỢC BẢNG HOẠT ĐỘNG',
        ],
    }
    for path, markers in required_markers.items():
        text = read(path)
        for marker in markers:
            if marker not in text:
                raise SystemExit(f'v4.8 source incomplete: {path} missing {marker!r}')
    print('v4.8 stability patch already applied; nothing to do')
    raise SystemExit(0)

if version != '4.7':
    raise SystemExit(f'Expected VERSION 4.7 before v4.8 migration, got {version!r}')

# Product/version migration.
write('VERSION.txt', '4.8\n')
replace_once('CMakeLists.txt',
             'project(ThanLongItemConsolidator VERSION 4.7',
             'project(ThanLongItemConsolidator VERSION 4.8')
replace_once('src/protocol.h',
             'constexpr std::uint32_t kProtocolVersion = 0x00040700u;',
             'constexpr std::uint32_t kProtocolVersion = 0x00040800u;')
replace_once('src/controller.cpp',
             'constexpr wchar_t kTitle[] = L"AUTO Thần Long đa tính năng Pro v4.7";',
             'constexpr wchar_t kTitle[] = L"AUTO Thần Long đa tính năng Pro v4.8";')
replace_once('src/dungeon_app_methods.inl',
             'DungeonMake(L"STATIC", L"AUTO PHÓ BẢN v4.7 • FIGHT: cứ 5 giây scan GMonster/HP • còn quái = đánh • hết quái = sang STEP",',
             'DungeonMake(L"STATIC", L"AUTO PHÓ BẢN v4.8 • core trạng thái/Đầu Thai theo Tab AUTO • FIGHT dùng scan ổn định v4.6",')

rc = read('resources/app.rc')
for old, new in [
    ('FILEVERSION 4,7,0,0', 'FILEVERSION 4,8,0,0'),
    ('PRODUCTVERSION 4,7,0,0', 'PRODUCTVERSION 4,8,0,0'),
    ('"FileVersion", "4.7\\0"', '"FileVersion", "4.8\\0"'),
    ('AUTO_Than_Long_da_tinh_nang_Pro_v4.7.exe', 'AUTO_Than_Long_da_tinh_nang_Pro_v4.8.exe'),
    ('"ProductVersion", "4.7\\0"', '"ProductVersion", "4.8\\0"'),
]:
    if old not in rc:
        raise SystemExit(f'resources/app.rc missing {old!r}')
    rc = rc.replace(old, new, 1)
write('resources/app.rc', rc)

# Client/state health: dungeon-owned accounts use the SAME freeze/recovery path
# as Tab AUTO instead of invalidating the snapshot after one failed read.
replace_once(
    'src/controller.cpp',
    '''            if (!ReadSnapshot(a, error, (a.runtime.running || a.pk.active || a.dungeonOwned) ? 700 : 900)) {
                if (a.runtime.running || a.pk.active) MarkReadStateFailure(a, error, now);
                else if (a.dungeonOwned) { a.snapshotValid = false; a.runtime.status = L"AUTO PHÓ BẢN • mất state/bridge"; }
                else a.runtime.status = L"Mất state/bridge";
                continue;
            }
''',
    '''            if (!ReadSnapshot(a, error, (a.runtime.running || a.pk.active || a.dungeonOwned) ? 700 : 900)) {
                if (a.runtime.running || a.pk.active || a.dungeonOwned) MarkReadStateFailure(a, error, now);
                else a.runtime.status = L"Mất state/bridge";
                continue;
            }
'''
)

# Life Guard: dungeonOwned is a valid owner even though runtime.running is false.
replace_once(
    'src/controller.cpp',
    '''        if (!a.runtime.running || !a.snapshotValid || !IsWindow(a.game.window) || rt.clientFreezeActive) return false;
''',
    '''        if ((!a.runtime.running && !a.dungeonOwned) || !a.snapshotValid || !IsWindow(a.game.window) || rt.clientFreezeActive) return false;
'''
)

# Dungeon movement keeps the mount after arriving. Other Tab AUTO/trade flows
# preserve their existing dismount behavior.
replace_once(
    'src/controller.cpp',
    '''            if (s.riding) {
                (void)SendDecision(a, Action::Dismount, targetProfile, context);
                return true;
            }
            ResetRobustTravel(rt);
''',
    '''            if (s.riding && !a.dungeonOwned) {
                (void)SendDecision(a, Action::Dismount, targetProfile, context);
                return true;
            }
            if (s.riding && a.dungeonOwned) {
                rt.status = L"AUTO PHÓ BẢN • đã tới đích • GIỮ THÚ CƯỠI";
            }
            ResetRobustTravel(rt);
'''
)

# Rebind by RoleID without erasing a healthy PID merely because one current
# snapshot is temporarily unreadable. Slot order 1..6 is never compressed.
dungeon = read('src/dungeon_app_methods.inl')
start_marker = '    void ResolveDungeonTeamBindings() {'
end_marker = '    void DungeonScanClients() {'
start = dungeon.find(start_marker)
end = dungeon.find(end_marker, start)
if start < 0 or end < 0:
    raise SystemExit('src/dungeon_app_methods.inl: cannot locate ResolveDungeonTeamBindings block')
new_bind = r'''    void ResolveDungeonTeamBindings() {
        for (DungeonTeamRuntime& team : dungeonTeams_) {
            const std::vector<std::uint32_t> previousPids = team.config.pids;
            const std::uint32_t previousLeaderPid = team.config.leaderPid;
            std::vector<std::uint32_t> candidate;
            candidate.reserve(team.memberRoleIDs.size());
            bool waitingState = false;
            bool complete = true;

            for (std::size_t slot = 0; slot < team.memberRoleIDs.size(); ++slot) {
                const std::int32_t wantedRole = team.memberRoleIDs[slot];
                Account* chosen = nullptr;

                for (auto& item : accounts_) {
                    Account& account = *item;
                    if (!account.snapshotValid || (account.snapshot.validMask & ValidIdentity) == 0) continue;
                    if (account.snapshot.roleID == wantedRole) {
                        chosen = &account;
                        break;
                    }
                }

                if (!chosen && slot < previousPids.size()) {
                    Account* previous = AccountByPid(previousPids[slot]);
                    if (previous && IsWindow(previous->game.window)) {
                        const bool identityKnown = previous->snapshotValid &&
                            (previous->snapshot.validMask & ValidIdentity) != 0;
                        if (!identityKnown || previous->snapshot.roleID == wantedRole) {
                            chosen = previous;
                            if (!identityKnown) waitingState = true;
                        }
                    }
                }

                if (!chosen) {
                    complete = false;
                    break;
                }
                candidate.push_back(chosen->game.pid);
            }

            if (complete && candidate.size() == team.memberRoleIDs.size()) {
                team.config.pids = std::move(candidate);
                team.config.leaderPid = 0;
                for (std::size_t i = 0; i < team.memberRoleIDs.size(); ++i) {
                    if (team.memberRoleIDs[i] == team.leaderRoleID) {
                        team.config.leaderPid = team.config.pids[i];
                        break;
                    }
                }
                complete = team.config.leaderPid != 0;
            } else {
                team.config.pids = previousPids;
                team.config.leaderPid = previousLeaderPid;
            }

            if (complete) {
                if (!cleanroute_dungeon::ActiveState(team.state)) {
                    if (team.state == cleanroute_dungeon::TeamState::Error && team.bindingError) {
                        team.state = cleanroute_dungeon::TeamState::Stopped;
                        team.bindingError = false;
                        team.status = waitingState ? L"Đã giữ binding • CHỜ STATE ổn định"
                                                   : L"Đã bind lại đủ client • STOP";
                    } else if (team.state != cleanroute_dungeon::TeamState::Error) {
                        team.status = waitingState ? L"Đủ client • CHỜ STATE ổn định"
                                                   : L"Đã bind đủ client • STOP";
                    }
                } else if (waitingState) {
                    team.status = L"WAIT CLIENT/STATE • giữ RUN/STEP • không xóa PID";
                }
            } else {
                const std::size_t verified = std::min(team.config.pids.size(), team.memberRoleIDs.size());
                team.status = L"WAIT CLIENT/STATE • giữ RUN/STEP • " +
                              std::to_wstring(verified) + L"/" +
                              std::to_wstring(team.memberRoleIDs.size()) +
                              L" slot đang giữ binding";
            }
        }
    }

'''
write('src/dungeon_app_methods.inl', dungeon[:start] + new_bind + dungeon[end:])

# Keep manual STEP selection while STOP so Start From Step is actually clickable.
replace_once(
    'src/dungeon_app_methods.inl',
    '''        ListView_SetItemState(dungeonStepList_, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
''',
    '''        // v4.8: giữ lựa chọn thủ công khi STOP; LVS_SINGLESEL tự chuyển selection khi RUN.
'''
)

# Temporary client/state instability must WAIT like Tab AUTO, not timeout-stop the team.
replace_once(
    'src/dungeon_app_methods.inl',
    '''            if (lifeRecovery) {
                team.phaseTick = now;
                team.status = L"CHỜ ĐẦU THAI • giữ nguyên STEP/lane hiện tại";
                return;
            }
            if (DungeonTimeout(team, now, 30)) FailDungeonTeam(team, error);
            else team.status = L"CHỜ STATE • " + error;
            return;
''',
    '''            team.phaseTick = now;
            if (lifeRecovery) {
                team.status = L"CHỜ ĐẦU THAI • giữ nguyên STEP/lane hiện tại";
            } else {
                team.status = L"WAIT CLIENT/STATE • giữ RUN/STEP • " + error;
            }
            return;
'''
)

# Restore the Activity reader and the actual fight scanner from v4.6 — the last
# user-verified stable core. Route/STEP/Parallel definitions stay on v4.7.
write('src/dungeon_activity_bridge.inl',
      git_show(OLD_STABLE_COMMIT, 'src/dungeon_activity_bridge.inl'))
restore_function_from_commit('src/dungeon_app_methods.inl', '    bool TickDungeonFight(')
restore_function_from_commit('src/dungeon_v45_methods.inl', '    void RefreshDungeonActivityBoard(')

# Re-resolve KEY before the restored Activity board chooses its authority client.
replace_once(
    'src/dungeon_v45_methods.inl',
    '''        DungeonTeamRuntime& team = dungeonTeams_[static_cast<std::size_t>(teamIndex)];
        cleanroute_dungeon::Preset* preset = DungeonPresetForTeam(team);
        Account* authority = AccountByPid(team.config.leaderPid);
        if (!preset || !authority) {
''',
    '''        ResolveDungeonTeamBindings();
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
'''
)

changelog = read('CHANGELOG.md')
entry = '''## v4.8 — Stable core restoration + approved v4.7 routes

- Phó Bản dùng cùng ReadState freeze/recovery với Tab AUTO; một nhịp bridge lỗi không còn xóa KEY/PID.
- Rebind theo RoleID giữ nguyên Slot 1..6 và PID cũ khi state tạm thời chưa đọc được; RUN/STEP chuyển WAIT thay vì STOP.
- Sửa Life Guard: dungeonOwned được Đầu Thai bằng cùng Priority Revive core dù runtime.running=false.
- AutoPath của Phó Bản giữ nguyên thú cưỡi tại đích; không phát Dismount cho dungeon-owned account.
- Sửa STEP selection: STOP giữ lựa chọn người dùng để BẮT ĐẦU TỪ STEP ĐÃ CHỌN hoạt động.
- Khôi phục Activity LIVE reader/controller behavior và TickDungeonFight từ v4.6 ổn định; giữ nguyên route/Parallel v4.7.
- Giữ toàn bộ STEP đã duyệt: Hoàng Kim 9394, Huyền Phật 5881,2901, Trúc Lâm MOVE-only, Dã Ngoại final clear, Dung Nham 27 điểm.

'''
if '## v4.8 — Stable core restoration' not in changelog:
    write('CHANGELOG.md', entry + changelog)

print('v4.8 stability migration applied')
