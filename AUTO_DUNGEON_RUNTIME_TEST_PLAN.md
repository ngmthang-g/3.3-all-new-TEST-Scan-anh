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

- While AUTO/AUTO PK is running, switch to AUTO PHÓ BẢN. Confirm both prior workflows STOP immediately before dungeon ownership is granted.
- Start AUTO TRAIN while dungeon mode runs; choose No, then Yes. Confirm one mode only.
- Use an invalid NPC ResID, wrong map ID, missing click coordinate and impossible kill count. Each must stop/error, never advance.
- Move the mouse during entry/exit clicks. The exact click row/repeat must remain pending for the 5-second guard.
- Minimize/close the game window and simulate bridge timeout. No further mutable action may be sent.

Record client hash/build tuple, profile, scanner rows, counts and logs for every pass/fail.


## F. v4.3 regressions

- Close/reopen three of six clients, press **QUÉT CLIENT**, confirm `3/6 -> 6/6` and a binding-only ERROR returns to STOP; an NPC/timeout ERROR must not auto-clear.
- Select both a KEY row and a member row, then START/PAUSE/STOP; both must resolve to the same team and never another visible row.
- Verify KEY rows have a light grey background and members render one-per-line indented below KEY.
- Open AUTO PHÓ BẢN and Telegram repeatedly: **ACC BÁO CÁO** must appear only in Telegram and never overlap dungeon STEP.
- With TASK API returning a Dictionary/value-type enumerator, verify `GetDoingTasks` can enumerate without boxed-this exception; if TASK still fails, GMonster fallback must continue and must not block AutoFight start.
