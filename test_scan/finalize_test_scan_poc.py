from pathlib import Path
import runpy

# Keep v2 as the proven visual/capture base, generate v3 compatibility first,
# then derive v4 optimized runtime from v3 without touching AUTO core/bridge.
template = Path('test_scan/image_scan_test_v2.inl')
if not template.exists():
    raise SystemExit('missing test_scan/image_scan_test_v2.inl')

v2 = template.read_text(encoding='utf-8')
for needle in ['CHỌN VÙNG BẰNG CHUỘT', 'XEM VÙNG', 'FindTemplate(', 'PrintWindow(', 'RAW TryClickUI']:
    if needle not in v2:
        raise SystemExit(f'TEST SCAN v2 template missing: {needle}')

runpy.run_path('test_scan/apply_filter_v3.py', run_name='__main__')
v3 = Path('src/image_scan_filter_v3.inl')
if not v3.exists():
    raise SystemExit('FILTER V3 generator did not create src/image_scan_filter_v3.inl')

runpy.run_path('test_scan/apply_filter_v4.py', run_name='__main__')
v4 = Path('src/image_scan_filter_v4.inl')
if not v4.exists():
    raise SystemExit('FILTER V4 generator did not create src/image_scan_filter_v4.inl')
impl = v4.read_text(encoding='utf-8')

required = [
    'kMaxClickSteps = 20',
    'kRunTimer = 92',
    'ẢNH 1 • ĐỒ ĐÚNG',
    'ROI VỨT',
    'ROI DẤU X',
    'CLICK SAU VỨT',
    'RunPhase::WaitItemReady',
    'ProcessRunTick',
    'MỖI PROBE CHỈ CHỤP HWND ĐÚNG 1 LẦN',
    'ScanGoodOnFrame',
    'ScanDiscardOnFrame',
    'ScanCloseOnFrame',
]
for needle in required:
    if needle not in impl:
        raise SystemExit(f'FILTER V4 missing: {needle}')
if 'Sleep(' in impl:
    raise SystemExit('FILTER V4 must not contain blocking Sleep()')

wrapper = '''#include "image_scan_test.h"\n\n// TEST FILTER v4 • 1 PrintWindow/probe + 3 independent ROI + non-blocking timer/state.\n#include "image_scan_filter_v4.inl"\n'''
Path('src/image_scan_test.cpp').write_text(wrapper, encoding='utf-8')

print('TEST FILTER v4 finalize PASS • 20 slots • 1 capture/probe • 3 ROI • adaptive timer/state • no Sleep')
