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


for _part in (
    "apply_v99_automation_cp4_part1.py",
    "apply_v99_automation_cp4_part2.py",
    "apply_v99_automation_cp4_part3.py",
    "apply_v99_automation_cp4_part4.py",
):
    _path = Path(__file__).with_name(_part)
    exec(compile(_path.read_text(encoding="utf-8"), str(_path), "exec"), globals(), globals())
