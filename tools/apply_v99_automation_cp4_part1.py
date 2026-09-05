# ---------------------------------------------------------------------------
# Protocol / bridge semantic: exact callback for "Mời vào nhóm".
# ---------------------------------------------------------------------------
protocol_path = out / 'protocol.h'
protocol, protocol_nl = load(protocol_path)
protocol = once(protocol,
'''    DenCacMonPhai = 3,\n    Trade = 4,\n};\n''',
'''    DenCacMonPhai = 3,\n    Trade = 4,\n    InviteParty = 5,\n};\n''', 'invite semantic enum')
save(protocol_path, protocol, protocol_nl)

bridge_path = out / 'bridge.cpp'
bridge, bridge_nl = load(bridge_path)
bridge = once(bridge,
'''        case TravelSemantic::DenCacMonPhai: return key == L"dencacmonphai";\n        case TravelSemantic::Trade: return key == L"giaodich";\n        default: return false;\n''',
'''        case TravelSemantic::DenCacMonPhai: return key == L"dencacmonphai";\n        case TravelSemantic::Trade: return key == L"giaodich";\n        case TravelSemantic::InviteParty: return key == L"moivaonhom";\n        default: return false;\n''', 'invite semantic token')
save(bridge_path, bridge, bridge_nl)

# ---------------------------------------------------------------------------
# Controller CP4.
# ---------------------------------------------------------------------------
p = out / 'controller.cpp'
t, nl = load(p)

t = once(t,
'''#include "automation_bulk_logic.h"\n''',
'''#include "automation_bulk_logic.h"\n#include "party_build_logic.h"\n''', 'include party logic')

t = once(t,
'''constexpr int IDC_GATHER_CAPTURE = 222;\nconstexpr int IDC_GATHER_TOGGLE = 223;\n''',
'''constexpr int IDC_GATHER_CAPTURE = 222;\nconstexpr int IDC_GATHER_TOGGLE = 223;\nconstexpr int IDC_SET_PARTY_KEY = 224;\nconstexpr int IDC_PARTY_BUILD_TOGGLE = 225;\nconstexpr int IDC_PB_CAPTURE_CLICK1 = 226;\nconstexpr int IDC_PB_CAPTURE_CLICK2 = 227;\nconstexpr int IDC_PB_CAPTURE_FACE = 228;\nconstexpr int IDC_PB_TEST_CLICK1 = 229;\nconstexpr int IDC_PB_TEST_CLICK2 = 230;\nconstexpr int IDC_PB_TEST_FACE = 231;\nconstexpr int IDC_PB_DELAY_CLICK1 = 232;\nconstexpr int IDC_PB_DELAY_CLICK2 = 233;\nconstexpr int IDC_PB_DELAY_FACE = 234;\nconstexpr int IDC_PB_TARGET_RETRY = 235;\nconstexpr int IDC_PB_INVITE_RETRY = 236;\n''', 'cp4 ids')

# Global settings data.
t = once(t,
'''struct ShortcutSettings {\n''',
'''struct PartyBuildSettings {\n    std::array<ClickPoint, 3> clicks{}; // 0=KEY click1, 1=KEY click2, 2=member face\n    std::array<int, 3> delaysMs{{300, 300, 700}};\n    int targetRetry = 6;\n    int inviteRetry = 8;\n};\n\nstruct ShortcutSettings {\n''', 'party settings struct')

# Per-account key persistence.
t = once(t,
'''    // Chỉ là nhãn UI để gom/nhìn/chọn nhanh; tuyệt đối không tham gia workflow.\n    int displayParty = 0;\n    std::wstring selectedSpot;\n''',
'''    // Chỉ là nhãn UI để gom/nhìn/chọn nhanh; tuyệt đối không tham gia workflow NORMAL.\n    int displayParty = 0;\n    // CP4: chỉ dùng bởi chế độ độc quyền TỰ TẠO PT. Mỗi PT được phép có đúng một KEY.\n    bool partyKey = false;\n    std::wstring selectedSpot;\n''', 'party key profile field')

t = once(t,
'''    p.displayParty = std::clamp(ReadIniInt(section, L"DisplayParty", 0), 0, kChildTradeCount);\n    if (p.tradeRole == kMainTradeRole) p.displayParty = 0;\n''',
'''    p.displayParty = std::clamp(ReadIniInt(section, L"DisplayParty", 0), 0, kChildTradeCount);\n    p.partyKey = ReadIniInt(section, L"PartyKey", 0) != 0;\n    if (p.tradeRole == kMainTradeRole) { p.displayParty = 0; p.partyKey = false; }\n    if (p.displayParty <= 0) p.partyKey = false;\n''', 'load party key')

t = once(t,
'''    WriteIniInt(p.section, L"TradeRole", p.tradeRole);\n    WriteIniInt(p.section, L"DisplayParty", p.displayParty);\n''',
'''    WriteIniInt(p.section, L"TradeRole", p.tradeRole);\n    WriteIniInt(p.section, L"DisplayParty", p.displayParty);\n    WriteIniInt(p.section, L"PartyKey", p.partyKey && p.tradeRole != kMainTradeRole && p.displayParty > 0 ? 1 : 0);\n''', 'save party key')

# Party builder settings persistence, independent from shortcut coordinates.
t = once(t,
'''std::array<SellNpcPosition, kSellNpcs.size()> LoadSharedSellNpcPositions() {\n''',
'''PartyBuildSettings LoadPartyBuildSettings() {\n    PartyBuildSettings s{};\n    const std::wstring section = L"PartyBuilder";\n    const std::array<std::wstring, 3> prefix{{L"KeyClick1", L"KeyClick2", L"MemberFace"}};\n    for (std::size_t i = 0; i < prefix.size(); ++i) {\n        ClickPoint& p = s.clicks[i];\n        p.x = ReadIniInt(section, prefix[i] + L"X", -1);\n        p.y = ReadIniInt(section, prefix[i] + L"Y", -1);\n        p.baseW = ReadIniInt(section, prefix[i] + L"W", 0);\n        p.baseH = ReadIniInt(section, prefix[i] + L"H", 0);\n        p.valid = ReadIniInt(section, prefix[i] + L"Valid", 0) != 0 &&\n                  p.x >= 0 && p.y >= 0 && p.baseW > 0 && p.baseH > 0;\n        const int fallback = i == 2 ? 700 : 300;\n        s.delaysMs[i] = std::clamp(ReadIniInt(section, prefix[i] + L"DelayMs", fallback), 0, 60000);\n    }\n    s.targetRetry = party_build_logic::ClampRetry(ReadIniInt(section, L"TargetRetry", 6));\n    s.inviteRetry = party_build_logic::ClampRetry(ReadIniInt(section, L"InviteRetry", 8));\n    return s;\n}\n\nvoid SavePartyBuildSettings(const PartyBuildSettings& s) {\n    EnsureUnicodeIni();\n    const std::wstring section = L"PartyBuilder";\n    const std::array<std::wstring, 3> prefix{{L"KeyClick1", L"KeyClick2", L"MemberFace"}};\n    for (std::size_t i = 0; i < prefix.size(); ++i) {\n        const ClickPoint& p = s.clicks[i];\n        WriteIniInt(section, prefix[i] + L"Valid", p.valid ? 1 : 0);\n        WriteIniInt(section, prefix[i] + L"X", p.valid ? p.x : -1);\n        WriteIniInt(section, prefix[i] + L"Y", p.valid ? p.y : -1);\n        WriteIniInt(section, prefix[i] + L"W", p.valid ? p.baseW : 0);\n        WriteIniInt(section, prefix[i] + L"H", p.valid ? p.baseH : 0);\n        WriteIniInt(section, prefix[i] + L"DelayMs", std::clamp(s.delaysMs[i], 0, 60000));\n    }\n    WriteIniInt(section, L"TargetRetry", party_build_logic::ClampRetry(s.targetRetry));\n    WriteIniInt(section, L"InviteRetry", party_build_logic::ClampRetry(s.inviteRetry));\n    FlushIni();\n}\n\nstd::array<SellNpcPosition, kSellNpcs.size()> LoadSharedSellNpcPositions() {\n''', 'party settings persistence')

# Runtime structs after Account.
t = once(t,
'''std::wstring TradeRoleLabel(int role) {\n''',
'''enum class PartyBuildPhase : int {\n    KeyClick1 = 0,\n    KeyClick2,\n    MemberTarget,\n    MemberFace,\n    MemberInvite,\n    Done,\n};\n\nstruct PartyBuildSession {\n    int party = 0;\n    DWORD keyPid = 0;\n    std::vector<DWORD> memberPids{};\n    std::size_t memberIndex = 0;\n    PartyBuildPhase phase = PartyBuildPhase::KeyClick1;\n    DWORD nextTick = 0;\n    DWORD phaseStartedTick = 0;\n    int targetAttempts = 0;\n    int inviteAttempts = 0;\n    int invited = 0;\n    int failed = 0;\n    std::vector<std::wstring> failures{};\n    bool summaryEmitted = false;\n};\n\nstd::wstring TradeRoleLabel(int role) {\n''', 'party runtime structs')

# Quick UI same row, compact.
t = once(t,
'''        addFont(Make(L"BUTTON", L"QUẢN LÝ NHANH • BÃI TRAIN / TẬP TRUNG", BS_GROUPBOX, 18, 662, 1005, 70, 0));\n        addFont(Make(L"BUTTON", L"ÁP BÃI PT", BS_PUSHBUTTON, 32, 686, 112, 28, IDC_APPLY_SPOT_PARTY));\n        addFont(Make(L"BUTTON", L"ÁP BÃI ALL CON", BS_PUSHBUTTON, 152, 686, 138, 28, IDC_APPLY_SPOT_ALL_CON));\n        gatherToggleButton_ = Make(L"BUTTON", L"TẬP TRUNG: OFF", BS_PUSHBUTTON, 304, 686, 145, 28, IDC_GATHER_TOGGLE); addFont(gatherToggleButton_);\n        addFont(Make(L"BUTTON", L"LẤY TỌA TẬP TRUNG", BS_PUSHBUTTON, 458, 686, 170, 28, IDC_GATHER_CAPTURE));\n        gatherLabel_ = Make(L"STATIC", L"CHƯA LẤY TỌA TẬP TRUNG", SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 638, 686, 365, 28, 0); addFont(gatherLabel_);\n''',
'''        addFont(Make(L"BUTTON", L"QUẢN LÝ NHANH • BÃI TRAIN / TẬP TRUNG / PT", BS_GROUPBOX, 18, 662, 1005, 70, 0));\n        addFont(Make(L"BUTTON", L"ÁP BÃI PT", BS_PUSHBUTTON, 32, 686, 100, 28, IDC_APPLY_SPOT_PARTY));\n        addFont(Make(L"BUTTON", L"ÁP ALL CON", BS_PUSHBUTTON, 140, 686, 112, 28, IDC_APPLY_SPOT_ALL_CON));\n        gatherToggleButton_ = Make(L"BUTTON", L"TẬP TRUNG: OFF", BS_PUSHBUTTON, 260, 686, 132, 28, IDC_GATHER_TOGGLE); addFont(gatherToggleButton_);\n        addFont(Make(L"BUTTON", L"LẤY TỌA TẬP TRUNG", BS_PUSHBUTTON, 400, 686, 155, 28, IDC_GATHER_CAPTURE));\n        gatherLabel_ = Make(L"STATIC", L"CHƯA TỌA", SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 563, 686, 150, 28, 0); addFont(gatherLabel_);\n        setPartyKeyButton_ = Make(L"BUTTON", L"ĐẶT KEY", BS_PUSHBUTTON, 721, 686, 92, 28, IDC_SET_PARTY_KEY); addFont(setPartyKeyButton_);\n        partyBuildToggleButton_ = Make(L"BUTTON", L"TỰ TẠO PT: OFF", BS_PUSHBUTTON, 821, 686, 182, 28, IDC_PARTY_BUILD_TOGGLE); addFont(partyBuildToggleButton_);\n''', 'cp4 quick ui')
