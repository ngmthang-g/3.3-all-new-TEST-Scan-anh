from pathlib import Path

PATH = Path('src/dungeon_app_methods.inl')
text = PATH.read_text(encoding='utf-8-sig')

bad = """            if (team.leaderRoleID <= 0) {
                Account* leader = DungeonAccountByRoleID(team.leaderRoleID);
                if (leader && leader->snapshotValid && (leader->snapshot.validMask & ValidIdentity))
                    team.leaderRoleID = leader->snapshot.roleID;
            }
"""
good = """            if (team.leaderRoleID <= 0) {
                // Legacy-config fallback only: RoleID is missing, so recover it once
                // from the cached leader PID. Runtime scheduling remains RoleID-first.
                Account* leader = AccountByPid(team.config.leaderPid);
                if (leader && leader->snapshotValid && (leader->snapshot.validMask & ValidIdentity))
                    team.leaderRoleID = leader->snapshot.roleID;
            }
"""

if bad in text:
    text = text.replace(bad, good, 1)
elif good not in text:
    raise SystemExit('SaveDungeonTeams leader persistence block not found')

PATH.write_text(text, encoding='utf-8')
print('v4.9 persistence hotfix applied')
