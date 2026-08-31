#pragma once

namespace travel_network_logic {

constexpr int kDaiLyMap = 2;
constexpr int kLauLanMap = 5;
constexpr int kHoangLongPhuMap = 49;
constexpr int kThachLamMap = 60;
constexpr int kNgocKheMap = 61;
constexpr int kNamChieuMap = 63;
constexpr int kMieuCuongMap = 64;
constexpr int kDiemHoMap = 65;
constexpr int kBachSaDiemKhanhMap = 66;
constexpr int kNamHaiMap = 85;

constexpr int kXaTruyenBinhNpcId = 387;
constexpr int kXaTruyenTinNpcId = 522;
constexpr int kXaTruyenTinX = 7236;
constexpr int kXaTruyenTinY = 1908;

enum class Semantic : int {
    None = 0,
    NamHai,
    MieuCuong,
    HoangLongPhu,
    ThachLam,
    DaiLy,
};

struct NpcTeleportPlan {
    bool valid = false;
    int fromMap = 0;
    int expectedMap = 0;
    int npcID = 0;
    int npcX = 0;
    int npcY = 0;
    bool useSharedXaTruyenBinhPosition = false;
    Semantic semantic = Semantic::None;
    bool needConfirm = true;
    const wchar_t* label = L"";
};

constexpr bool IsThachLamPortalNetworkMap(int mapID) {
    return mapID == kThachLamMap || mapID == kNgocKheMap || mapID == kNamChieuMap ||
           mapID == kDiemHoMap || mapID == kBachSaDiemKhanhMap;
}

// Select only the special NPC leg proven by source/DATA. Normal portal routing
// remains owned by the existing AutoPath engine after this leg finishes.
// This intentionally does not invent return NPCs whose ResID is still unknown.
constexpr NpcTeleportPlan SelectNpcTeleport(int currentMap, int destinationMap) {
    if (currentMap == kLauLanMap) {
        if (destinationMap == kNamHaiMap) {
            return {true, kLauLanMap, kNamHaiMap, kXaTruyenBinhNpcId, 0, 0, true,
                    Semantic::NamHai, true, L"Xa Truyền Bình → Nam Hải"};
        }
        if (destinationMap == kMieuCuongMap) {
            return {true, kLauLanMap, kMieuCuongMap, kXaTruyenBinhNpcId, 0, 0, true,
                    Semantic::MieuCuong, true, L"Xa Truyền Bình → Miêu Cương"};
        }
        if (destinationMap == kHoangLongPhuMap) {
            return {true, kLauLanMap, kHoangLongPhuMap, kXaTruyenBinhNpcId, 0, 0, true,
                    Semantic::HoangLongPhu, true, L"Xa Truyền Bình → Hoàng Long Phủ"};
        }
        if (IsThachLamPortalNetworkMap(destinationMap)) {
            return {true, kLauLanMap, kThachLamMap, kXaTruyenBinhNpcId, 0, 0, true,
                    Semantic::ThachLam, true, L"Xa Truyền Bình → Thạch Lâm"};
        }
    }

    // Only this return NPC is proven. Miêu Cương/Hoàng Long Phủ/Thạch Lâm and
    // downstream maps deliberately fall through to normal AutoPath rather than
    // guessing Xa Truyền Chỉ/Sảng/Quy or any other NPC ID.
    if (currentMap == kNamHaiMap && destinationMap == kDaiLyMap) {
        return {true, kNamHaiMap, kDaiLyMap, kXaTruyenTinNpcId,
                kXaTruyenTinX, kXaTruyenTinY, false,
                Semantic::DaiLy, true, L"Xa Truyền Tín → Đại Lý"};
    }

    return {};
}

} // namespace travel_network_logic
