#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace itemtrade_coordinator {

constexpr std::size_t kMaxQueuedChildren = 3;
constexpr int kReceivedSlotsFinishThreshold = 8;
constexpr int kTradePassMaxItems = kReceivedSlotsFinishThreshold + 1;

enum class PassDecision { FinishChild, RepeatSameChild };

inline bool CanStartTradePass(int freeBagSpace) {
    return freeBagSpace >= kTradePassMaxItems;
}

inline int ReceivedSlots(int beforeFree, int afterFree) {
    if (beforeFree < 0 || afterFree < 0) return 0;
    return std::max(0, beforeFree - afterFree);
}

inline PassDecision DecidePass(int beforeFree, int afterFree) {
    return ReceivedSlots(beforeFree, afterFree) <= kReceivedSlotsFinishThreshold
        ? PassDecision::FinishChild
        : PassDecision::RepeatSameChild;
}

inline bool EarlierWorkflowEntry(std::uint64_t aEntry, int aSlot,
                                 std::uint64_t bEntry, int bSlot) {
    if (aEntry == 0) return false;
    if (bEntry == 0) return true;
    if (aEntry != bEntry) return aEntry < bEntry;
    return aSlot < bSlot;
}

inline bool ShouldAdmitFullChild(bool stateReady, int freeBagSpace, std::size_t queuedCount) {
    return stateReady && freeBagSpace == 0 && queuedCount < kMaxQueuedChildren;
}

inline bool ShouldAutoSell(bool consolidationEnabled, int tradeRole, bool enableSell,
                           int freeBagSpace) {
    if (freeBagSpace < 0 || !enableSell) return false;
    if (!consolidationEnabled) return freeBagSpace <= 0;
    if (tradeRole >= 2) return false;
    if (tradeRole == 1) return !CanStartTradePass(freeBagSpace);
    return freeBagSpace <= 0;
}

} // namespace itemtrade_coordinator
