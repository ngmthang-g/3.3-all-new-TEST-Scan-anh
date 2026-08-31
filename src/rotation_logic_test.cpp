#include <iostream>
#include <vector>
#include "rotation_logic.h"

using namespace cleanroute_rotation;

static int failures = 0;
static void Check(bool ok, const char* name) {
    if (ok) std::cout << "PASS " << name << "\n";
    else { std::cout << "FAIL " << name << "\n"; ++failures; }
}

int main() {
    Check(NextRotationIndex(0, 1) == 0, "one-spot-stays");
    Check(NextRotationIndex(0, 3) == 1, "next-1-of-3");
    Check(NextRotationIndex(2, 3) == 0, "wrap-3-to-1");
    Check(!DeathLimitExceeded(10, 10), "death-10-not-over-limit");
    Check(DeathLimitExceeded(11, 10), "death-11-over-limit");
    std::vector<std::uint32_t> deaths{10000, 12000, 610000};
    PruneDeathTicks(deaths, 611000, 600000);
    Check(deaths.size() == 2 && deaths[0] == 12000 && deaths[1] == 610000, "death-window-prune");
    Check(!NoFullBagWindowReached(899999, 15), "bag-window-before-15m");
    Check(NoFullBagWindowReached(900000, 15), "bag-window-at-15m");
    std::cout << "RESULT " << (8 - failures) << "/8 PASS\n";
    return failures == 0 ? 0 : 1;
}
