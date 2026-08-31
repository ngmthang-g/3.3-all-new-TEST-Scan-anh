#pragma once

#include <algorithm>
#include <cstdint>

namespace cleanroute_dungeon_progress {

enum ProofSource : std::uint32_t {
    ProofNone    = 0,
    ProofTask    = 1u << 0,
    ProofFuBen   = 1u << 1,
    ProofScanner = 1u << 2,
};

enum class Decision {
    KeepFighting,
    Advance,
    WaitForServerSync,
    Conflict,
    FailClosed,
};

struct Input {
    bool taskAvailable = false;
    bool taskFresh = false;
    bool taskBound = false;
    int taskDelta = 0;

    bool fubenAvailable = false;
    bool fubenFresh = false;
    bool fubenBound = false;
    int fubenDelta = 0;

    bool scannerAvailable = false;
    int scannerKills = 0;
    int required = 0;

    // When an authoritative source exists but trails a scanner-complete step, give the
    // server a bounded synchronization window. This never converts a timeout into PASS.
    bool serverSyncGraceExpired = false;
};

struct Result {
    Decision decision = Decision::FailClosed;
    std::uint32_t sources = ProofNone;
    int effectiveProgress = 0;
    bool authoritative = false;
};

inline Result Decide(const Input& in) {
    Result out{};
    if (in.required <= 0) return out;

    const bool taskAuthoritative = in.taskAvailable && in.taskFresh && in.taskBound;
    const bool fubenAuthoritative = in.fubenAvailable && in.fubenFresh && in.fubenBound;
    if (taskAuthoritative) out.sources |= ProofTask;
    if (fubenAuthoritative) out.sources |= ProofFuBen;
    if (in.scannerAvailable) out.sources |= ProofScanner;
    out.authoritative = taskAuthoritative || fubenAuthoritative;

    // Two fresh server/runtime authorities must agree on completion. Never pick the larger
    // value and silently advance through a disagreement.
    if (taskAuthoritative && fubenAuthoritative) {
        const bool taskDone = in.taskDelta >= in.required;
        const bool fubenDone = in.fubenDelta >= in.required;
        if (taskDone != fubenDone) {
            out.decision = Decision::Conflict;
            out.effectiveProgress = std::min(in.taskDelta, in.fubenDelta);
            return out;
        }
        out.effectiveProgress = std::min(in.taskDelta, in.fubenDelta);
        out.decision = taskDone ? Decision::Advance : Decision::KeepFighting;
        return out;
    }

    if (taskAuthoritative || fubenAuthoritative) {
        const int serverProgress = taskAuthoritative ? in.taskDelta : in.fubenDelta;
        out.effectiveProgress = std::max(0, serverProgress);
        if (serverProgress >= in.required) {
            out.decision = Decision::Advance;
            return out;
        }

        // Scanner may observe the death before UpdateTask/FuBen state is committed. Scanner
        // completion therefore requests synchronization; it cannot override a fresh incomplete
        // server proof.
        if (in.scannerAvailable && in.scannerKills >= in.required) {
            out.decision = in.serverSyncGraceExpired ? Decision::KeepFighting
                                                     : Decision::WaitForServerSync;
            return out;
        }
        out.decision = Decision::KeepFighting;
        return out;
    }

    // A stale/unbound task snapshot is not authority. When the semantic task API itself is
    // unavailable/unbound, retain v4.0's strict alive->dead scanner as the documented fallback.
    if (in.scannerAvailable) {
        out.effectiveProgress = std::max(0, in.scannerKills);
        out.decision = in.scannerKills >= in.required ? Decision::Advance
                                                      : Decision::KeepFighting;
        return out;
    }

    out.decision = Decision::FailClosed;
    return out;
}

inline const wchar_t* DecisionText(Decision value) {
    switch (value) {
        case Decision::KeepFighting: return L"KEEP";
        case Decision::Advance: return L"PASS";
        case Decision::WaitForServerSync: return L"SYNC";
        case Decision::Conflict: return L"CONFLICT";
        case Decision::FailClosed: return L"FAIL-CLOSED";
    }
    return L"UNKNOWN";
}

} // namespace cleanroute_dungeon_progress
