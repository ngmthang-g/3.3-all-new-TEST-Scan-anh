#pragma once
#include <windows.h>
#include <cstdint>
#include <cstddef>

namespace cleanroute {

constexpr std::uint32_t kMagic = 0x4352544Cu; // CRTL
// v3.0 keeps all hidden UI actions on the proven InputSyncManager path and adds
// strict THĐC routing plus the configured three-click Côn Lôn exit sequence.
// Controller + Bridge are always shipped together; mismatched versions fail attach.
constexpr std::uint32_t kProtocolVersion = 0x00040100u;
constexpr UINT kWakeMessage = WM_APP + 0x531;
constexpr wchar_t kMappingPrefix[] = L"Local\\ThanLongCleanRoute_";

enum class Command : std::uint32_t {
    None = 0,
    ReadState = 1,
    ToggleRide = 2,
    StartPath = 3,
    StopPath = 4,
    ClickNpc = 5,
    ConfirmMap = 6,
    Revive = 7,
    StartAutoFight = 8,
    StopAutoFight = 9,
    BeginBackgroundSell = 10,
    AdvanceBackgroundSell = 11,
    SellNextBagItem = 12,
    CloseBackgroundSell = 13,
    ClickInternalPoint = 14,
    BeginBackgroundTreatment = 15,
    AdvanceBackgroundTreatment = 16,
    CloseBackgroundTreatment = 17,
    ReadCurrency = 18,
    ReadBagPage = 19,
    DropBagItem = 20,
    SellBagItem = 21,
    SelectTargetByRoleID = 22,
    ClickTravelSemantic = 23,
    ConfirmTravelSemantic = 24,
    ScanNearbyMonsters = 25,
    ClickDialogText = 26,
    ReadDungeonProgress = 27,
};

enum class TravelSemantic : std::int32_t {
    None = 0,
    KunLunSon = 1,
    TinhTucHai = 2,
    DenCacMonPhai = 3,
};

enum class ActionResult : std::int32_t {
    None = 0,
    ActionInvoked = 1,
    StageReady = 2,
    NoCandidate = 3,
    UiClosed = 4,
    NothingToClose = 5,
};


constexpr std::size_t kMaxMonsterRecords = 96;
enum MonsterValid : std::uint32_t {
    MonsterValidIdentity=1u<<0, MonsterValidTemplate=1u<<1, MonsterValidVitals=1u<<2,
    MonsterValidDeath=1u<<3, MonsterValidPosition=1u<<4, MonsterValidType=1u<<5,
    MonsterValidName=1u<<6, MonsterValidClassProof=1u<<7, MonsterValidLiveVitals=1u<<8,
};
enum class MonsterHpSource : std::int32_t { None=0, SemanticGetter=1, GuardedGRoleSubclassRva=2 };
struct MonsterRecord {
    std::uint32_t validMask=0; std::int32_t roleID=0,resID=0,type=0,hp=-1,maxHP=-1,dead=0,x=0,y=0;
    std::int32_t hpSource=static_cast<std::int32_t>(MonsterHpSource::None); wchar_t name[64]{}; wchar_t className[40]{};
};

enum SnapshotValid : std::uint32_t {
    ValidMapTransition = 1u << 0,
    ValidIdentity      = 1u << 1,
    ValidMap           = 1u << 2,
    ValidPosition      = 1u << 3,
    ValidRiding        = 1u << 4,
    ValidAutoPath      = 1u << 5,
    ValidLifeState     = 1u << 6,
    ValidAutoFight     = 1u << 7,
    ValidBagSpace      = 1u << 8,
};

struct Snapshot {
    std::uint32_t validMask = 0;
    std::int32_t roleID = 0;
    std::int32_t mapID = 0;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t riding = 0;
    std::int32_t autoPathing = 0;
    std::int32_t mapReady = 0;
    std::int32_t waitingChangeMap = 0;
    std::int32_t dead = 0;
    std::int32_t autoFight = 0;
    std::int32_t freeBagSpace = -1;
    wchar_t characterName[64]{};
};

struct Request {
    std::uint32_t command = 0;
    std::int32_t arg0 = 0;
    std::int32_t arg1 = 0;
    std::int32_t arg2 = 0;
    wchar_t text[160]{};
};


struct BagItemSnapshot {
    std::int64_t instanceID = 0;
    std::int32_t itemID = 0;
    std::int32_t site = 0;
    std::int32_t position = -1;
    std::int32_t quantity = 0;
    std::int32_t bound = 0;
    std::int32_t throwable = 0;
    std::int32_t sellable = 0;
    std::int32_t isEquip = 0;
    std::int32_t isWeapon = 0;
    std::int32_t itemTypeCode = 0;
    std::int32_t equipTypeCode = 0;
    wchar_t name[96]{};
    wchar_t itemType[32]{};
    wchar_t equipType[32]{};
};

constexpr std::size_t kBagPageCapacity = 20;

struct BagPageSnapshot {
    std::int32_t totalCount = 0;
    std::int32_t pageStart = 0;
    std::int32_t pageCount = 0;
    std::int32_t freeBagSpace = -1;
    BagItemSnapshot items[kBagPageCapacity]{};
};


constexpr std::size_t kMaxDungeonTasks = 12;
constexpr std::size_t kMaxDungeonTaskParameters = 48;

enum DungeonTaskValid : std::uint32_t {
    DungeonTaskValidIdentity   = 1u << 0,
    DungeonTaskValidName       = 1u << 1,
    DungeonTaskValidParameters = 1u << 2,
};

struct DungeonTaskParameter {
    std::int32_t key = 0;
    std::int32_t value = 0;
};

struct DungeonTaskRecord {
    std::uint32_t validMask = 0;
    std::int32_t taskID = 0;
    std::uint32_t parameterCount = 0;
    std::int32_t parameterTruncated = 0;
    wchar_t name[96]{};
    DungeonTaskParameter parameters[kMaxDungeonTaskParameters]{};
};

struct DungeonProgressSnapshot {
    std::uint32_t validMask = 0;
    std::uint64_t capturedTick = 0;
    std::uint32_t taskCount = 0;
    std::int32_t taskTruncated = 0;
    // FuBen fields are intentionally reserved until a concrete inbound/runtime response store is
    // proven on the frozen client. Packet IDs alone are not treated as progress proof.
    std::uint32_t fubenValidMask = 0;
    std::int32_t fubenCurrent = -1;
    std::int32_t fubenTarget = -1;
    std::int32_t fubenCompleted = -1;
    DungeonTaskRecord tasks[kMaxDungeonTasks]{};
};

struct Response {
    std::int32_t ok = 0;
    std::int32_t resultCode = 0;
    std::int32_t value0 = 0;
    std::int32_t value1 = 0;
    std::int64_t value64_0 = 0;
    std::int64_t value64_1 = 0;
    Snapshot snapshot{};
    BagPageSnapshot bagPage{};
    DungeonProgressSnapshot dungeonProgress{};
    std::uint32_t monsterCount=0, scannedEntries=0, excludedPlayerRoles=0, excludedOtherSprites=0, monsterHpReadFailures=0;
    std::int32_t monsterTruncated=0;
    MonsterRecord monsters[kMaxMonsterRecords]{};
    wchar_t detail[512]{};
};

struct SharedBlock {
    std::uint32_t magic = kMagic;
    std::uint32_t protocolVersion = kProtocolVersion;
    std::uint32_t targetPid = 0;
    std::uint32_t targetWindowThreadId = 0;
    volatile LONG requestSeq = 0;
    volatile LONG completedSeq = 0;
    volatile LONG bridgeLoaded = 0;
    volatile LONG bridgeBusy = 0;
    Request request{};
    Response response{};
};

static_assert(sizeof(SharedBlock) < 256u * 1024u, "Shared bridge block must remain bounded");

inline void MappingName(DWORD pid, wchar_t* output, std::size_t count) {
    if (!output || count == 0) return;
    wsprintfW(output, L"%s%lu", kMappingPrefix, static_cast<unsigned long>(pid));
}

} // namespace cleanroute
