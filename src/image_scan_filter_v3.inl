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

constexpr wchar_t kClassName[] = L"ThanLongImageFilterTestPocV3";
constexpr wchar_t kRegionClassName[] = L"ThanLongImageScanRegionPickerV2";
constexpr wchar_t kPreviewClassName[] = L"ThanLongImageScanPreviewV2";
constexpr UINT kPwRenderFullContent = 0x00000002u;
constexpr UINT_PTR kF8PollTimer = 91;
constexpr std::size_t kMaxClickSteps = 20;

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
    // Ảnh 1 = dấu hiệu món đồ ĐÚNG. Chỉ scan, KHÔNG click ảnh 1.
    std::wstring templatePath;
    // Hai ảnh dùng chung cho cả 20 ô và được click đúng tâm match.
    std::wstring discardTemplatePath;
    std::wstring closeTemplatePath;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int roiBaseW = 0;
    int roiBaseH = 0;
    int thresholdPercent = 90;
    std::vector<ClickStep> steps;
    // Click ngay sau khi click tâm NÚT VỨT (ví dụ nút xác nhận), tự gán bằng F8.
    ClickStep afterDiscard;
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
    step.delayMs = std::clamp(ReadInt(s.hwnd, IDC_STEP_DELAY, step.delayMs), 0, 60000);
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
    s.config.thresholdPercent = std::clamp(ReadInt(s.hwnd, IDC_THRESHOLD, 90), 1, 100);
    const int afterX = ReadInt(s.hwnd, IDC_AFTER_X, s.config.afterDiscard.x);
    const int afterY = ReadInt(s.hwnd, IDC_AFTER_Y, s.config.afterDiscard.y);
    int afterW=0,afterH=0;
    if(afterX>=0&&afterY>=0&&CurrentClientSize(s.target.gameWindow,afterW,afterH)&&afterX<afterW&&afterY<afterH){
        s.config.afterDiscard.x=afterX;s.config.afterDiscard.y=afterY;s.config.afterDiscard.baseW=afterW;s.config.afterDiscard.baseH=afterH;s.config.afterDiscard.valid=true;s.config.afterDiscard.repeat=1;
    }
    int cw = 0, ch = 0;
    if (CurrentClientSize(s.target.gameWindow, cw, ch)) { s.config.roiBaseW = cw; s.config.roiBaseH = ch; }
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
    if(s.config.steps.size()>=kMaxClickSteps){SetStatus(s.hwnd,L"BỘ LỌC cố định 20 CLICK");return;}
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
    const int row=SelectedStep(s);if(row<0||row>=static_cast<int>(s.config.steps.size())){SetStatus(s.hwnd,L"F8: chọn một ô 1-20 trước");return;}
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
    if(s.config.steps.size()!=kMaxClickSteps){error=L"phải có đúng 20 tọa click";return false;}
    for(std::size_t i=0;i<s.config.steps.size();++i){
        const ClickStep& step=s.config.steps[i];
        if(!step.valid||step.baseW<=0||step.baseH<=0){error=L"CLICK "+std::to_wstring(i+1)+L" chưa gán tọa F8";return false;}
        if(step.delayMs<0||step.delayMs>60000){error=L"Delay CLICK "+std::to_wstring(i+1)+L" không hợp lệ";return false;}
    }
    if(!s.config.afterDiscard.valid||s.config.afterDiscard.baseW<=0||s.config.afterDiscard.baseH<=0){
        error=L"chưa gán CLICK SAU VỨT bằng F8";return false;
    }
    return true;
}

void ResolveScanRoi(const Config& c,int currentW,int currentH,int& rx,int& ry,int& rw,int& rh){
    rx=c.x;ry=c.y;rw=c.w;rh=c.h;
    if(c.roiBaseW>0&&c.roiBaseH>0&&(c.roiBaseW!=currentW||c.roiBaseH!=currentH)){
        rx=ScaleCoord(rx,c.roiBaseW,currentW);ry=ScaleCoord(ry,c.roiBaseH,currentH);
        if(rw>0)rw=std::max(1,ScaleCoord(rw,c.roiBaseW,currentW));
        if(rh>0)rh=std::max(1,ScaleCoord(rh,c.roiBaseH,currentH));
    }
}

std::wstring ScoreText(double score){
    if(score<0.0)return L"N/A";wchar_t b[40]{};swprintf_s(b,L"%.2f%%",score*100.0);return b;
}

bool ScanLoaded(State& s,const Image& tpl,Match& match,std::wstring& backend,std::wstring& error,int& frameW,int& frameH){
    Image frame{};if(!CaptureClient(s.target.gameWindow,frame,backend,error))return false;
    frameW=frame.width;frameH=frame.height;int rx=0,ry=0,rw=0,rh=0;ResolveScanRoi(s.config,frame.width,frame.height,rx,ry,rw,rh);
    match=FindTemplate(frame,tpl,rx,ry,rw,rh,static_cast<double>(s.config.thresholdPercent)/100.0,error);
    return match.score>=0.0;
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

void FinishRun(State& s){s.runningChain=false;EnableWindow(GetDlgItem(s.hwnd,IDC_TEST),TRUE);}

void RunTest(State& s) {
    if(s.runningChain)return;SyncConfig(s);
    if(!s.target.gameWindow||!IsWindow(s.target.gameWindow)){SetStatus(s.hwnd,L"FAIL • cửa sổ game đã mất");return;}
    std::wstring error;if(!ValidateFilter(s,error)){SetStatus(s.hwnd,L"CHƯA THỂ CHẠY • "+error);return;}

    Image goodTpl{},discardTpl{},closeTpl{};
    if(!LoadImageWic(s.config.templatePath,goodTpl,error)){SetStatus(s.hwnd,L"ẢNH 1 FAIL • "+error);return;}
    if(!LoadImageWic(s.config.discardTemplatePath,discardTpl,error)){SetStatus(s.hwnd,L"ẢNH VỨT FAIL • "+error);return;}
    if(!LoadImageWic(s.config.closeTemplatePath,closeTpl,error)){SetStatus(s.hwnd,L"ẢNH X FAIL • "+error);return;}

    s.runningChain=true;EnableWindow(GetDlgItem(s.hwnd,IDC_TEST),FALSE);
    SetStatus(s.hwnd,L"BẮT ĐẦU LỌC 20 Ô • ESC để dừng khẩn • không MapID/combat/AUTO/state machine");

    for(std::size_t i=0;i<kMaxClickSteps;++i){
        std::size_t discardCount=0;
        for(;;){
            if((GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0){SetStatus(s.hwnd,L"ĐÃ DỪNG BẰNG ESC tại CLICK "+std::to_wstring(i+1));FinishRun(s);return;}
            int cw=0,ch=0,x=0,y=0;
            if(!CurrentClientSize(s.target.gameWindow,cw,ch)||!ResolveStepPoint(s.config.steps[i],cw,ch,x,y)){
                SetStatus(s.hwnd,L"FAIL • không resolve được CLICK "+std::to_wstring(i+1));FinishRun(s);return;
            }
            error.clear();
            SetStatus(s.hwnd,L"CLICK "+std::to_wstring(i+1)+L" • mở ô @ "+std::to_wstring(x)+L","+std::to_wstring(y)+L" • lượt vứt tại ô="+std::to_wstring(discardCount));
            if(!RawClick(s,x,y,cw,ch,error)){SetStatus(s.hwnd,L"RAW CLICK "+std::to_wstring(i+1)+L" FAIL • "+error);FinishRun(s);return;}
            const DWORD delay=static_cast<DWORD>(std::max(0,s.config.steps[i].delayMs));if(delay)Sleep(delay);

            Match good{};std::wstring backend;int fw=0,fh=0;error.clear();
            if(!ScanLoaded(s,goodTpl,good,backend,error,fw,fh)){
                SetStatus(s.hwnd,L"SCAN ẢNH 1 FAIL tại CLICK "+std::to_wstring(i+1)+L" • "+error);FinishRun(s);return;
            }

            if(good.found){
                // ĐÚNG ĐỒ: Ảnh 1 chỉ là điều kiện. Không click Ảnh 1. Tìm dấu X rồi click tâm X.
                Match close{};std::wstring backendX;int xw=0,xh=0;error.clear();
                if(!ScanLoaded(s,closeTpl,close,backendX,error,xw,xh)){
                    SetStatus(s.hwnd,L"ẢNH 1 PASS nhưng scan DẤU X FAIL tại CLICK "+std::to_wstring(i+1)+L" • "+error);FinishRun(s);return;
                }
                if(!close.found){
                    SetStatus(s.hwnd,L"ẢNH 1 PASS "+ScoreText(good.score)+L" • DẤU X NOT FOUND "+ScoreText(close.score)+L" tại CLICK "+std::to_wstring(i+1)+L" → DỪNG");FinishRun(s);return;
                }
                const int cx=close.x+closeTpl.width/2,cy=close.y+closeTpl.height/2;
                error.clear();if(!RawClick(s,cx,cy,xw,xh,error)){
                    SetStatus(s.hwnd,L"CLICK TÂM DẤU X FAIL tại CLICK "+std::to_wstring(i+1)+L" • "+error);FinishRun(s);return;
                }
                SetStatus(s.hwnd,L"CLICK "+std::to_wstring(i+1)+L" • ẢNH 1 PASS "+ScoreText(good.score)+L" → DẤU X PASS "+ScoreText(close.score)+L" → click tâm X → SANG CLICK "+std::to_wstring(i+2));
                if(delay)Sleep(delay);break;
            }

            // SAI ĐỒ: tìm NÚT VỨT, click tâm VỨT, click tọa sau VỨT, rồi quay lại đúng CLICK i.
            Match discard{};std::wstring backendD;int dw=0,dh=0;error.clear();
            if(!ScanLoaded(s,discardTpl,discard,backendD,error,dw,dh)){
                SetStatus(s.hwnd,L"ẢNH 1 NOT FOUND và scan NÚT VỨT FAIL tại CLICK "+std::to_wstring(i+1)+L" • "+error);FinishRun(s);return;
            }
            if(!discard.found){
                SetStatus(s.hwnd,L"CLICK "+std::to_wstring(i+1)+L" • ẢNH 1 NOT FOUND "+ScoreText(good.score)+L" • NÚT VỨT NOT FOUND "+ScoreText(discard.score)+L" → DỪNG");FinishRun(s);return;
            }
            const int dx=discard.x+discardTpl.width/2,dy=discard.y+discardTpl.height/2;
            error.clear();if(!RawClick(s,dx,dy,dw,dh,error)){
                SetStatus(s.hwnd,L"CLICK TÂM NÚT VỨT FAIL tại CLICK "+std::to_wstring(i+1)+L" • "+error);FinishRun(s);return;
            }
            if(delay)Sleep(delay);
            int ax=0,ay=0;int acw=0,ach=0;
            if(!CurrentClientSize(s.target.gameWindow,acw,ach)||!ResolveStepPoint(s.config.afterDiscard,acw,ach,ax,ay)){
                SetStatus(s.hwnd,L"CLICK SAU VỨT resolve FAIL");FinishRun(s);return;
            }
            error.clear();if(!RawClick(s,ax,ay,acw,ach,error)){
                SetStatus(s.hwnd,L"CLICK SAU VỨT FAIL tại CLICK "+std::to_wstring(i+1)+L" • "+error);FinishRun(s);return;
            }
            ++discardCount;
            SetStatus(s.hwnd,L"CLICK "+std::to_wstring(i+1)+L" • ẢNH 1 SAI → VỨT PASS "+ScoreText(discard.score)+L" → click sau VỨT → LẶP LẠI CLICK "+std::to_wstring(i+1));
            if(delay)Sleep(delay);
        }
    }
    FinishRun(s);
    SetStatus(s.hwnd,L"LỌC 20 CLICK HOÀN TẤT • chỉ tăng số thứ tự sau ẢNH 1 PASS + click tâm DẤU X PASS");
}

void BuildControls(State& s) {
    Add(s.hwnd,L"STATIC",(L"ACC TEST: "+s.target.accountLabel).c_str(),SS_LEFT|SS_CENTERIMAGE,18,10,930,24,0);

    Add(s.hwnd,L"STATIC",L"ẢNH 1 • ĐỒ ĐÚNG (chỉ scan, KHÔNG click):",SS_LEFT|SS_CENTERIMAGE,18,39,230,25,0);
    Add(s.hwnd,L"EDIT",s.config.templatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,250,39,585,25,IDC_TEMPLATE);
    Add(s.hwnd,L"BUTTON",L"CHỌN ẢNH 1",BS_PUSHBUTTON,842,38,116,27,IDC_PICK);
    Add(s.hwnd,L"STATIC",L"NÚT VỨT • scan rồi click đúng tâm:",SS_LEFT|SS_CENTERIMAGE,18,69,230,25,0);
    Add(s.hwnd,L"EDIT",s.config.discardTemplatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,250,69,585,25,IDC_TEMPLATE_DISCARD);
    Add(s.hwnd,L"BUTTON",L"CHỌN VỨT",BS_PUSHBUTTON,842,68,116,27,IDC_PICK_DISCARD);
    Add(s.hwnd,L"STATIC",L"DẤU X • scan rồi click đúng tâm:",SS_LEFT|SS_CENTERIMAGE,18,99,230,25,0);
    Add(s.hwnd,L"EDIT",s.config.closeTemplatePath.c_str(),WS_BORDER|ES_AUTOHSCROLL,250,99,585,25,IDC_TEMPLATE_CLOSE);
    Add(s.hwnd,L"BUTTON",L"CHỌN DẤU X",BS_PUSHBUTTON,842,98,116,27,IDC_PICK_CLOSE);

    Add(s.hwnd,L"STATIC",L"ROI chung: X",SS_LEFT|SS_CENTERIMAGE,18,132,70,25,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.x).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,88,132,58,25,IDC_X);
    Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,150,132,18,25,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.y).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,170,132,58,25,IDC_Y);
    Add(s.hwnd,L"STATIC",L"W",SS_CENTER|SS_CENTERIMAGE,232,132,18,25,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.w).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,252,132,62,25,IDC_W);
    Add(s.hwnd,L"STATIC",L"H",SS_CENTER|SS_CENTERIMAGE,318,132,18,25,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.h).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,338,132,62,25,IDC_H);
    Add(s.hwnd,L"STATIC",L"Ngưỡng %",SS_LEFT|SS_CENTERIMAGE,410,132,62,25,0);
    Add(s.hwnd,L"EDIT",std::to_wstring(s.config.thresholdPercent).c_str(),WS_BORDER|ES_NUMBER|ES_CENTER,474,132,52,25,IDC_THRESHOLD);
    Add(s.hwnd,L"BUTTON",L"CHỌN VÙNG BẰNG CHUỘT",BS_PUSHBUTTON,538,129,185,30,IDC_PICK_REGION);
    Add(s.hwnd,L"BUTTON",L"XEM VÙNG",BS_PUSHBUTTON,730,129,104,30,IDC_PREVIEW_REGION);
    Add(s.hwnd,L"BUTTON",L"FULL CLIENT",BS_PUSHBUTTON,842,129,116,30,IDC_FULL_REGION);

    Add(s.hwnd,L"BUTTON",L"20 TỌA CLICK Ô ĐỒ • mỗi CLICK N: click ô → scan Ảnh 1 → giữ hoặc vứt → chỉ PASS mới sang N+1",BS_GROUPBOX,18,168,940,350,0);
    s.stepList=Add(s.hwnd,WC_LISTVIEWW,L"",LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS|WS_BORDER,32,193,912,225,IDC_STEP_LIST);
    ListView_SetExtendedListViewStyle(s.stepList,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);
    AddColumn(s.stepList,0,45,L"#");AddColumn(s.stepList,1,80,L"X");AddColumn(s.stepList,2,80,L"Y");
    AddColumn(s.stepList,3,110,L"Base size");AddColumn(s.stepList,4,90,L"Delay ms");AddColumn(s.stepList,5,485,L"Logic");
    Add(s.hwnd,L"STATIC",L"X:",SS_LEFT|SS_CENTERIMAGE,32,428,20,27,0);
    Add(s.hwnd,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,54,428,64,27,IDC_STEP_X);
    Add(s.hwnd,L"STATIC",L"Y:",SS_LEFT|SS_CENTERIMAGE,126,428,20,27,0);
    Add(s.hwnd,L"EDIT",L"",WS_BORDER|ES_NUMBER|ES_CENTER,148,428,64,27,IDC_STEP_Y);
    Add(s.hwnd,L"STATIC",L"Delay:",SS_LEFT|SS_CENTERIMAGE,222,428,45,27,0);
    Add(s.hwnd,L"EDIT",L"500",WS_BORDER|ES_NUMBER|ES_CENTER,269,428,64,27,IDC_STEP_DELAY);
    Add(s.hwnd,L"BUTTON",L"LƯU CLICK",BS_PUSHBUTTON,344,428,105,27,IDC_STEP_SAVE);
    Add(s.hwnd,L"BUTTON",L"LẤY TỌA F8",BS_PUSHBUTTON,458,428,130,27,IDC_STEP_CAPTURE);
    Add(s.hwnd,L"BUTTON",L"TEST CLICK ẨN",BS_PUSHBUTTON,597,428,130,27,IDC_STEP_TEST);
    Add(s.hwnd,L"STATIC",L"Chọn CLICK 1-20 → đưa chuột vào ô đồ → F8.",SS_LEFT|SS_CENTERIMAGE,738,428,205,27,0);

    Add(s.hwnd,L"STATIC",L"CLICK SAU VỨT (tọa chung):",SS_LEFT|SS_CENTERIMAGE,32,467,160,27,0);
    Add(s.hwnd,L"STATIC",L"X",SS_CENTER|SS_CENTERIMAGE,194,467,16,27,0);
    Add(s.hwnd,L"EDIT",s.config.afterDiscard.valid?std::to_wstring(s.config.afterDiscard.x).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,212,467,65,27,IDC_AFTER_X);
    Add(s.hwnd,L"STATIC",L"Y",SS_CENTER|SS_CENTERIMAGE,283,467,16,27,0);
    Add(s.hwnd,L"EDIT",s.config.afterDiscard.valid?std::to_wstring(s.config.afterDiscard.y).c_str():L"",WS_BORDER|ES_NUMBER|ES_CENTER,301,467,65,27,IDC_AFTER_Y);
    Add(s.hwnd,L"BUTTON",L"LẤY F8 SAU VỨT",BS_PUSHBUTTON,377,467,155,27,IDC_AFTER_CAPTURE);
    Add(s.hwnd,L"BUTTON",L"TEST SAU VỨT",BS_PUSHBUTTON,541,467,135,27,IDC_AFTER_TEST);
    Add(s.hwnd,L"STATIC",L"Nút VỨT và DẤU X không dùng tọa cố định: luôn click tâm ảnh match.",SS_LEFT|SS_CENTERIMAGE,688,467,255,27,0);

    Add(s.hwnd,L"STATIC",L"SAI: CLICK N → Ảnh1 không thấy → tìm VỨT → click tâm VỨT → click SAU VỨT → quay lại CLICK N.  ĐÚNG: thấy Ảnh1 → tìm X → click tâm X → N+1.",SS_LEFT|SS_CENTERIMAGE,32,493,912,24,0);
    Add(s.hwnd,L"BUTTON",L"CHẠY TEST LỌC 20 CLICK ẨN",BS_DEFPUSHBUTTON,18,530,940,38,IDC_TEST);
    Add(s.hwnd,L"STATIC",L"Sẵn sàng. ESC = dừng khẩn khi chuỗi đang chạy.",SS_LEFT|SS_CENTERIMAGE|WS_BORDER,18,578,940,84,IDC_STATUS);
    Add(s.hwnd,L"BUTTON",L"ĐÓNG",BS_PUSHBUTTON,838,673,120,30,IDCANCEL);
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
                case IDC_PICK_DISCARD:PickDiscardTemplate(*s);return 0;
                case IDC_PICK_CLOSE:PickCloseTemplate(*s);return 0;
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
                case IDC_AFTER_CAPTURE:ArmAfterDiscardF8(*s);return 0;
                case IDC_AFTER_TEST:TestAfterDiscard(*s);return 0;
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
    State state{};state.target=target;state.config=g_lastConfig;RebaseForCurrentClient(state.config,target.gameWindow);if(state.config.steps.size()<kMaxClickSteps)state.config.steps.resize(kMaxClickSteps);if(state.config.steps.size()>kMaxClickSteps)state.config.steps.resize(kMaxClickSteps);
    WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.lpfnWndProc=WndProc;wc.hInstance=GetModuleHandleW(nullptr);wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);wc.lpszClassName=kClassName;
    if(!RegisterClassExW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return;
    EnableWindow(target.owner,FALSE);
    HWND hwnd=CreateWindowExW(WS_EX_TOOLWINDOW,kClassName,L"TEST LỌC ĐỒ ẢNH ẨN • 20 CLICK • v3",
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
