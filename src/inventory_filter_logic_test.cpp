#include "inventory_filter_logic.h"
#include <cassert>
#include <iostream>
using namespace inventory_filter_logic;

static ItemView Item(std::int32_t id, const wchar_t* type, bool equip=false, bool weapon=false,
                     bool bound=false, bool throwable=true, bool sellable=true) {
    ItemView x{}; x.instanceID = 1000 + id; x.itemID = id; x.site = 10; x.position = 1; x.quantity = 1;
    x.itemType = type; x.isEquip = equip; x.isWeapon = weapon; x.bound = bound;
    x.throwable = throwable; x.sellable = sellable; return x;
}

int main() {
    Settings s{}; s.enabled = true; s.dropEquipNonWeapon = true;
    auto armor = Item(100, L"Equip", true, false);
    auto weapon = Item(101, L"Equip", true, true);
    assert(DesiredAction(s, armor) == RuleAction::Drop);
    assert(DesiredAction(s, weapon) == RuleAction::Keep);

    auto boundArmor = Item(102, L"Equip", true, false, true);
    assert(DesiredAction(s, boundArmor) == RuleAction::Keep);
    s.protectBound = false;
    assert(DesiredAction(s, boundArmor) == RuleAction::Drop);

    auto noThrow = Item(103, L"Equip", true, false, false, false, true);
    assert(DesiredAction(s, noThrow) == RuleAction::Keep);

    Settings explicitSell{}; explicitSell.enabled = true;
    explicitSell.rules.push_back({200, RuleAction::Sell, L"Test"});
    assert(DesiredAction(explicitSell, Item(200, L"CommonItem")) == RuleAction::Sell);
    assert(HasSellRules(explicitSell));

    Settings explicitKeep{}; explicitKeep.enabled = true; explicitKeep.dropGem = true;
    explicitKeep.rules.push_back({300, RuleAction::Keep, L"Ngoc quy"});
    assert(DesiredAction(explicitKeep, Item(300, L"Gem")) == RuleAction::Keep);
    assert(DesiredAction(explicitKeep, Item(301, L"Gem")) == RuleAction::Drop);

    auto quest = Item(40000001, L"CommonItem");
    Settings dangerous{}; dangerous.enabled = true; dangerous.dropCommon = true;
    dangerous.rules.push_back({40000001, RuleAction::Drop, L"Quest"});
    assert(DesiredAction(dangerous, quest) == RuleAction::Keep);

    std::vector<ItemView> bag{weapon, armor, Item(500, L"Medicine")};
    assert(FindCandidate(s, bag, RuleAction::Drop) == 1);
    std::cout << "inventory_filter_logic PASS\n";
    return 0;
}
