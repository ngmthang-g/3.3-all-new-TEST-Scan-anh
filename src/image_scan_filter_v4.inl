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

constexpr wchar_t kClassName[] = L"ThanLongImageFilterTestPocV4";
constexpr wchar_t kRegionClassName[] = L"ThanLongImageScanRegionPickerV2";
constexpr wchar_t kPreviewClassName[] = L"ThanLongImageScanPreviewV2";
constexpr UINT kPwRenderFullContent = 0x00000002u;
constexpr UINT_PTR kF8PollTimer = 91;
constexpr UINT_PTR kRunTimer = 92;
constexpr UINT kProbeIntervalMs = 80;
constexpr std::size_t kDefaultInitialSteps = 20;

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
constexpr int IDC_TEMPLATE_DISCARD = 1040;
constexpr int IDC_PICK_DISCARD = 1041;
constexpr int IDC_TEMPLATE_CLOSE = 1042;
constexpr int IDC_PICK_CLOSE = 1043;
constexpr int IDC_AFTER_X = 1044;
constexpr int IDC_AFTER_Y = 1045;
constexpr int IDC_AFTER_CAPTURE = 1046;
constexpr int IDC_AFTER_TEST = 1047;
constexpr int IDC_DISCARD_X = 1050;
constexpr int IDC_DISCARD_Y = 1051;
constexpr int IDC_DISCARD_W = 1052;
constexpr int IDC_DISCARD_H = 1053;
constexpr int IDC_DISCARD_REGION = 1054;
constexpr int IDC_DISCARD_PREVIEW = 1055;
constexpr int IDC_DISCARD_FULL = 1056;
constexpr int IDC_CLOSE_X = 1060;
constexpr int IDC_CLOSE_Y = 1061;
constexpr int IDC_CLOSE_W = 1062;
constexpr int IDC_CLOSE_H = 1063;
constexpr int IDC_CLOSE_REGION = 1064;
constexpr int IDC_CLOSE_PREVIEW = 1065;
constexpr int IDC_CLOSE_FULL = 1066;

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

struct ScanRoi {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int baseW = 0;
    int baseH = 0;
};

struct Config {
    // Ảnh 1 = dấu hiệu món đồ ĐÚNG. Chỉ scan, KHÔNG click ảnh 1.
    std::wstring templatePath;
    std::wstring discardTemplatePath;
    std::wstring closeTemplatePath;

    // ROI ẢNH 1 giữ các field cũ để tương thích cấu hình v3.
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int roiBaseW = 0;
    int roiBaseH = 0;

    // V4: NÚT VỨT và DẤU X có ROI riêng. Cả 3 ROI dùng trên CÙNG MỘT frame capture.
    ScanRoi discardRoi{};
    ScanRoi closeRoi{};

    int thresholdPercent = 90;
    std::vector<ClickStep> steps;
    ClickStep afterDiscard;
};

Config g_lastConfig{};

enum class RunPhase {
    Idle,
    WaitItemReady,
    WaitDiscardGone,
    WaitPopupGoneAfterConfirm,
    WaitCloseGone,
};

struct State {
    Target target{};
    Config config{};
    HWND hwnd = nullptr;
    HWND stepList = nullptr;
    int captureRow = -1;
    bool f8WasDown = false;
    bool runningChain = false;

    // Runtime v4: template load đúng 1 lần khi Start; timer/state không khóa UI thread bằng Sleep.
    Image goodTpl{};
    Image discardTpl{};
    Image closeTpl{};
    RunPhase runPhase = RunPhase::Idle;
    std::size_t slotIndex = 0;
    std::size_t discardCount = 0;
    ULONGLONG nextProbeTick = 0;
    ULONGLONG phaseDeadlineTick = 0;
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

void RebaseNamedRoi(ScanRoi& r, int cw, int ch) {
    if (r.baseW > 0 && r.baseH > 0 && (r.baseW != cw || r.baseH != ch)) {
        r.x = std::max(0, ScaleCoord(r.x, r.baseW, cw));
        r.y = std::max(0, ScaleCoord(r.y, r.baseH, ch));
        if (r.w > 0) r.w = std::max(1, ScaleCoord(r.w, r.baseW, cw));
        if (r.h > 0) r.h = std::max(1, ScaleCoord(r.h, r.baseH, ch));
    }
    r.baseW = cw;
    r.baseH = ch;
}

void RebaseClickStep(ClickStep& step, int cw, int ch) {
    if (!step.valid || step.baseW <= 0 || step.baseH <= 0) return;
    if (step.baseW != cw || step.baseH != ch) {
        step.x = ScaleCoord(step.x, step.baseW, cw);
        step.y = ScaleCoord(step.y, step.baseH, ch);
        step.baseW = cw;
        step.baseH = ch;
    }
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
    RebaseNamedRoi(c.discardRoi, cw, ch);
    RebaseNamedRoi(c.closeRoi, cw, ch);
    for (ClickStep& step : c.steps) RebaseClickStep(step, cw, ch);
    RebaseClickStep(c.afterDiscard, cw, ch);
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
        SetStatus(s.hwnd, L"CHỌN ROI ẢNH 1 FAIL • " + error); return;
    }
    RECT region{};
    if (!PickRegionModal(s.hwnd, frame, region)) {
        SetStatus(s.hwnd, L"ROI ẢNH 1: đã hủy, giữ nguyên vùng cũ"); return;
    }
    s.config.x = region.left; s.config.y = region.top;
    s.config.w = region.right - region.left; s.config.h = region.bottom - region.top;
    s.config.roiBaseW = frame.width; s.config.roiBaseH = frame.height;
    SetEditInt(s.hwnd, IDC_X, s.config.x); SetEditInt(s.hwnd, IDC_Y, s.config.y);
    SetEditInt(s.hwnd, IDC_W, s.config.w); SetEditInt(s.hwnd, IDC_H, s.config.h);
    g_lastConfig = s.config;
    SetStatus(s.hwnd, L"ROI ẢNH 1 PASS • " + std::to_wstring(s.config.x) + L"," + std::to_wstring(s.config.y) +
                       L"/" + std::to_wstring(s.config.w) + L"x" + std::to_wstring(s.config.h) + L" • " + backend);
}

void PreviewRegion(State& s) {
    SyncConfig(s);
    Image frame{}, crop{}; std::wstring backend, error;
    if (!CaptureClient(s.target.gameWindow, frame, backend, error)) { SetStatus(s.hwnd, L"XEM ROI ẢNH 1 FAIL • " + error); return; }
    if (!CropImage(frame, s.config.x, s.config.y, s.config.w, s.config.h, crop, error)) { SetStatus(s.hwnd, L"XEM ROI ẢNH 1 FAIL • " + error); return; }
    ShowPreviewModal(s.hwnd, crop, L"ROI ẢNH 1 • " + std::to_wstring(crop.width) + L"x" + std::to_wstring(crop.height) + L" • " + backend);
}

void ResetFullRegion(State& s) {
    int cw=0,ch=0;CurrentClientSize(s.target.gameWindow,cw,ch);
    s.config.x=0;s.config.y=0;s.config.w=0;s.config.h=0;s.config.roiBaseW=cw;s.config.roiBaseH=ch;
    SetEditInt(s.hwnd,IDC_X,0);SetEditInt(s.hwnd,IDC_Y,0);SetEditInt(s.hwnd,IDC_W,0);SetEditInt(s.hwnd,IDC_H,0);
    g_lastConfig=s.config;SetStatus(s.hwnd,L"ROI ẢNH 1 = FULL CLIENT");
}

void SelectNamedRegion(State& s, ScanRoi& roi, int xId, int yId, int wId, int hId, const wchar_t* label) {
    SyncConfig(s);
    Image frame{}; std::wstring backend, error;
    if (!CaptureClient(s.target.gameWindow, frame, backend, error)) {
        SetStatus(s.hwnd, std::wstring(L"CHỌN ") + label + L" FAIL • " + error); return;
    }
    RECT region{};
    if (!PickRegionModal(s.hwnd, frame, region)) {
        SetStatus(s.hwnd, std::wstring(label) + L": đã hủy, giữ nguyên vùng cũ"); return;
    }
    roi.x = region.left; roi.y = region.top; roi.w = region.right-region.left; roi.h = region.bottom-region.top;
    roi.baseW = frame.width; roi.baseH = frame.height;
    SetEditInt(s.hwnd,xId,roi.x);SetEditInt(s.hwnd,yId,roi.y);SetEditInt(s.hwnd,wId,roi.w);SetEditInt(s.hwnd,hId,roi.h);
    g_lastConfig=s.config;
    SetStatus(s.hwnd,std::wstring(label)+L" PASS • "+std::to_wstring(roi.x)+L","+std::to_wstring(roi.y)+L"/"+std::to_wstring(roi.w)+L"x"+std::to_wstring(roi.h)+L" • "+backend);
}

void PreviewNamedRegion(State& s, const ScanRoi& roi, const wchar_t* label) {
    SyncConfig(s);
    Image frame{},crop{};std::wstring backend,error;
    if(!CaptureClient(s.target.gameWindow,frame,backend,error)){SetStatus(s.hwnd,std::wstring(L"XEM ")+label+L" FAIL • "+error);return;}
    int rx=roi.x,ry=roi.y,rw=roi.w,rh=roi.h;
    if(roi.baseW>0&&roi.baseH>0&&(roi.baseW!=frame.width||roi.baseH!=frame.height)){
        rx=ScaleCoord(rx,roi.baseW,frame.width);ry=ScaleCoord(ry,roi.baseH,frame.height);
        if(rw>0)rw=std::max(1,ScaleCoord(rw,roi.baseW,frame.width));
        if(rh>0)rh=std::max(1,ScaleCoord(rh,roi.baseH,frame.height));
    }
    if(!CropImage(frame,rx,ry,rw,rh,crop,error)){SetStatus(s.hwnd,std::wstring(L"XEM ")+label+L" FAIL • "+error);return;}
    ShowPreviewModal(s.hwnd,crop,std::wstring(label)+L" • "+std::to_wstring(crop.width)+L"x"+std::to_wstring(crop.height)+L" • "+backend);
}

void ResetNamedRegion(State& s, ScanRoi& roi, int xId, int yId, int wId, int hId, const wchar_t* label) {
    int cw=0,ch=0;CurrentClientSize(s.target.gameWindow,cw,ch);roi={};roi.baseW=cw;roi.baseH=ch;
    SetEditInt(s.hwnd,xId,0);SetEditInt(s.hwnd,yId,0);SetEditInt(s.hwnd,wId,0);SetEditInt(s.hwnd,hId,0);
    g_lastConfig=s.config;SetStatus(s.hwnd,std::wstring(label)+L" = FULL CLIENT");
}

void SelectDiscardRegion(State& s){SelectNamedRegion(s,s.config.discardRoi,IDC_DISCARD_X,IDC_DISCARD_Y,IDC_DISCARD_W,IDC_DISCARD_H,L"ROI VỨT");}
void PreviewDiscardRegion(State& s){PreviewNamedRegion(s,s.config.discardRoi,L"ROI VỨT");}
void ResetDiscardRegion(State& s){ResetNamedRegion(s,s.config.discardRoi,IDC_DISCARD_X,IDC_DISCARD_Y,IDC_DISCARD_W,IDC_DISCARD_H,L"ROI VỨT");}
void SelectCloseRegion(State& s){SelectNamedRegion(s,s.config.closeRoi,IDC_CLOSE_X,IDC_CLOSE_Y,IDC_CLOSE_W,IDC_CLOSE_H,L"ROI DẤU X");}
void PreviewCloseRegion(State& s){PreviewNamedRegion(s,s.config.closeRoi,L"ROI DẤU X");}
void ResetCloseRegion(State& s){ResetNamedRegion(s,s.config.closeRoi,IDC_CLOSE_X,IDC_CLOSE_Y,IDC_CLOSE_W,IDC_CLOSE_H,L"ROI DẤU X");}

void AddStep(State& s) {
    SaveStepEditor(s);
    ClickStep step{};int cw=0,ch=0;if(CurrentClientSize(s.target.gameWindow,cw,ch)){step.baseW=cw;step.baseH=ch;}
    s.config.steps.push_back(step);g_lastConfig=s.config;RefreshStepList(s,static_cast<int>(s.config.steps.size()-1));
    SetStatus(s.hwnd,L"Đã thêm CLICK "+std::to_wstring(s.config.steps.size())+L" • không có giới hạn cứng");
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
    const int row=SelectedStep(s);if(row<0||row>=static_cast<int>(s.config.steps.size())){SetStatus(s.hwnd,L"F8: chọn một dòng CLICK trước");return;}
    s.captureRow=row;s.f8WasDown=(GetAsyncKeyState(VK_F8)&0x8000)!=0;SetTimer(s.hwnd,kF8PollTimer,30,nullptr);
    SetStatus(s.hwnd,L"ĐÃ ARM F8 CHO CLICK "+std::to_wstring(row+1)+L" • đưa chuột vào đúng ô đồ trong GAME rồi F8");
}

void ArmAfterDiscardF8(State& s){
    s.captureRow=-2;s.f8WasDown=(GetAsyncKeyState(VK_F8)&0x8000)!=0;SetTimer(s.hwnd,kF8PollTimer,30,nullptr);
    SetStatus(s.hwnd,L"ĐÃ ARM F8 CHO CLICK SAU VỨT • đưa chuột vào nút xác nhận/phía sau rồi F8");
}

void PollF8(State& s){
    const bool down=(GetAsyncKeyState(VK_F8)&0x8000)!=0;
    if(s.captureRow!=-1&&down&&!s.f8WasDown){
        POINT p{};GetCursorPos(&p);POINT client=p;
        if(!ScreenToClient(s.target.gameWindow,&client)){SetStatus(s.hwnd,L"F8 FAIL • ScreenToClient");}
        else{
            int cw=0,ch=0;if(!CurrentClientSize(s.target.gameWindow,cw,ch)||client.x<0||client.y<0||client.x>=cw||client.y>=ch){
                SetStatus(s.hwnd,L"F8: chuột chưa nằm trong client game • vẫn đang chờ F8");
            }else if(s.captureRow==-2){
                ClickStep& step=s.config.afterDiscard;step.x=client.x;step.y=client.y;step.baseW=cw;step.baseH=ch;step.valid=true;step.repeat=1;
                s.captureRow=-1;KillTimer(s.hwnd,kF8PollTimer);g_lastConfig=s.config;
                SetEditInt(s.hwnd,IDC_AFTER_X,step.x);SetEditInt(s.hwnd,IDC_AFTER_Y,step.y);
                SetStatus(s.hwnd,L"F8 PASS • CLICK SAU VỨT = "+std::to_wstring(step.x)+L","+std::to_wstring(step.y)+L" @ "+std::to_wstring(cw)+L"x"+std::to_wstring(ch));
            }else if(s.captureRow>=0&&s.captureRow<static_cast<int>(s.config.steps.size())){
                ClickStep& step=s.config.steps[static_cast<std::size_t>(s.captureRow)];
                step.x=client.x;step.y=client.y;step.baseW=cw;step.baseH=ch;step.valid=true;step.repeat=1;
                const int row=s.captureRow;s.captureRow=-1;KillTimer(s.hwnd,kF8PollTimer);g_lastConfig=s.config;RefreshStepList(s,row);
                SetStatus(s.hwnd,L"F8 PASS • CLICK "+std::to_wstring(row+1)+L" = "+std::to_wstring(step.x)+L","+std::to_wstring(step.y)+L" @ "+std::to_wstring(cw)+L"x"+std::to_wstring(ch));
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

bool ValidateFilter(const State& s,std::wstring& error){
    if(s.config.templatePath.empty()){error=L"chưa chọn ẢNH 1 (đồ đúng)";return false;}
    if(s.config.discardTemplatePath.empty()){error=L"chưa chọn ảnh NÚT VỨT";return false;}
    if(s.config.closeTemplatePath.empty()){error=L"chưa chọn ảnh DẤU X";return false;}
    if(s.config.steps.empty()){error=L"danh sách CLICK đang rỗng";return false;}
    for(std::size_t i=0;i<s.config.steps.size();++i){
        const ClickStep& step=s.config.steps[i];
        if(!step.valid||step.baseW<=0||step.baseH<=0){error=L"CLICK "+std::to_wstring(i+1)+L" chưa gán tọa F8";return false;}
        if(step.delayMs<50||step.delayMs>10000){error=L"Delay CLICK "+std::to_wstring(i+1)+L" không hợp lệ (50-10000 ms)";return false;}
    }
    if(!s.config.afterDiscard.valid||s.config.afterDiscard.baseW<=0||s.config.afterDiscard.baseH<=0){
        error=L"chưa gán CLICK SAU VỨT bằng F8";return false;
    }
    return true;
}

void ResolveGoodRoi(const Config& c,int currentW,int currentH,int& rx,int& ry,int& rw,int& rh){
    rx=c.x;ry=c.y;rw=c.w;rh=c.h;
    if(c.roiBaseW>0&&c.roiBaseH>0&&(c.roiBaseW!=currentW||c.roiBaseH!=currentH)){
        rx=ScaleCoord(rx,c.roiBaseW,currentW);ry=ScaleCoord(ry,c.roiBaseH,currentH);
        if(rw>0)rw=std::max(1,ScaleCoord(rw,c.roiBaseW,currentW));
        if(rh>0)rh=std::max(1,ScaleCoord(rh,c.roiBaseH,currentH));
    }
}

void ResolveNamedRoi(const ScanRoi& r,int currentW,int currentH,int& rx,int& ry,int& rw,int& rh){
    rx=r.x;ry=r.y;rw=r.w;rh=r.h;
    if(r.baseW>0&&r.baseH>0&&(r.baseW!=currentW||r.baseH!=currentH)){
        rx=ScaleCoord(rx,r.baseW,currentW);ry=ScaleCoord(ry,r.baseH,currentH);
        if(rw>0)rw=std::max(1,ScaleCoord(rw,r.baseW,currentW));
        if(rh>0)rh=std::max(1,ScaleCoord(rh,r.baseH,currentH));
    }
}

std::wstring ScoreText(double score){
    if(score<0.0)return L"N/A";wchar_t b[40]{};swprintf_s(b,L"%.2f%%",score*100.0);return b;
}

bool ScanFrameRect(const State& s,const Image& frame,const Image& tpl,int rx,int ry,int rw,int rh,Match& match,std::wstring& error){
    match=FindTemplate(frame,tpl,rx,ry,rw,rh,static_cast<double>(s.config.thresholdPercent)/100.0,error);
    return match.score>=0.0;
}

bool ScanGoodOnFrame(const State& s,const Image& frame,Match& match,std::wstring& error){
    int rx=0,ry=0,rw=0,rh=0;ResolveGoodRoi(s.config,frame.width,frame.height,rx,ry,rw,rh);
    return ScanFrameRect(s,frame,s.goodTpl,rx,ry,rw,rh,match,error);
}

bool ScanDiscardOnFrame(const State& s,const Image& frame,Match& match,std::wstring& error){
    int rx=0,ry=0,rw=0,rh=0;ResolveNamedRoi(s.config.discardRoi,frame.width,frame.height,rx,ry,rw,rh);
    return ScanFrameRect(s,frame,s.discardTpl,rx,ry,rw,rh,match,error);
}

bool ScanCloseOnFrame(const State& s,const Image& frame,Match& match,std::wstring& error){
    int rx=0,ry=0,rw=0,rh=0;ResolveNamedRoi(s.config.closeRoi,frame.width,frame.height,rx,ry,rw,rh);
    return ScanFrameRect(s,frame,s.closeTpl,rx,ry,rw,rh,match,error);
}

bool RawClick(State& s,int x,int y,int cw,int ch,std::wstring& error){
    if(!s.target.hiddenClick){error=L"không có RAW hidden-click callback";return false;}
    std::wstring detail;if(!s.target.hiddenClick(s.target.context,x,y,cw,ch,detail)){error=detail;return false;}
    return true;
}

void TestAfterDiscard(State& s){
    SyncConfig(s);int cw=0,ch=0,x=0,y=0;
    if(!CurrentClientSize(s.target.gameWindow,cw,ch)||!ResolveStepPoint(s.config.afterDiscard,cw,ch,x,y)){
        SetStatus(s.hwnd,L"TEST CLICK SAU VỨT FAIL • chưa F8 tọa hợp lệ");return;
    }
    std::wstring error;const bool ok=RawClick(s,x,y,cw,ch,error);
    SetStatus(s.hwnd,L"TEST CLICK SAU VỨT "+std::wstring(ok?L"PASS • ":L"FAIL • ")+std::to_wstring(x)+L","+std::to_wstring(y)+(error.empty()?L"":L" • "+error));
}

int StepDelayMs(const State& s){
    if(s.slotIndex<s.config.steps.size())return std::clamp(s.config.steps[s.slotIndex].delayMs,50,10000);
    return 500;
}

constexpr UINT kStateTimeoutMs = 5000;

void ArmRunPhase(State& s,RunPhase phase,UINT firstProbeDelay=40){
    const ULONGLONG now=GetTickCount64();s.runPhase=phase;s.nextProbeTick=now+firstProbeDelay;s.phaseDeadlineTick=now+static_cast<ULONGLONG>(kStateTimeoutMs);
}

void FinishRun(State& s,const std::wstring& status){
    KillTimer(s.hwnd,kRunTimer);s.runningChain=false;s.runPhase=RunPhase::Idle;EnableWindow(GetDlgItem(s.hwnd,IDC_TEST),TRUE);SetStatus(s.hwnd,status);
}

bool ClickCurrentSlot(State& s,std::wstring& error){
    if(s.slotIndex>=s.config.steps.size()){error=L"slot index vượt cấu hình";return false;}
    int cw=0,ch=0,x=0,y=0;if(!CurrentClientSize(s.target.gameWindow,cw,ch)||!ResolveStepPoint(s.config.steps[s.slotIndex],cw,ch,x,y)){error=L"không resolve được tọa CLICK";return false;}
    return RawClick(s,x,y,cw,ch,error);
}

bool ClickAfterDiscard(State& s,std::wstring& error){
    int cw=0,ch=0,x=0,y=0;if(!CurrentClientSize(s.target.gameWindow,cw,ch)||!ResolveStepPoint(s.config.afterDiscard,cw,ch,x,y)){error=L"không resolve được CLICK SAU VỨT";return false;}
    return RawClick(s,x,y,cw,ch,error);
}

bool ProbeTimedOut(const State& s,ULONGLONG now){return now>=s.phaseDeadlineTick;}
void ScheduleNextProbe(State& s,ULONGLONG now){s.nextProbeTick=now+kProbeIntervalMs;}

void ProcessRunTick(State& s){
    if(!s.runningChain)return;
    if((GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0){FinishRun(s,L"ĐÃ DỪNG BẰNG ESC tại CLICK "+std::to_wstring(s.slotIndex+1));return;}
    if(!s.target.gameWindow||!IsWindow(s.target.gameWindow)){FinishRun(s,L"FAIL • cửa sổ game đã mất");return;}
    const ULONGLONG now=GetTickCount64();if(now<s.nextProbeTick)return;

    // V4: MỖI PROBE CHỈ CHỤP HWND ĐÚNG 1 LẦN. Mọi scan cần thiết dùng chung frame này.
    Image frame{};std::wstring backend,error;
    if(!CaptureClient(s.target.gameWindow,frame,backend,error)){FinishRun(s,L"CAPTURE FAIL • "+error);return;}

    if(s.runPhase==RunPhase::WaitItemReady){
        Match good{};error.clear();
        if(!ScanGoodOnFrame(s,frame,good,error)){FinishRun(s,L"ROI ẢNH 1 FAIL • "+error);return;}
        if(good.found){
            Match close{};error.clear();
            if(!ScanCloseOnFrame(s,frame,close,error)){FinishRun(s,L"ROI DẤU X FAIL • "+error);return;}
            if(close.found){
                const int cx=close.x+s.closeTpl.width/2,cy=close.y+s.closeTpl.height/2;
                if(!RawClick(s,cx,cy,frame.width,frame.height,error)){FinishRun(s,L"CLICK TÂM X FAIL • "+error);return;}
                Sleep(static_cast<DWORD>(StepDelayMs(s)));
                SetStatus(s.hwnd,L"CLICK "+std::to_wstring(s.slotIndex+1)+L" • ẢNH 1 PASS "+ScoreText(good.score)+L" • X PASS "+ScoreText(close.score)+L" • đợi X biến mất");
                ArmRunPhase(s,RunPhase::WaitCloseGone,40);return;
            }
            if(ProbeTimedOut(s,now)){FinishRun(s,L"TIMEOUT • ẢNH 1 đã PASS nhưng DẤU X chưa xuất hiện tại CLICK "+std::to_wstring(s.slotIndex+1));return;}
            ScheduleNextProbe(s,now);return;
        }

        Match discard{};error.clear();
        if(!ScanDiscardOnFrame(s,frame,discard,error)){FinishRun(s,L"ROI VỨT FAIL • "+error);return;}
        if(discard.found){
            const int dx=discard.x+s.discardTpl.width/2,dy=discard.y+s.discardTpl.height/2;
            if(!RawClick(s,dx,dy,frame.width,frame.height,error)){FinishRun(s,L"CLICK TÂM VỨT FAIL • "+error);return;}
            Sleep(static_cast<DWORD>(StepDelayMs(s)));
            SetStatus(s.hwnd,L"CLICK "+std::to_wstring(s.slotIndex+1)+L" • ẢNH 1 SAI • VỨT PASS "+ScoreText(discard.score)+L" • đợi nút VỨT biến mất");
            ArmRunPhase(s,RunPhase::WaitDiscardGone,40);return;
        }
        if(ProbeTimedOut(s,now)){FinishRun(s,L"TIMEOUT • chưa thấy ẢNH 1 hoặc NÚT VỨT tại CLICK "+std::to_wstring(s.slotIndex+1));return;}
        ScheduleNextProbe(s,now);return;
    }

    if(s.runPhase==RunPhase::WaitDiscardGone){
        Match discard{};error.clear();
        if(!ScanDiscardOnFrame(s,frame,discard,error)){FinishRun(s,L"ROI VỨT FAIL • "+error);return;}
        if(!discard.found){
            if(!ClickAfterDiscard(s,error)){FinishRun(s,L"CLICK SAU VỨT FAIL • "+error);return;}
            Sleep(static_cast<DWORD>(StepDelayMs(s)));
            SetStatus(s.hwnd,L"CLICK "+std::to_wstring(s.slotIndex+1)+L" • VỨT đã biến mất → click SAU VỨT → đợi popup đóng");
            ArmRunPhase(s,RunPhase::WaitPopupGoneAfterConfirm,40);return;
        }
        if(ProbeTimedOut(s,now)){FinishRun(s,L"TIMEOUT • nút VỨT không biến mất tại CLICK "+std::to_wstring(s.slotIndex+1));return;}
        ScheduleNextProbe(s,now);return;
    }

    if(s.runPhase==RunPhase::WaitPopupGoneAfterConfirm){
        Match good{},discard{},close{};std::wstring e1,e2,e3;
        const bool ok1=ScanGoodOnFrame(s,frame,good,e1);const bool ok2=ScanDiscardOnFrame(s,frame,discard,e2);const bool ok3=ScanCloseOnFrame(s,frame,close,e3);
        if(!ok1||!ok2||!ok3){FinishRun(s,L"SCAN trạng thái sau VỨT FAIL • "+(!ok1?e1:(!ok2?e2:e3)));return;}
        if(!good.found&&!discard.found&&!close.found){
            ++s.discardCount;
            if(!ClickCurrentSlot(s,error)){FinishRun(s,L"LẶP CLICK "+std::to_wstring(s.slotIndex+1)+L" FAIL • "+error);return;}
            Sleep(static_cast<DWORD>(StepDelayMs(s)));
            SetStatus(s.hwnd,L"CLICK "+std::to_wstring(s.slotIndex+1)+L" • popup đã đóng • đã vứt "+std::to_wstring(s.discardCount)+L" món → LẶP LẠI CÙNG Ô");
            ArmRunPhase(s,RunPhase::WaitItemReady,40);return;
        }
        if(ProbeTimedOut(s,now)){FinishRun(s,L"TIMEOUT • popup sau VỨT chưa đóng tại CLICK "+std::to_wstring(s.slotIndex+1));return;}
        ScheduleNextProbe(s,now);return;
    }

    if(s.runPhase==RunPhase::WaitCloseGone){
        Match close{};error.clear();
        if(!ScanCloseOnFrame(s,frame,close,error)){FinishRun(s,L"ROI DẤU X FAIL • "+error);return;}
        if(!close.found){
            ++s.slotIndex;s.discardCount=0;
            if(s.slotIndex>=s.config.steps.size()){FinishRun(s,L"LỌC "+std::to_wstring(s.config.steps.size())+L" CLICK HOÀN TẤT • 1 capture/probe • 3 ROI riêng • Sleep sau mỗi thao tác");return;}
            if(!ClickCurrentSlot(s,error)){FinishRun(s,L"CLICK "+std::to_wstring(s.slotIndex+1)+L" FAIL • "+error);return;}
            Sleep(static_cast<DWORD>(StepDelayMs(s)));
            SetStatus(s.hwnd,L"DẤU X đã biến mất → SANG CLICK "+std::to_wstring(s.slotIndex+1));
            ArmRunPhase(s,RunPhase::WaitItemReady,40);return;
        }
        if(ProbeTimedOut(s,now)){FinishRun(s,L"TIMEOUT • DẤU X không biến mất tại CLICK "+std::to_wstring(s.slotIndex+1));return;}
        ScheduleNextProbe(s,now);return;
    }
}

void RunTest(State& s) {
    if(s.runningChain)return;SyncConfig(s);
    if(!s.target.gameWindow||!IsWindow(s.target.gameWindow)){SetStatus(s.hwnd,L"FAIL • cửa sổ game đã mất");return;}
    std::wstring error;if(!ValidateFilter(s,error)){SetStatus(s.hwnd,L"CHƯA THỂ CHẠY • "+error);return;}
    if(!LoadImageWic(s.config.templatePath,s.goodTpl,error)){SetStatus(s.hwnd,L"ẢNH 1 FAIL • "+error);return;}
    if(!LoadImageWic(s.config.discardTemplatePath,s.discardTpl,error)){SetStatus(s.hwnd,L"ẢNH VỨT FAIL • "+error);return;}
    if(!LoadImageWic(s.config.closeTemplatePath,s.closeTpl,error)){SetStatus(s.hwnd,L"ẢNH X FAIL • "+error);return;}

    s.runningChain=true;s.slotIndex=0;s.discardCount=0;s.runPhase=RunPhase::Idle;EnableWindow(GetDlgItem(s.hwnd,IDC_TEST),FALSE);
    if(!ClickCurrentSlot(s,error)){FinishRun(s,L"CLICK 1 FAIL • "+error);return;}
    Sleep(static_cast<DWORD>(StepDelayMs(s)));
    ArmRunPhase(s,RunPhase::WaitItemReady,40);SetTimer(s.hwnd,kRunTimer,25,nullptr);
    SetStatus(s.hwnd,L"V4 SLEEP START • CLICK 1 → Sleep Delay → probe 1 frame • ROI ẢNH1/VỨT/X riêng");
}

void AddRoiEditors(State& s,int y,int xId,int yId,int wId,int hId,int pickId,int previewId,int fullId,
                   int roiX,int roiY,int roiW,int roiH,const wchar_t* label){
    const int left=18;
    Add(s.hwnd,L"STATIC",label,SS_LEFT|SS_CENTERIMAGE,left,y,80,24,0);
    Add(s.hwnd,L"STATIC",L"X",SS_CENTER|SS_CENTERIMAGE,left+82,y,16,24,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(roiX).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,left+98,y,54,24,xId);
    Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,left+154,y,16,24,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(roiY).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,left+170,y,54,24,yId);
    Add(s.hwnd,L"STATIC",L"W",SS_CENTER|SS_CENTERIMAGE,left+226,y,16,24,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(roiW).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,left+242,y,58,24,wId);
    Add(s.hwnd,L"STATIC",L"H",SS_CENTER|SS_CENTERIMAGE,left+302,y,16,24,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(roiH).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,left+318,y,58,24,hId);
    Add(s.hwnd,L"BUTTON",L"CHỌN VÙNG",BS_PUSHBUTTON,left+386,y-2,112,28,pickId);
    Add(s.hwnd,L"BUTTON",L"XEM",BS_PUSHBUTTON,left+504,y-2,68,28,previewId);
    Add(s.hwnd,L"BUTTON",L"FULL",BS_PUSHBUTTON,left+578,y-2,68,28,fullId);
}

void BuildControls(State& s) {
    Add(s.hwnd,L"STATIC",(L"ACC TEST: "+s.target.accountLabel).c_str(),SS_LEFT|SS_CENTERIMAGE,18,9,990,23,0);

    Add(s.hwnd,L"STATIC",L"ẢNH 1 • ĐỒ ĐÚNG (chỉ scan):",SS_LEFT|SS_CENTERIMAGE,18,38,205,25,0);
    Add(s.hwnd,L"EDIT",s.config.templatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,225,38,555,25,IDC_TEMPLATE);
    Add(s.hwnd,L"BUTTON",L"CHỌN ẢNH 1",BS_PUSHBUTTON,790,37,110,27,IDC_PICK);
    AddRoiEditors(s,69,IDC_X,IDC_Y,IDC_W,IDC_H,IDC_PICK_REGION,IDC_PREVIEW_REGION,IDC_FULL_REGION,s.config.x,s.config.y,s.config.w,s.config.h,L"ROI ẢNH 1");
    Add(s.hwnd,L"STATIC",L"Ngưỡng %",SS_LEFT|SS_CENTERIMAGE,835,69,65,24,0);Add(s.hwnd,L"EDIT",std::to_wstring(s.config.thresholdPercent).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,902,69,58,24,IDC_THRESHOLD);

    Add(s.hwnd,L"STATIC",L"NÚT VỨT • scan rồi click tâm:",SS_LEFT|SS_CENTERIMAGE,18,105,205,25,0);
    Add(s.hwnd,L"EDIT",s.config.discardTemplatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,225,105,555,25,IDC_TEMPLATE_DISCARD);
    Add(s.hwnd,L"BUTTON",L"CHỌN VỨT",BS_PUSHBUTTON,790,104,110,27,IDC_PICK_DISCARD);
    AddRoiEditors(s,136,IDC_DISCARD_X,IDC_DISCARD_Y,IDC_DISCARD_W,IDC_DISCARD_H,IDC_DISCARD_REGION,IDC_DISCARD_PREVIEW,IDC_DISCARD_FULL,s.config.discardRoi.x,s.config.discardRoi.y,s.config.discardRoi.w,s.config.discardRoi.h,L"ROI VỨT");

    Add(s.hwnd,L"STATIC",L"DẤU X • scan rồi click tâm:",SS_LEFT|SS_CENTERIMAGE,18,172,205,25,0);
    Add(s.hwnd,L"EDIT",s.config.closeTemplatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,225,172,555,25,IDC_TEMPLATE_CLOSE);
    Add(s.hwnd,L"BUTTON",L"CHỌN DẤU X",BS_PUSHBUTTON,790,171,110,27,IDC_PICK_CLOSE);
    AddRoiEditors(s,203,IDC_CLOSE_X,IDC_CLOSE_Y,IDC_CLOSE_W,IDC_CLOSE_H,IDC_CLOSE_REGION,IDC_CLOSE_PREVIEW,IDC_CLOSE_FULL,s.config.closeRoi.x,s.config.closeRoi.y,s.config.closeRoi.w,s.config.closeRoi.h,L"ROI DẤU X");
    Add(s.hwnd,L"STATIC",L"V4: 3 ROI khác nhau nhưng mọi scan trong cùng một probe dùng CHUNG 1 frame PrintWindow.",SS_LEFT|SS_CENTERIMAGE,675,203,330,24,0);

    Add(s.hwnd,L"BUTTON",L"DANH SÁCH CLICK Ô ĐỒ • mặc định 20 • +THÊM / -XÓA không giới hạn cứng • Delay = Sleep sau mỗi thao tác",BS_GROUPBOX,18,238,987,330,0);
    s.stepList=Add(s.hwnd,WC_LISTVIEWW,L"",LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS|WS_BORDER,32,263,959,205,IDC_STEP_LIST);
    ListView_SetExtendedListViewStyle(s.stepList,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);
    AddColumn(s.stepList,0,45,L"#");AddColumn(s.stepList,1,80,L"X");AddColumn(s.stepList,2,80,L"Y");AddColumn(s.stepList,3,110,L"Base size");AddColumn(s.stepList,4,100,L"Delay ms");AddColumn(s.stepList,5,515,L"Logic");

    Add(s.hwnd,L"STATIC",L"X:",SS_LEFT|SS_CENTERIMAGE,32,480,20,27,0);Add(s.hwnd,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,54,480,64,27,IDC_STEP_X);
    Add(s.hwnd,L"STATIC",L"Y:",SS_LEFT|SS_CENTERIMAGE,126,480,20,27,0);Add(s.hwnd,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,148,480,64,27,IDC_STEP_Y);
    Add(s.hwnd,L"STATIC",L"Delay:",SS_LEFT|SS_CENTERIMAGE,222,480,55,27,0);Add(s.hwnd,L"EDIT",L"500",WS_BORDER|ES_NUMBER|ES_CENTER,279,480,70,27,IDC_STEP_DELAY);
    Add(s.hwnd,L"BUTTON",L"LƯU CLICK",BS_PUSHBUTTON,360,480,105,27,IDC_STEP_SAVE);Add(s.hwnd,L"BUTTON",L"LẤY TỌA F8",BS_PUSHBUTTON,474,480,130,27,IDC_STEP_CAPTURE);Add(s.hwnd,L"BUTTON",L"TEST CLICK ẨN",BS_PUSHBUTTON,613,480,130,27,IDC_STEP_TEST);
    Add(s.hwnd,L"STATIC",L"Mặc định 500ms; Sleep sau MỖI click để game không nhận thao tác quá nhanh.",SS_LEFT|SS_CENTERIMAGE,752,480,235,27,0);

    Add(s.hwnd,L"STATIC",L"CLICK SAU VỨT:",SS_LEFT|SS_CENTERIMAGE,32,522,112,27,0);Add(s.hwnd,L"STATIC",L"X",SS_CENTER|SS_CENTERIMAGE,146,522,16,27,0);
    Add(s.hwnd,L"EDIT",s.config.afterDiscard.valid?std::to_wstring(s.config.afterDiscard.x).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,164,522,65,27,IDC_AFTER_X);
    Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,235,522,16,27,0);Add(s.hwnd,L"EDIT",s.config.afterDiscard.valid?std::to_wstring(s.config.afterDiscard.y).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,253,522,65,27,IDC_AFTER_Y);
    Add(s.hwnd,L"BUTTON",L"LẤY F8 SAU VỨT",BS_PUSHBUTTON,329,522,155,27,IDC_AFTER_CAPTURE);Add(s.hwnd,L"BUTTON",L"TEST SAU VỨT",BS_PUSHBUTTON,493,522,135,27,IDC_AFTER_TEST);
    Add(s.hwnd,L"STATIC",L"Sau VỨT: tool chờ ảnh VỨT biến mất rồi mới click tọa này.",SS_LEFT|SS_CENTERIMAGE,640,522,345,27,0);

    Add(s.hwnd,L"STATIC",L"STATE: CLICK N → probe 1 frame → GOOD? {scan X cùng frame} : {scan VỨT cùng frame} → chờ dấu hiệu UI biến đổi → bước tiếp.",SS_LEFT|SS_CENTERIMAGE,32,548,955,20,0);
    Add(s.hwnd,L"BUTTON",L"CHẠY TEST LỌC CLICK ẨN • V4 SLEEP",BS_DEFPUSHBUTTON,18,582,987,39,IDC_TEST);
    Add(s.hwnd,L"STATIC",L"Sẵn sàng. ESC = dừng khẩn. V4 SLEEP: mỗi RAW click đều chờ Delay ms trước bước tiếp theo.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,632,987,90,IDC_STATUS);
    Add(s.hwnd,L"BUTTON",L"ĐÓNG",BS_PUSHBUTTON,885,735,120,30,IDCANCEL);
    RefreshStepList(s);
}

LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    State* s=reinterpret_cast<State*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE){auto* cs=reinterpret_cast<CREATESTRUCTW*>(lp);s=static_cast<State*>(cs->lpCreateParams);SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));if(s)s->hwnd=hwnd;}
    if(!s)return DefWindowProcW(hwnd,msg,wp,lp);
    switch(msg){
        case WM_CREATE:BuildControls(*s);return 0;
        case WM_TIMER:if(wp==kF8PollTimer){PollF8(*s);return 0;}if(wp==kRunTimer){ProcessRunTick(*s);return 0;}break;
        case WM_NOTIFY:{
            auto* hdr=reinterpret_cast<NMHDR*>(lp);
            if(hdr&&hdr->idFrom==IDC_STEP_LIST&&hdr->code==LVN_ITEMCHANGED){
                const auto* n=reinterpret_cast<NMLISTVIEW*>(hdr);
                if((n->uChanged&LVIF_STATE)!=0&&(n->uNewState&LVIS_SELECTED)!=0)LoadStepEditor(*s,n->iItem);
            }
            return 0;
        }
        case WM_COMMAND:
            if(s->runningChain && LOWORD(wp)!=IDCANCEL){SetStatus(s->hwnd,L"Đang chạy FILTER v4 • ESC để dừng trước khi sửa cấu hình");return 0;}
            switch(LOWORD(wp)){
                case IDC_PICK:PickTemplate(*s);return 0;
                case IDC_PICK_DISCARD:PickDiscardTemplate(*s);return 0;
                case IDC_PICK_CLOSE:PickCloseTemplate(*s);return 0;
                case IDC_PICK_REGION:SelectRegion(*s);return 0;
                case IDC_PREVIEW_REGION:PreviewRegion(*s);return 0;
                case IDC_FULL_REGION:ResetFullRegion(*s);return 0;
                case IDC_DISCARD_REGION:SelectDiscardRegion(*s);return 0;
                case IDC_DISCARD_PREVIEW:PreviewDiscardRegion(*s);return 0;
                case IDC_DISCARD_FULL:ResetDiscardRegion(*s);return 0;
                case IDC_CLOSE_REGION:SelectCloseRegion(*s);return 0;
                case IDC_CLOSE_PREVIEW:PreviewCloseRegion(*s);return 0;
                case IDC_CLOSE_FULL:ResetCloseRegion(*s);return 0;
                case IDC_STEP_ADD:AddStep(*s);return 0;
                case IDC_STEP_DELETE:DeleteStep(*s);return 0;
                case IDC_STEP_UP:MoveStep(*s,-1);return 0;
                case IDC_STEP_DOWN:MoveStep(*s,1);return 0;
                case IDC_STEP_SAVE:SaveSelectedStep(*s);return 0;
                case IDC_STEP_CAPTURE:ArmF8(*s);return 0;
                case IDC_STEP_TEST:TestSelectedStep(*s);return 0;
                case IDC_AFTER_CAPTURE:ArmAfterDiscardF8(*s);return 0;
                case IDC_AFTER_TEST:TestAfterDiscard(*s);return 0;
                case IDC_TEST:RunTest(*s);return 0;
                case IDCANCEL:if(!s->runningChain){SyncConfig(*s);DestroyWindow(hwnd);}return 0;
            }
            break;
        case WM_CLOSE:if(!s->runningChain){SyncConfig(*s);DestroyWindow(hwnd);}return 0;
        case WM_NCDESTROY:KillTimer(hwnd,kF8PollTimer);KillTimer(hwnd,kRunTimer);s->hwnd=nullptr;s->stepList=nullptr;return DefWindowProcW(hwnd,msg,wp,lp);
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}

} // namespace

void RunDialog(const Target& target){
    if(!target.owner||!target.gameWindow)return;
    State state{};state.target=target;state.config=g_lastConfig;RebaseForCurrentClient(state.config,target.gameWindow);if(state.config.steps.empty())state.config.steps.resize(kDefaultInitialSteps);
    WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.lpfnWndProc=WndProc;wc.hInstance=GetModuleHandleW(nullptr);wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);wc.lpszClassName=kClassName;
    if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return;
    EnableWindow(target.owner,FALSE);
    HWND hwnd=CreateWindowExW(WS_EX_TOOLWINDOW,kClassName,L"TEST LỌC ĐỒ ẢNH ẨN • V4 SLEEP • CLICK ĐỘNG",
                              WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
                              CW_USEDEFAULT,CW_USEDEFAULT,1000,755,target.owner,nullptr,GetModuleHandleW(nullptr),&state);
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
