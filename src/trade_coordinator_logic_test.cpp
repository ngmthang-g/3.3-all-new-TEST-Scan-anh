#include "trade_coordinator_logic.h"
#include <cassert>
#include <cstdint>
#include <iostream>

using namespace itemtrade_coordinator;

int main() {
    // v1.4 pass rule: MAIN delta <=8 finishes, >8 repeats the same active CON.
    assert(kReceivedSlotsFinishThreshold == 8);
    assert(ReceivedSlots(40, 32) == 8);
    assert(ReceivedSlots(40, 31) == 9);
    assert(ReceivedSlots(31, 22) == 9);
    assert(ReceivedSlots(20, 12) == 8);
    assert(ReceivedSlots(20, 11) == 9);
    assert(ReceivedSlots(40, 40) == 0);
    assert(ReceivedSlots(40, 42) == 0);
    assert(DecidePass(40, 32) == PassDecision::FinishChild);
    assert(DecidePass(20, 12) == PassDecision::FinishChild);
    assert(DecidePass(40, 31) == PassDecision::RepeatSameChild);
    assert(DecidePass(31, 22) == PassDecision::RepeatSameChild);
    assert(DecidePass(20, 11) == PassDecision::RepeatSameChild);

    // MAIN capacity is a pass-start condition only; it is not part of DecidePass().
    assert(CanStartTradePass(9));
    assert(CanStartTradePass(30));
    assert(!CanStartTradePass(8));

    // FIFO behavior is unchanged.
    assert(EarlierWorkflowEntry(1, 3, 2, 1));
    assert(!EarlierWorkflowEntry(2, 1, 1, 3));
    assert(EarlierWorkflowEntry(10, 1, 10, 2));
    assert(!EarlierWorkflowEntry(10, 2, 10, 1));

    // FULL is admission-only; repeat logic never consults CON fullness again.
    assert(ShouldAdmitFullChild(true, 0, 0));
    assert(ShouldAdmitFullChild(true, 0, 2));
    assert(!ShouldAdmitFullChild(true, 1, 0));
    assert(!ShouldAdmitFullChild(true, 0, 3));
    assert(!ShouldAdmitFullChild(false, 0, 0));

    // Auto Sell: while consolidation is ON, MAIN uses the same 9-slot pass gate.
    assert(!ShouldAutoSell(false, 0, false, 0));
    assert(ShouldAutoSell(false, 0, true, 0));
    assert(!ShouldAutoSell(false, 0, true, 1));
    assert(!ShouldAutoSell(true, 1, false, 8));
    assert(ShouldAutoSell(true, 1, true, 0));
    assert(ShouldAutoSell(true, 1, true, 8));
    assert(!ShouldAutoSell(true, 1, true, 9));
    assert(!ShouldAutoSell(true, 1, true, 30));
    assert(!ShouldAutoSell(true, 2, true, 0));

    std::cout << "trade_coordinator_logic_tests PASS\n";
    return 0;
}
