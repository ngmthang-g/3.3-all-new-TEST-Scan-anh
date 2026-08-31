#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <algorithm>

namespace cleanroute_rotation {

inline void PruneDeathTicks(std::vector<std::uint32_t>& ticks,
                            std::uint32_t now,
                            std::uint32_t windowMs) {
    ticks.erase(std::remove_if(ticks.begin(), ticks.end(), [=](std::uint32_t t) {
        return static_cast<std::uint32_t>(now - t) > windowMs;
    }), ticks.end());
}

inline bool DeathLimitExceeded(std::size_t count, int limit) {
    if (limit < 1) limit = 1;
    return count > static_cast<std::size_t>(limit);
}

inline std::size_t NextRotationIndex(std::size_t current, std::size_t count) {
    if (count == 0) return 0;
    return (current + 1u) % count;
}

inline bool NoFullBagWindowReached(std::uint64_t activeTrainMs, int minutes) {
    if (minutes < 1) minutes = 1;
    const std::uint64_t threshold = static_cast<std::uint64_t>(minutes) * 60ull * 1000ull;
    return activeTrainMs >= threshold;
}

} // namespace cleanroute_rotation
