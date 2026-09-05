# Gather and normal modes are mutually exclusive with party build.
t = once(t,
'''    void StartGatherMode() {\n        if (!gatherTarget_.valid) {\n''',
'''    void StartGatherMode() {\n        if (partyBuildModeActive_) StopPartyBuildMode(L"chuyển sang TẬP TRUNG");\n        if (!gatherTarget_.valid) {\n''', 'party to gather handoff')

t = once(t,
'''    void StartChecked() {\n        if (gatherModeActive_) {\n            Log(L"TẬP TRUNG đang ON • START auto thường bị chặn; hãy tắt TẬP TRUNG trước.");\n            return;\n        }\n''',
'''    void StartChecked() {\n        if (gatherModeActive_ || partyBuildModeActive_) {\n            Log(gatherModeActive_ ? L"TẬP TRUNG đang ON • START auto thường bị chặn; hãy tắt TẬP TRUNG trước." :\n                                   L"TỰ TẠO PT đang ON • START auto thường bị chặn; hãy tắt TỰ TẠO PT trước.");\n            return;\n        }\n''', 'block normal start in party')

t = once(t,
'''    void StopChecked() {\n        if (gatherModeActive_) {\n            Log(L"TẬP TRUNG đang ON • dùng nút TẬP TRUNG để tắt toàn bộ mode độc quyền.");\n            return;\n        }\n''',
'''    void StopChecked() {\n        if (gatherModeActive_ || partyBuildModeActive_) {\n            Log(gatherModeActive_ ? L"TẬP TRUNG đang ON • dùng nút TẬP TRUNG để tắt mode độc quyền." :\n                                   L"TỰ TẠO PT đang ON • dùng nút TỰ TẠO PT để tắt mode độc quyền.");\n            return;\n        }\n''', 'block normal stop in party')

# F8 capture integrates one new capture mode.
t = once(t,
'''        const bool hasMode = shortcutPostTradeCapture_ || shortcutKunlunCaptureIndex_ >= 0 || captureSlot_ != ClickSlot::None || captureMacroIndex_ >= 0 ||\n                             captureTradeSequenceIndex_ >= 0;\n''',
'''        const bool hasMode = partyBuildCaptureIndex_ >= 0 || shortcutPostTradeCapture_ || shortcutKunlunCaptureIndex_ >= 0 || captureSlot_ != ClickSlot::None || captureMacroIndex_ >= 0 ||\n                             captureTradeSequenceIndex_ >= 0;\n''', 'party f8 has mode')

t = once(t,
'''        if (shortcutPostTradeCapture_) {\n            shortcutSettings_.postTradeClick = captured;\n''',
'''        if (partyBuildCaptureIndex_ >= 0 && partyBuildCaptureIndex_ < 3) {\n            const int index = partyBuildCaptureIndex_;\n            partyBuildSettings_.clicks[static_cast<std::size_t>(index)] = captured;\n            SavePartyBuildSettings(partyBuildSettings_);\n            LoadPartyBuildSettingsToUi();\n            const wchar_t* label = index == 0 ? L"KEY CLICK 1" : (index == 1 ? L"KEY CLICK 2" : L"MẶT MEMBER");\n            LogAccount(*captureAccount, std::wstring(L"AUTO PT F8 PASS • ") + label + L" = " + PointDescription(captured));\n        } else if (shortcutPostTradeCapture_) {\n            shortcutSettings_.postTradeClick = captured;\n''', 'party f8 store')

t = once(t,
'''        shortcutKunlunCaptureIndex_ = -1; shortcutPostTradeCapture_ = false;\n    }\n''',
'''        shortcutKunlunCaptureIndex_ = -1; shortcutPostTradeCapture_ = false; partyBuildCaptureIndex_ = -1;\n    }\n''', 'party f8 reset')

# Prevent safety/P3/normal FSM and trade coordinator from running in PartyBuild. Tick sessions once globally.
t = once(t,
'''                if (PriorityLauLanGateConfirmClick(a, priorityNow)) continue;\n                if (PriorityReviveClick(a, priorityNow)) continue;\n                if (!gatherModeActive_ && a.runtime.priorityAutoRequestSlot != ClickSlot::None) {\n''',
'''                if (partyBuildModeActive_) continue; // exclusive: no revive/XN/P3 automation while KEY builds PT.\n                if (PriorityLauLanGateConfirmClick(a, priorityNow)) continue;\n                if (PriorityReviveClick(a, priorityNow)) continue;\n                if (!gatherModeActive_ && !partyBuildModeActive_ && a.runtime.priorityAutoRequestSlot != ClickSlot::None) {\n''', 'disable priority automation in party')

t = once(t,
'''                    if (HandleAutoPathFightInvariant(a, now)) {\n                        // Hard invariant owns this tick for both normal and held accounts.\n                    } else if (gatherModeActive_) {\n''',
'''                    if (!partyBuildModeActive_ && HandleAutoPathFightInvariant(a, now)) {\n                        // Hard invariant owns this tick for normal/gather flows; PartyBuild is exclusive.\n                    } else if (partyBuildModeActive_) {\n                        a.runtime.status = L"AUTO PT • KEY đang được PartyBuild điều phối";\n                    } else if (gatherModeActive_) {\n''', 'party account branch')

t = once(t,
'''        if (!globalPaused_ && !gatherModeActive_) {\n            Account* activeMain = AccountByPid(tradeTxn_.mainPid);\n''',
'''        if (!globalPaused_ && partyBuildModeActive_) TickPartyBuild(GetTickCount());\n        if (!globalPaused_ && !gatherModeActive_ && !partyBuildModeActive_) {\n            Account* activeMain = AccountByPid(tradeTxn_.mainPid);\n''', 'party tick and skip trade')

# Commands.
t = once(t,
'''                    case IDC_GATHER_TOGGLE:\n                        if (HIWORD(wp) == BN_CLICKED) ToggleGatherMode();\n                        break;\n                    case IDC_TRADE_ROLE:\n''',
'''                    case IDC_GATHER_TOGGLE:\n                        if (HIWORD(wp) == BN_CLICKED) ToggleGatherMode();\n                        break;\n                    case IDC_SET_PARTY_KEY:\n                        if (HIWORD(wp) == BN_CLICKED) SetPartyKeyForSelected();\n                        break;\n                    case IDC_PARTY_BUILD_TOGGLE:\n                        if (HIWORD(wp) == BN_CLICKED) TogglePartyBuildMode();\n                        break;\n                    case IDC_PB_CAPTURE_CLICK1:\n                        if (HIWORD(wp) == BN_CLICKED) BeginPartyBuildCapture(0);\n                        break;\n                    case IDC_PB_CAPTURE_CLICK2:\n                        if (HIWORD(wp) == BN_CLICKED) BeginPartyBuildCapture(1);\n                        break;\n                    case IDC_PB_CAPTURE_FACE:\n                        if (HIWORD(wp) == BN_CLICKED) BeginPartyBuildCapture(2);\n                        break;\n                    case IDC_PB_TEST_CLICK1:\n                        if (HIWORD(wp) == BN_CLICKED) TestPartyBuildClick(0);\n                        break;\n                    case IDC_PB_TEST_CLICK2:\n                        if (HIWORD(wp) == BN_CLICKED) TestPartyBuildClick(1);\n                        break;\n                    case IDC_PB_TEST_FACE:\n                        if (HIWORD(wp) == BN_CLICKED) TestPartyBuildClick(2);\n                        break;\n                    case IDC_PB_DELAY_CLICK1:\n                    case IDC_PB_DELAY_CLICK2:\n                    case IDC_PB_DELAY_FACE:\n                    case IDC_PB_TARGET_RETRY:\n                    case IDC_PB_INVITE_RETRY:\n                        if (HIWORD(wp) == EN_KILLFOCUS) PersistPartyBuildSettingsFromUi();\n                        break;\n                    case IDC_TRADE_ROLE:\n''', 'party commands')

# Initial UI labels.
t = once(t,
'''        UpdateRoleActionButtons();\n        UpdateGatherLabel();\n        ScanClients();\n''',
'''        UpdateRoleActionButtons();\n        UpdateGatherLabel();\n        UpdatePartyBuildToggleLabel();\n        ScanClients();\n''', 'party ui init')

# Main class members.
t = once(t,
'''    HWND gatherToggleButton_ = nullptr;\n    HWND gatherLabel_ = nullptr;\n    bool pidExpanded_ = false;\n''',
'''    HWND gatherToggleButton_ = nullptr;\n    HWND gatherLabel_ = nullptr;\n    HWND setPartyKeyButton_ = nullptr;\n    HWND partyBuildToggleButton_ = nullptr;\n    bool pidExpanded_ = false;\n''', 'party main ui members')

t = once(t,
'''    std::vector<HWND> developerUnlockedControls_{};\n    HFONT aboutHeadingFont_ = nullptr;\n''',
'''    std::vector<HWND> developerUnlockedControls_{};\n    std::vector<HWND> partyBuildDevControls_{};\n    std::array<HWND, 3> partyBuildPointLabels_{};\n    std::array<HWND, 3> partyBuildDelayEdits_{};\n    HWND partyBuildTargetRetryEdit_ = nullptr;\n    HWND partyBuildInviteRetryEdit_ = nullptr;\n    int partyBuildCaptureIndex_ = -1;\n    HFONT aboutHeadingFont_ = nullptr;\n''', 'party dev members')

t = once(t,
'''    bool gatherModeActive_ = false;\n    TargetProfile gatherTarget_ = LoadGatherTarget();\n    std::vector<DWORD> gatherPids_{};\n\n    std::vector<TradeSequenceStep> mainTradeSequence_{};\n''',
'''    bool gatherModeActive_ = false;\n    TargetProfile gatherTarget_ = LoadGatherTarget();\n    std::vector<DWORD> gatherPids_{};\n    bool partyBuildModeActive_ = false;\n    PartyBuildSettings partyBuildSettings_ = LoadPartyBuildSettings();\n    std::vector<PartyBuildSession> partyBuildSessions_{};\n    std::vector<std::wstring> partyBuildPreflightSummary_{};\n\n    std::vector<TradeSequenceStep> mainTradeSequence_{};\n''', 'party runtime members')

save(p, t, nl)
print('apply_v99_automation_cp4.py: PASS')
