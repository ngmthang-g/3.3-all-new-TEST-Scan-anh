from pathlib import Path
import argparse

ap = argparse.ArgumentParser()
ap.add_argument('--source-root', required=True)
ap.add_argument('--output-dir', required=True)
a = ap.parse_args()
out = Path(a.output_dir)


def load(path: Path):
    raw = path.read_bytes()
    text = raw.decode('utf-8-sig')
    nl = '\r\n' if '\r\n' in text else '\n'
    return text.replace('\r\n', '\n').replace('\r', '\n'), nl


def save(path: Path, text: str, nl: str):
    path.write_bytes(text.replace('\n', nl).encode('utf-8'))


def once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f'{label}: expected one marker, found {count}')
    return text.replace(old, new, 1)


p = out / 'controller.cpp'
t, nl = load(p)

# CP5 hardening: an invite semantic miss can mean the player popup never opened or
# disappeared. Re-open the popup with the already configured MEMBER FACE hidden click
# before trying the exact semantic again, matching the proven trade-menu pattern.
t = once(t,
'''            key->runtime.status = L"AUTO PT • MEMBER FACE PASS • chờ menu Mời vào nhóm";\n            s.phase = PartyBuildPhase::MemberInvite; s.phaseStartedTick = now; s.nextTick = now + static_cast<DWORD>(partyBuildSettings_.delaysMs[2]);\n            s.inviteAttempts = 0;\n            return;\n''',
'''            key->runtime.status = L"AUTO PT • MEMBER FACE PASS • chờ menu Mời vào nhóm";\n            s.phase = PartyBuildPhase::MemberInvite; s.phaseStartedTick = now; s.nextTick = now + static_cast<DWORD>(partyBuildSettings_.delaysMs[2]);\n            // Do not reset inviteAttempts here: retry path deliberately re-enters MemberFace\n            // so each semantic miss gets a fresh popup-open click without losing the retry budget.\n            return;\n''', 'cp5 preserve invite retry budget across popup reopen')

t = once(t,
'''            key->runtime.status = L"AUTO PT • chờ Mời vào nhóm • retry " + std::to_wstring(s.inviteAttempts);\n            s.nextTick = now + 350;\n            return;\n''',
'''            key->runtime.status = L"AUTO PT • retry mở lại menu member • lần " + std::to_wstring(s.inviteAttempts);\n            s.phase = PartyBuildPhase::MemberFace;\n            s.phaseStartedTick = now;\n            s.nextTick = now + 350;\n            return;\n''', 'cp5 invite retry reopens member popup')

save(p, t, nl)
print('apply_v99_automation_cp5.py: PASS')
