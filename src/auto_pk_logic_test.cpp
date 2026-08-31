#include "auto_pk_logic.h"
#include <cassert>

using namespace auto_pk_logic;

int main() {
    assert(NeedsWorldTarget(StepKind::Treatment));
    assert(NeedsWorldTarget(StepKind::Rally));
    assert(NeedsWorldTarget(StepKind::EnterPk));
    assert(!NeedsWorldTarget(StepKind::Buff));
    assert(RequiresClicks(StepKind::Buff));
    assert(RequiresClicks(StepKind::EnterPk));
    assert(!RequiresClicks(StepKind::Treatment));
    assert(PhaseAllowed(StepKind::EnterPk, ClickPhase::EnterPkMode));
    assert(PhaseAllowed(StepKind::EnterPk, ClickPhase::AutoPk));
    assert(!PhaseAllowed(StepKind::EnterPk, ClickPhase::Normal));
    assert(PhaseAllowed(StepKind::Buff, ClickPhase::Normal));
    assert(!PhaseAllowed(StepKind::Buff, ClickPhase::AutoPk));
    assert(EnterPkLayoutReady(2, 2));
    assert(!EnterPkLayoutReady(1, 2));
    return 0;
}
