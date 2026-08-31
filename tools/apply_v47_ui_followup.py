from pathlib import Path
import re

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
        raise SystemExit(f'{path}: missing expected text: {old[:180]!r}')
    if text.count(old) != 1:
        raise SystemExit(f'{path}: expected one match, found {text.count(old)}: {old[:180]!r}')
    write(path, text.replace(old, new, 1))
    return True


def regex_once(path, pattern, replacement, flags=re.S):
    text = read(path)
    if re.search(pattern, text, flags) is None:
        if replacement in text:
            return False
        raise SystemExit(f'{path}: regex did not match: {pattern[:180]!r}')
    out, count = re.subn(pattern, replacement, text, count=1, flags=flags)
    if count != 1:
        raise SystemExit(f'{path}: regex expected one match, got {count}: {pattern[:180]!r}')
    write(path, out)
    return True


if read('VERSION.txt').strip() != '4.7':
    raise SystemExit('apply_v47_ui_followup.py must run after the v4.7 migration')

controller = read('src/controller.cpp')
app = read('src/dungeon_app_methods.inl')
manager = read('src/dungeon_v45_methods.inl')
if ('IDC_DG_AUTO_SELL=729' in controller and
        'DungeonV47AddMemberFromManager' in manager and
        'AUTO BÁN khi ô trống <' in app):
    print('v4.7 UI/team/sell follow-up already applied; nothing to do')
    raise SystemExit(0)

# ---- Controller IDs + persistent HWND ----
replace_once('src/controller.cpp',
'''constexpr int IDC_DG_MONSTER_CATALOG=728;
// Dungeon preset editor window (740-779 reserved).''',
'''constexpr int IDC_DG_MONSTER_CATALOG=728;
constexpr int IDC_DG_AUTO_SELL=729;
// Dungeon preset editor window (740-779 reserved).''')
replace_once('src/controller.cpp',
'''constexpr int IDC_DGC_DELETE=792;
constexpr int IDC_DGC_CLOSE=793;
''',
'''constexpr int IDC_DGC_DELETE=792;
constexpr int IDC_DGC_CLOSE=793;
constexpr int IDC_DGM_ADD_MEMBER=794;
constexpr int IDC_DGM_REMOVE_MEMBER=795;
''')
replace_once('src/controller.cpp',
'''    std::vector<MonsterRecord> dungeonLastScan_{};
    HWND dungeonSellThresholdEdit_=nullptr;
''',
'''    std::vector<MonsterRecord> dungeonLastScan_{};
    HWND dungeonSellThresholdEdit_=nullptr;
    HWND dungeonAutoSellCheck_=nullptr;
''')

# ---- Tab visibility: when Dungeon controls are shown, foreign tab controls are forcibly hidden ----
replace_once('src/dungeon_app_methods.inl',
'''    void ShowDungeonControls(bool show) {
        for (HWND h : dungeonControls_) if (h) ShowWindow(h, show ? SW_SHOW : SW_HIDE);
    }
''',
'''    void ShowDungeonControls(bool show) {
        if (show && hwnd_) {
            for (HWND h = GetWindow(hwnd_, GW_CHILD); h; h = GetWindow(h, GW_HWNDNEXT)) {
                if (h == mainTab_ || IsDungeonControl(h)) continue;
                ShowWindow(h, SW_HIDE);
            }
        }
        for (HWND h : dungeonControls_) if (h) ShowWindow(h, show ? SW_SHOW : SW_HIDE);
    }
''')

# ---- Dungeon Auto Sell UI: checkbox + free-slot threshold, default 15 ----
replace_once('src/dungeon_app_methods.inl',
'''        DungeonMake(L"STATIC", L"AUTO PHÓ BẢN v4.6 • FIGHT: cứ 5 giây scan GMonster/HP • còn quái = đánh • hết quái = sang STEP",
''',
'''        DungeonMake(L"STATIC", L"AUTO PHÓ BẢN v4.7 • FIGHT: cứ 5 giây scan GMonster/HP • còn quái = đánh • hết quái = sang STEP",
''')
replace_once('src/dungeon_app_methods.inl',
'''        DungeonMake(L"STATIC", L"Bán <", SS_LEFT | SS_CENTERIMAGE, 245, 234, 42, 28, 0);
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
''',
'''        dungeonAutoSellCheck_ = DungeonMake(L"BUTTON", L"AUTO BÁN khi ô trống <", BS_AUTOCHECKBOX,
                                               245, 234, 155, 28, IDC_DG_AUTO_SELL);
        dungeonSellThresholdEdit_ = DungeonMake(L"EDIT", L"15", WS_BORDER | ES_NUMBER | ES_CENTER,
                                                404, 234, 42, 28, IDC_DG_SELL_THRESHOLD);
        DungeonMake(L"BUTTON", L"LƯU BÁN", BS_PUSHBUTTON, 450, 234, 72, 28, IDC_DG_SAVE_THRESHOLD);
        DungeonMake(L"STATIC", L"KEY:", SS_LEFT | SS_CENTERIMAGE, 530, 234, 32, 28, 0);
        dungeonLeaderCombo_ = DungeonMake(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL,
                                          564, 234, 120, 300, IDC_DG_LEADER);
        DungeonMake(L"STATIC", L"Phó bản:", SS_LEFT | SS_CENTERIMAGE, 690, 234, 58, 28, 0);
        dungeonPresetCombo_ = DungeonMake(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL,
                                          750, 234, 130, 400, IDC_DG_PRESET);
        DungeonMake(L"STATIC", L"Lượt:", SS_LEFT | SS_CENTERIMAGE, 886, 234, 38, 28, 0);
        dungeonRunsEdit_ = DungeonMake(L"EDIT", L"1", WS_BORDER | ES_NUMBER | ES_CENTER,
                                       926, 234, 38, 28, IDC_DG_RUNS);
        DungeonMake(L"BUTTON", L"TẠO ĐỘI", BS_DEFPUSHBUTTON, 968, 234, 55, 28, IDC_DG_CREATE_TEAM);
''')

# Default threshold becomes 15, preserving any already-saved per-role value.
text = read('src/dungeon_app_methods.inl')
text = text.replace('if (roleID <= 0) return 40;', 'if (roleID <= 0) return 15;')
text = text.replace('L"SellThreshold", 40), 0, 200, 40)', 'L"SellThreshold", 15), 0, 200, 15)')
text = text.replace('? DungeonSellThresholdForRole(account.snapshot.roleID) : 40;',
                    '? DungeonSellThresholdForRole(account.snapshot.roleID) : 15;')
write('src/dungeon_app_methods.inl', text)

replace_once('src/dungeon_app_methods.inl',
'''        SetText(dungeonSellThresholdEdit_, std::to_wstring(DungeonSellThresholdForRole(account->snapshot.roleID)));
    }

    void SaveDungeonSellThresholdSelected() {''',
'''        SetText(dungeonSellThresholdEdit_, std::to_wstring(DungeonSellThresholdForRole(account->snapshot.roleID)));
        if (dungeonAutoSellCheck_)
            SendMessageW(dungeonAutoSellCheck_, BM_SETCHECK, account->profile.enableSell ? BST_CHECKED : BST_UNCHECKED, 0);
    }

    void SaveDungeonSellThresholdSelected() {''')
replace_once('src/dungeon_app_methods.inl',
'''        const int threshold = ParseEditInt(dungeonSellThresholdEdit_, 40, 0, 200);
        SaveDungeonSellThresholdForRole(account->snapshot.roleID, threshold);
        RefreshDungeonAccountList();
        if (dungeonStatus_) SetText(dungeonStatus_, L"Đã lưu ngưỡng bán PB cho " + AccountTag(*account));
''',
'''        const int threshold = ParseEditInt(dungeonSellThresholdEdit_, 15, 0, 200);
        account->profile.enableSell = dungeonAutoSellCheck_ &&
            SendMessageW(dungeonAutoSellCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        SaveProfile(account->profile);
        SaveDungeonSellThresholdForRole(account->snapshot.roleID, threshold);
        RefreshDungeonAccountList();
        if (dungeonStatus_) SetText(dungeonStatus_, L"AUTO BÁN PB " +
            std::wstring(account->profile.enableSell ? L"ON" : L"OFF") + L" • ô trống < " +
            std::to_wstring(threshold) + L" • " + AccountTag(*account));
''')

# ---- Team manager: only current team rows; dedicated ADD/REMOVE ----
replace_once('src/dungeon_v45_methods.inl',
'''                case IDC_DGM_UPDATE_TEAM: self->DungeonV45UpdateTeamFromManager(); return 0;
                case IDC_DGM_QUEUE_ADD: self->DungeonV45QueueAdd(false); return 0;
''',
'''                case IDC_DGM_UPDATE_TEAM: self->DungeonV45UpdateTeamFromManager(); return 0;
                case IDC_DGM_ADD_MEMBER: self->DungeonV47AddMemberFromManager(); return 0;
                case IDC_DGM_REMOVE_MEMBER: self->DungeonV47RemoveMemberFromManager(); return 0;
                case IDC_DGM_QUEUE_ADD: self->DungeonV45QueueAdd(false); return 0;
''')
replace_once('src/dungeon_v45_methods.inl',
'''            L"QUẢN LÝ TỔ ĐỘI / KẾ HOẠCH PHÓ BẢN v4.5", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
''',
'''            L"QUẢN LÝ TỔ ĐỘI / KẾ HOẠCH PHÓ BẢN v4.7", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
''')
replace_once('src/dungeon_v45_methods.inl',
'''        mk(L"STATIC", L"Thành viên: tick = THÊM • bỏ tick = XÓA • KEY luôn Slot 1", SS_LEFT | SS_CENTERIMAGE, 14, 48, 385, 24, 0);
        dungeonManagerMembers_ = mk(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER,
                                    14, 74, 385, 250, IDC_DGM_MEMBERS);
        ListView_SetExtendedListViewStyle(dungeonManagerMembers_, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        DungeonListColumn(dungeonManagerMembers_, 0, 240, L"Nhân vật / RoleID");
        DungeonListColumn(dungeonManagerMembers_, 1, 110, L"PID");
        mk(L"STATIC", L"KEY:", SS_LEFT | SS_CENTERIMAGE, 14, 332, 45, 28, 0);
        dungeonManagerLeader_ = mk(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL, 62, 332, 337, 280, IDC_DGM_LEADER);
        mk(L"BUTTON", L"CẬP NHẬT ĐỘI", BS_DEFPUSHBUTTON, 14, 370, 385, 34, IDC_DGM_UPDATE_TEAM);
''',
'''        mk(L"STATIC", L"THÀNH VIÊN ĐỘI ĐANG CHỌN • KEY luôn Slot 1", SS_LEFT | SS_CENTERIMAGE, 14, 48, 385, 24, 0);
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
''')

regex_once('src/dungeon_v45_methods.inl',
    r'''        ListView_DeleteAllItems\(dungeonManagerMembers_\);\n        SendMessageW\(dungeonManagerLeader_, CB_RESETCONTENT, 0, 0\);\n        int row = 0, leaderSel = -1;\n        for \(const auto& ptr : accounts_\) \{.*?\n        \}\n        if \(leaderSel >= 0\) SendMessageW\(dungeonManagerLeader_, CB_SETCURSEL, leaderSel, 0\);''',
    '''        ListView_DeleteAllItems(dungeonManagerMembers_);
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
        if (leaderSel >= 0) SendMessageW(dungeonManagerLeader_, CB_SETCURSEL, leaderSel, 0);''')

insert_anchor = '    void DungeonV45UpdateTeamFromManager() {\n'
methods = read('src/dungeon_v45_methods.inl')
if insert_anchor not in methods:
    raise SystemExit('src/dungeon_v45_methods.inl: update-team insertion anchor missing')
extra = r'''    bool DungeonV47RoleInOtherTeam(int currentIndex, std::int32_t roleID) const {
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

'''
methods = methods.replace(insert_anchor, extra + insert_anchor, 1)
write('src/dungeon_v45_methods.inl', methods)

regex_once('src/dungeon_v45_methods.inl',
    r'''        std::vector<std::int32_t> roles; std::vector<std::uint32_t> pids;\n        const int rows = ListView_GetItemCount\(dungeonManagerMembers_\);\n        for \(int row = 0; row < rows; \+\+row\) \{.*?\n        \}\n        if \(roles.empty\(\) \|\| roles.size\(\) > cleanroute_dungeon::kMaxTeamMembers\) \{''',
    '''        std::vector<std::int32_t> roles = team.memberRoleIDs;
        std::vector<std::uint32_t> pids = team.config.pids;
        if (roles.empty() || roles.size() != pids.size() || roles.size() > cleanroute_dungeon::kMaxTeamMembers) {''')

# Manager help text: no checkbox-based membership anymore.
replace_once('src/dungeon_v45_methods.inl',
'''           L"Quy tắc: chỉ STOP/LỖI/XONG mới sửa. QUÉT CLIENT ở cửa sổ chính trước khi thêm acc. RoleID là identity; PID tự bind lại.\\r\\n"
''',
'''           L"Quy tắc: bảng thành viên chỉ hiện người của đội đang chọn. Dùng + THÊM / XÓA THÀNH VIÊN; KEY luôn là Slot 1 sau khi lưu.\\r\\n"
''')

# Version label cleanup inside manager/main surface.
text = read('src/dungeon_v45_methods.inl').replace('ThanLongDungeonManagerV45', 'ThanLongDungeonManagerV47')
write('src/dungeon_v45_methods.inl', text)

print('v4.7 UI/team/sell follow-up applied')
