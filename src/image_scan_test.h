#pragma once

#include <windows.h>
#include <cstddef>
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

// Modal FILTER V4 settings/test dialog. It never foregrounds the game window.
void RunDialog(const Target& target);

enum class AutoFilterState {
    Disabled,
    Idle,
    Busy,
    WaitingEmpty,
    CompletedUntilFull,
    FullYieldReady,
    Error,
};

struct AutoFilterResult {
    AutoFilterState state = AutoFilterState::Idle;
    bool ownsInput = false;
    bool fullYieldReady = true;
    std::size_t slotNumber = 0; // 1-based, 0 when no slot is active.
    std::wstring status;
};

// Scan settings are auto-loaded/saved in LOCALAPPDATA; export/import remains manual backup.
void EnsurePersistentConfigLoaded();
void SavePersistentConfig();

// CON1..CON12 enable switches live in the FILTER V4 settings/config.
bool IsChildAutoFilterEnabled(int childSlot);

// Background FILTER V4 tick. Call only after the CON has reached its train spot and
// initial AutoFight startup has succeeded. freeBagSpace==0 is passed as bagFull.
AutoFilterResult TickAutoFilter(const Target& target, int childSlot, bool bagFull);

// FULL is latched once seen so discarding the final bad item cannot cancel travel.
bool FullBagTravelLatched(HWND gameWindow, int childSlot);

// FULL-bag coordinator gate: false only while FILTER V4 is finishing the current item
// and/or executing the single configured CloseBag click.
bool FullBagYieldReady(HWND gameWindow, int childSlot);

// Death owns priority. NotifyDeath aborts scan immediately without closing the bag;
// after the existing revive callback succeeds, NotifyReviveClicked performs the one
// configured CloseBag click and resets scan so the next train arrival starts at slot 1.
void NotifyDeath(HWND gameWindow, int childSlot);
void NotifyReviveClicked(const Target& target, int childSlot);

// START/STOP session boundaries.
void ResetAutoFilter(HWND gameWindow, int childSlot);
void StopAutoFilter(HWND gameWindow, int childSlot);

} // namespace image_scan_test
