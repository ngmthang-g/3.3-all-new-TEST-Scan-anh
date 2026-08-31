#include "dungeon_progress_logic.h"
#include <cassert>
using namespace cleanroute_dungeon_progress;

int main() {
    {
        Input in{}; in.required=6; in.scannerAvailable=true; in.scannerKills=6;
        assert(Decide(in).decision == Decision::Advance);
    }
    {
        Input in{}; in.required=6; in.taskAvailable=true; in.taskFresh=true; in.taskBound=true;
        in.taskDelta=5; in.scannerAvailable=true; in.scannerKills=6;
        assert(Decide(in).decision == Decision::WaitForServerSync);
        in.serverSyncGraceExpired=true;
        assert(Decide(in).decision == Decision::KeepFighting);
    }
    {
        Input in{}; in.required=6; in.taskAvailable=true; in.taskFresh=true; in.taskBound=true;
        in.taskDelta=6; in.scannerAvailable=true; in.scannerKills=4;
        const Result r=Decide(in); assert(r.decision == Decision::Advance); assert(r.authoritative);
    }
    {
        Input in{}; in.required=1; in.taskAvailable=true; in.taskFresh=true; in.taskBound=true; in.taskDelta=1;
        in.fubenAvailable=true; in.fubenFresh=true; in.fubenBound=true; in.fubenDelta=0;
        assert(Decide(in).decision == Decision::Conflict);
    }
    {
        Input in{}; in.required=10; in.taskAvailable=true; in.taskFresh=false; in.taskBound=true; in.taskDelta=10;
        in.scannerAvailable=true; in.scannerKills=9;
        assert(Decide(in).decision == Decision::KeepFighting);
    }
    {
        Input in{}; in.required=10;
        assert(Decide(in).decision == Decision::FailClosed);
    }
}
