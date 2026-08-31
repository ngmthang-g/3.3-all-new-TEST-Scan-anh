from pathlib import Path
import base64, gzip, re

ROOT = Path(__file__).resolve().parents[1]
PARTS = ROOT / 'tools' / 'v44_patch_parts'
DELETE = {'src/dungeon_progress_logic.h','src/dungeon_progress_logic_test.cpp'}

def apply_unified(text):
    lines=text.splitlines()
    i=0
    while i < len(lines):
        if not lines[i].startswith('--- a/'):
            i += 1
            continue
        old_path=lines[i][6:]
        if i+1>=len(lines) or not lines[i+1].startswith('+++ b/'):
            raise RuntimeError('bad patch header '+lines[i])
        new_path=lines[i+1][6:]
        if old_path != new_path:
            raise RuntimeError('rename not supported '+old_path+' -> '+new_path)
        path=ROOT/old_path
        old=path.read_text(encoding='utf-8').splitlines() if path.exists() else []
        out=[]; cursor=0; i += 2
        while i < len(lines) and not lines[i].startswith('--- a/'):
            if not lines[i].startswith('@@ '):
                i += 1
                continue
            m=re.match(r'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@', lines[i])
            if not m: raise RuntimeError('bad hunk '+lines[i])
            old_start=int(m.group(1)); target=max(0,old_start-1)
            if target < cursor: raise RuntimeError('overlap '+old_path)
            out.extend(old[cursor:target]); cursor=target; i += 1
            while i < len(lines) and not lines[i].startswith('@@ ') and not lines[i].startswith('--- a/'):
                pl=lines[i]
                if not pl: raise RuntimeError('empty patch line in '+old_path)
                tag=pl[0]; val=pl[1:]
                if tag == ' ':
                    if cursor>=len(old) or old[cursor] != val: raise RuntimeError(f'context mismatch {old_path}:{cursor+1}')
                    out.append(old[cursor]); cursor += 1
                elif tag == '-':
                    if cursor>=len(old) or old[cursor] != val: raise RuntimeError(f'delete mismatch {old_path}:{cursor+1}')
                    cursor += 1
                elif tag == '+': out.append(val)
                elif tag == '\\': pass
                else: raise RuntimeError('bad patch tag '+tag)
                i += 1
        if old_path in DELETE:
            path.unlink(missing_ok=True); print('v4.4 delete', old_path)
        else:
            out.extend(old[cursor:])
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('\n'.join(out)+'\n', encoding='utf-8')
            print('v4.4 patch', old_path)

def main():
    encoded=''.join(p.read_text(encoding='ascii').strip() for p in sorted(PARTS.glob('part*.txt')))
    if not encoded: raise RuntimeError('v4.4 patch parts missing')
    patch=gzip.decompress(base64.b64decode(encoded)).decode('utf-8')
    apply_unified(patch)
    print('v4.4 dungeon presence integration applied')

if __name__ == '__main__': main()
