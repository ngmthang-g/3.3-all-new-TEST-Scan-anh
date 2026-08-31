from pathlib import Path

path = Path('src/dungeon_v45_methods.inl')
text = path.read_text(encoding='utf-8-sig')
marker = 'activityCacheKey'
if marker in text:
    print('v4.7 Activity cache hotfix already applied; nothing to do')
    raise SystemExit(0)

old = '''        static std::map<int, CachedActivity> lastGood;

        auto render = [&](const DungeonActivitySnapshot& board, bool cached, const std::wstring& extra) {'''
new = '''        // Scope last-good data to one concrete team + queue entry + run + dungeon map.
        // This keeps a disappearing objective visible during the SAME run, but never leaks
        // stale counters into the next run or a different dungeon selected by the same team.
        const std::wstring activityCacheKey = std::to_wstring(team.config.id) + L"|" + preset->id + L"|" +
            std::to_wstring(team.queueIndex) + L"|" + std::to_wstring(team.queueRunIndex) + L"|" +
            std::to_wstring(preset->dungeonMap);
        static std::map<std::wstring, CachedActivity> lastGood;

        auto render = [&](const DungeonActivitySnapshot& board, bool cached, const std::wstring& extra) {'''
if old not in text:
    raise SystemExit('Activity cache declaration anchor not found')
text = text.replace(old, new, 1)
text = text.replace('lastGood[team.config.id].board = response.dungeonActivity;',
                    'lastGood[activityCacheKey].board = response.dungeonActivity;', 1)
text = text.replace('lastGood[team.config.id].valid = true;',
                    'lastGood[activityCacheKey].valid = true;', 1)
text = text.replace('const auto it = lastGood.find(team.config.id);',
                    'const auto it = lastGood.find(activityCacheKey);', 1)
path.write_text(text, encoding='utf-8')
print('v4.7 Activity cache hotfix applied')
