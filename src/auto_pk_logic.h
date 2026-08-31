#pragma once

#include <cstddef>

namespace auto_pk_logic {

enum class StepKind : int {
    Treatment = 0,
    Buff = 1,
    Rally = 2,
    EnterPk = 3,
    Custom = 4,
};

enum class ClickPhase : int {
    Normal = 0,
    EnterPkMode = 1,
    AutoPk = 2,
};

inline const wchar_t* StepKindLabel(StepKind kind) {
    switch (kind) {
        case StepKind::Treatment: return L"TRỊ LIỆU";
        case StepKind::Buff: return L"AUTO BUFF";
        case StepKind::Rally: return L"TỤ ĐIỂM PHỤ";
        case StepKind::EnterPk: return L"LAO VÀO PK";
        case StepKind::Custom: return L"TÙY CHỌN";
    }
    return L"?";
}

inline const wchar_t* ClickPhaseLabel(ClickPhase phase) {
    switch (phase) {
        case ClickPhase::Normal: return L"THƯỜNG";
        case ClickPhase::EnterPkMode: return L"BẬT PK";
        case ClickPhase::AutoPk: return L"AUTO PK";
    }
    return L"?";
}

inline bool NeedsWorldTarget(StepKind kind) {
    return kind == StepKind::Treatment || kind == StepKind::Rally || kind == StepKind::EnterPk;
}

inline bool UsesSemanticTreatment(StepKind kind) {
    return kind == StepKind::Treatment;
}

inline bool RequiresClicks(StepKind kind) {
    return kind == StepKind::Buff || kind == StepKind::EnterPk || kind == StepKind::Custom;
}

inline bool PhaseAllowed(StepKind kind, ClickPhase phase) {
    if (kind == StepKind::EnterPk) return phase == ClickPhase::EnterPkMode || phase == ClickPhase::AutoPk;
    return phase == ClickPhase::Normal;
}

inline bool EnterPkLayoutReady(std::size_t preCount, std::size_t postCount) {
    return preCount >= 2 && postCount >= 2;
}

} // namespace auto_pk_logic
