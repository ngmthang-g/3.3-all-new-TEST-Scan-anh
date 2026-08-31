// Included by bridge.cpp after UI discovery + ReadDungeonProgress helpers.

std::wstring DungeonActivityStripMarkup(const std::wstring& input) {
    std::wstring out; out.reserve(input.size()); bool tag = false;
    for (wchar_t ch : input) {
        if (ch == L'<') { tag = true; continue; }
        if (tag) { if (ch == L'>') tag = false; continue; }
        if (ch == L'\r' || ch == L'\n' || ch == L'\t') ch = L' ';
        out.push_back(ch);
    }
    while (out.find(L"  ") != std::wstring::npos) out.replace(out.find(L"  "), 2, L" ");
    while (!out.empty() && iswspace(out.front())) out.erase(out.begin());
    while (!out.empty() && iswspace(out.back())) out.pop_back();
    return out;
}

bool DungeonActivityParseRatio(const std::wstring& raw, std::wstring& label, int& current, int& target) {
    label.clear(); current = target = -1;
    const std::wstring text = DungeonActivityStripMarkup(raw);
    for (std::size_t slash = text.find(L'/'); slash != std::wstring::npos; slash = text.find(L'/', slash + 1)) {
        std::size_t left = slash;
        while (left > 0 && iswdigit(text[left - 1])) --left;
        std::size_t right = slash + 1;
        while (right < text.size() && iswdigit(text[right])) ++right;
        if (left == slash || right == slash + 1) continue;
        try {
            const long long c = std::stoll(text.substr(left, slash - left));
            const long long t = std::stoll(text.substr(slash + 1, right - slash - 1));
            if (c < 0 || t <= 0 || c > 100000000 || t > 100000000) continue;
            std::wstring prefix = text.substr(0, left);
            while (!prefix.empty() && (iswspace(prefix.back()) || prefix.back() == L':' || prefix.back() == L'-')) prefix.pop_back();
            const bool hasAlpha = std::any_of(prefix.begin(), prefix.end(), [](wchar_t ch) { return iswalpha(ch) != 0; });
            if (!hasAlpha || prefix.size() < 2) continue;
            label = prefix; current = static_cast<int>(c); target = static_cast<int>(t); return true;
        } catch (...) { continue; }
    }
    return false;
}

bool DungeonActivityParseTime(const std::wstring& raw, int& seconds) {
    seconds = -1; const std::wstring text = DungeonActivityStripMarkup(raw);
    if (background_ui_logic::Key(text).find(L"thoigiancon") == std::wstring::npos) return false;
    const std::size_t colon = text.rfind(L':'); if (colon == std::wstring::npos) return false;
    std::size_t mmStart = colon; while (mmStart > 0 && iswdigit(text[mmStart - 1])) --mmStart;
    std::size_t ssEnd = colon + 1; while (ssEnd < text.size() && iswdigit(text[ssEnd])) ++ssEnd;
    if (mmStart == colon || ssEnd == colon + 1) return false;
    try {
        const int mm = std::stoi(text.substr(mmStart, colon - mmStart));
        const int ss = std::stoi(text.substr(colon + 1, ssEnd - colon - 1));
        if (mm < 0 || ss < 0 || ss > 59) return false;
        seconds = mm * 60 + ss; return true;
    } catch (...) { return false; }
}

void DungeonActivityAppendTokens(const std::wstring& raw, std::vector<std::wstring>& tokens) {
    if (raw.empty()) return;
    std::wstring part;
    auto flush = [&]() {
        std::wstring token = DungeonActivityStripMarkup(part);
        if (!token.empty()) tokens.push_back(std::move(token));
        part.clear();
    };
    for (std::size_t i = 0; i < raw.size(); ++i) {
        const wchar_t ch = raw[i];
        const bool ratioSlash = ch == L'/' && i > 0 && i + 1 < raw.size() &&
                                iswdigit(raw[i - 1]) && iswdigit(raw[i + 1]);
        if (ch == L'/' && !ratioSlash) { flush(); continue; }
        part.push_back(ch);
    }
    flush();
}

void DungeonActivityStaticIdentity(int mapID, int& activityId, const wchar_t*& name) {
    activityId = 0; name = L"HOẠT ĐỘNG PHÓ BẢN";
    switch (mapID) {
        case 92: activityId = 7; name = L"THỦY LAO"; break;
        case 93: activityId = 4; name = L"BIÊN GIỚI TỐNG LIÊU"; break;
        case 94: activityId = 5; name = L"TRÚC LÂM"; break;
        case 95: activityId = 6; name = L"DÃ NGOẠI PHỈ"; break;
        case 77: activityId = 3; name = L"TRÂN LONG KỲ CUỘC"; break;
        default: break;
    }
}

bool ReadDungeonActivityBoard(int requestedMap, Response& response, wchar_t* detail, std::size_t cap) {
    response.dungeonActivity = DungeonActivitySnapshot{};
    Snapshot state{}; wchar_t stateDetail[160]{};
    if (!ReadState(state, stateDetail, _countof(stateDetail))) {
        SetText(detail, cap, L"ACTIVITY chưa đọc được authoritative Map state"); return false;
    }
    if ((state.validMask & ValidMap) == 0 || requestedMap <= 0 || state.mapID != requestedMap) {
        SetText(detail, cap, L"ACTIVITY fail-closed • KEY không ở đúng dungeon Map"); return false;
    }
    std::vector<UiControl> objects;
    if (!EnumerateActiveUiObjects(objects, detail, cap)) return false;
    std::vector<std::wstring> tokens; tokens.reserve(objects.size() * 3);
    for (const auto& row : objects) {
        DungeonActivityAppendTokens(row.labels.text, tokens);
        DungeonActivityAppendTokens(row.labels.descendants, tokens);
        if (!row.labels.name.empty()) tokens.push_back(DungeonActivityStripMarkup(row.labels.name));
    }

    DungeonActivitySnapshot board{}; board.mapID = requestedMap; board.capturedTick = GetTickCount64();
    const wchar_t* staticName = nullptr; DungeonActivityStaticIdentity(requestedMap, board.activityId, staticName);
    wcsncpy_s(board.activityName, staticName ? staticName : L"HOẠT ĐỘNG PHÓ BẢN", _TRUNCATE);
    wcsncpy_s(board.source, L"LIVE_UI/FUBEN", _TRUNCATE);
    board.remainingSeconds = -1;

    bool titleSeen = false;
    const std::wstring wantedTitle = background_ui_logic::Key(board.activityName);
    for (const auto& token : tokens) {
        const std::wstring key = background_ui_logic::Key(token);
        if (!wantedTitle.empty() && key.find(wantedTitle) != std::wstring::npos) titleSeen = true;
        int seconds = -1; if (board.remainingSeconds < 0 && DungeonActivityParseTime(token, seconds)) board.remainingSeconds = seconds;
        std::wstring label; int current = -1, target = -1;
        if (!DungeonActivityParseRatio(token, label, current, target)) continue;
        bool duplicate = false;
        for (std::uint32_t i = 0; i < board.objectiveCount; ++i) {
            if (background_ui_logic::Key(board.objectives[i].name) == background_ui_logic::Key(label) &&
                board.objectives[i].target == target) { board.objectives[i].current = current; duplicate = true; break; }
        }
        if (duplicate || board.objectiveCount >= kMaxDungeonActivityObjectives) continue;
        auto& objective = board.objectives[board.objectiveCount++]; objective.current = current; objective.target = target;
        wcsncpy_s(objective.name, label.c_str(), _TRUNCATE);
    }

    // Current counters are accepted only when they came from live active UI text. Static map/name
    // data may label that live snapshot, but it never supplies progress values.
    if (board.objectiveCount == 0) {
        SetText(detail, cap, L"ACTIVITY LIVE_UI chưa thấy dòng mục tiêu current/target • không giả bằng scanner");
        return false;
    }
    // A known title is strong context. Some client layouts hide the title after entering the map;
    // in that case authoritative Map + semantic objective lines remain the proof source.
    board.synchronized = 1; board.validMask = 1u | (titleSeen ? 2u : 0u) | (board.remainingSeconds >= 0 ? 4u : 0u);
    response.dungeonActivity = board;
    response.resultCode = static_cast<std::int32_t>(ActionResult::StageReady);
    SetText(detail, cap, L"ACTIVITY LIVE_UI PASS • Map="); AppendInt(detail, cap, requestedMap);
    Append(detail, cap, L" • objectives="); AppendInt(detail, cap, static_cast<int>(board.objectiveCount));
    Append(detail, cap, titleSeen ? L" • title=LIVE" : L" • title=STATIC-LABEL/live-objectives");
    return true;
}
