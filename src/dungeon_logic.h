#pragma once

#include <algorithm>
#include <cstdint>
#include <cwctype>
#include <string>
#include <vector>

namespace cleanroute_dungeon {

constexpr std::size_t kMaxTeamMembers = 6;
constexpr std::uint32_t kAllParticipantsMask = (1u << kMaxTeamMembers) - 1u;

enum class StepKind {
    Move,
    Fight,
    StopFight,
    WaitMap,
    Wait,
    Portal,
};

enum class TeamState {
    Stopped,
    Running,
    Paused,
    Error,
    Completed,
};

enum class TeamPhase {
    Precheck,
    Gather,
    Npc,
    Dialog,
    WaitEnter,
    Steps,
    WaitExit,
    PostSell,
    Complete,
};

struct MonsterObservation {
    std::int32_t roleID = 0;       // Dynamic life-instance identity.
    std::int32_t resID = 0;        // Stable configured/template identity when exposed.
    std::int32_t hp = -1;
    std::int32_t maxHP = -1;
    std::int32_t x = 0;
    std::int32_t y = 0;
    bool dead = false;
    bool positionValid = false;
    bool verifiedMonster = false;  // Exact GMonster ancestry proof.
    bool liveVitalsValid = false;  // HP/MaxHP came from a live actor getter.
    std::wstring name;
};

struct MonsterRule {
    std::wstring name;
    std::int32_t resID = 0;
    std::wstring group = L"THUONG";
    bool enabled = true;
    bool boss = false;
};

inline bool EqualFolded(const std::wstring& left, const std::wstring& right) {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (std::towlower(left[i]) != std::towlower(right[i])) return false;
    }
    return true;
}

inline bool MatchesRule(const MonsterObservation& monster, const MonsterRule& rule,
                        const std::wstring& requiredGroup) {
    if (!rule.enabled) return false;
    if (!requiredGroup.empty() && !EqualFolded(rule.group, requiredGroup)) return false;
    if (rule.resID > 0) return monster.resID == rule.resID;
    return !rule.name.empty() && EqualFolded(monster.name, rule.name);
}

inline bool InRadius(const MonsterObservation& monster, std::int32_t centerX,
                     std::int32_t centerY, std::int32_t radius) {
    if (radius <= 0) return true;
    if (!monster.positionValid) return false;
    const std::int64_t dx = static_cast<std::int64_t>(monster.x) - centerX;
    const std::int64_t dy = static_cast<std::int64_t>(monster.y) - centerY;
    const std::int64_t rr = static_cast<std::int64_t>(radius) * radius;
    return dx * dx + dy * dy <= rr;
}

struct Step {
    StepKind kind = StepKind::Move;
    std::wstring label;
    int mapID = 0;
    int x = 0;
    int y = 0;
    int tolerance = 120;
    int radius = 800;
    int delayMs = 0;
    int timeoutSec = 180;
    std::wstring monsterName;
    std::wstring group = L"THUONG";
    int monsterResID = 0;
    bool boss = false;
    // Only for DATA steps where the old donor cannot identify a stable template/name.
    // The editor exposes this explicitly; it must never be silently enabled by runtime.
    bool matchAnyVerified = false;
    // Bit 0..5 correspond to the six ordered slots inside the team.
    std::uint32_t participantMask = kAllParticipantsMask;
};

struct Preset {
    std::wstring id;
    std::wstring name;
    int dungeonMap = 0;
    int gatherMap = 0;
    int npcResID = 0;
    int gatherX = 0;
    int gatherY = 0;
    std::wstring dialogText;
    int minPlayers = 1;
    std::vector<Step> steps;
};

struct TeamConfig {
    int id = 0;
    int presetIndex = 0;
    int runs = 1;
    std::vector<std::uint32_t> pids;
    std::uint32_t leaderPid = 0;
};

inline bool ValidateTeam(const TeamConfig& team, std::wstring& error) {
    if (team.pids.empty() || team.pids.size() > kMaxTeamMembers) {
        error = L"Tổ đội phải có 1–6 acc";
        return false;
    }
    std::vector<std::uint32_t> sorted = team.pids;
    std::sort(sorted.begin(), sorted.end());
    if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end()) {
        error = L"Một acc bị chọn lặp";
        return false;
    }
    if (team.leaderPid == 0 ||
        std::find(team.pids.begin(), team.pids.end(), team.leaderPid) == team.pids.end()) {
        error = L"Đội trưởng phải nằm trong tổ đội";
        return false;
    }
    if (team.runs < 1 || team.runs > 10) {
        error = L"Số lượt phải 1–10";
        return false;
    }
    return true;
}

inline bool ActiveState(TeamState state) {
    return state == TeamState::Running || state == TeamState::Paused;
}

inline bool TeamsOverlap(const TeamConfig& left, TeamState leftState,
                         const TeamConfig& right, TeamState rightState) {
    if (!ActiveState(leftState) || !ActiveState(rightState)) return false;
    for (std::uint32_t pid : left.pids) {
        if (std::find(right.pids.begin(), right.pids.end(), pid) != right.pids.end()) return true;
    }
    return false;
}

inline std::uint32_t ValidParticipantMask(std::size_t memberCount) {
    if (memberCount == 0) return 0;
    if (memberCount >= kMaxTeamMembers) return kAllParticipantsMask;
    return (1u << static_cast<unsigned>(memberCount)) - 1u;
}

inline std::uint32_t NormalizeParticipantMask(std::uint32_t mask, std::size_t memberCount) {
    const std::uint32_t valid = ValidParticipantMask(memberCount);
    const std::uint32_t selected = mask & valid;
    return selected ? selected : valid;
}

inline bool ParticipantSelected(std::uint32_t mask, std::size_t memberIndex,
                                std::size_t memberCount) {
    if (memberIndex >= memberCount || memberIndex >= kMaxTeamMembers) return false;
    const std::uint32_t normalized = NormalizeParticipantMask(mask, memberCount);
    return (normalized & (1u << static_cast<unsigned>(memberIndex))) != 0;
}

inline bool ValidateStep(const Step& step, std::wstring& error) {
    if (step.timeoutSec < 1 || step.timeoutSec > 86400) {
        error = L"Timeout STEP phải 1–86400 giây";
        return false;
    }
    if ((step.kind == StepKind::Move || step.kind == StepKind::Portal) && step.mapID <= 0) {
        error = L"STEP tọa độ/cổng phải có MapID";
        return false;
    }
    if (step.kind == StepKind::Fight && step.monsterResID <= 0 &&
        step.monsterName.empty() && !step.matchAnyVerified) {
        error = L"STEP đánh quái phải có Monster/ResID hoặc bật Match ANY GMonster";
        return false;
    }
    if (step.participantMask == 0) {
        error = L"STEP phải chọn ít nhất một slot ACC";
        return false;
    }
    return true;
}

inline const wchar_t* StepKindLabel(StepKind kind) {
    switch (kind) {
        case StepKind::Move: return L"TỌA ĐỘ";
        case StepKind::Fight: return L"AUTO-ĐÁNH QUÁI";
        case StepKind::StopFight: return L"DỪNG AUTO";
        case StepKind::WaitMap: return L"ĐỢI MAP";
        case StepKind::Wait: return L"ĐỢI";
        case StepKind::Portal: return L"CỔNG";
    }
    return L"?";
}

inline const wchar_t* TeamStateLabel(TeamState state) {
    switch (state) {
        case TeamState::Stopped: return L"STOP";
        case TeamState::Running: return L"RUN";
        case TeamState::Paused: return L"PAUSE";
        case TeamState::Error: return L"LỖI";
        case TeamState::Completed: return L"XONG";
    }
    return L"?";
}


} // namespace cleanroute_dungeon
