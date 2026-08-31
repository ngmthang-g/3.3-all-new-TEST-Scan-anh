from pathlib import Path
import runpy

# Keep v2 as the stable visual/capture base, then generate v3 filter logic from it.
template = Path('test_scan/image_scan_test_v2.inl')
if not template.exists():
    raise SystemExit('missing test_scan/image_scan_test_v2.inl')

v2 = template.read_text(encoding='utf-8')
for needle in ['CHỌN VÙNG BẰNG CHUỘT', 'XEM VÙNG', 'FindTemplate(', 'PrintWindow(', 'RAW TryClickUI']:
    if needle not in v2:
        raise SystemExit(f'TEST SCAN v2 template missing: {needle}')

# Generate src/image_scan_filter_v3.inl from the proven v2 base.
runpy.run_path('test_scan/apply_filter_v3.py', run_name='__main__')
v3 = Path('src/image_scan_filter_v3.inl')
if not v3.exists():
    raise SystemExit('FILTER V3 generator did not create src/image_scan_filter_v3.inl')
impl = v3.read_text(encoding='utf-8')
required = [
    'kMaxClickSteps = 20',
    'ẢNH 1 • ĐỒ ĐÚNG',
    'NÚT VỨT • scan rồi click đúng tâm',
    'DẤU X • scan rồi click đúng tâm',
    'CLICK SAU VỨT',
    'LẶP LẠI CLICK',
    'GetAsyncKeyState(VK_ESCAPE)',
]
for needle in required:
    if needle not in impl:
        raise SystemExit(f'FILTER V3 missing: {needle}')

wrapper = '''#include "image_scan_test.h"\n\n// TEST FILTER v3 • proven HWND PrintWindow + ROI matcher + RAW hidden clicks.\n#include "image_scan_filter_v3.inl"\n'''
Path('src/image_scan_test.cpp').write_text(wrapper, encoding='utf-8')

print('TEST FILTER v3 finalize PASS • 20 slots • good/discard/X • retry same slot • RAW hidden click')
