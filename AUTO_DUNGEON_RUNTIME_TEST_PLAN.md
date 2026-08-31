# AUTO PHÓ BẢN — live runtime test plan

Do not mark v0.6.2 runtime-pass from compilation or CI alone. Test on the exact frozen client build with one disposable account and one short profile first.

## A. Read-only scanner

- Open a stable dungeon map and click `QUÉT MONSTER / HP` five times.
- Confirm the character does not freeze and scanner response stays below its timeout.
- Verify every returned row reports `GMonster✓`; visible players, NPCs and pets must not appear in the monster table.
- Stand next to another player with visible HP and scan repeatedly; expected returned player rows = 0 and excluded GRole count increases.
- Verify visible monster Name, live HP/MaxHP and dynamic RoleID against the game. Test both HP sources shown by the UI if present (`HP:SEM` or guarded `HP:RVA`).
- Verify ResID is non-zero and stable for two separate spawns of the same monster. If it is zero, document the actual class/property before changing offsets.
- Verify a manually persisted rule whose name/ResID resembles a player still cannot arm or increment the counter.

## B. Death counter

- Observe one target alive, then kill it; expected +1.
- Keep its corpse visible for at least five scans; expected no additional count.
- Enter range after a corpse already exists; expected 0 for that corpse.
- Let the same monster type respawn, observe it alive, kill it; expected one new count.
- Walk far enough that a live target disappears from AOI; expected 0.
- Change stage/map; expected prior alive evidence is cleared.
- Repeat with a BOSS group and required kill count 1.

## C. AutoFight semantic action

- On an idle map, issue dungeon StartFight and verify Snapshot AutoFight changes OFF -> ON.
- Issue StopFight and verify ON -> OFF.
- Confirm no command is treated as success before the snapshot changes.
- Confirm a mismatched PE/signature build is blocked without action.

## D. One non-loop run

- Start with AUTO TRAIN off.
- Verify NPC arrival, ClickNPC ResID and every entry click.
- Verify map transition freezes actions until stable, then exact dungeon MapID passes.
- Verify each stage travels only after the previous counter reaches its configured value.
- Verify boss death triggers AutoFight OFF before the exit sequence.
- Verify completion only after MapID differs from the dungeon MapID.

## E. Conflict and failure tests

- Start AUTO PHÓ BẢN while AUTO TRAIN runs; choose No, then Yes. Confirm one mode only.
- Start AUTO TRAIN while dungeon mode runs; choose No, then Yes. Confirm one mode only.
- Use an invalid NPC ResID, wrong map ID, missing click coordinate and impossible kill count. Each must stop/error, never advance.
- Move the mouse during entry/exit clicks. The exact click row/repeat must remain pending for the 5-second guard.
- Minimize/close the game window and simulate bridge timeout. No further mutable action may be sent.

Record client hash/build tuple, profile, scanner rows, counts and logs for every pass/fail.
