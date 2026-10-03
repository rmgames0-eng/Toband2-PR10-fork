"""Exercise terminal-class party limits against the real data and XP routines."""
from pathlib import Path
import subprocess, tempfile
ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT/'build/party'
GCC = ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS = ['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h = (ROOT/'tests/party_integration.c').read_text(encoding='euc_jp')
extra = (ROOT/'tests/party_level_cap.inc').read_text(encoding='utf-8')
h = h.replace('int main(int argc,char **argv)',extra+'\nint main(int argc,char **argv)',1)
h = h.replace('    display_regressions();','    party_level_cap_regressions();\n    return 0;\n    display_regressions();',1)
with tempfile.TemporaryDirectory(prefix='toband-level-cap-') as directory:
    tmp=Path(directory); source=tmp/'test.c'; source.write_bytes(h.encode('euc_jp'))
    objects=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    exe=tmp/'test.exe'
    subprocess.run([str(GCC),*FLAGS,str(source),*map(str,objects),'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(exe),(BUILD/'lib').as_posix()+'/'],cwd=tmp,capture_output=True,timeout=40)
    print(result.stdout.decode('cp932',errors='replace'),end='')
    if result.returncode:
        print('EXIT',result.returncode,result.stderr.decode('cp932',errors='replace'))
        raise SystemExit(result.returncode)
