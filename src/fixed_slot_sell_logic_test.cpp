#include "fixed_slot_sell_logic.h"

#include <cassert>

using namespace fixed_slot_sell_logic;

int main() {
    assert(ConfigRowIndex(0) == -1);
    assert(ConfigRowIndex(1) == 0);
    assert(ConfigRowIndex(4) == 3);
    assert(ConfigRowIndex(5) == 4);
    assert(ConfigRowIndex(12) == 4);

    assert(EffectiveClickCount(-1) == 90);
    assert(EffectiveClickCount(0) == 90);
    assert(EffectiveClickCount(1) == 1);
    assert(EffectiveClickCount(37) == 37);
    assert(EffectiveClickCount(90) == 90);
    assert(EffectiveClickCount(100) == 90);

    assert(NormalizeClientCoordinate(0, 930) == 0);
    assert(NormalizeClientCoordinate(929, 930) < kCoordinateScale);
    assert(NormalizeClientCoordinate(465, 930) == 500000);
    assert(NormalizeClientCoordinate(10, 0) == -1);
    return 0;
}
