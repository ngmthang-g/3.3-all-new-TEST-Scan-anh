#pragma once

#include <windows.h>
#include <string>

namespace image_scan_test {

using HiddenClickFn = bool (*)(void* context,
                               int clientX, int clientY,
                               int clientWidth, int clientHeight,
                               std::wstring& detail);

struct Target {
    HWND owner = nullptr;
    HWND gameWindow = nullptr;
    std::wstring accountLabel;
    HiddenClickFn hiddenClick = nullptr;
    void* context = nullptr;
};

// Modal proof-of-concept dialog. It never foregrounds the game window.
// Capture is attempted against the target HWND, template matching is exact-scale,
// and a PASS invokes the supplied hidden-click callback at the match center.
void RunDialog(const Target& target);

} // namespace image_scan_test
