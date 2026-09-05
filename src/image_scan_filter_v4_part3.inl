void SetEditInt(HWND hwnd, int id, int value) {
    SetDlgItemTextW(hwnd, id, std::to_wstring(value).c_str());
}

int SelectedStep(const State& s) {
    return s.stepList ? ListView_GetNextItem(s.stepList, -1, LVNI_SELECTED) : -1;
}

void AddColumn(HWND list, int index, int width, const wchar_t* text) {
    LVCOLUMNW c{};
    c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    c.pszText = const_cast<wchar_t*>(text);
    c.cx = width; c.iSubItem = index;
    ListView_InsertColumn(list, index, &c);
}

void LoadStepEditor(State& s, int row) {
    if (row < 0 || row >= static_cast<int>(s.config.steps.size())) return;
    const ClickStep& step = s.config.steps[static_cast<std::size_t>(row)];
    if (step.valid) { SetEditInt(s.hwnd, IDC_STEP_X, step.x); SetEditInt(s.hwnd, IDC_STEP_Y, step.y); }
    else { SetDlgItemTextW(s.hwnd, IDC_STEP_X, L""); SetDlgItemTextW(s.hwnd, IDC_STEP_Y, L""); }
    SetEditInt(s.hwnd, IDC_STEP_DELAY, step.delayMs);
}

void RefreshStepList(State& s, int select = -1) {
    if (!s.stepList) return;
    ListView_DeleteAllItems(s.stepList);
    for (std::size_t i = 0; i < s.config.steps.size(); ++i) {
        const ClickStep& step = s.config.steps[i];
        std::wstring no = std::to_wstring(i + 1);
        LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = static_cast<int>(i); item.pszText = no.data();
        ListView_InsertItem(s.stepList, &item);
        std::wstring x = step.valid ? std::to_wstring(step.x) : L"CHƯA F8";
        std::wstring y = step.valid ? std::to_wstring(step.y) : L"-";
        std::wstring base = step.valid ? std::to_wstring(step.baseW) + L"x" + std::to_wstring(step.baseH) : L"-";
        std::wstring delay = std::to_wstring(step.delayMs);
        std::wstring state = step.valid ? L"READY • click ô rồi scan Ảnh 1" : L"CHƯA CÓ TỌA ĐỘ";
        ListView_SetItemText(s.stepList, static_cast<int>(i), 1, x.data());
        ListView_SetItemText(s.stepList, static_cast<int>(i), 2, y.data());
        ListView_SetItemText(s.stepList, static_cast<int>(i), 3, base.data());
        ListView_SetItemText(s.stepList, static_cast<int>(i), 4, delay.data());
        ListView_SetItemText(s.stepList, static_cast<int>(i), 5, state.data());
    }
    if (select < 0 && !s.config.steps.empty()) select = 0;
    if (select >= 0 && select < static_cast<int>(s.config.steps.size())) {
        ListView_SetItemState(s.stepList, select, LVIS_SELECTED | LVIS_FOCUSED,
                              LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(s.stepList, select, FALSE);
        LoadStepEditor(s, select);
    } else {
        SetDlgItemTextW(s.hwnd, IDC_STEP_X, L""); SetDlgItemTextW(s.hwnd, IDC_STEP_Y, L"");
    }
}

void SaveStepEditor(State& s) {
    const int row = SelectedStep(s);
    if (row < 0 || row >= static_cast<int>(s.config.steps.size())) return;
    ClickStep& step = s.config.steps[static_cast<std::size_t>(row)];
    const int x = ReadInt(s.hwnd, IDC_STEP_X, -1);
    const int y = ReadInt(s.hwnd, IDC_STEP_Y, -1);
    step.delayMs = std::clamp(ReadInt(s.hwnd, IDC_STEP_DELAY, step.delayMs), 50, 10000);
    step.repeat = 1;
    int cw = 0, ch = 0;
    if (x >= 0 && y >= 0 && CurrentClientSize(s.target.gameWindow, cw, ch) && x < cw && y < ch) {
        step.x = x; step.y = y; step.baseW = cw; step.baseH = ch; step.valid = true;
    } else if (x < 0 || y < 0) {
        step.valid = false; step.x = -1; step.y = -1;
    }
    g_lastConfig = s.config;
}

void SyncConfig(State& s) {
    SaveStepEditor(s);
    s.config.templatePath = ReadText(s.hwnd, IDC_TEMPLATE);
    s.config.discardTemplatePath = ReadText(s.hwnd, IDC_TEMPLATE_DISCARD);
    s.config.closeTemplatePath = ReadText(s.hwnd, IDC_TEMPLATE_CLOSE);

    s.config.x = std::max(0, ReadInt(s.hwnd, IDC_X, 0));
    s.config.y = std::max(0, ReadInt(s.hwnd, IDC_Y, 0));
    s.config.w = std::max(0, ReadInt(s.hwnd, IDC_W, 0));
    s.config.h = std::max(0, ReadInt(s.hwnd, IDC_H, 0));

    s.config.discardRoi.x = std::max(0, ReadInt(s.hwnd, IDC_DISCARD_X, 0));
    s.config.discardRoi.y = std::max(0, ReadInt(s.hwnd, IDC_DISCARD_Y, 0));
    s.config.discardRoi.w = std::max(0, ReadInt(s.hwnd, IDC_DISCARD_W, 0));
    s.config.discardRoi.h = std::max(0, ReadInt(s.hwnd, IDC_DISCARD_H, 0));

    s.config.closeRoi.x = std::max(0, ReadInt(s.hwnd, IDC_CLOSE_X, 0));
    s.config.closeRoi.y = std::max(0, ReadInt(s.hwnd, IDC_CLOSE_Y, 0));
    s.config.closeRoi.w = std::max(0, ReadInt(s.hwnd, IDC_CLOSE_W, 0));
    s.config.closeRoi.h = std::max(0, ReadInt(s.hwnd, IDC_CLOSE_H, 0));

    s.config.thresholdPercent = std::clamp(ReadInt(s.hwnd, IDC_THRESHOLD, 90), 1, 100);

    s.config.discardClickDelayMs = std::clamp(ReadInt(s.hwnd, IDC_DELAY_DISCARD, s.config.discardClickDelayMs), 50, 10000);
    s.config.afterDiscardClickDelayMs = std::clamp(ReadInt(s.hwnd, IDC_DELAY_AFTER_DISCARD, s.config.afterDiscardClickDelayMs), 50, 10000);
    s.config.closeClickDelayMs = std::clamp(ReadInt(s.hwnd, IDC_DELAY_CLOSE, s.config.closeClickDelayMs), 50, 10000);

    const int afterX = ReadInt(s.hwnd, IDC_AFTER_X, s.config.afterDiscard.x);
    const int afterY = ReadInt(s.hwnd, IDC_AFTER_Y, s.config.afterDiscard.y);
    int cw = 0, ch = 0;
    if (CurrentClientSize(s.target.gameWindow, cw, ch)) {
        s.config.roiBaseW = cw;
        s.config.roiBaseH = ch;
        s.config.discardRoi.baseW = cw;
        s.config.discardRoi.baseH = ch;
        s.config.closeRoi.baseW = cw;
        s.config.closeRoi.baseH = ch;
        if (afterX >= 0 && afterY >= 0 && afterX < cw && afterY < ch) {
            s.config.afterDiscard.x = afterX;
            s.config.afterDiscard.y = afterY;
            s.config.afterDiscard.baseW = cw;
            s.config.afterDiscard.baseH = ch;
            s.config.afterDiscard.valid = true;
            s.config.afterDiscard.repeat = 1;
        }
    }
    g_lastConfig = s.config;
}

void PickImageFile(State& s, int editId, std::wstring& dest) {
    SyncConfig(s);
    wchar_t file[MAX_PATH * 4]{};
    if (!dest.empty()) wcsncpy_s(file, dest.c_str(), _TRUNCATE);
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = s.hwnd;
    ofn.lpstrFilter = L"Ảnh mẫu (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0Tất cả file\0*.*\0\0";
    ofn.lpstrFile = file; ofn.nMaxFile = static_cast<DWORD>(std::size(file));
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;
    dest = file;
    SetDlgItemTextW(s.hwnd, editId, file);
    g_lastConfig = s.config;
}

void PickTemplate(State& s) { PickImageFile(s, IDC_TEMPLATE, s.config.templatePath); }
void PickDiscardTemplate(State& s) { PickImageFile(s, IDC_TEMPLATE_DISCARD, s.config.discardTemplatePath); }
void PickCloseTemplate(State& s) { PickImageFile(s, IDC_TEMPLATE_CLOSE, s.config.closeTemplatePath); }

RECT NormalizedDragRect(const RegionPickerState& s) {
    RECT r{};
    r.left = std::min(s.dragStart.x, s.dragCurrent.x);
    r.top = std::min(s.dragStart.y, s.dragCurrent.y);
    r.right = std::max(s.dragStart.x, s.dragCurrent.x);
    r.bottom = std::max(s.dragStart.y, s.dragCurrent.y);
    r.left = std::clamp(r.left, s.imageRect.left, s.imageRect.right - 1);
    r.top = std::clamp(r.top, s.imageRect.top, s.imageRect.bottom - 1);
    r.right = std::clamp(r.right, s.imageRect.left + 1, s.imageRect.right);
    r.bottom = std::clamp(r.bottom, s.imageRect.top + 1, s.imageRect.bottom);
    return r;
}

bool PointInsideImage(const RegionPickerState& s, POINT p) {
    return p.x >= s.imageRect.left && p.x < s.imageRect.right &&
           p.y >= s.imageRect.top && p.y < s.imageRect.bottom;
}

POINT ClampToImage(const RegionPickerState& s, POINT p) {
    p.x = std::clamp(p.x, s.imageRect.left, s.imageRect.right - 1);
    p.y = std::clamp(p.y, s.imageRect.top, s.imageRect.bottom - 1);
    return p;
}

LRESULT CALLBACK RegionWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    RegionPickerState* s = reinterpret_cast<RegionPickerState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        s = static_cast<RegionPickerState*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(s));
        if (s) s->hwnd = hwnd;
    }
    if (!s) return DefWindowProcW(hwnd, msg, wp, lp);
    switch (msg) {
        case WM_ERASEBKGND: return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps{}; HDC dc = BeginPaint(hwnd, &ps);
            RECT client{}; GetClientRect(hwnd, &client);
            FillRect(dc, &client, GetSysColorBrush(COLOR_WINDOW));
            SetBkMode(dc, TRANSPARENT);
            RECT note{10, 8, client.right - 10, 34};
