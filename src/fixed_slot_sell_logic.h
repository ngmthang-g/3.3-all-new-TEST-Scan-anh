#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace fixed_slot_sell_logic {

constexpr int kInitialClickCount = 90;
constexpr int kCoordinateScale = 1000000;

// v0.5 profiles normally keep the item-cell click at row 5.  v0.6.1 users may
// already have reduced the now-semantic sell sequence to one coordinate row.
inline int ConfigRowIndex(std::size_t rowCount) {
    if (rowCount == 0) return -1;
    return rowCount >= 5 ? 4 : static_cast<int>(rowCount - 1);
}

// The first completed sale uses the historical 90-click ceiling.  Just like
// v0.5 Step 5, later sales use the stable FreeBagSpace learned after the prior
// sale and never exceed that ceiling.
inline int EffectiveClickCount(int learnedFreeBagSpace) {
    return learnedFreeBagSpace > 0
        ? std::clamp(learnedFreeBagSpace, 1, kInitialClickCount)
        : kInitialClickCount;
}

inline int NormalizeClientCoordinate(int value, int extent) {
    if (extent <= 0) return -1;
    const int bounded = std::clamp(value, 0, extent - 1);
    return static_cast<int>((static_cast<std::int64_t>(bounded) * kCoordinateScale) / extent);
}

} // namespace fixed_slot_sell_logic
