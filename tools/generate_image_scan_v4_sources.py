from pathlib import Path
import argparse

ap = argparse.ArgumentParser()
ap.add_argument('--source-root', required=True)
ap.add_argument('--output-dir', required=True)
a = ap.parse_args()
root = Path(a.source_root)
out = Path(a.output_dir)
out.mkdir(parents=True, exist_ok=True)

def once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise SystemExit(f'{label}: expected one marker, found {count}')
    return text.replace(old, new, 1)

def replace_between(text, start, end, replacement, label):
    if text.count(start) != 1:
        raise SystemExit(f'{label} start: expected one marker, found {text.count(start)}')
    i = text.index(start)
    j = text.find(end, i + len(start))
    if j < 0:
        raise SystemExit(f'{label} end: marker not found')
    return text[:i] + replacement + text[j:]

# protocol.h: isolated raw scan command. Keep protocol version pinned by v9.9 contracts.
p = (root/'src/protocol.h').read_text(encoding='utf-8-sig')
p = once(p,
'''    ClickTravelSemantic = 23,\n    ConfirmTravelSemantic = 24,\n    TestOpenBag = 25,\n};''',
'''    ClickTravelSemantic = 23,\n    ConfirmTravelSemantic = 24,\n    TestOpenBag = 25,\n    // FILTER V4 only: image recognition -> raw InputSync click, independent of AUTO state.\n    ClickInternalPointRawTest = 26,\n};''', 'protocol command')
(out/'protocol.h').write_text(p, encoding='utf-8', newline='\n')

# bridge.cpp: raw InputSync primitive for FILTER V4, bypassing AUTO SafeForAction only.
b = (root/'src/bridge.cpp').read_text(encoding='utf-8-sig')
needle = '''bool ClickInternalPoint(int normalizedX, int normalizedY, bool rawMacro, Response& response,\n                        wchar_t* detail, std::size_t cap) {\n    Classes c{};\n    if (!ResolveClasses(c, detail, cap) || !SafeForAction(c, detail, cap)) return false;\n    if (!InvokeInternalPointClick(normalizedX, normalizedY, detail, cap, rawMacro)) return false;\n    response.resultCode = static_cast<std::int32_t>(ActionResult::ActionInvoked);\n    SetText(detail, cap, rawMacro\n        ? L"RAW MACRO InputSync: TryClickUI → EndUIDrag; không gate theo raycast/_uiDragging"\n        : L"InputSync click nội bộ hoàn chỉnh: TryClickUI → EndUIDrag");\n    return true;\n}\n'''
raw = needle + '''\nbool ClickInternalPointRawTest(int normalizedX, int normalizedY, Response& response,\n                               wchar_t* detail, std::size_t cap) {\n    if (!InvokeInternalPointClick(normalizedX, normalizedY, detail, cap, true)) return false;\n    response.resultCode = static_cast<std::int32_t>(ActionResult::ActionInvoked);\n    SetText(detail, cap, L"RAW SCAN InputSync PASS: TryClickUI → EndUIDrag • không SafeForAction");\n    return true;\n}\n'''
b = once(b, needle, raw, 'bridge raw function')
b = once(b,
'''            case Command::ClickInternalPoint:\n                ok = ClickInternalPoint(g_shared->request.arg0, g_shared->request.arg1,\n                                        g_shared->request.arg2 != 0,\n                                        r, detail, _countof(detail)); break;\n            case Command::BeginBackgroundTreatment:''',
'''            case Command::ClickInternalPoint:\n                ok = ClickInternalPoint(g_shared->request.arg0, g_shared->request.arg1,\n                                        g_shared->request.arg2 != 0,\n                                        r, detail, _countof(detail)); break;\n            case Command::ClickInternalPointRawTest:\n                ok = ClickInternalPointRawTest(g_shared->request.arg0, g_shared->request.arg1,\n                                               r, detail, _countof(detail)); break;\n            case Command::BeginBackgroundTreatment:''', 'bridge command switch')
(out/'bridge.cpp').write_text(b, encoding='utf-8', newline='\n')

# controller.cpp: generated integration keeps original huge core file untouched in the repo.
c = (root/'src/controller.cpp').read_text(encoding='utf-8-sig')
c = once(c, '#include "thdc_route_logic.h"\n', '#include "thdc_route_logic.h"\n#include "image_scan_test.h"\n', 'controller include')
c = once(c, 'constexpr int IDC_SCAN = 101;\n', 'constexpr int IDC_SCAN = 101;\nconstexpr int IDC_TEST_IMAGE_SCAN = 106;\n', 'controller id')

method = '''    int ChildScanSlot(const Account& account) const {\n        return account.profile.tradeRole >= kFirstChildTradeRole && account.profile.tradeRole <= kLastChildTradeRole\n            ? account.profile.tradeRole - 1 : 0;\n    }\n\n    image_scan_test::Target MakeImageScanTarget(Account& account) {\n        image_scan_test::Target target{};\n        target.owner = hwnd_;\n        target.gameWindow = account.game.window;\n        target.accountLabel = AccountTag(account);\n        target.context = &account;\n        target.hiddenClick = [](void* context, int x, int y, int clientW, int clientH, std::wstring& detail) -> bool {\n            Account* acc = static_cast<Account*>(context);\n            if (!acc || !acc->bridge.Attached()) { detail = L"bridge chưa attach"; return false; }\n            const int nx = fixed_slot_sell_logic::NormalizeClientCoordinate(x, clientW);\n            const int ny = fixed_slot_sell_logic::NormalizeClientCoordinate(y, clientH);\n            if (nx < 0 || ny < 0) { detail = L"không chuẩn hóa được tọa độ match"; return false; }\n            Response response{}; std::wstring error;\n            if (!acc->bridge.Call(Command::ClickInternalPointRawTest, nx, ny, 0, response, error, 1800)) {\n                detail = error; return false;\n            }\n            detail = response.detail[0] ? response.detail : L"TryClickUI → EndUIDrag PASS";\n            return true;\n        };\n        return target;\n    }\n\n    void OpenImageScanTest() {\n        Account* account = SelectedAccount();\n        if (!account) { Log(L"SCAN LỌC VK: hãy chọn 1 acc trong danh sách client trước."); return; }\n        if (!account->game.window || !IsWindow(account->game.window)) {\n            LogAccount(*account, L"SCAN LỌC VK: HWND game không còn tồn tại."); return;\n        }\n        std::wstring attachError;\n        if (!EnsureAttach(*account, attachError)) {\n            LogAccount(*account, L"SCAN LỌC VK: không attach được bridge • " + attachError); return;\n        }\n        LogAccount(*account, L"MỞ SCAN LỌC VK V4 • cấu hình CON/open/close/export-import + test FILTER V4.");\n        image_scan_test::RunDialog(MakeImageScanTarget(*account));\n    }\n\n'''
c = once(c, '    void BuildUi() {\n', method + '    void BuildUi() {\n', 'controller scan methods')
c = once(c,
'''        addFont(Make(L"BUTTON", L"XUẤT TẤT CẢ", BS_PUSHBUTTON, 678, 628, 145, 27, IDC_EXPORT_CLICK_CONFIG));\n        addFont(Make(L"BUTTON", L"NHẬP TẤT CẢ", BS_PUSHBUTTON, 831, 628, 145, 27, IDC_IMPORT_CLICK_CONFIG));\n''',
'''        addFont(Make(L"BUTTON", L"XUẤT TẤT CẢ", BS_PUSHBUTTON, 678, 628, 145, 27, IDC_EXPORT_CLICK_CONFIG));\n        addFont(Make(L"BUTTON", L"NHẬP TẤT CẢ", BS_PUSHBUTTON, 831, 628, 145, 27, IDC_IMPORT_CLICK_CONFIG));\n        addFont(Make(L"BUTTON", L"SCAN LỌC VK V4", BS_PUSHBUTTON, 18, 656, 180, 27, IDC_TEST_IMAGE_SCAN));\n''', 'controller scan button')
c = once(c,
'''                    case IDC_SCAN:\n                        ScanClients();\n                        break;\n''',
'''                    case IDC_SCAN:\n                        ScanClients();\n                        break;\n                    case IDC_TEST_IMAGE_SCAN:\n                        OpenImageScanTest();\n                        break;\n''', 'controller command')

# Restore the existing sell-NPC coordinate editor that was accidentally hidden from BuildUi.
# Runtime/data/handlers already exist; this only recreates the visible controls.
c = once(c,
'''        // Dòng mô tả kỹ thuật InputSync/callback được ẩn khỏi giao diện khách hàng.\n''',
'''        addFont(Make(L"STATIC", L"NPC BÁN:", SS_LEFT | SS_CENTERIMAGE, 18, 382, 62, 27, 0));\n        sellNpcCombo_ = Make(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL, 82, 378, 465, 260, IDC_SELL_NPC); addFont(sellNpcCombo_);\n        for (const auto& npc : kSellNpcs)\n            SendMessageW(sellNpcCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(npc.name));\n        SendMessageW(sellNpcCombo_, CB_SETCURSEL, 0, 0);\n        addFont(Make(L"STATIC", L"X:", SS_LEFT | SS_CENTERIMAGE, 557, 382, 18, 27, 0));\n        sellNpcX_ = Make(L"EDIT", L"", WS_BORDER | ES_NUMBER | ES_CENTER, 577, 378, 72, 27, IDC_SELL_NPC_X); addFont(sellNpcX_);\n        addFont(Make(L"STATIC", L"Y:", SS_LEFT | SS_CENTERIMAGE, 658, 382, 18, 27, 0));\n        sellNpcY_ = Make(L"EDIT", L"", WS_BORDER | ES_NUMBER | ES_CENTER, 678, 378, 72, 27, IDC_SELL_NPC_Y); addFont(sellNpcY_);\n        addFont(Make(L"BUTTON", L"LẤY TỌA NPC BÁN", BS_PUSHBUTTON, 760, 378, 190, 27, IDC_SELL_NPC_CAPTURE));\n        addFont(Make(L"STATIC", L"TỌA NPC:", SS_LEFT | SS_CENTERIMAGE, 18, 416, 70, 27, 0));\n        sellNpcPosText_ = Make(L"STATIC", L"CHƯA LẤY", SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 90, 414, 860, 27, IDC_SELL_NPC_POS); addFont(sellNpcPosText_);\n\n        // Dòng mô tả kỹ thuật InputSync/callback được ẩn khỏi giao diện khách hàng.\n''', 'restore sell NPC coordinate UI')

# START/STOP reset each CON scan session; settings themselves remain global FILTER config.
c = once(c,
'''            a.deathSessionLatched = false;\n            a.runtime.running = true;\n            ResetRuntime(a.runtime);\n            a.runtime.running = true;\n''',
'''            a.deathSessionLatched = false;\n            a.runtime.running = true;\n            ResetRuntime(a.runtime);\n            a.runtime.running = true;\n            if (const int scanSlot = ChildScanSlot(a); scanSlot > 0)\n                image_scan_test::ResetAutoFilter(a.game.window, scanSlot);\n''', 'start scan reset')
c = once(c,
'''    void StopAccount(Account& a) {\n        const bool wasFrozen = a.runtime.clientFreezeActive;\n''',
'''    void StopAccount(Account& a) {\n        const bool wasFrozen = a.runtime.clientFreezeActive;\n        if (const int scanSlot = ChildScanSlot(a); scanSlot > 0)\n            image_scan_test::StopAutoFilter(a.game.window, scanSlot);\n''', 'stop scan reset')

# Death: scanner yields immediately; revive callback remains P2 priority, then one CloseBag click.
c = once(c,
'''        // Keep the existing FIRST-death boundary in the priority pre-pass so a click\n        // sequence in another window cannot postpone detecting this account's death.\n        // HandleDeath() sees deathSessionLatched and therefore does not reset twice.\n        ResetRuntimeForLifeBoundary(a);\n''',
'''        // Keep the existing FIRST-death boundary in the priority pre-pass so a click\n        // sequence in another window cannot postpone detecting this account's death.\n        // FILTER V4 yields immediately; revive P2 owns priority and CloseBag runs only AFTER revive PASS.\n        if (const int scanSlot = ChildScanSlot(a); scanSlot > 0)\n            image_scan_test::NotifyDeath(a.game.window, scanSlot);\n        // HandleDeath() sees deathSessionLatched and therefore does not reset twice.\n        ResetRuntimeForLifeBoundary(a);\n''', 'death scan yield')
c = once(c,
'''            a.runtime.status = L"ĐẦU THAI NỘI BỘ PASS • callback đúng acc chết • không chiếm chuột";\n            LogAccount(a, L"ĐẦU THAI P2 PASS: Bridge xác minh IsDeath rồi gọi UIButton.HandleClickEvent nội bộ; chuỗi acc khác không mất index/repeat.");\n            return true;\n''',
'''            a.runtime.status = L"ĐẦU THAI NỘI BỘ PASS • callback đúng acc chết • không chiếm chuột";\n            LogAccount(a, L"ĐẦU THAI P2 PASS: Bridge xác minh IsDeath rồi gọi UIButton.HandleClickEvent nội bộ; chuỗi acc khác không mất index/repeat.");\n            if (const int scanSlot = ChildScanSlot(a); scanSlot > 0)\n                image_scan_test::NotifyReviveClicked(MakeImageScanTarget(a), scanSlot);\n            return true;\n''', 'revive then close bag')

# Initial AutoFight success arms steady train exactly once; periodic 60s checks are intentionally dormant.
c = once(c,
'''            if (!rt.trainPositionMonitorArmed) {\n                rt.trainPositionMonitorArmed = true;\n                rt.lastTrainPositionCheckTick = now;\n                LogAccount(a, L"AutoFight ON • bắt đầu check tọa độ train 1 phút/lần.");\n            }\n            rt.lastAutoFightCheckTick = now;\n            rt.status = L"Đúng bãi • AutoFight ON • check Auto mỗi 1 phút";\n''',
'''            if (!rt.trainPositionMonitorArmed) {\n                rt.trainPositionMonitorArmed = true;\n                rt.lastTrainPositionCheckTick = now;\n                if (const int scanSlot = ChildScanSlot(a); scanSlot > 0)\n                    image_scan_test::ResetAutoFilter(a.game.window, scanSlot);\n                LogAccount(a, L"AutoFight ON • train ổn định • tạm ngắt check tọa/AutoFight định kỳ 60s.");\n            }\n            rt.lastAutoFightCheckTick = now;\n            rt.status = L"Đúng bãi • AutoFight ON • periodic 60s OFF";\n''', 'fight already on status')
c = once(c,
'''                rt.status = L"AutoFight ON • P3 InputSync bật thành công";\n                LogAccount(a, L"PRIORITY #3 AUTO→ĐÁNH QUÁI InputSync verify AutoFight ON.");\n''',
'''                rt.status = L"AutoFight ON • P3 InputSync bật thành công • periodic 60s OFF";\n                LogAccount(a, L"PRIORITY #3 AUTO→ĐÁNH QUÁI InputSync verify ON • sau đó không check lại 60s.");\n''', 'fight verify status')
c = once(c,
'''                if (!rt.trainPositionMonitorArmed) {\n                    rt.trainPositionMonitorArmed = true;\n                    rt.lastTrainPositionCheckTick = now;\n                }\n                rt.status = L"AutoFight ON • P3 InputSync bật thành công • periodic 60s OFF";\n''',
'''                if (!rt.trainPositionMonitorArmed) {\n                    rt.trainPositionMonitorArmed = true;\n                    rt.lastTrainPositionCheckTick = now;\n                    if (const int scanSlot = ChildScanSlot(a); scanSlot > 0)\n                        image_scan_test::ResetAutoFilter(a.game.window, scanSlot);\n                }\n                rt.status = L"AutoFight ON • P3 InputSync bật thành công • periodic 60s OFF";\n''', 'fight verify new train scan reset')

steady_start = '''        // Semantic MessageBox Confirm remains disabled. Lâu Lan P1 XN is scheduled per account\n'''
steady_end = '''        // V3.0 SHORTCUT FIRST: the initial AUTO TRAIN route must use the same shortcut selector\n'''
steady = '''        // Steady train after the ONE initial AutoFight startup. Periodic coordinate and\n        // AutoFight 60s checks are intentionally disabled. FILTER V4 is the low-priority\n        // background work for enabled CONs; death/full/trade ownership remains above it.\n        if (rt.trainPositionMonitorArmed) {\n            const int scanSlot = ChildScanSlot(a);\n            if (scanSlot > 0 && image_scan_test::IsChildAutoFilterEnabled(scanSlot)) {\n                if ((s.validMask & ValidBagSpace) == 0) {\n                    rt.status = L"SCAN VK • chờ BagSpace authoritative";\n                    return;\n                }\n                const bool bagFull = s.freeBagSpace == 0;\n                const auto scan = image_scan_test::TickAutoFilter(MakeImageScanTarget(a), scanSlot, bagFull);\n                if (!scan.status.empty()) rt.status = scan.status;\n                else rt.status = L"Train ổn định • SCAN VK nền";\n                if (scan.state == image_scan_test::AutoFilterState::Error)\n                    rt.status += L" • train vẫn tiếp tục";\n                return;\n            }\n            rt.status = L"Train ổn định • periodic tọa/AutoFight 60s OFF";\n            return;\n        }\n\n'''
c = replace_between(c, steady_start, steady_end, steady, 'steady periodic block')

# FULL coordinator waits for FILTER V4 to finish current item + one CloseBag click.
c = once(c,
'''            Account* child = AccountByTradeRole(slot + 1);\n            if (!child || TradeTravelContains(child->game.pid)) continue;\n            const bool fullReady = TradeStateReady(*child) && child->snapshot.freeBagSpace == 0;\n            if (!fullReady) {\n                if (child) child->runtime.tradeAdmissionReport.clear();\n                continue;\n            }\n''',
'''            Account* child = AccountByTradeRole(slot + 1);\n            if (!child || TradeTravelContains(child->game.pid)) continue;\n            const bool fullRequested = child->snapshot.freeBagSpace == 0 ||\n                                       image_scan_test::FullBagTravelLatched(child->game.window, slot);\n            const bool fullReady = TradeStateReady(*child) && fullRequested;\n            if (!fullReady) {\n                if (child) child->runtime.tradeAdmissionReport.clear();\n                continue;\n            }\n            if (!image_scan_test::FullBagYieldReady(child->game.window, slot)) {\n                ReportChildAdmissionWait(*child, L"SCAN VK đang xử lý nốt item/đóng tay nải trước khi về TỌA GD");\n                continue;\n            }\n''', 'full scan handoff gate')

(out/'controller.cpp').write_text(c, encoding='utf-8', newline='\n')
print('IMAGE SCAN V5 AUTO-CON generated source integration PASS')

# FILTER V5 generated image scanner: keep original V4 split files untouched and apply
# exact line-index/hash patches fail-closed at configure time.
import hashlib
import sys
sys.path.insert(0, str(root/'tools'))
from image_scan_v5_patch_data import PATCHES

scan_parts = [
    'image_scan_filter_v4_part1.inl','image_scan_filter_v4_part2.inl','image_scan_filter_v4_part3.inl',
    'image_scan_filter_v4_part4.inl','image_scan_filter_v4_part5.inl','image_scan_filter_v4_part6.inl',
    'image_scan_filter_v4_part7a.inl','image_scan_filter_v4_part7b.inl','image_scan_filter_v4_part8a.inl','image_scan_filter_v4_part8b.inl'
]
scan_core = ''.join((root/'src'/name).read_text(encoding='utf-8-sig') for name in scan_parts)
scan_lines = scan_core.splitlines(True)
rendered = []
cursor = 0
for i1, i2, expected_hash, new_block, label in PATCHES:
    if i1 < cursor or i2 < i1 or i2 > len(scan_lines):
        raise SystemExit(f'{label}: invalid patch span {i1}:{i2}')
    old_block = ''.join(scan_lines[i1:i2])
    actual_hash = hashlib.sha256(old_block.encode('utf-8')).hexdigest()
    if actual_hash != expected_hash:
        raise SystemExit(f'{label}: FILTER V4 source drift; expected {expected_hash}, got {actual_hash}')
    rendered.extend(scan_lines[cursor:i1])
    rendered.append(new_block)
    cursor = i2
rendered.extend(scan_lines[cursor:])
scan_v5 = ''.join(rendered)
scan_v5 = once(scan_v5, 'constexpr std::size_t kAutoRequiredSlots = 42;\n', '', 'remove fixed 42 runtime requirement')
scan_v5 = once(scan_v5,
    'void RunDialog(const Target& target){\n    if(!target.owner||!target.gameWindow)return;\n    State state{};state.target=target;state.config=g_lastConfig;',
    'void RunDialog(const Target& target){\n    if(!target.owner||!target.gameWindow)return;\n    EnsurePersistentConfigLoaded();\n    State state{};state.target=target;state.config=g_lastConfig;',
    'load persistent scan config')
scan_v5 = once(scan_v5,
    's.config=c;g_lastConfig=c;RefreshAllConfigEditors(s);',
    's.config=c;g_lastConfig=c;SavePersistentConfig();RefreshAllConfigEditors(s);',
    'persist imported scan config')
scan_v5 = once(scan_v5,
    'g_lastConfig=state.config;EnableWindow(target.owner,TRUE);',
    'g_lastConfig=state.config;SavePersistentConfig();EnableWindow(target.owner,TRUE);',
    'persist scan config on dialog close')
scan_cpp = '#include "image_scan_test.h"\n\n' + scan_v5 + '\n#include "image_scan_auto_ext.inl"\n'
(out/'image_scan_test.cpp').write_text(scan_cpp, encoding='utf-8', newline='\n')
print('IMAGE SCAN V5 FILTER core generation PASS')
