#include "travel_network_logic.h"
#include <cassert>

int main() {
    using namespace travel_network_logic;

    {
        const auto p = SelectNpcTeleport(kLauLanMap, kNamHaiMap);
        assert(p.valid && p.npcID == 387 && p.expectedMap == 85);
        assert(p.useSharedXaTruyenBinhPosition && p.semantic == Semantic::NamHai && p.needConfirm);
    }
    {
        const auto p = SelectNpcTeleport(kLauLanMap, kMieuCuongMap);
        assert(p.valid && p.expectedMap == 64 && p.semantic == Semantic::MieuCuong);
    }
    {
        const auto p = SelectNpcTeleport(kLauLanMap, kHoangLongPhuMap);
        assert(p.valid && p.expectedMap == 49 && p.semantic == Semantic::HoangLongPhu);
    }
    {
        const auto p = SelectNpcTeleport(kLauLanMap, kBachSaDiemKhanhMap);
        assert(p.valid && p.expectedMap == kThachLamMap && p.semantic == Semantic::ThachLam);
    }
    {
        const auto p = SelectNpcTeleport(kLauLanMap, kNgocKheMap);
        assert(p.valid && p.expectedMap == kThachLamMap);
    }
    {
        const auto p = SelectNpcTeleport(kNamHaiMap, kDaiLyMap);
        assert(p.valid && p.npcID == 522 && p.npcX == 7236 && p.npcY == 1908);
        assert(!p.useSharedXaTruyenBinhPosition && p.semantic == Semantic::DaiLy);
    }

    // Current map already inside the Thạch Lâm portal network: never loop back
    // through Lâu Lan. Existing AutoPath owns the direct portal chain.
    assert(!SelectNpcTeleport(kDiemHoMap, kBachSaDiemKhanhMap).valid);
    assert(!SelectNpcTeleport(kThachLamMap, kNgocKheMap).valid);
    assert(!SelectNpcTeleport(kNamChieuMap, kThachLamMap).valid);

    // Unproven return NPCs must never be guessed/hardcoded by this planner.
    assert(!SelectNpcTeleport(kMieuCuongMap, kDaiLyMap).valid);
    assert(!SelectNpcTeleport(kHoangLongPhuMap, kDaiLyMap).valid);
    assert(!SelectNpcTeleport(kThachLamMap, kDaiLyMap).valid);

    // Existing unrelated shortcut families remain outside this planner.
    assert(!SelectNpcTeleport(kLauLanMap, 75).valid);
    assert(!SelectNpcTeleport(kLauLanMap, 55).valid);
    return 0;
}
