#include "image_scan_test.h"

#include <windows.h>
#include <commdlg.h>
#include <wincodec.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <limits>
#include <iterator>
#include <string>
#include <vector>

namespace image_scan_test {
namespace {

constexpr wchar_t kClassName[] = L"ThanLongImageScanTestPocV1";
constexpr int IDC_TEMPLATE = 1001;
constexpr int IDC_PICK = 1002;
constexpr int IDC_X = 1003;
constexpr int IDC_Y = 1004;
constexpr int IDC_W = 1005;
constexpr int IDC_H = 1006;
constexpr int IDC_THRESHOLD = 1007;
constexpr int IDC_TEST = 1008;
constexpr int IDC_STATUS = 1009;
constexpr UINT kPwRenderFullContent = 0x00000002u;

struct Image {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> bgra;
};

struct Match {
    bool found = false;
    double score = -1.0;
    int x = 0;
    int y = 0;
};

struct Config {
    std::wstring templatePath;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int thresholdPercent = 90;
};

Config g_lastConfig{};

struct State {
    Target target{};
    Config config{};
    HWND hwnd = nullptr;
};

void ReleaseUnknown(IUnknown*& p) {
    if (p) { p->Release(); p = nullptr; }
}

template <typename T>
void ReleaseCom(T*& p) {
    if (p) { p->Release(); p = nullptr; }
}

std::wstring HrText(HRESULT hr) {
    wchar_t buf[32]{};
    swprintf_s(buf, L"0x%08X", static_cast<unsigned int>(hr));
    return buf;
}

bool LoadImageWic(const std::wstring& path, Image& out, std::wstring& error) {
    out = {};
    if (path.empty()) { error = L"Chưa chọn ảnh mẫu"; return false; }

    const HRESULT initHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool uninit = SUCCEEDED(initHr);
    if (FAILED(initHr) && initHr != RPC_E_CHANGED_MODE) {
        error = L"CoInitializeEx FAIL " + HrText(initHr);
        return false;
    }

    IWICImagingFactory* factory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&factory));
    if (SUCCEEDED(hr)) hr = factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                                WICDecodeMetadataCacheOnLoad, &decoder);
    if (SUCCEEDED(hr)) hr = decoder->GetFrame(0, &frame);
    if (SUCCEEDED(hr)) hr = factory->CreateFormatConverter(&converter);
    if (SUCCEEDED(hr)) hr = converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA,
                                                   WICBitmapDitherTypeNone, nullptr, 0.0,
                                                   WICBitmapPaletteTypeCustom);
    UINT w = 0, h = 0;
    if (SUCCEEDED(hr)) hr = converter->GetSize(&w, &h);
    if (SUCCEEDED(hr) && (w == 0 || h == 0 || w > 8192 || h > 8192)) hr = E_INVALIDARG;
    if (SUCCEEDED(hr)) {
        const std::size_t bytes = static_cast<std::size_t>(w) * h * 4u;
        out.bgra.resize(bytes);
        hr = converter->CopyPixels(nullptr, w * 4u, static_cast<UINT>(bytes), out.bgra.data());
        if (SUCCEEDED(hr)) { out.width = static_cast<int>(w); out.height = static_cast<int>(h); }
    }

    ReleaseCom(converter);
    ReleaseCom(frame);
    ReleaseCom(decoder);
    ReleaseCom(factory);
    if (uninit) CoUninitialize();
    if (FAILED(hr)) {
        out = {};
        error = L"Không đọc được ảnh mẫu bằng WIC: " + HrText(hr);
        return false;
    }
    return true;
}

bool CaptureClient(HWND hwnd, Image& out, std::wstring& backend, std::wstring& error) {
    out = {};
    backend.clear();
    if (!hwnd || !IsWindow(hwnd)) { error = L"HWND game không còn tồn tại"; return false; }
    RECT rc{};
    if (!GetClientRect(hwnd, &rc)) { error = L"GetClientRect FAIL"; return false; }
    const int w = rc.right - rc.left;
    const int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0 || w > 8192 || h > 8192) { error = L"Client size không hợp lệ"; return false; }

    HDC src = GetDC(hwnd);
    if (!src) { error = L"GetDC(game) FAIL"; return false; }
    HDC mem = CreateCompatibleDC(src);
    if (!mem) { ReleaseDC(hwnd, src); error = L"CreateCompatibleDC FAIL"; return false; }

    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h; // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(mem, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib || !bits) {
        if (dib) DeleteObject(dib);
        DeleteDC(mem); ReleaseDC(hwnd, src);
        error = L"CreateDIBSection FAIL"; return false;
    }
    HGDIOBJ old = SelectObject(mem, dib);
    PatBlt(mem, 0, 0, w, h, BLACKNESS);

    BOOL ok = PrintWindow(hwnd, mem, PW_CLIENTONLY | kPwRenderFullContent);
    if (ok) {
        backend = L"PrintWindow(HWND/PW_CLIENTONLY)";
    } else {
        ok = BitBlt(mem, 0, 0, w, h, src, 0, 0, SRCCOPY | CAPTUREBLT);
        backend = L"BitBlt fallback (không chứng minh capture khi bị che)";
    }

    if (ok) {
        out.width = w;
        out.height = h;
        const std::size_t bytes = static_cast<std::size_t>(w) * h * 4u;
        out.bgra.resize(bytes);
        std::memcpy(out.bgra.data(), bits, bytes);
    }

    SelectObject(mem, old);
    DeleteObject(dib);
    DeleteDC(mem);
    ReleaseDC(hwnd, src);
    if (!ok) { out = {}; error = L"PrintWindow và BitBlt đều FAIL"; return false; }
    return true;
}

inline int ColorDiff(const std::uint8_t* a, const std::uint8_t* b) {
    return std::abs(static_cast<int>(a[0]) - static_cast<int>(b[0])) +
           std::abs(static_cast<int>(a[1]) - static_cast<int>(b[1])) +
           std::abs(static_cast<int>(a[2]) - static_cast<int>(b[2]));
}

double ScoreAt(const Image& frame, const Image& tpl, int ox, int oy, int sampleStep) {
    std::uint64_t diff = 0;
    std::uint64_t samples = 0;
    for (int ty = 0; ty < tpl.height; ty += sampleStep) {
        const std::uint8_t* tr = tpl.bgra.data() + static_cast<std::size_t>(ty) * tpl.width * 4u;
        const std::uint8_t* fr = frame.bgra.data() +
            (static_cast<std::size_t>(oy + ty) * frame.width + ox) * 4u;
        for (int tx = 0; tx < tpl.width; tx += sampleStep) {
            diff += static_cast<std::uint64_t>(ColorDiff(fr + static_cast<std::size_t>(tx) * 4u,
                                                        tr + static_cast<std::size_t>(tx) * 4u));
            ++samples;
        }
    }
    if (samples == 0) return -1.0;
    const double maxDiff = static_cast<double>(samples) * 3.0 * 255.0;
    return 1.0 - static_cast<double>(diff) / maxDiff;
}

Match FindTemplate(const Image& frame, const Image& tpl,
                   int rx, int ry, int rw, int rh, double threshold,
                   std::wstring& error) {
    Match best{};
    if (frame.width <= 0 || frame.height <= 0 || tpl.width <= 0 || tpl.height <= 0) {
        error = L"Ảnh/frame rỗng"; return best;
    }
    rx = std::clamp(rx, 0, frame.width - 1);
    ry = std::clamp(ry, 0, frame.height - 1);
    if (rw <= 0) rw = frame.width - rx;
    if (rh <= 0) rh = frame.height - ry;
    const int right = std::clamp(rx + rw, rx + 1, frame.width);
    const int bottom = std::clamp(ry + rh, ry + 1, frame.height);
    const int maxX = right - tpl.width;
    const int maxY = bottom - tpl.height;
    if (maxX < rx || maxY < ry) {
        error = L"Ảnh mẫu lớn hơn vùng quét"; return best;
    }

    const std::int64_t area = static_cast<std::int64_t>(right - rx) * (bottom - ry);
    const int posStep = area > 800000 ? 2 : 1;
    const int sampleStep = std::max(1, std::min(tpl.width, tpl.height) / 18);
    for (int y = ry; y <= maxY; y += posStep) {
        for (int x = rx; x <= maxX; x += posStep) {
            const double score = ScoreAt(frame, tpl, x, y, sampleStep);
            if (score > best.score) { best.score = score; best.x = x; best.y = y; }
        }
    }

    if (posStep > 1 && best.score >= 0.0) {
        const int x0 = std::max(rx, best.x - posStep);
        const int y0 = std::max(ry, best.y - posStep);
        const int x1 = std::min(maxX, best.x + posStep);
        const int y1 = std::min(maxY, best.y + posStep);
        for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x) {
            const double score = ScoreAt(frame, tpl, x, y, sampleStep);
            if (score > best.score) { best.score = score; best.x = x; best.y = y; }
        }
    }
    if (best.score >= 0.0) best.score = ScoreAt(frame, tpl, best.x, best.y, 1);
    best.found = best.score >= threshold;
    return best;
}

int ReadInt(HWND hwnd, int id, int fallback) {
    wchar_t buf[64]{};
    GetDlgItemTextW(hwnd, id, buf, static_cast<int>(std::size(buf)));
    wchar_t* end = nullptr;
    long value = wcstol(buf, &end, 10);
    return end == buf ? fallback : static_cast<int>(value);
}

std::wstring ReadText(HWND hwnd, int id) {
    const int n = GetWindowTextLengthW(GetDlgItem(hwnd, id));
    if (n <= 0) return {};
    std::wstring text(static_cast<std::size_t>(n + 1), L'\0');
    GetDlgItemTextW(hwnd, id, text.data(), n + 1);
    text.resize(static_cast<std::size_t>(n));
    return text;
}

void SetStatus(HWND hwnd, const std::wstring& text) {
    HWND h = GetDlgItem(hwnd, IDC_STATUS);
    if (h) SetWindowTextW(h, text.c_str());
}

void SyncConfig(State& s) {
    s.config.templatePath = ReadText(s.hwnd, IDC_TEMPLATE);
    s.config.x = std::max(0, ReadInt(s.hwnd, IDC_X, 0));
    s.config.y = std::max(0, ReadInt(s.hwnd, IDC_Y, 0));
    s.config.w = std::max(0, ReadInt(s.hwnd, IDC_W, 0));
    s.config.h = std::max(0, ReadInt(s.hwnd, IDC_H, 0));
    s.config.thresholdPercent = std::clamp(ReadInt(s.hwnd, IDC_THRESHOLD, 90), 1, 100);
    g_lastConfig = s.config;
}

void PickTemplate(State& s) {
    wchar_t file[MAX_PATH * 4]{};
    if (!s.config.templatePath.empty()) wcsncpy_s(file, s.config.templatePath.c_str(), _TRUNCATE);
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = s.hwnd;
    ofn.lpstrFilter = L"Ảnh mẫu (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0Tất cả file\0*.*\0\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = static_cast<DWORD>(std::size(file));
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;
    s.config.templatePath = file;
    SetDlgItemTextW(s.hwnd, IDC_TEMPLATE, file);
    g_lastConfig = s.config;
}

void RunTest(State& s) {
    SyncConfig(s);
    if (!s.target.gameWindow || !IsWindow(s.target.gameWindow)) {
        SetStatus(s.hwnd, L"FAIL • cửa sổ game đã mất"); return;
    }

    Image tpl{}, frame{};
    std::wstring error, backend;
    if (!LoadImageWic(s.config.templatePath, tpl, error)) {
        SetStatus(s.hwnd, L"TEMPLATE FAIL • " + error); return;
    }
    if (!CaptureClient(s.target.gameWindow, frame, backend, error)) {
        SetStatus(s.hwnd, L"CAPTURE FAIL • " + error); return;
    }

    const double threshold = static_cast<double>(s.config.thresholdPercent) / 100.0;
    Match match = FindTemplate(frame, tpl, s.config.x, s.config.y, s.config.w, s.config.h,
                               threshold, error);
    if (match.score < 0.0) {
        SetStatus(s.hwnd, L"SCAN FAIL • " + error + L" • capture=" + backend); return;
    }

    wchar_t score[64]{};
    swprintf_s(score, L"%.2f%%", match.score * 100.0);
    std::wstring line = L"CAPTURE=" + backend + L" • " + std::to_wstring(frame.width) + L"x" +
                        std::to_wstring(frame.height) + L" • BEST=" + score;
    if (!match.found) {
        SetStatus(s.hwnd, line + L" • NOT FOUND → KHÔNG CLICK"); return;
    }

    const int clickX = match.x + tpl.width / 2;
    const int clickY = match.y + tpl.height / 2;
    line += L" • PASS @ " + std::to_wstring(match.x) + L"," + std::to_wstring(match.y) +
            L" • tâm=" + std::to_wstring(clickX) + L"," + std::to_wstring(clickY);
    if (!s.target.hiddenClick) {
        SetStatus(s.hwnd, line + L" • không có callback click"); return;
    }
    std::wstring clickDetail;
    const bool clicked = s.target.hiddenClick(s.target.context, clickX, clickY,
                                              frame.width, frame.height, clickDetail);
    SetStatus(s.hwnd, line + (clicked ? L" • RAW HIDDEN CLICK PASS • " : L" • RAW HIDDEN CLICK FAIL • ") + clickDetail);
}

HWND Add(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style,
         int x, int y, int w, int h, int id) {
    HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style,
                             x, y, w, h, parent,
                             reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                             GetModuleHandleW(nullptr), nullptr);
    if (c) SendMessageW(c, WM_SETFONT,
                         reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    return c;
}

void BuildControls(State& s) {
    Add(s.hwnd, L"STATIC", (L"ACC TEST: " + s.target.accountLabel).c_str(), SS_LEFT | SS_CENTERIMAGE,
        18, 14, 690, 25, 0);
    Add(s.hwnd, L"STATIC", L"Ảnh mẫu:", SS_LEFT | SS_CENTERIMAGE, 18, 49, 70, 26, 0);
    Add(s.hwnd, L"EDIT", s.config.templatePath.c_str(), WS_BORDER | ES_AUTOHSCROLL,
        90, 49, 525, 26, IDC_TEMPLATE);
    Add(s.hwnd, L"BUTTON", L"CHỌN ẢNH", BS_PUSHBUTTON, 625, 48, 105, 28, IDC_PICK);

    Add(s.hwnd, L"STATIC", L"Vùng quét client: X", SS_LEFT | SS_CENTERIMAGE, 18, 88, 118, 25, 0);
    Add(s.hwnd, L"EDIT", std::to_wstring(s.config.x).c_str(), WS_BORDER | ES_NUMBER | ES_CENTER,
        138, 88, 62, 26, IDC_X);
    Add(s.hwnd, L"STATIC", L"Y", SS_CENTER | SS_CENTERIMAGE, 205, 88, 20, 25, 0);
    Add(s.hwnd, L"EDIT", std::to_wstring(s.config.y).c_str(), WS_BORDER | ES_NUMBER | ES_CENTER,
        228, 88, 62, 26, IDC_Y);
    Add(s.hwnd, L"STATIC", L"W", SS_CENTER | SS_CENTERIMAGE, 295, 88, 20, 25, 0);
    Add(s.hwnd, L"EDIT", std::to_wstring(s.config.w).c_str(), WS_BORDER | ES_NUMBER | ES_CENTER,
        318, 88, 70, 26, IDC_W);
    Add(s.hwnd, L"STATIC", L"H", SS_CENTER | SS_CENTERIMAGE, 393, 88, 20, 25, 0);
    Add(s.hwnd, L"EDIT", std::to_wstring(s.config.h).c_str(), WS_BORDER | ES_NUMBER | ES_CENTER,
        416, 88, 70, 26, IDC_H);
    Add(s.hwnd, L"STATIC", L"Ngưỡng %", SS_LEFT | SS_CENTERIMAGE, 500, 88, 65, 25, 0);
    Add(s.hwnd, L"EDIT", std::to_wstring(s.config.thresholdPercent).c_str(), WS_BORDER | ES_NUMBER | ES_CENTER,
        568, 88, 60, 26, IDC_THRESHOLD);
    Add(s.hwnd, L"BUTTON", L"TEST SCAN + CLICK ẨN", BS_DEFPUSHBUTTON,
        638, 86, 92, 31, IDC_TEST);

    Add(s.hwnd, L"STATIC", L"W/H = 0 nghĩa là quét tới hết client. Bản PoC chưa scale ảnh mẫu: hãy chụp mẫu ở cùng size game đang test.",
        SS_LEFT | SS_CENTERIMAGE, 18, 124, 712, 28, 0);
    Add(s.hwnd, L"STATIC", L"Sẵn sàng. TEST không cần AUTO chạy, không kiểm tra MapID/trạng thái nhân vật.",
        SS_LEFT | SS_CENTERIMAGE | WS_BORDER, 18, 160, 712, 76, IDC_STATUS);
    Add(s.hwnd, L"BUTTON", L"ĐÓNG", BS_PUSHBUTTON, 625, 246, 105, 30, IDCANCEL);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    State* s = reinterpret_cast<State*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        s = static_cast<State*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(s));
        if (s) s->hwnd = hwnd;
    }
    if (!s) return DefWindowProcW(hwnd, msg, wp, lp);
    switch (msg) {
        case WM_CREATE: BuildControls(*s); return 0;
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDC_PICK: PickTemplate(*s); return 0;
                case IDC_TEST: RunTest(*s); return 0;
                case IDCANCEL: SyncConfig(*s); DestroyWindow(hwnd); return 0;
            }
            break;
        case WM_CLOSE: SyncConfig(*s); DestroyWindow(hwnd); return 0;
        case WM_NCDESTROY: s->hwnd = nullptr; return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace

void RunDialog(const Target& target) {
    if (!target.owner || !target.gameWindow) return;
    State state{};
    state.target = target;
    state.config = g_lastConfig;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kClassName;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return;

    EnableWindow(target.owner, FALSE);
    HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, kClassName, L"TEST SCAN ẢNH ẨN • PoC",
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                                CW_USEDEFAULT, CW_USEDEFAULT, 770, 330,
                                target.owner, nullptr, GetModuleHandleW(nullptr), &state);
    if (!hwnd) { EnableWindow(target.owner, TRUE); return; }
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg{};
    bool sawQuit = false;
    int quitCode = 0;
    while (IsWindow(hwnd)) {
        const BOOL gm = GetMessageW(&msg, nullptr, 0, 0);
        if (gm <= 0) {
            if (gm == 0) { sawQuit = true; quitCode = static_cast<int>(msg.wParam); }
            break;
        }
        if (!IsDialogMessageW(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    EnableWindow(target.owner, TRUE);
    SetActiveWindow(target.owner);
    if (sawQuit) PostQuitMessage(quitCode);
}

} // namespace image_scan_test
