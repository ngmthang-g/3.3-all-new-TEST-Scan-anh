from pathlib import Path

p = Path('tools/generate_image_scan_v4_sources.py')
text = p.read_text(encoding='utf-8-sig')
old = """p = once(p,
'''    ClickTravelSemantic = 23,\\n    ConfirmTravelSemantic = 24,\\n};''',
'''    ClickTravelSemantic = 23,\\n    ConfirmTravelSemantic = 24,\\n    // FILTER V4 only: image recognition -> raw InputSync click, independent of AUTO state.\\n    ClickInternalPointRawTest = 25,\\n};''', 'protocol command')"""
new = """p = once(p,
'''    ClickTravelSemantic = 23,\\n    ConfirmTravelSemantic = 24,\\n    TestOpenBag = 25,\\n};''',
'''    ClickTravelSemantic = 23,\\n    ConfirmTravelSemantic = 24,\\n    TestOpenBag = 25,\\n    // FILTER V4 only: image recognition -> raw InputSync click, independent of AUTO state.\\n    ClickInternalPointRawTest = 26,\\n};''', 'protocol command')"""
count = text.count(old)
if count != 1:
    raise SystemExit(f'generator compatibility marker expected 1, found {count}')
text = text.replace(old, new, 1)
p.write_text(text, encoding='utf-8', newline='\n')
print('TEST OPEN BAG generator compatibility PASS')
