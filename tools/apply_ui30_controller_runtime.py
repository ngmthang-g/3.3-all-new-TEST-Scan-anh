from pathlib import Path
import argparse

ap=argparse.ArgumentParser()
ap.add_argument('--source-root',required=True)
ap.add_argument('--output-dir',required=True)
a=ap.parse_args()
out_dir=Path(a.output_dir)
p=out_dir/'controller.cpp'
b=p.read_bytes(); text=b.decode('utf-8-sig'); nl='\r\n' if '\r\n' in text else '\n'; t=text.replace('\r\n','\n')

def once(old,new,label):
    global t
    c=t.count(old)
    if c!=1: raise SystemExit(f'{label}: expected1 got{c}')
    t=t.replace(old,new,1)

# Update row columns
once('''        SetRowText(row, 0, a.displayName);\n        SetRowText(row, 1, TradeRoleLabel(a.profile.tradeRole));\n        SetRowText(row, 2, std::to_wstring(a.game.pid));\n        if (a.profile.tradeRole == kMainTradeRole) {\n            SetRowText(row, 3, (a.runtime.running ? L"RUN • " : L"STOP • ") + MainVisibleStatus(a));\n        } else {\n            SetRowText(row, 3, (a.runtime.running ? L"RUN • " : L"STOP • ") + a.runtime.status);\n        }\n        if (a.snapshotValid && (a.snapshot.validMask & (ValidMap | ValidPosition)) == (ValidMap | ValidPosition)) {\n            std::wstring mapText = L"M" + std::to_wstring(a.snapshot.mapID) + L" • " +\n                                   std::to_wstring(a.snapshot.x) + L"," + std::to_wstring(a.snapshot.y);\n            if (a.snapshot.validMask & ValidBagSpace) mapText += L" • Trống " + std::to_wstring(a.snapshot.freeBagSpace);\n            SetRowText(row, 4, mapText);\n        } else {\n            SetRowText(row, 4, L"?");\n        }\n        if (a.profile.tradeRole == kMainTradeRole) {\n            SetRowText(row, 5, L"MAIN • đứng tại TỌA GD");\n        } else if (a.profile.target.valid) {\n            SetRowText(row, 5, a.profile.target.name + L" • M" + std::to_wstring(a.profile.target.mapID) +\n                             L" • " + std::to_wstring(a.profile.target.x) + L"," + std::to_wstring(a.profile.target.y));\n        } else {\n            SetRowText(row, 5, L"CHƯA CHỌN BÃI");\n        }''',
'''        SetRowText(row, 0, a.displayName);\n        SetRowText(row, 1, TradeRoleLabel(a.profile.tradeRole));\n        SetRowText(row, 2, a.profile.tradeRole == kMainTradeRole || a.profile.displayParty == 0 ? L"-" : L"PT" + std::to_wstring(a.profile.displayParty));\n        SetRowText(row, 3, std::to_wstring(a.game.pid));\n        if (a.profile.tradeRole == kMainTradeRole) {\n            SetRowText(row, 4, (a.runtime.running ? L"RUN • " : L"STOP • ") + MainVisibleStatus(a));\n        } else {\n            SetRowText(row, 4, (a.runtime.running ? L"RUN • " : L"STOP • ") + a.runtime.status);\n        }\n        if (a.snapshotValid && (a.snapshot.validMask & (ValidMap | ValidPosition)) == (ValidMap | ValidPosition)) {\n            std::wstring mapText = L"M" + std::to_wstring(a.snapshot.mapID) + L" • " +\n                                   std::to_wstring(a.snapshot.x) + L"," + std::to_wstring(a.snapshot.y);\n            if (a.snapshot.validMask & ValidBagSpace) mapText += L" • Trống " + std::to_wstring(a.snapshot.freeBagSpace);\n            SetRowText(row, 5, mapText);\n        } else {\n            SetRowText(row, 5, L"?");\n        }\n        if (a.profile.tradeRole == kMainTradeRole) {\n            SetRowText(row, 6, L"MAIN • đứng tại TỌA GD");\n        } else if (a.profile.target.valid) {\n            SetRowText(row, 6, a.profile.target.name + L" • M" + std::to_wstring(a.profile.target.mapID) +\n                             L" • " + std::to_wstring(a.profile.target.x) + L"," + std::to_wstring(a.profile.target.y));\n        } else {\n            SetRowText(row, 6, L"CHƯA CHỌN BÃI");\n        }''','row columns')
# InsertAccountRow redundant SetRowText PID on old col
once('''        ListView_InsertItem(clientList_, &item);\n        SetRowText(row, 2, std::to_wstring(a.game.pid));\n        UpdateAccountRow(row, a);''',
'''        ListView_InsertItem(clientList_, &item);\n        UpdateAccountRow(row, a);''','insert row pid')
# Role apply: clear party for main and refresh groups
once('''        selected->profile.tradeRole = newRole;\n        if (newRole == 1) {\n            selected->profile.enableSell = true;''',
'''        selected->profile.tradeRole = newRole;\n        if (newRole == 1) {\n            selected->profile.displayParty = 0;\n            selected->profile.enableSell = true;''','main party clear')
once('''        SaveProfile(selected->profile);\n        for (std::size_t i = 0; i < accounts_.size(); ++i) UpdateAccountRow(static_cast<int>(i), *accounts_[i]);\n        LoadSelectedProfileToUi();''',
'''        SaveProfile(selected->profile);\n        for (std::size_t i = 0; i < accounts_.size(); ++i) UpdateAccountRow(static_cast<int>(i), *accounts_[i]);\n        RefreshAccountGroups();\n        LoadSelectedProfileToUi();''','role refresh groups')
# Identity persistence includes party
once('''        const bool persistentHasData = persistent.tradeRole != 0 ||\n            !persistent.selectedSpot.empty() || persistent.target.valid || persistent.enableSell ||''',
'''        const bool persistentHasData = persistent.tradeRole != 0 || persistent.displayParty != 0 ||\n            !persistent.selectedSpot.empty() || persistent.target.valid || persistent.enableSell ||''','identity has party')
once('''            if (persistent.tradeRole == 0 && a.profile.tradeRole != 0) persistent.tradeRole = a.profile.tradeRole;\n            if (persistent.selectedSpot.empty() && !a.profile.selectedSpot.empty()) persistent.selectedSpot = a.profile.selectedSpot;''',
'''            if (persistent.tradeRole == 0 && a.profile.tradeRole != 0) persistent.tradeRole = a.profile.tradeRole;\n            if (persistent.displayParty == 0 && a.profile.displayParty != 0) persistent.displayParty = a.profile.displayParty;\n            if (persistent.selectedSpot.empty() && !a.profile.selectedSpot.empty()) persistent.selectedSpot = a.profile.selectedSpot;''','identity merge party')
# Notification: header PID + group task link
once('''        if (hdr->hwndFrom == clientList_ && hdr->code == LVN_ITEMCHANGED) {\n            const auto* n = reinterpret_cast<const NMLISTVIEW*>(hdr);\n            if ((n->uChanged & LVIF_STATE) != 0 && (n->uNewState & LVIS_SELECTED) != 0) {\n                PersistSelectedEditorSafeBeforeSwitch(n->iItem);\n                LoadSelectedProfileToUi();\n            }\n            return;\n        }''',
'''        if (clientList_ && hdr->hwndFrom == ListView_GetHeader(clientList_) && hdr->code == HDN_ITEMCLICKW) {\n            const auto* h = reinterpret_cast<const NMHEADERW*>(hdr);\n            if (h->iItem == 3) TogglePidColumn();\n            return;\n        }\n        if (hdr->hwndFrom == clientList_ && hdr->code == LVN_LINKCLICK) {\n            const auto* link = reinterpret_cast<const NMLVLINK*>(hdr);\n            CheckPartyGroup(link->iSubItem);\n            return;\n        }\n        if (hdr->hwndFrom == clientList_ && hdr->code == LVN_ITEMCHANGED) {\n            const auto* n = reinterpret_cast<const NMLISTVIEW*>(hdr);\n            if ((n->uChanged & LVIF_STATE) != 0 && (n->uNewState & LVIS_SELECTED) != 0) {\n                PersistSelectedEditorSafeBeforeSwitch(n->iItem);\n                LoadSelectedProfileToUi();\n            }\n            return;\n        }''','notify')
# Command cases
once('''                    case IDC_SCAN:\n                        ScanClients();\n                        break;\n                    case IDC_TEST_IMAGE_SCAN:\n                        OpenImageScanTest();\n                        break;\n                    case IDC_TRADE_ROLE:''',
'''                    case IDC_SCAN:\n                        ScanClients();\n                        break;\n                    case IDC_TEST_IMAGE_SCAN:\n                        OpenImageScanTest();\n                        break;\n                    case IDC_SELECT_ALL_ACCOUNTS:\n                        if (HIWORD(wp) == BN_CLICKED) SetAllAccountChecks(true);\n                        break;\n                    case IDC_CLEAR_ALL_ACCOUNTS:\n                        if (HIWORD(wp) == BN_CLICKED) SetAllAccountChecks(false);\n                        break;\n                    case IDC_ASSIGN_PARTY:\n                        if (HIWORD(wp) == BN_CLICKED) AssignPartyToChecked();\n                        break;\n                    case IDC_TRADE_ROLE:''','commands')
# members
once('''    HWND clientList_ = nullptr;\n    HWND selected_ = nullptr;\n    HWND live_ = nullptr;\n    HWND tradeRoleCombo_ = nullptr;''',
'''    HWND clientList_ = nullptr;\n    HWND selectAllButton_ = nullptr;\n    HWND clearAllButton_ = nullptr;\n    HWND partyCombo_ = nullptr;\n    HWND assignPartyButton_ = nullptr;\n    bool pidExpanded_ = false;\n    HWND selected_ = nullptr;\n    HWND live_ = nullptr;\n    HWND tradeRoleCombo_ = nullptr;''','members')
# stale strings/comments
for old,new in [
('CON1→CON12','CON1→CON30'),('CON1..CON12','CON1..CON30'),('CON1–CON12','CON1–CON30')]: t=t.replace(old,new)

# MAIN không giữ PT; compact vẫn giữ hai nút chọn nhanh; group dùng LVGROUP (Unicode-only struct).
once('''    p.displayParty = std::clamp(ReadIniInt(section, L"DisplayParty", 0), 0, kChildTradeCount);\n    p.tolerance = ReadIniInt(section, L"Tolerance", 120);''',
'''    p.displayParty = std::clamp(ReadIniInt(section, L"DisplayParty", 0), 0, kChildTradeCount);\n    if (p.tradeRole == kMainTradeRole) p.displayParty = 0;\n    p.tolerance = ReadIniInt(section, L"Tolerance", 120);''','sanitize main party')
once('''    bool IsCompactKeepControl(HWND h) const {\n        return h == clientList_ || h == scanButton_ ||\n               h == startCheckedButton_ || h == stopCheckedButton_ || h == compactButton_;\n    }''',
'''    bool IsCompactKeepControl(HWND h) const {\n        return h == clientList_ || h == selectAllButton_ || h == clearAllButton_ || h == scanButton_ ||\n               h == startCheckedButton_ || h == stopCheckedButton_ || h == compactButton_;\n    }''','compact keep')
once('''            SetWindowPos(hwnd_, nullptr, 0, 0, 1060, 310, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);''',
'''            SetWindowPos(hwnd_, nullptr, 0, 0, 1060, 350, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);''','compact height')

p.write_bytes(t.replace('\n',nl).encode('utf-8'))
print('apply_ui30_controller_runtime.py: PASS')
