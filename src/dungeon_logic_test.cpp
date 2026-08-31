#include "dungeon_logic.h"
#include "dungeon_presets.h"

#include <cassert>

int main() {
    using namespace cleanroute_dungeon;

    TeamConfig first{};
    first.pids = {1, 2, 3};
    first.leaderPid = 1;
    first.runs = 3;
    std::wstring error;
    assert(ValidateTeam(first, error));

    TeamConfig second{};
    second.pids = {3, 4};
    second.leaderPid = 4;
    assert(TeamsOverlap(first, TeamState::Running, second, TeamState::Paused));
    assert(!TeamsOverlap(first, TeamState::Stopped, second, TeamState::Running));

    TeamConfig tooLarge{};
    tooLarge.pids = {1, 2, 3, 4, 5, 6, 7};
    tooLarge.leaderPid = 1;
    assert(!ValidateTeam(tooLarge, error));

    assert(ValidParticipantMask(1) == 0x01u);
    assert(ValidParticipantMask(6) == 0x3Fu);
    assert(NormalizeParticipantMask(0, 3) == 0x07u);
    assert(ParticipantSelected(0x05u, 0, 3));
    assert(!ParticipantSelected(0x05u, 1, 3));
    assert(ParticipantSelected(0x05u, 2, 3));

    MonsterRule bossRule{L"Boss", 10, L"BOSS", true, true};
    MonsterObservation monster{};
    monster.roleID = 7;
    monster.resID = 10;
    monster.hp = 100;
    monster.maxHP = 100;
    monster.x = 100;
    monster.y = 100;
    monster.positionValid = true;
    monster.verifiedMonster = true;
    monster.liveVitalsValid = true;
    monster.name = L"Boss";
    assert(MatchesRule(monster, bossRule, L"BOSS"));
    assert(InRadius(monster, 100, 100, 800));
    assert(!InRadius(monster, 5000, 5000, 800));

    Step invalidFight{};
    invalidFight.kind = StepKind::Fight;
    invalidFight.timeoutSec = 60;
    assert(!ValidateStep(invalidFight, error));
    invalidFight.monsterName = L"Boss";
    assert(ValidateStep(invalidFight, error));

    const auto presets = CanonicalPresets();
    assert(presets.size() == 19);
    assert(presets[0].dungeonMap == 92);
    assert(presets[10].dungeonMap == 103);
    for (const Preset& preset : presets) {
        assert(!preset.id.empty());
        assert(!preset.name.empty());
        assert(preset.dungeonMap > 0);
        for (const Step& step : preset.steps) assert(ValidateStep(step, error));
    }
    return 0;
}
