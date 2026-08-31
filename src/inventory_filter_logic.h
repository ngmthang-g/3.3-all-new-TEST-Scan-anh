#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>

namespace inventory_filter_logic {

enum class RuleAction : int { Keep = 0, Drop = 1, Sell = 2 };

struct ItemRule {
    std::int32_t itemID = 0;
    RuleAction action = RuleAction::Keep;
    std::wstring name{};
};

struct Settings {
    bool enabled = false;
    bool protectBound = true;
    bool keepWeapons = true;
    bool dropEquipNonWeapon = false;
    bool sellEquipNonWeapon = false;
    bool dropCommon = false;
    bool sellCommon = false;
    bool dropGem = false;
    bool sellGem = false;
    bool dropMedicine = false;
    bool sellMedicine = false;
    bool dropPetEquip = false;
    bool sellPetEquip = false;
    std::vector<ItemRule> rules{};
};

struct ItemView {
    std::int64_t instanceID = 0;
    std::int32_t itemID = 0;
    std::int32_t site = 0;
    std::int32_t position = -1;
    std::int32_t quantity = 0;
    bool bound = false;
    bool throwable = false;
    bool sellable = false;
    bool isEquip = false;
    bool isWeapon = false;
    std::wstring itemType{};
};

inline bool QuestProtected(std::int32_t itemID) {
    return itemID >= 40000000 && itemID < 50000000;
}

inline const ItemRule* ExplicitRule(const Settings& settings, std::int32_t itemID) {
    for (const ItemRule& rule : settings.rules) if (rule.itemID == itemID) return &rule;
    return nullptr;
}

inline bool TypeEq(const std::wstring& a, const wchar_t* b) {
    if (!b) return false;
    std::size_t i = 0;
    for (; i < a.size() && b[i]; ++i) {
        if (std::towlower(a[i]) != std::towlower(b[i])) return false;
    }
    return i == a.size() && b[i] == 0;
}

inline RuleAction CategoryAction(const Settings& s, const ItemView& item) {
    if (item.isEquip && !item.isWeapon) {
        if (s.dropEquipNonWeapon) return RuleAction::Drop;
        if (s.sellEquipNonWeapon) return RuleAction::Sell;
    }
    if (TypeEq(item.itemType, L"CommonItem") || TypeEq(item.itemType, L"ScriptItem")) {
        if (s.dropCommon) return RuleAction::Drop;
        if (s.sellCommon) return RuleAction::Sell;
    }
    if (TypeEq(item.itemType, L"Gem")) {
        if (s.dropGem) return RuleAction::Drop;
        if (s.sellGem) return RuleAction::Sell;
    }
    if (TypeEq(item.itemType, L"Medicine")) {
        if (s.dropMedicine) return RuleAction::Drop;
        if (s.sellMedicine) return RuleAction::Sell;
    }
    if (TypeEq(item.itemType, L"PetEquip")) {
        if (s.dropPetEquip) return RuleAction::Drop;
        if (s.sellPetEquip) return RuleAction::Sell;
    }
    return RuleAction::Keep;
}

inline RuleAction DesiredAction(const Settings& s, const ItemView& item) {
    // Hard guards are deliberately above all user rules. Quest-family items and
    // optionally bound items/weapons must never be destroyed/sold by a stale rule.
    if (item.site != 10 || item.itemID <= 0 || item.instanceID <= 0) return RuleAction::Keep;
    if (QuestProtected(item.itemID)) return RuleAction::Keep;
    if (s.protectBound && item.bound) return RuleAction::Keep;
    if (s.keepWeapons && item.isEquip && item.isWeapon) return RuleAction::Keep;

    if (const ItemRule* rule = ExplicitRule(s, item.itemID)) {
        if (rule->action == RuleAction::Drop) return item.throwable ? RuleAction::Drop : RuleAction::Keep;
        if (rule->action == RuleAction::Sell) return item.sellable ? RuleAction::Sell : RuleAction::Keep;
        return RuleAction::Keep;
    }

    const RuleAction category = CategoryAction(s, item);
    if (category == RuleAction::Drop) return item.throwable ? RuleAction::Drop : RuleAction::Keep;
    if (category == RuleAction::Sell) return item.sellable ? RuleAction::Sell : RuleAction::Keep;
    return RuleAction::Keep;
}

inline bool HasSellRules(const Settings& s) {
    if (s.sellEquipNonWeapon || s.sellCommon || s.sellGem || s.sellMedicine || s.sellPetEquip) return true;
    return std::any_of(s.rules.begin(), s.rules.end(), [](const ItemRule& r){ return r.action == RuleAction::Sell; });
}

inline int FindCandidate(const Settings& s, const std::vector<ItemView>& items, RuleAction wanted) {
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (DesiredAction(s, items[i]) == wanted) return static_cast<int>(i);
    }
    return -1;
}

} // namespace inventory_filter_logic
