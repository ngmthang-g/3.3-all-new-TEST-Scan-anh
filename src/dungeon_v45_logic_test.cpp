#include "dungeon_v45_logic.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace cleanroute_dungeon;
    using namespace dungeon_v45;

    std::wstring error;
    assert(ValidateQueue({{L"ThuyLao", 2}, {L"Q1_ToChau", 3}}, error));
    assert(!ValidateQueue({}, error));
    assert(!ValidateQueue({{L"ThuyLao", 11}}, error));

    std::vector<Step> steps;
    Step move{};
    move.kind = StepKind::Move;
    move.mapID = 93;
    move.x = 1735;
    move.y = 2478;
    steps.push_back(move);

    Step fight{};
    fight.kind = StepKind::Fight;
    fight.mapID = 93;
    fight.monsterName = L"Ngụy Tống Binh";
    steps.push_back(fight);

    Step stop{};
    stop.kind = StepKind::StopFight;
    stop.mapID = 93;
    steps.push_back(stop);

    auto inherited = ResolveNearestCoordinate(steps, 1, 93);
    assert(inherited.valid && inherited.sourceStep == 0);
    assert(inherited.x == 1735 && inherited.y == 2478);
    assert(CoordinateDisplay(steps, 1).find(L"STEP 1") != std::wstring::npos);
    assert(ValidateInheritedCoordinates(steps, error));

    steps[1].mapID = 92;
    assert(!ValidateInheritedCoordinates(steps, error));

    assert(EditableTeam(TeamState::Stopped));
    assert(EditableTeam(TeamState::Error));
    assert(!EditableTeam(TeamState::Running));
    assert(!EditableTeam(TeamState::Paused));

    ActivityBoard a{};
    a.synchronized = true;
    a.activityId = 4;
    a.scenarioMap = 93;
    a.activityName = L"Biên Giới Tống Liêu";
    a.objectives.push_back({L"Ngụy Tống Binh", 33, 50, 1});
    ActivityBoard b = a;
    assert(SameActivityBoard(a, b));
    b.objectives[0].current = 34;
    assert(!SameActivityBoard(a, b));

    assert(CompletionDedupKey(13, L"ThuyLao", 0, 1) == L"13|ThuyLao|0|1");

    std::wcout << L"dungeon v4.5 logic tests PASS\n";
    return 0;
}
