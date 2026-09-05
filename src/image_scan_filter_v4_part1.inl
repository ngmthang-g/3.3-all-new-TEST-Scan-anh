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
constexpr int IDC_DELAY_DISCARD = 1070;
constexpr int IDC_DELAY_AFTER_DISCARD = 1071;
constexpr int IDC_DELAY_CLOSE = 1072;

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

    // Delay cố định sau từng loại click đặc biệt.
    int discardClickDelayMs = 500;
    int afterDiscardClickDelayMs = 500;
    int closeClickDelayMs = 500;
};

Config g_lastConfig{};

enum class RunPhase {
    Idle,
    WaitItemReady,
    WaitDiscardGone,
    WaitPopupGoneAfterConfirm,
    WaitCloseGone,
};

enum class DelayKind {
    Slot,
    Discard,
    AfterDiscard,
    Close,
};

struct State {
    Target target{};
    Config config{};
    HWND hwnd = nullptr;
    HWND stepList = nullptr;
    int captureRow = -1;
    bool f8WasDown = false;
    bool runningChain = false;
    DelayKind delayKind = DelayKind::Slot;

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
