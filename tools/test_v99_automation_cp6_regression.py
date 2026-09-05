from pathlib import Path
import subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[1]
cp6=ROOT/'tools'/'apply_v99_automation_cp6.py'
if not cp6.exists(): raise SystemExit('CP6 FAIL: missing apply_v99_automation_cp6.py')
with tempfile.TemporaryDirectory() as td:
    out=Path(td)
    steps=['generate_image_scan_v4_sources.py','apply_pre_close_x_patch.py','apply_ui30_controller_base.py','apply_ui30_controller_groups.py','apply_ui30_controller_runtime.py','apply_ui30_scanner.py','apply_v99_automation_cp1.py','apply_v99_automation_cp2.py','apply_v99_automation_cp3.py','apply_v99_automation_cp4.py','apply_v99_automation_cp5.py','apply_v99_automation_cp6.py']
    for s in steps:
        subprocess.run([sys.executable,str(ROOT/'tools'/s),'--source-root',str(ROOT),'--output-dir',str(out)],check=True,stdout=subprocess.DEVNULL)
    c=(out/'controller.cpp').read_text(encoding='utf-8-sig')
checks={
'gather auto fight state required':'ValidAutoFight' in c[c.find('void TickGatherAccount'):c.find('void UpdatePartyBuildToggleLabel')],
'gather explicit stop autofight':'TẬP TRUNG • tắt AutoFight trước khi rời điểm hiện tại' in c,
'gather underworld guard preserved':'HandleUnderworldAutoFightGuard(a, now)' in c[c.find('void TickGatherAccount'):c.find('void UpdatePartyBuildToggleLabel')],
'gather map-confirm priority preserved':'PriorityLauLanGateConfirmClick(a, priorityNow)' in c,
'gather uses robust travel':'HandleRobustTravel(a, now, gatherTarget_, L"bãi tập trung"' in c,
'gather starts fight at target':'HandleFightClicks(a, now)' in c[c.find('void TickGatherAccount'):c.find('void UpdatePartyBuildToggleLabel')],
'gather P3 allowed':'gatherModeActive_ && IsGatherPid(a.game.pid)' in c,
'party popup xong':'MessageBoxW(hwnd_, allPass ? L"XONG" : L"CHƯA XONG"' in c,
'party detail logged':'AUTO PT CHI TIẾT' in c,
'party no valid minimal':'MessageBoxW(hwnd_, L"CHƯA XONG", L"TỰ TẠO PT"' in c,
}
fail=[k for k,v in checks.items() if not v]
if fail: raise SystemExit('CP6 FAIL: '+'; '.join(fail))
print('V9.9 CP6 GATHER/PARTY POPUP REGRESSION: PASS')
