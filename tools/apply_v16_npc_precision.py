from pathlib import Path

root = Path(__file__).resolve().parents[1]
path = root / "src" / "controller.cpp"
text = path.read_text(encoding="utf-8")
old = 'HandleRobustTravel(a,now,step.target,L"NPC Đỗ Thanh Đằng ResID 339",arrived,step.tolerance)'
new = 'HandleRobustTravel(a,now,step.target,L"NPC Đỗ Thanh Đằng ResID 339",arrived,kPreciseWorldTolerance)'

if new in text:
    print("v1.6 treatment NPC precision already applied")
elif old in text:
    if text.count(old) != 1:
        raise RuntimeError(f"expected one treatment NPC travel call, got {text.count(old)}")
    text = text.replace(old, new, 1)
    path.write_text(text, encoding="utf-8")
    print("v1.6 treatment NPC travel tolerance set to 20")
else:
    raise RuntimeError("treatment NPC travel call not found; refusing blind patch")
