#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace fixed_slot_sell_logic {

constexpr int kDefaultFullBatchClickCount = 90;
constexpr int kDefaultClickDelayMs = 600;
constexpr int kCoordinateScale = 1000000;

// Parser/migration compatibility only. Runtime no longer has a first-sale or
// adaptive-sale mode; old config readers may still use this historical name.
constexpr int kInitialClickCount = kDefaultFullBatchClickCount;

// One-time compatibility bridge for old multi-row sell configs. The runtime no
// longer executes a sell sequence. If an old profile still has >=5 rows, only
// the former item-cell row (#5) is retained as the single MAIN idle click.
inline int LegacyIdleClickSourceIndex(std::size_t rowCount) {
    if (rowCount == 0) return -1;
    return rowCount >= 5 ? 4 : static_cast<int>(rowCount - 1);
}

// Repeat is used for one complete capacity-recovery batch whenever a waiting
// or active CON exists and MAIN has <9 free slots. Idle mode itself is infinite
// and performs one click per delay while no CON has arrived at TỌA GD.
inline int ClampFullBatchClickCount(int configuredCount) {
    const int value = configuredCount > 0 ? configuredCount : kDefaultFullBatchClickCount;
    return std::clamp(value, 1, 999);
}

inline int ClampClickDelayMs(int configuredDelayMs) {
    const int value = configuredDelayMs > 0 ? configuredDelayMs : kDefaultClickDelayMs;
    return std::clamp(value, 50, 60000);
}

inline int NormalizeClientCoordinate(int value, int extent) {
    if (extent <= 0) return -1;
    const int bounded = std::clamp(value, 0, extent - 1);
    return static_cast<int>((static_cast<std::int64_t>(bounded) * kCoordinateScale) / extent);
}

} // namespace fixed_slot_sell_logic
