#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <wincodec.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <iterator>
#include <string>
#include <vector>

namespace image_scan_test {
namespace {

constexpr wchar_t kClassName[] = L"ThanLongImageScanTestPocV2";
constexpr wchar_t kRegionClassName[] = L"ThanLongImageScanRegionPickerV2";
constexpr wchar_t kPreviewClassName[] = L"ThanLongImageScanPreviewV2";
constexpr UINT kPwRenderFullContent = 0x00000002u;
constexpr UINT_PTR kF8PollTimer = 91;
constexpr std::size_t kMaxClickSteps = 10;

constexpr int IDC_TEMPLATE = 1001;
constexpr int IDC_PICK = 1002;
constexpr int IDC_X = 1003;
constexpr int IDC_Y = 1004;
constexpr int IDC_W = 1005;
constexpr int IDC_H = 1006;
constexpr int IDC_THRESHOLD = 1007;
constexpr int IDC_TEST = 1008;
constexpr int IDC_STATUS = 1009;
constexpr int IDC_PICK_REGION = 1010;
constexpr int IDC_PREVIEW_REGION = 1011;
constexpr int IDC_FULL_REGION = 1012;
constexpr int IDC_STEP_LIST = 1020;
constexpr int IDC_STEP_ADD = 1021;
constexpr int IDC_STEP_DELETE = 1022;
constexpr int IDC_STEP_UP = 1023;
constexpr int IDC_STEP_DOWN = 1024;
constexpr int IDC_STEP_X = 1025;
constexpr int IDC_STEP_Y = 1026;
constexpr int IDC_STEP_DELAY = 1027;
constexpr int IDC_STEP_REPEAT = 1028;
constexpr int IDC_STEP_SAVE = 1029;
constexpr int IDC_STEP_CAPTURE = 1030;
constexpr int IDC_STEP_TEST = 1031;

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

struct ClickStep {
    int x = -1;
    int y = -1;
    int baseW = 0;
    int baseH = 0;
    int delayMs = 500;
    int repeat = 1;
    bool valid = false;
};

struct Config {
    std::wstring templatePath;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int roiBaseW = 0;
    int roiBaseH = 0;
    int thresholdPercent = 90;
    std::vector<ClickStep> steps;
};

Config g_lastConfig{};

struct State {
    Target target{};
    Config config{};
    HWND hwnd = nullptr;
    HWND stepList = nullptr;
    int captureRow = -1;
    bool f8WasDown = false;
    bool runningChain = false;
};

struct RegionPickerState {
    const Image* frame = nullptr;
    HWND hwnd = nullptr;
    RECT imageRect{};
    POINT dragStart{};
    POINT dragCurrent{};
    bool dragging = false;
    bool accepted = false;
    RECT selected{};
};

struct PreviewState {
    const Image* image = nullptr;
    HWND hwnd = nullptr;
    RECT imageRect{};
    std::wstring note;
};

template <typename T>
void ReleaseCom(T*& p) {
    if (p) { p->Release(); p = nullptr; }
}

std::wstring HrText(HRESULT hr) {
    wchar_t buf[32]{};
    swprintf_s(buf, L"0x%08X", static_cast<unsigned int>(hr));
    return buf;
}

bool CurrentClientSize(HWND hwnd, int& w, int& h) {
    w = 0; h = 0;
    RECT rc{};
    if (!hwnd || !IsWindow(hwnd) || !GetClientRect(hwnd, &rc)) return false;
    w = rc.right - rc.left;
    h = rc.bottom - rc.top;
    return w > 0 && h > 0;
}

int ScaleCoord(int value, int fromExtent, int toExtent) {
    if (value < 0 || fromExtent <= 0 || toExtent <= 0) return value;
    const long long scaled = static_cast<long long>(value) * toExtent;
    return static_cast<int>((scaled + fromExtent / 2) / fromExtent);
}

void RebaseForCurrentClient(Config& c, HWND gameWindow) {
    int cw = 0, ch = 0;
    if (!CurrentClientSize(gameWindow, cw, ch)) return;
    if (c.roiBaseW > 0 && c.roiBaseH > 0 && (c.roiBaseW != cw || c.roiBaseH != ch)) {
        c.x = std::max(0, ScaleCoord(c.x, c.roiBaseW, cw));
        c.y = std::max(0, ScaleCoord(c.y, c.roiBaseH, ch));
        if (c.w > 0) c.w = std::max(1, ScaleCoord(c.w, c.roiBaseW, cw));
        if (c.h > 0) c.h = std::max(1, ScaleCoord(c.h, c.roiBaseH, ch));
    }
    c.roiBaseW = cw;
    c.roiBaseH = ch;
    for (ClickStep& step : c.steps) {
        if (!step.valid || step.baseW <= 0 || step.baseH <= 0) continue;
        if (step.baseW != cw || step.baseH != ch) {
            step.x = ScaleCoord(step.x, step.baseW, cw);
            step.y = ScaleCoord(step.y, step.baseH, ch);
            step.baseW = cw;
            step.baseH = ch;
        }
    }
}

bool LoadImageWic(const std::wstring& path, Image& out, std::wstring& error) {
    out = {};
    if (path.empty()) { error = L"Chưa chọn ảnh mẫu"; return false; }
    const HRESULT initHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool uninit = SUCCEEDED(initHr);
    if (FAILED(initHr) && initHr != RPC_E_CHANGED_MODE) {
        error = L"CoInitializeEx FAIL " + HrText(initHr); return false;
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
    ReleaseCom(converter); ReleaseCom(frame); ReleaseCom(decoder); ReleaseCom(factory);
    if (uninit) CoUninitialize();
    if (FAILED(hr)) {
        out = {}; error = L"Không đọc được ảnh mẫu bằng WIC: " + HrText(hr); return false;
    }
    return true;
}

bool CaptureClient(HWND hwnd, Image& out, std::wstring& backend, std::wstring& error) {
    out = {}; backend.clear();
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
    bi.bmiHeader.biHeight = -h;
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
    if (ok) backend = L"PrintWindow(HWND/PW_CLIENTONLY)";
    else {
        ok = BitBlt(mem, 0, 0, w, h, src, 0, 0, SRCCOPY | CAPTUREBLT);
        backend = L"BitBlt fallback";
    }
    if (ok) {
        out.width = w; out.height = h;
        const std::size_t bytes = static_cast<std::size_t>(w) * h * 4u;
        out.bgra.resize(bytes);
        std::memcpy(out.bgra.data(), bits, bytes);
    }
    SelectObject(mem, old); DeleteObject(dib); DeleteDC(mem); ReleaseDC(hwnd, src);
    if (!ok) { out = {}; error = L"PrintWindow và BitBlt đều FAIL"; return false; }
    return true;
}

inline int ColorDiff(const std::uint8_t* a, const std::uint8_t* b) {
    return std::abs(static_cast<int>(a[0]) - static_cast<int>(b[0])) +
           std::abs(static_cast<int>(a[1]) - static_cast<int>(b[1])) +
           std::abs(static_cast<int>(a[2]) - static_cast<int>(b[2]));
}

double ScoreAt(const Image& frame, const Image& tpl, int ox, int oy, int sampleStep) {
    std::uint64_t diff = 0, samples = 0;
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
    if (maxX < rx || maxY < ry) { error = L"Ảnh mẫu lớn hơn vùng quét"; return best; }

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

bool CropImage(const Image& src, int x, int y, int w, int h, Image& out, std::wstring& error) {
    out = {};
    if (src.width <= 0 || src.height <= 0) { error = L"Frame rỗng"; return false; }
    x = std::clamp(x, 0, src.width - 1);
    y = std::clamp(y, 0, src.height - 1);
    if (w <= 0) w = src.width - x;
    if (h <= 0) h = src.height - y;
    const int right = std::clamp(x + w, x + 1, src.width);
    const int bottom = std::clamp(y + h, y + 1, src.height);
    out.width = right - x; out.height = bottom - y;
    out.bgra.resize(static_cast<std::size_t>(out.width) * out.height * 4u);
    for (int row = 0; row < out.height; ++row) {
        const std::uint8_t* from = src.bgra.data() +
            (static_cast<std::size_t>(y + row) * src.width + x) * 4u;
        std::uint8_t* to = out.bgra.data() + static_cast<std::size_t>(row) * out.width * 4u;
        std::memcpy(to, from, static_cast<std::size_t>(out.width) * 4u);
    }
    return true;
}

void DrawImage(HDC dc, const Image& image, const RECT& dest) {
    if (image.width <= 0 || image.height <= 0 || image.bgra.empty()) return;
    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = image.width;
    bi.bmiHeader.biHeight = -image.height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    SetStretchBltMode(dc, COLORONCOLOR);
    StretchDIBits(dc, dest.left, dest.top, dest.right - dest.left, dest.bottom - dest.top,
                  0, 0, image.width, image.height, image.bgra.data(), &bi,
                  DIB_RGB_COLORS, SRCCOPY);
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
    if (h) { SetWindowTextW(h, text.c_str()); UpdateWindow(h); }
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
    SetEditInt(s.hwnd, IDC_STEP_REPEAT, step.repeat);
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
        std::wstring repeat = std::to_wstring(step.repeat);
        std::wstring state = step.valid ? L"READY • RAW hidden" : L"CHƯA CÓ TỌA ĐỘ";
        ListView_SetItemText(s.stepList, static_cast<int>(i), 1, x.data());
        ListView_SetItemText(s.stepList, static_cast<int>(i), 2, y.data());
        ListView_SetItemText(s.stepList, static_cast<int>(i), 3, base.data());
        ListView_SetItemText(s.stepList, static_cast<int>(i), 4, delay.data());
        ListView_SetItemText(s.stepList, static_cast<int>(i), 5, repeat.data());
        ListView_SetItemText(s.stepList, static_cast<int>(i), 6, state.data());
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
    step.delayMs = std::clamp(ReadInt(s.hwnd, IDC_STEP_DELAY, step.delayMs), 0, 60000);
    step.repeat = std::clamp(ReadInt(s.hwnd, IDC_STEP_REPEAT, step.repeat), 1, 999);
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
    s.config.x = std::max(0, ReadInt(s.hwnd, IDC_X, 0));
    s.config.y = std::max(0, ReadInt(s.hwnd, IDC_Y, 0));
    s.config.w = std::max(0, ReadInt(s.hwnd, IDC_W, 0));
    s.config.h = std::max(0, ReadInt(s.hwnd, IDC_H, 0));
    s.config.thresholdPercent = std::clamp(ReadInt(s.hwnd, IDC_THRESHOLD, 90), 1, 100);
    int cw = 0, ch = 0;
    if (CurrentClientSize(s.target.gameWindow, cw, ch)) { s.config.roiBaseW = cw; s.config.roiBaseH = ch; }
    g_lastConfig = s.config;
}

void PickTemplate(State& s) {
    SyncConfig(s);
    wchar_t file[MAX_PATH * 4]{};
    if (!s.config.templatePath.empty()) wcsncpy_s(file, s.config.templatePath.c_str(), _TRUNCATE);
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = s.hwnd;
    ofn.lpstrFilter = L"Ảnh mẫu (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0Tất cả file\0*.*\0\0";
    ofn.lpstrFile = file; ofn.nMaxFile = static_cast<DWORD>(std::size(file));
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;
    s.config.templatePath = file;
    SetDlgItemTextW(s.hwnd, IDC_TEMPLATE, file);
    g_lastConfig = s.config;
}

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
                              CW_USEDEFAULT,CW_USEDEFAULT,winW,winH,owner,nullptr,GetModuleHandleW(nullptr),&state);
    if(!hwnd){EnableWindow(owner,TRUE);return;}
    ShowWindow(hwnd,SW_SHOW);UpdateWindow(hwnd);SetFocus(hwnd);
    MSG msg{};bool sawQuit=false;int quitCode=0;
    while(IsWindow(hwnd)){
        const BOOL gm=GetMessageW(&msg,nullptr,0,0);
        if(gm<=0){if(gm==0){sawQuit=true;quitCode=static_cast<int>(msg.wParam);}break;}
        TranslateMessage(&msg);DispatchMessageW(&msg);
    }
    EnableWindow(owner,TRUE);SetActiveWindow(owner);if(sawQuit)PostQuitMessage(quitCode);
}

void SelectRegion(State& s) {
    SyncConfig(s);
    Image frame{}; std::wstring backend, error;
    if (!CaptureClient(s.target.gameWindow, frame, backend, error)) {
        SetStatus(s.hwnd, L"CHỌN VÙNG FAIL • " + error); return;
    }
    RECT region{};
    if (!PickRegionModal(s.hwnd, frame, region)) {
        SetStatus(s.hwnd, L"CHỌN VÙNG: đã hủy, giữ nguyên vùng cũ"); return;
    }
    s.config.x = region.left; s.config.y = region.top;
    s.config.w = region.right - region.left; s.config.h = region.bottom - region.top;
    s.config.roiBaseW = frame.width; s.config.roiBaseH = frame.height;
    SetEditInt(s.hwnd, IDC_X, s.config.x); SetEditInt(s.hwnd, IDC_Y, s.config.y);
    SetEditInt(s.hwnd, IDC_W, s.config.w); SetEditInt(s.hwnd, IDC_H, s.config.h);
    g_lastConfig = s.config;
    SetStatus(s.hwnd, L"ĐÃ CHỌN VÙNG • X=" + std::to_wstring(s.config.x) + L" Y=" + std::to_wstring(s.config.y) +
                       L" W=" + std::to_wstring(s.config.w) + L" H=" + std::to_wstring(s.config.h) +
                       L" • " + backend);
}

void PreviewRegion(State& s) {
    SyncConfig(s);
    Image frame{}, crop{}; std::wstring backend, error;
    if (!CaptureClient(s.target.gameWindow, frame, backend, error)) { SetStatus(s.hwnd, L"XEM VÙNG FAIL • " + error); return; }
    if (!CropImage(frame, s.config.x, s.config.y, s.config.w, s.config.h, crop, error)) { SetStatus(s.hwnd, L"XEM VÙNG FAIL • " + error); return; }
    const std::wstring note=L"ROI "+std::to_wstring(s.config.x)+L","+std::to_wstring(s.config.y)+L" • "+
                            std::to_wstring(crop.width)+L"x"+std::to_wstring(crop.height)+L" • "+backend+L" • ESC/Enter để đóng";
    ShowPreviewModal(s.hwnd,crop,note);
}

void ResetFullRegion(State& s) {
    int cw=0,ch=0;CurrentClientSize(s.target.gameWindow,cw,ch);
    s.config.x=0;s.config.y=0;s.config.w=0;s.config.h=0;s.config.roiBaseW=cw;s.config.roiBaseH=ch;
    SetEditInt(s.hwnd,IDC_X,0);SetEditInt(s.hwnd,IDC_Y,0);SetEditInt(s.hwnd,IDC_W,0);SetEditInt(s.hwnd,IDC_H,0);
    g_lastConfig=s.config;SetStatus(s.hwnd,L"VÙNG SCAN = FULL CLIENT • W/H=0");
}

void AddStep(State& s) {
    SaveStepEditor(s);
    if(s.config.steps.size()>=kMaxClickSteps){SetStatus(s.hwnd,L"CHUỖI CLICK tối đa 10 dòng");return;}
    ClickStep step{};int cw=0,ch=0;if(CurrentClientSize(s.target.gameWindow,cw,ch)){step.baseW=cw;step.baseH=ch;}
    s.config.steps.push_back(step);g_lastConfig=s.config;RefreshStepList(s,static_cast<int>(s.config.steps.size()-1));
}

void DeleteStep(State& s) {
    const int row=SelectedStep(s);if(row<0||row>=static_cast<int>(s.config.steps.size()))return;
    s.config.steps.erase(s.config.steps.begin()+row);g_lastConfig=s.config;
    RefreshStepList(s,std::min(row,static_cast<int>(s.config.steps.size())-1));
}

void MoveStep(State& s,int delta){
    SaveStepEditor(s);const int row=SelectedStep(s),next=row+delta;
    if(row<0||next<0||row>=static_cast<int>(s.config.steps.size())||next>=static_cast<int>(s.config.steps.size()))return;
    std::swap(s.config.steps[static_cast<std::size_t>(row)],s.config.steps[static_cast<std::size_t>(next)]);
    g_lastConfig=s.config;RefreshStepList(s,next);
}

void SaveSelectedStep(State& s){
    const int row=SelectedStep(s);if(row<0){SetStatus(s.hwnd,L"Chọn một dòng click trước");return;}
    SaveStepEditor(s);RefreshStepList(s,row);SetStatus(s.hwnd,L"Đã lưu dòng click "+std::to_wstring(row+1));
}

void ArmF8(State& s){
    const int row=SelectedStep(s);if(row<0||row>=static_cast<int>(s.config.steps.size())){SetStatus(s.hwnd,L"F8: chọn một dòng click trước");return;}
    s.captureRow=row;s.f8WasDown=(GetAsyncKeyState(VK_F8)&0x8000)!=0;SetTimer(s.hwnd,kF8PollTimer,30,nullptr);
    SetStatus(s.hwnd,L"ĐÃ ARM F8 CHO DÒNG "+std::to_wstring(row+1)+L" • đưa chuột vào đúng vị trí trên cửa sổ GAME rồi nhấn F8");
}

void PollF8(State& s){
    const bool down=(GetAsyncKeyState(VK_F8)&0x8000)!=0;
    if(s.captureRow>=0&&down&&!s.f8WasDown){
        POINT p{};GetCursorPos(&p);POINT client=p;
        if(!ScreenToClient(s.target.gameWindow,&client)){SetStatus(s.hwnd,L"F8 FAIL • ScreenToClient");}
        else{
            int cw=0,ch=0;if(!CurrentClientSize(s.target.gameWindow,cw,ch)||client.x<0||client.y<0||client.x>=cw||client.y>=ch){
                SetStatus(s.hwnd,L"F8: chuột chưa nằm trong client game • vẫn đang chờ F8 lần tiếp theo");
            }else if(s.captureRow<static_cast<int>(s.config.steps.size())){
                ClickStep& step=s.config.steps[static_cast<std::size_t>(s.captureRow)];
                step.x=client.x;step.y=client.y;step.baseW=cw;step.baseH=ch;step.valid=true;
                const int row=s.captureRow;s.captureRow=-1;KillTimer(s.hwnd,kF8PollTimer);g_lastConfig=s.config;RefreshStepList(s,row);
                SetStatus(s.hwnd,L"F8 PASS • dòng "+std::to_wstring(row+1)+L" = "+std::to_wstring(step.x)+L","+std::to_wstring(step.y)+
                                   L" @ "+std::to_wstring(cw)+L"x"+std::to_wstring(ch));
            }
        }
    }
    s.f8WasDown=down;
}

bool ResolveStepPoint(const ClickStep& step,int currentW,int currentH,int& x,int& y){
    if(!step.valid||step.baseW<=0||step.baseH<=0||currentW<=0||currentH<=0)return false;
    x=ScaleCoord(step.x,step.baseW,currentW);y=ScaleCoord(step.y,step.baseH,currentH);
    x=std::clamp(x,0,currentW-1);y=std::clamp(y,0,currentH-1);return true;
}

void TestSelectedStep(State& s){
    SaveStepEditor(s);const int row=SelectedStep(s);
    if(row<0||row>=static_cast<int>(s.config.steps.size())){SetStatus(s.hwnd,L"TEST DÒNG: chưa chọn dòng");return;}
    int cw=0,ch=0,x=0,y=0;if(!CurrentClientSize(s.target.gameWindow,cw,ch)||!ResolveStepPoint(s.config.steps[static_cast<std::size_t>(row)],cw,ch,x,y)){
        SetStatus(s.hwnd,L"TEST DÒNG FAIL • dòng chưa có tọa F8 hợp lệ");return;
    }
    std::wstring detail;const bool ok=s.target.hiddenClick&&s.target.hiddenClick(s.target.context,x,y,cw,ch,detail);
    SetStatus(s.hwnd,L"TEST DÒNG "+std::to_wstring(row+1)+(ok?L" PASS • ":L" FAIL • ")+std::to_wstring(x)+L","+std::to_wstring(y)+L" • "+detail);
}

bool ValidateChain(const State& s,std::wstring& error){
    if(s.config.steps.empty()){error=L"CHUỖI CLICK đang rỗng";return false;}
    if(s.config.steps.size()>kMaxClickSteps){error=L"CHUỖI CLICK vượt 10 dòng";return false;}
    for(std::size_t i=0;i<s.config.steps.size();++i){
        const ClickStep& step=s.config.steps[i];
        if(!step.valid||step.baseW<=0||step.baseH<=0){error=L"dòng "+std::to_wstring(i+1)+L" chưa F8/lưu tọa";return false;}
        if(step.delayMs<0||step.delayMs>60000||step.repeat<1||step.repeat>999){error=L"Delay/Repeat dòng "+std::to_wstring(i+1)+L" không hợp lệ";return false;}
    }
    return true;
}

void RunTest(State& s) {
    if(s.runningChain)return;SyncConfig(s);
    if(!s.target.gameWindow||!IsWindow(s.target.gameWindow)){SetStatus(s.hwnd,L"FAIL • cửa sổ game đã mất");return;}
    Image tpl{},frame{};std::wstring error,backend;
    if(!LoadImageWic(s.config.templatePath,tpl,error)){SetStatus(s.hwnd,L"TEMPLATE FAIL • "+error);return;}
    if(!CaptureClient(s.target.gameWindow,frame,backend,error)){SetStatus(s.hwnd,L"CAPTURE FAIL • "+error);return;}
    const double threshold=static_cast<double>(s.config.thresholdPercent)/100.0;
    Match match=FindTemplate(frame,tpl,s.config.x,s.config.y,s.config.w,s.config.h,threshold,error);
    if(match.score<0.0){SetStatus(s.hwnd,L"SCAN FAIL • "+error+L" • capture="+backend);return;}
    wchar_t score[64]{};swprintf_s(score,L"%.2f%%",match.score*100.0);
    std::wstring prefix=L"CAPTURE="+backend+L" • ROI="+std::to_wstring(s.config.x)+L","+std::to_wstring(s.config.y)+L"/"+
                        std::to_wstring(s.config.w)+L"x"+std::to_wstring(s.config.h)+L" • BEST="+score;
    if(!match.found){SetStatus(s.hwnd,prefix+L" • NOT FOUND → DỪNG, KHÔNG CLICK");return;}
    const int centerX=match.x+tpl.width/2,centerY=match.y+tpl.height/2;
    prefix+=L" • PASS @ "+std::to_wstring(match.x)+L","+std::to_wstring(match.y)+L" • tâm="+std::to_wstring(centerX)+L","+std::to_wstring(centerY);
    if(!ValidateChain(s,error)){SetStatus(s.hwnd,prefix+L" • PASS nhưng "+error+L" → KHÔNG CLICK");return;}
    if(!s.target.hiddenClick){SetStatus(s.hwnd,prefix+L" • không có RAW hidden-click callback");return;}

    std::size_t totalClicks=0;for(const ClickStep& step:s.config.steps)totalClicks+=static_cast<std::size_t>(step.repeat);
    std::size_t clickNo=0;s.runningChain=true;EnableWindow(GetDlgItem(s.hwnd,IDC_TEST),FALSE);
    for(std::size_t i=0;i<s.config.steps.size();++i){
        const ClickStep& step=s.config.steps[i];int x=0,y=0;
        if(!ResolveStepPoint(step,frame.width,frame.height,x,y)){
            SetStatus(s.hwnd,prefix+L" • CHAIN FAIL dòng "+std::to_wstring(i+1)+L" resolve tọa");s.runningChain=false;EnableWindow(GetDlgItem(s.hwnd,IDC_TEST),TRUE);return;
        }
        for(int r=0;r<step.repeat;++r){
            ++clickNo;std::wstring detail;
            SetStatus(s.hwnd,prefix+L" • CHAIN "+std::to_wstring(clickNo)+L"/"+std::to_wstring(totalClicks)+L" • dòng "+
                               std::to_wstring(i+1)+L" @ "+std::to_wstring(x)+L","+std::to_wstring(y));
            if(!s.target.hiddenClick(s.target.context,x,y,frame.width,frame.height,detail)){
                SetStatus(s.hwnd,prefix+L" • CHAIN FAIL tại dòng "+std::to_wstring(i+1)+L" repeat "+std::to_wstring(r+1)+L" • "+detail);
                s.runningChain=false;EnableWindow(GetDlgItem(s.hwnd,IDC_TEST),TRUE);return;
            }
            if(clickNo<totalClicks&&step.delayMs>0)Sleep(static_cast<DWORD>(step.delayMs));
        }
    }
    s.runningChain=false;EnableWindow(GetDlgItem(s.hwnd,IDC_TEST),TRUE);
    SetStatus(s.hwnd,prefix+L" • CHUỖI CLICK ẨN PASS "+std::to_wstring(totalClicks)+L" click • KHÔNG MapID/combat/AUTO/state machine");
}

void BuildControls(State& s) {
    Add(s.hwnd,L"STATIC",(L"ACC TEST: "+s.target.accountLabel).c_str(),SS_LEFT|SS_CENTERIMAGE,18,12,850,25,0);
    Add(s.hwnd,L"STATIC",L"Ảnh mẫu:",SS_LEFT|SS_CENTERIMAGE,18,43,70,26,0);
    Add(s.hwnd,L"EDIT",s.config.templatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,90,43,650,26,IDC_TEMPLATE);
    Add(s.hwnd,L"BUTTON",L"CHỌN ẢNH",BS_PUSHBUTTON,750,42,120,28,IDC_PICK);

    Add(s.hwnd,L"STATIC",L"Vùng scan: X",SS_LEFT|SS_CENTERIMAGE,18,78,80,25,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.x).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,98,78,60,26,IDC_X);
    Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,163,78,20,25,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.y).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,185,78,60,26,IDC_Y);
    Add(s.hwnd,L"STATIC",L"W",SS_CENTER|SS_CENTERIMAGE,250,78,20,25,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.w).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,272,78,65,26,IDC_W);
    Add(s.hwnd,L"STATIC",L"H",SS_CENTER|SS_CENTERIMAGE,342,78,20,25,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.h).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,364,78,65,26,IDC_H);
    Add(s.hwnd,L"STATIC",L"Ngưỡng %",SS_LEFT|SS_CENTERIMAGE,440,78,65,25,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.thresholdPercent).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,507,78,55,26,IDC_THRESHOLD);
    Add(s.hwnd,L"BUTTON",L"CHỌN VÙNG BẰNG CHUỘT",BS_PUSHBUTTON,575,75,190,31,IDC_PICK_REGION);
    Add(s.hwnd,L"BUTTON",L"XEM VÙNG",BS_PUSHBUTTON,774,75,96,31,IDC_PREVIEW_REGION);
    Add(s.hwnd,L"BUTTON",L"FULL CLIENT",BS_PUSHBUTTON,774,110,96,27,IDC_FULL_REGION);
    Add(s.hwnd,L"STATIC",L"Khoanh vùng trên ảnh chụp HWND; thả chuột tự điền X/Y/W/H. Tọa vùng + click tự scale theo size client.",SS_LEFT|SS_CENTERIMAGE,18,111,745,26,0);

    Add(s.hwnd,L"BUTTON",L"CHUỖI CLICK SAU KHI SCAN PASS • 1–10 dòng • RAW TryClickUI → EndUIDrag",BS_GROUPBOX,18,145,852,330,0);
    s.stepList=Add(s.hwnd,WC_LISTVIEWW,L"",LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS|WS_BORDER,32,170,824,185,IDC_STEP_LIST);
    ListView_SetExtendedListViewStyle(s.stepList,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);
    AddColumn(s.stepList,0,38,L"#");AddColumn(s.stepList,1,75,L"X");AddColumn(s.stepList,2,75,L"Y");
    AddColumn(s.stepList,3,105,L"Base size");AddColumn(s.stepList,4,85,L"Delay ms");AddColumn(s.stepList,5,70,L"Repeat");AddColumn(s.stepList,6,360,L"Trạng thái");
    Add(s.hwnd,L"BUTTON",L"+ THÊM",BS_PUSHBUTTON,32,362,78,27,IDC_STEP_ADD);
    Add(s.hwnd,L"BUTTON",L"- XÓA",BS_PUSHBUTTON,116,362,76,27,IDC_STEP_DELETE);
    Add(s.hwnd,L"BUTTON",L"LÊN",BS_PUSHBUTTON,198,362,64,27,IDC_STEP_UP);
    Add(s.hwnd,L"BUTTON",L"XUỐNG",BS_PUSHBUTTON,268,362,70,27,IDC_STEP_DOWN);
    Add(s.hwnd,L"STATIC",L"X:",SS_LEFT|SS_CENTERIMAGE,350,362,18,27,0);
    Add(s.hwnd,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,370,362,58,27,IDC_STEP_X);
    Add(s.hwnd,L"STATIC",L"Y:",SS_LEFT|SS_CENTERIMAGE,434,362,18,27,0);
    Add(s.hwnd,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,454,362,58,27,IDC_STEP_Y);
    Add(s.hwnd,L"STATIC",L"Delay:",SS_LEFT|SS_CENTERIMAGE,520,362,42,27,0);
    Add(s.hwnd,L"EDIT",L"500",WS_BORDER|ES_NUMBER|ES_CENTER,564,362,62,27,IDC_STEP_DELAY);
    Add(s.hwnd,L"STATIC",L"Lặp:",SS_LEFT|SS_CENTERIMAGE,634,362,34,27,0);
    Add(s.hwnd,L"EDIT",L"1",WS_BORDER|ES_NUMBER|ES_CENTER,670,362,48,27,IDC_STEP_REPEAT);
    Add(s.hwnd,L"BUTTON",L"LƯU DÒNG",BS_PUSHBUTTON,725,362,131,27,IDC_STEP_SAVE);
    Add(s.hwnd,L"BUTTON",L"LẤY TỌA F8",BS_PUSHBUTTON,32,397,140,30,IDC_STEP_CAPTURE);
    Add(s.hwnd,L"BUTTON",L"TEST DÒNG ẨN",BS_PUSHBUTTON,180,397,135,30,IDC_STEP_TEST);
    Add(s.hwnd,L"STATIC",L"F8: chọn dòng → LẤY TỌA F8 → đưa chuột vào đúng điểm trong game → F8. Delay chạy sau mỗi click/repeat.",SS_LEFT|SS_CENTERIMAGE,330,397,526,30,0);
    Add(s.hwnd,L"STATIC",L"SCAN PASS mới chạy toàn bộ chuỗi; NOT FOUND dừng ngay. Không dùng MapID, combat, AUTO, bag, route hay state machine game.",SS_LEFT|SS_CENTERIMAGE,32,435,824,28,0);

    Add(s.hwnd,L"BUTTON",L"TEST SCAN + CLICK ẨN • CHẠY TOÀN BỘ CHUỖI",BS_DEFPUSHBUTTON,18,486,852,38,IDC_TEST);
    Add(s.hwnd,L"STATIC",L"Sẵn sàng. Nên CHỌN VÙNG thật nhỏ rồi XEM VÙNG trước khi test.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,535,852,88,IDC_STATUS);
    Add(s.hwnd,L"BUTTON",L"ĐÓNG",BS_PUSHBUTTON,750,635,120,31,IDCANCEL);
    RefreshStepList(s);
}

LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    State* s=reinterpret_cast<State*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE){auto* cs=reinterpret_cast<CREATESTRUCTW*>(lp);s=static_cast<State*>(cs->lpCreateParams);SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));if(s)s->hwnd=hwnd;}
    if(!s)return DefWindowProcW(hwnd,msg,wp,lp);
    switch(msg){
        case WM_CREATE:BuildControls(*s);return 0;
        case WM_TIMER:if(wp==kF8PollTimer){PollF8(*s);return 0;}break;
        case WM_NOTIFY:{
            auto* hdr=reinterpret_cast<NMHDR*>(lp);
            if(hdr&&hdr->idFrom==IDC_STEP_LIST&&hdr->code==LVN_ITEMCHANGED){
                const auto* n=reinterpret_cast<NMLISTVIEW*>(hdr);
                if((n->uChanged&LVIF_STATE)!=0&&(n->uNewState&LVIS_SELECTED)!=0)LoadStepEditor(*s,n->iItem);
            }
            return 0;
        }
        case WM_COMMAND:
            switch(LOWORD(wp)){
                case IDC_PICK:PickTemplate(*s);return 0;
                case IDC_PICK_REGION:SelectRegion(*s);return 0;
                case IDC_PREVIEW_REGION:PreviewRegion(*s);return 0;
                case IDC_FULL_REGION:ResetFullRegion(*s);return 0;
                case IDC_STEP_ADD:AddStep(*s);return 0;
                case IDC_STEP_DELETE:DeleteStep(*s);return 0;
                case IDC_STEP_UP:MoveStep(*s,-1);return 0;
                case IDC_STEP_DOWN:MoveStep(*s,1);return 0;
                case IDC_STEP_SAVE:SaveSelectedStep(*s);return 0;
                case IDC_STEP_CAPTURE:ArmF8(*s);return 0;
                case IDC_STEP_TEST:TestSelectedStep(*s);return 0;
                case IDC_TEST:RunTest(*s);return 0;
                case IDCANCEL:if(!s->runningChain){SyncConfig(*s);DestroyWindow(hwnd);}return 0;
            }
            break;
        case WM_CLOSE:if(!s->runningChain){SyncConfig(*s);DestroyWindow(hwnd);}return 0;
        case WM_NCDESTROY:KillTimer(hwnd,kF8PollTimer);s->hwnd=nullptr;s->stepList=nullptr;return DefWindowProcW(hwnd,msg,wp,lp);
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}

} // namespace

void RunDialog(const Target& target){
    if(!target.owner||!target.gameWindow)return;
    State state{};state.target=target;state.config=g_lastConfig;RebaseForCurrentClient(state.config,target.gameWindow);
    WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.lpfnWndProc=WndProc;wc.hInstance=GetModuleHandleW(nullptr);wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);wc.lpszClassName=kClassName;
    if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return;
    EnableWindow(target.owner,FALSE);
    HWND hwnd=CreateWindowExW(WS_EX_TOOLWINDOW,kClassName,L"TEST SCAN ẢNH ẨN • VÙNG NHỎ + CHUỖI CLICK v2",
                              WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
                              CW_USEDEFAULT,CW_USEDEFAULT,910,715,target.owner,nullptr,GetModuleHandleW(nullptr),&state);
    if(!hwnd){EnableWindow(target.owner,TRUE);return;}
    ShowWindow(hwnd,SW_SHOW);UpdateWindow(hwnd);
    MSG msg{};bool sawQuit=false;int quitCode=0;
    while(IsWindow(hwnd)){
        const BOOL gm=GetMessageW(&msg,nullptr,0,0);
        if(gm<=0){if(gm==0){sawQuit=true;quitCode=static_cast<int>(msg.wParam);}break;}
        if(!IsDialogMessageW(hwnd,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    }
    g_lastConfig=state.config;EnableWindow(target.owner,TRUE);SetActiveWindow(target.owner);if(sawQuit)PostQuitMessage(quitCode);
}

} // namespace image_scan_test
