from pathlib import Path


def load(path):
    p = Path(path)
    data = p.read_bytes()
    nl = b'\r\n' if b'\r\n' in data else b'\n'
    return p, data, nl


def replace_once(data, old, new, label):
    count = data.count(old)
    if count != 1:
        raise SystemExit(f'{label}: expected exactly 1 anchor, got {count}')
    return data.replace(old, new, 1)


# protocol.h — dedicated command only; no SharedBlock layout change.
p, data, nl = load('src/protocol.h')
if b'TestOpenBag = 25' not in data:
    data = replace_once(
        data,
        b'constexpr std::uint32_t kProtocolVersion = 0x00030200u;',
        b'constexpr std::uint32_t kProtocolVersion = 0x00030300u;',
        'protocol version')
    old = b'    ConfirmTravelSemantic = 24,' + nl + b'};'
    new = b'    ConfirmTravelSemantic = 24,' + nl + b'    TestOpenBag = 25,' + nl + b'};'
    data = replace_once(data, old, new, 'TestOpenBag enum')
    p.write_bytes(data)


# bridge.cpp — TEST-ONLY semantic GUI path. No InputSync/click fallback.
p, data, nl = load('src/bridge.cpp')
if b'bool TestOpenBagSemantic(' not in data:
    marker = b'bool InvokeLuaAction(const char* uiName, const char* functionName,'
    if data.count(marker) != 1:
        raise SystemExit('bridge: InvokeLuaAction marker missing/ambiguous')

    block = r'''// TEST-ONLY OPEN BAG ---------------------------------------------------------
// Isolated probe requested for runtime validation. It never uses InputSync,
// coordinates, UIButton.HandleClickEvent, sell/trade state, or any fallback click.
// Remove this block + Command::TestOpenBag + controller test button after approval.
bool GuiCallReferenceParam(const MethodInfo* method, std::uint32_t index) {
    if (!method || index >= g_api.method_get_param_count(method)) return false;
    const Il2CppType* type = g_api.method_get_param(method, index);
    Il2CppClass* klass = type ? g_api.class_from_type(type) : nullptr;
    return klass && !g_api.class_is_valuetype(klass);
}

bool TrySemanticCallUi(const char* uiName, wchar_t* detail, std::size_t cap) {
    if (!EnsureUiLua(true, detail, cap)) return false;
    if (!uiName || !*uiName) { SetText(detail, cap, L"TEST BAG: tên UI rỗng"); return false; }

    Il2CppString* managedName = g_api.string_new(uiName);
    Il2CppObject* emptyArgs = g_api.array_new(g_ui.systemObject, 0);
    if (!managedName || !emptyArgs) {
        SetText(detail, cap, L"TEST BAG: không tạo được managed name/object[]");
        return false;
    }

    const char* methods[] = {"MainCallUI", "CallUI"};
    for (const char* methodName : methods) {
        for (int argc = 1; argc <= 3; ++argc) {
            const MethodInfo* method = FindMethod(g_ui.guiApi, methodName, argc);
            if (!method || !StaticMethod(method) || !ParamType(method, 0, "System.String")) continue;

            Il2CppObject* null1 = nullptr;
            Il2CppObject* null2 = nullptr;
            void* args[3] = {&managedName, nullptr, nullptr};
            bool compatible = true;
            if (argc >= 2) {
                if (ParamType(method, 1, "System.Object[]")) args[1] = &emptyArgs;
                else if (GuiCallReferenceParam(method, 1)) args[1] = &null1;
                else compatible = false;
            }
            if (argc >= 3) {
                if (ParamType(method, 2, "System.Object[]")) args[2] = &emptyArgs;
                else if (GuiCallReferenceParam(method, 2)) args[2] = &null2;
                else compatible = false;
            }
            if (!compatible) continue;

            void* exc = nullptr;
            (void)g_api.runtime_invoke(method, nullptr, args, &exc);
            if (exc) continue;

            SetText(detail, cap, L"TEST BAG semantic ");
            Append(detail, cap, methodName[0] == 'M' ? L"MainCallUI" : L"CallUI");
            Append(detail, cap, L" argc=");
            AppendInt(detail, cap, argc);
            Append(detail, cap, L" • ");
            wchar_t wideName[96]{};
            MultiByteToWideChar(CP_UTF8, 0, uiName, -1, wideName, static_cast<int>(_countof(wideName)));
            Append(detail, cap, wideName);
            return true;
        }
    }

    SetText(detail, cap, L"TEST BAG: không có overload MainCallUI/CallUI tương thích");
    return false;
}

bool TestOpenBagSemantic(bool verifyOnly, Response& response,
                         wchar_t* detail, std::size_t cap) {
    if (!EnsureUiLua(true, detail, cap)) return false;

    Il2CppObject* bagUi = nullptr;
    wchar_t findDetail[192]{};
    if (FindUiByName("RoleInfo_BagTab", bagUi, findDetail, _countof(findDetail)) && bagUi) {
        response.resultCode = static_cast<std::int32_t>(ActionResult::StageReady);
        response.value0 = 1;
        SetText(detail, cap, L"TEST BAG VERIFY PASS • RoleInfo_BagTab đang tồn tại");
        return true;
    }
    if (verifyOnly) {
        SetText(detail, cap, L"TEST BAG VERIFY FAIL • chưa thấy RoleInfo_BagTab");
        return false;
    }

    Classes c{};
    if (!ResolveClasses(c, detail, cap) || !SafeForAction(c, detail, cap)) return false;

    wchar_t parentDetail[192]{};
    wchar_t bagDetail[192]{};
    const bool parentCalled = TrySemanticCallUi("RoleInfo", parentDetail, _countof(parentDetail));
    const bool bagCalled = TrySemanticCallUi("RoleInfo_BagTab", bagDetail, _countof(bagDetail));
    response.value0 = parentCalled ? 1 : 0;
    response.value1 = bagCalled ? 1 : 0;
    if (!bagCalled) {
        SetText(detail, cap, L"TEST BAG OPEN FAIL • parent=");
        Append(detail, cap, parentCalled ? L"PASS" : L"FAIL");
        Append(detail, cap, L" • ");
        Append(detail, cap, bagDetail);
        return false;
    }

    response.resultCode = static_cast<std::int32_t>(ActionResult::ActionInvoked);
    SetText(detail, cap, L"TEST BAG OPEN DISPATCH PASS • parent=");
    Append(detail, cap, parentCalled ? L"PASS" : L"FAIL");
    Append(detail, cap, L" • ");
    Append(detail, cap, bagDetail);
    return true;
}
// END TEST-ONLY OPEN BAG -----------------------------------------------------

'''.replace('\n', '\r\n' if nl == b'\r\n' else '\n').encode('utf-8')
    data = data.replace(marker, block + marker, 1)

    old = (b'            case Command::ConfirmTravelSemantic:' + nl +
           b'                ok = ConfirmTravelSemantic(r, detail, _countof(detail)); break;' + nl +
           b'            case Command::ToggleRide:')
    new = (b'            case Command::ConfirmTravelSemantic:' + nl +
           b'                ok = ConfirmTravelSemantic(r, detail, _countof(detail)); break;' + nl +
           b'            case Command::TestOpenBag:' + nl +
           b'                ok = TestOpenBagSemantic(g_shared->request.arg0 != 0, r, detail, _countof(detail)); break;' + nl +
           b'            case Command::ToggleRide:')
    data = replace_once(data, old, new, 'bridge command switch')
    p.write_bytes(data)


# controller.cpp — one visible test button + one isolated handler/method.
p, data, nl = load('src/controller.cpp')
if b'IDC_TEST_OPEN_BAG' not in data:
    old = b'constexpr int IDC_SHORTCUT_SETTINGS = 215;'
    new = old + nl + b'constexpr int IDC_TEST_OPEN_BAG = 216; // TEST-ONLY semantic open bag probe'
    data = replace_once(data, old, new, 'controller test id')

    old = '        addFont(Make(L"STATIC", L"Không foreground/không chiếm chuột", 0, 878, 350, 145, 22, 0));'.encode('utf-8')
    new = old + nl + '        addFont(Make(L"BUTTON", L"TEST MỞ TAY NẢI • KHÔNG CLICK", BS_PUSHBUTTON, 18, 386, 290, 30, IDC_TEST_OPEN_BAG));'.encode('utf-8')
    data = replace_once(data, old, new, 'controller test button')

    old = (b'                    case IDC_CAPTURE_STOP_AUTO_2:' + nl +
           b'                        BeginCapture(ClickSlot::StopAuto2);' + nl +
           b'                        break;' + nl +
           b'                    case IDC_TEST_AUTO:')
    new = (b'                    case IDC_CAPTURE_STOP_AUTO_2:' + nl +
           b'                        BeginCapture(ClickSlot::StopAuto2);' + nl +
           b'                        break;' + nl +
           b'                    case IDC_TEST_OPEN_BAG:' + nl +
           b'                        TestOpenBag();' + nl +
           b'                        break;' + nl +
           b'                    case IDC_TEST_AUTO:')
    data = replace_once(data, old, new, 'controller WM_COMMAND')

    marker = b'    void TestClick(ClickSlot slot) {'
    if data.count(marker) != 1:
        raise SystemExit('controller: TestClick marker missing/ambiguous')
    block = r'''    // TEST-ONLY: isolated from Auto/Trade/Sell/route state machines.
    void TestOpenBag() {
        Account* a = SelectedAccount();
        if (!a) { Log(L"TEST TAY NẢI: chưa chọn acc"); return; }
        std::wstring attachError;
        if (!EnsureAttach(*a, attachError)) {
            LogAccount(*a, L"TEST TAY NẢI: không attach được Bridge • " + attachError);
            return;
        }

        Response openResponse{};
        std::wstring openError;
        if (!a->bridge.Call(Command::TestOpenBag, 0, 0, 0,
                            openResponse, openError, 2200)) {
            LogAccount(*a, L"TEST TAY NẢI OPEN FAIL • " + openError);
            return;
        }
        LogAccount(*a, L"TEST TAY NẢI OPEN • " + std::wstring(openResponse.detail));

        // MainCallUI may materialize the Lua panel on the following UI frame.
        // This short wait is only for the manual TEST button and never enters Auto logic.
        Sleep(350);
        Response verifyResponse{};
        std::wstring verifyError;
        if (a->bridge.Call(Command::TestOpenBag, 1, 0, 0,
                           verifyResponse, verifyError, 1200)) {
            LogAccount(*a, L"TEST TAY NẢI VERIFY PASS • " + std::wstring(verifyResponse.detail));
        } else {
            LogAccount(*a, L"TEST TAY NẢI VERIFY FAIL • " + verifyError);
        }
    }

'''.replace('\n', '\r\n' if nl == b'\r\n' else '\n').encode('utf-8')
    data = data.replace(marker, block + marker, 1)
    p.write_bytes(data)


# Isolation and contract checks.
bridge = Path('src/bridge.cpp').read_text(encoding='utf-8')
start = bridge.index('// TEST-ONLY OPEN BAG')
end = bridge.index('// END TEST-ONLY OPEN BAG')
test_block = bridge[start:end]
for token in ['ClickInternalPoint(', 'InvokeInternalPointClick(', 'HandleClickEvent', 'InputSyncManager']:
    if token in test_block:
        raise SystemExit(f'TEST BAG isolation violated by token: {token}')

protocol = Path('src/protocol.h').read_text(encoding='utf-8')
controller = Path('src/controller.cpp').read_text(encoding='utf-8')
for token in ['TestOpenBag = 25', 'IDC_TEST_OPEN_BAG', 'TEST MỞ TAY NẢI', 'Command::TestOpenBag']:
    if token not in protocol + controller + bridge:
        raise SystemExit(f'missing contract token: {token}')

print('isolated TEST OPEN BAG patch contracts PASS')
