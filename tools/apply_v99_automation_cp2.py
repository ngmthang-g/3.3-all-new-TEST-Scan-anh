from pathlib import Path
import runpy

runpy.run_path(str(Path(__file__).with_name('apply_v99_automation_cp2_impl.py')), run_name='__main__')
