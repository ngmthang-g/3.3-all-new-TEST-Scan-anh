#pragma once

#include "dungeon_logic.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace dungeon_v45 {

constexpr int kConfigFormatVersion = 1;
constexpr int kMaxQueueEntries = 32;
constexpr int kMaxRunsPerEntry = 10;

struct QueueEntry {
    std::wstring presetId;
    int runs = 1;
};

struct ActivityObjective {
    std::wstring name;
    int current = -1;
    int target = -1;
    int objectiveId = 0;
};

struct ActivityBoard {
    bool synchronized = false;
    std::wstring source;
    std::wstring activityName;
    int activityId = 0;
    int scenarioMap = 0;
    int remainingSeconds = -1;
    std::uint64_t capturedTick = 0;
    std::vector<ActivityObjective> objectives;
};

inline bool ValidateQueue(const std::vector<QueueEntry>& queue, std::wstring& error) {
    if (queue.empty() || queue.size() > static_cast<std::size_t>(kMaxQueueEntries)) {
        error = L"Danh sách phó bản phải có 1–32 dòng";
        return false;
    }
    for (const auto& entry : queue) {
        if (entry.presetId.empty()) {
            error = L"Một dòng kế hoạch chưa chọn phó bản";
            return false;
        }
        if (entry.runs < 1 || entry.runs > kMaxRunsPerEntry) {
            error = L"Số lượt mỗi phó bản phải 1–10";
            return false;
        }
    }
    return true;
}

inline bool EditableTeam(cleanroute_dungeon::TeamState state) {
    return state == cleanroute_dungeon::TeamState::Stopped ||
           state == cleanroute_dungeon::TeamState::Error ||
           state == cleanroute_dungeon::TeamState::Completed;
}

struct EffectiveCoordinate {
    bool valid = false;
    int sourceStep = -1;
    int mapID = 0;
    int x = 0;
    int y = 0;
};

inline bool StepDefinesCoordinate(const cleanroute_dungeon::Step& step) {
    if (step.kind == cleanroute_dungeon::StepKind::Move ||
        step.kind == cleanroute_dungeon::StepKind::Portal) return step.mapID > 0;
    return step.mapID > 0 && (step.x != 0 || step.y != 0);
}

inline EffectiveCoordinate ResolveNearestCoordinate(const std::vector<cleanroute_dungeon::Step>& steps,
                                                    int stepIndex,
                                                    int requiredMap = 0) {
    EffectiveCoordinate out{};
    if (stepIndex < 0 || stepIndex >= static_cast<int>(steps.size())) return out;

    const auto& current = steps[static_cast<std::size_t>(stepIndex)];
    const int wantedMap = requiredMap > 0 ? requiredMap : current.mapID;
    if (StepDefinesCoordinate(current) && (wantedMap <= 0 || current.mapID == wantedMap)) {
        out = {true, stepIndex, current.mapID, current.x, current.y};
        return out;
    }

    for (int i = stepIndex - 1; i >= 0; --i) {
        const auto& candidate = steps[static_cast<std::size_t>(i)];
        if (!StepDefinesCoordinate(candidate)) continue;
        if (wantedMap > 0 && candidate.mapID != wantedMap) continue;
        out = {true, i, candidate.mapID, candidate.x, candidate.y};
        return out;
    }
    return out;
}

inline bool ValidateParallelGroups(const std::vector<cleanroute_dungeon::Step>& steps,
                                   std::wstring& error) {
    int activeGroup = 0;
    std::vector<int> closed;
    std::vector<std::uint32_t> laneMasks;
    for (std::size_t i = 0; i < steps.size(); ++i) {
        const auto& step = steps[i];
        if (step.parallelGroup <= 0) {
            if (activeGroup > 0) closed.push_back(activeGroup);
            activeGroup = 0;
            laneMasks.clear();
            continue;
        }
        if (step.kind != cleanroute_dungeon::StepKind::Move) {
            error = L"STEP " + std::to_wstring(i + 1) + L" • Parallel chỉ hỗ trợ TỌA ĐỘ";
            return false;
        }
        if (step.parallelGroup != activeGroup) {
            if (activeGroup > 0) closed.push_back(activeGroup);
            if (std::find(closed.begin(), closed.end(), step.parallelGroup) != closed.end()) {
                error = L"Parallel Group " + std::to_wstring(step.parallelGroup) + L" phải nằm liền nhau";
                return false;
            }
            activeGroup = step.parallelGroup;
            laneMasks.clear();
        }
        const std::uint32_t mask = step.participantMask;
        bool sameLane = false;
        for (std::uint32_t existing : laneMasks) {
            if (existing == mask) { sameLane = true; break; }
            if ((existing & mask) != 0) {
                error = L"Parallel Group " + std::to_wstring(step.parallelGroup) +
                        L" có hai lane khác nhau dùng trùng Slot ACC";
                return false;
            }
        }
        if (!sameLane) laneMasks.push_back(mask);
    }
    return true;
}

inline bool ValidateInheritedCoordinates(const std::vector<cleanroute_dungeon::Step>& steps,
                                         std::wstring& error) {
    for (int i = 0; i < static_cast<int>(steps.size()); ++i) {
        const auto& step = steps[static_cast<std::size_t>(i)];
        const bool inherits = step.kind == cleanroute_dungeon::StepKind::Fight ||
                              step.kind == cleanroute_dungeon::StepKind::StopFight ||
                              step.kind == cleanroute_dungeon::StepKind::Wait;
        if (!inherits || StepDefinesCoordinate(step)) continue;
        const auto resolved = ResolveNearestCoordinate(steps, i, step.mapID);
        if (!resolved.valid) {
            error = L"STEP " + std::to_wstring(i + 1) +
                    L" chưa có tọa độ hợp lệ gần nhất phía trên cùng Map";
            return false;
        }
    }
    return true;
}

inline std::wstring CoordinateDisplay(const std::vector<cleanroute_dungeon::Step>& steps, int stepIndex) {
    if (stepIndex < 0 || stepIndex >= static_cast<int>(steps.size())) return L"-";
    const auto& step = steps[static_cast<std::size_t>(stepIndex)];
    if (StepDefinesCoordinate(step)) {
        return L"M" + std::to_wstring(step.mapID) + L" • " +
               std::to_wstring(step.x) + L"," + std::to_wstring(step.y);
    }
    const auto inherited = ResolveNearestCoordinate(steps, stepIndex, step.mapID);
    if (!inherited.valid) return L"LỖI • chưa có tọa độ tham chiếu";
    return L"↳ STEP " + std::to_wstring(inherited.sourceStep + 1) + L" • M" +
           std::to_wstring(inherited.mapID) + L" • " + std::to_wstring(inherited.x) + L"," +
           std::to_wstring(inherited.y);
}

inline bool SameActivityBoard(const ActivityBoard& a, const ActivityBoard& b) {
    if (a.synchronized != b.synchronized || a.activityId != b.activityId ||
        a.scenarioMap != b.scenarioMap || a.activityName != b.activityName ||
        a.objectives.size() != b.objectives.size()) return false;
    for (std::size_t i = 0; i < a.objectives.size(); ++i) {
        const auto& x = a.objectives[i];
        const auto& y = b.objectives[i];
        if (x.name != y.name || x.current != y.current || x.target != y.target ||
            x.objectiveId != y.objectiveId) return false;
    }
    return true;
}

inline std::wstring CompletionDedupKey(int teamId, const std::wstring& presetId,
                                       int queueIndex, int runIndex) {
    return std::to_wstring(teamId) + L"|" + presetId + L"|" +
           std::to_wstring(queueIndex) + L"|" + std::to_wstring(runIndex);
}

} // namespace dungeon_v45
