            DrawTextW(dc, L"KÉO CHUỘT KHOANH VÙNG CẦN SCAN • thả chuột = lưu ngay • ESC = hủy",
                      -1, &note, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            DrawImage(dc, *s->frame, s->imageRect);
            FrameRect(dc, &s->imageRect, GetSysColorBrush(COLOR_WINDOWFRAME));
            if (s->dragging) {
                RECT r = NormalizedDragRect(*s);
                FrameRect(dc, &r, GetSysColorBrush(COLOR_HIGHLIGHT));
                InflateRect(&r, -1, -1);
                if (r.right > r.left && r.bottom > r.top)
                    FrameRect(dc, &r, GetSysColorBrush(COLOR_HIGHLIGHT));
            }
            EndPaint(hwnd, &ps); return 0;
        }
        case WM_LBUTTONDOWN: {
            POINT p{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            if (!PointInsideImage(*s, p)) return 0;
            s->dragStart = s->dragCurrent = p; s->dragging = true;
            SetCapture(hwnd); InvalidateRect(hwnd, nullptr, FALSE); return 0;
        }
        case WM_MOUSEMOVE:
            if (s->dragging) {
                POINT p{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
                s->dragCurrent = ClampToImage(*s, p); InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_LBUTTONUP:
            if (s->dragging) {
                POINT p{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
                s->dragCurrent = ClampToImage(*s, p); s->dragging = false; ReleaseCapture();
                RECT r = NormalizedDragRect(*s);
                const int dw = s->imageRect.right - s->imageRect.left;
                const int dh = s->imageRect.bottom - s->imageRect.top;
                const int fw = s->frame->width, fh = s->frame->height;
                const int x0 = static_cast<int>((static_cast<long long>(r.left - s->imageRect.left) * fw) / dw);
                const int y0 = static_cast<int>((static_cast<long long>(r.top - s->imageRect.top) * fh) / dh);
                const int x1 = static_cast<int>((static_cast<long long>(r.right - s->imageRect.left) * fw + dw - 1) / dw);
                const int y1 = static_cast<int>((static_cast<long long>(r.bottom - s->imageRect.top) * fh + dh - 1) / dh);
                if (x1 - x0 >= 2 && y1 - y0 >= 2) {
                    s->selected = {x0, y0, std::min(fw, x1), std::min(fh, y1)};
                    s->accepted = true; DestroyWindow(hwnd);
                } else InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_KEYDOWN:
            if (wp == VK_ESCAPE) { DestroyWindow(hwnd); return 0; }
            break;
        case WM_CLOSE: DestroyWindow(hwnd); return 0;
        case WM_NCDESTROY: s->hwnd = nullptr; return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

bool PickRegionModal(HWND owner, const Image& frame, RECT& selected) {
    RECT work{}; SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    const int workW = std::max(640L, work.right - work.left);
    const int workH = std::max(480L, work.bottom - work.top);
    const int maxImageW = std::min(1200, workW - 80);
    const int maxImageH = std::min(820, workH - 130);
    double scale = 1.0;
    if (frame.width > maxImageW) scale = std::min(scale, static_cast<double>(maxImageW) / frame.width);
    if (frame.height > maxImageH) scale = std::min(scale, static_cast<double>(maxImageH) / frame.height);
    const int displayW = std::max(1, static_cast<int>(std::lround(frame.width * scale)));
    const int displayH = std::max(1, static_cast<int>(std::lround(frame.height * scale)));

    RegionPickerState state{}; state.frame = &frame;
    state.imageRect = {10, 40, 10 + displayW, 40 + displayH};
    WNDCLASSEXW wc{}; wc.cbSize = sizeof(wc); wc.lpfnWndProc = RegionWndProc;
    wc.hInstance = GetModuleHandleW(nullptr); wc.hCursor = LoadCursor(nullptr, IDC_CROSS);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); wc.lpszClassName = kRegionClassName;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
    RECT wr{0, 0, displayW + 20, displayH + 55};
    AdjustWindowRectEx(&wr, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW);
    const int winW = wr.right - wr.left, winH = wr.bottom - wr.top;
    const int px = work.left + std::max(0, (workW - winW) / 2);
    const int py = work.top + std::max(0, (workH - winH) / 2);
    EnableWindow(owner, FALSE);
    HWND hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, kRegionClassName,
                                L"CHỌN VÙNG SCAN • kéo chuột trực tiếp trên ảnh client",
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                                px, py, winW, winH, owner, nullptr, GetModuleHandleW(nullptr), &state);
    if (!hwnd) { EnableWindow(owner, TRUE); return false; }
    ShowWindow(hwnd, SW_SHOW); UpdateWindow(hwnd); SetFocus(hwnd);
    MSG msg{}; bool sawQuit = false; int quitCode = 0;
    while (IsWindow(hwnd)) {
        const BOOL gm = GetMessageW(&msg, nullptr, 0, 0);
        if (gm <= 0) { if (gm == 0) { sawQuit = true; quitCode = static_cast<int>(msg.wParam); } break; }
        TranslateMessage(&msg); DispatchMessageW(&msg);
    }
    EnableWindow(owner, TRUE); SetActiveWindow(owner);
    if (sawQuit) PostQuitMessage(quitCode);
    if (state.accepted) selected = state.selected;
    return state.accepted;
}

LRESULT CALLBACK PreviewWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    PreviewState* s = reinterpret_cast<PreviewState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        s = static_cast<PreviewState*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(s));
        if (s) s->hwnd = hwnd;
    }
    if (!s) return DefWindowProcW(hwnd, msg, wp, lp);
    switch (msg) {
        case WM_ERASEBKGND: return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps{}; HDC dc = BeginPaint(hwnd, &ps);
            RECT client{}; GetClientRect(hwnd, &client); FillRect(dc, &client, GetSysColorBrush(COLOR_WINDOW));
            SetBkMode(dc, TRANSPARENT); RECT note{10, 8, client.right - 10, 34};
            DrawTextW(dc, s->note.c_str(), -1, &note, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            DrawImage(dc, *s->image, s->imageRect); FrameRect(dc, &s->imageRect, GetSysColorBrush(COLOR_WINDOWFRAME));
            EndPaint(hwnd, &ps); return 0;
        }
        case WM_KEYDOWN: if (wp == VK_ESCAPE || wp == VK_RETURN) { DestroyWindow(hwnd); return 0; } break;
        case WM_CLOSE: DestroyWindow(hwnd); return 0;
        case WM_NCDESTROY: s->hwnd = nullptr; return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void ShowPreviewModal(HWND owner, const Image& image, const std::wstring& note) {
    RECT work{}; SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    const int workW = std::max(640L, work.right - work.left), workH = std::max(480L, work.bottom - work.top);
    const int maxW = std::min(760, workW - 100), maxH = std::min(560, workH - 140);
    double scale = 1.0;
    if (image.width > maxW) scale = std::min(scale, static_cast<double>(maxW) / image.width);
    if (image.height > maxH) scale = std::min(scale, static_cast<double>(maxH) / image.height);
    if (image.width < 260 && image.height < 180)
        scale = std::min(3.0, std::min(static_cast<double>(maxW) / image.width,
                                       static_cast<double>(maxH) / image.height));
    const int dw = std::max(1, static_cast<int>(std::lround(image.width * scale)));
    const int dh = std::max(1, static_cast<int>(std::lround(image.height * scale)));
    PreviewState state{}; state.image = &image; state.note = note; state.imageRect = {10, 40, 10 + dw, 40 + dh};
    WNDCLASSEXW wc{}; wc.cbSize = sizeof(wc); wc.lpfnWndProc = PreviewWndProc;
    wc.hInstance = GetModuleHandleW(nullptr); wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); wc.lpszClassName = kPreviewClassName;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return;
    RECT wr{0,0,dw+20,dh+55}; AdjustWindowRectEx(&wr, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_TOOLWINDOW);
    const int winW=wr.right-wr.left, winH=wr.bottom-wr.top;
    EnableWindow(owner, FALSE);
    HWND hwnd=CreateWindowExW(WS_EX_TOOLWINDOW,kPreviewClassName,L"XEM VÙNG SCAN",
                              WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
