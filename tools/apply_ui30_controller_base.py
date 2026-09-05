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

once('constexpr int kChildTradeCount = 12;','constexpr int kChildTradeCount = 30;','child30')
once('L"• CON1–CON12 train, dùng đường tắt và tự về TỌA GD khi full túi.\\r\\n"','L"• CON1–CON30 train, dùng đường tắt và tự về TỌA GD khi full túi.\\r\\n"','about30')
once('// 0=NONE, 1=MAIN, 2..13=CON1..CON12. Persisted by RoleID profile.\n    int tradeRole = 0;',
'''// 0=NONE, 1=MAIN, 2..31=CON1..CON30. Persisted by RoleID profile.\n    int tradeRole = 0;\n    // Chỉ là nhãn UI để gom/nhìn/chọn nhanh; tuyệt đối không tham gia workflow.\n    int displayParty = 0;''','profile party')
once('    p.tradeRole = ReadIniInt(section, L"TradeRole", 0);\n    if (p.tradeRole < 0 || p.tradeRole > kLastChildTradeRole) p.tradeRole = 0;',
'''    p.tradeRole = ReadIniInt(section, L"TradeRole", 0);\n    if (p.tradeRole < 0 || p.tradeRole > kLastChildTradeRole) p.tradeRole = 0;\n    p.displayParty = std::clamp(ReadIniInt(section, L"DisplayParty", 0), 0, kChildTradeCount);''','load party')
once('    WriteIniInt(p.section, L"TradeRole", p.tradeRole);',
'''    WriteIniInt(p.section, L"TradeRole", p.tradeRole);\n    WriteIniInt(p.section, L"DisplayParty", p.displayParty);''','save party')
# IDs
once('constexpr int IDC_SHORTCUT_SETTINGS = 215;','''constexpr int IDC_SHORTCUT_SETTINGS = 215;\nconstexpr int IDC_SELECT_ALL_ACCOUNTS = 216;\nconstexpr int IDC_CLEAR_ALL_ACCOUNTS = 217;\nconstexpr int IDC_PARTY_COMBO = 218;\nconstexpr int IDC_ASSIGN_PARTY = 219;''','ids')
# Build list block
old='''        clientList_ = Make(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER,\n                           18, 40, 1005, 157, IDC_CLIENT_LIST);\n        addFont(clientList_);\n        ListView_SetExtendedListViewStyle(clientList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);\n        AddListColumn(0, 170, L"Nhân vật / RoleID");\n        AddListColumn(1, 72, L"Vai trò");\n        AddListColumn(2, 62, L"PID");\n        AddListColumn(3, 220, L"Trạng thái");\n        AddListColumn(4, 190, L"Map / X,Y / Túi");\n        AddListColumn(5, 280, L"Bãi train");\n\n        scanButton_ = Make(L"BUTTON", L"QUÉT CLIENT", BS_PUSHBUTTON, 18, 205, 120, 30, IDC_SCAN); addFont(scanButton_);\n        startCheckedButton_ = Make(L"BUTTON", L"BẮT ĐẦU ACC TICK", BS_DEFPUSHBUTTON, 148, 205, 175, 30, IDC_START_CHECKED); addFont(startCheckedButton_);\n        stopCheckedButton_ = Make(L"BUTTON", L"DỪNG ACC TICK", BS_PUSHBUTTON, 333, 205, 155, 30, IDC_STOP_CHECKED); addFont(stopCheckedButton_);\n        addFont(Make(L"STATIC", L"Vai trò:", SS_LEFT | SS_CENTERIMAGE, 500, 205, 55, 30, 0));\n        tradeRoleCombo_ = Make(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL, 558, 205, 105, 330, IDC_TRADE_ROLE); addFont(tradeRoleCombo_);'''
new='''        selectAllButton_ = Make(L"BUTTON", L"CHỌN TẤT CẢ", BS_PUSHBUTTON, 18, 40, 105, 22, IDC_SELECT_ALL_ACCOUNTS); addFont(selectAllButton_);\n        clearAllButton_ = Make(L"BUTTON", L"BỎ TẤT CẢ", BS_PUSHBUTTON, 128, 40, 95, 22, IDC_CLEAR_ALL_ACCOUNTS); addFont(clearAllButton_);\n        addFont(Make(L"STATIC", L"PT:", SS_LEFT | SS_CENTERIMAGE, 235, 40, 25, 22, 0));\n        partyCombo_ = Make(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL, 262, 39, 80, 350, IDC_PARTY_COMBO); addFont(partyCombo_);\n        SendMessageW(partyCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"KHÔNG PT"));\n        for (int party = 1; party <= kChildTradeCount; ++party) {\n            const std::wstring label = L"PT" + std::to_wstring(party);\n            SendMessageW(partyCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));\n        }\n        SendMessageW(partyCombo_, CB_SETCURSEL, 0, 0);\n        assignPartyButton_ = Make(L"BUTTON", L"GÁN PT", BS_PUSHBUTTON, 348, 40, 78, 22, IDC_ASSIGN_PARTY); addFont(assignPartyButton_);\n\n        clientList_ = Make(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER,\n                           18, 66, 1005, 220, IDC_CLIENT_LIST);\n        addFont(clientList_);\n        ListView_SetExtendedListViewStyle(clientList_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);\n        ListView_EnableGroupView(clientList_, TRUE);\n        AddListColumn(0, 170, L"Nhân vật / RoleID");\n        AddListColumn(1, 72, L"Vai trò");\n        AddListColumn(2, 48, L"PT");\n        AddListColumn(3, 24, L"▶");\n        AddListColumn(4, 220, L"Trạng thái");\n        AddListColumn(5, 190, L"Map / X,Y / Túi");\n        AddListColumn(6, 280, L"Bãi train");\n\n        scanButton_ = Make(L"BUTTON", L"QUÉT CLIENT", BS_PUSHBUTTON, 18, 294, 120, 30, IDC_SCAN); addFont(scanButton_);\n        startCheckedButton_ = Make(L"BUTTON", L"BẮT ĐẦU ACC TICK", BS_DEFPUSHBUTTON, 148, 294, 175, 30, IDC_START_CHECKED); addFont(startCheckedButton_);\n        stopCheckedButton_ = Make(L"BUTTON", L"DỪNG ACC TICK", BS_PUSHBUTTON, 333, 294, 155, 30, IDC_STOP_CHECKED); addFont(stopCheckedButton_);\n        addFont(Make(L"STATIC", L"Vai trò:", SS_LEFT | SS_CENTERIMAGE, 500, 294, 55, 30, 0));\n        tradeRoleCombo_ = Make(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL, 558, 294, 105, 500, IDC_TRADE_ROLE); addFont(tradeRoleCombo_);'''
once(old,new,'list build')

p.write_bytes(t.replace('\n',nl).encode('utf-8'))
print('apply_ui30_controller_base.py: PASS')
