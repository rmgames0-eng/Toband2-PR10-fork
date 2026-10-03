"""Exercise production cooperative attacks, aura damage and derived class bonuses."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT/'build/party'
GCC = ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS = ['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h = (ROOT/'tests/party_integration.c').read_text(encoding='euc_jp')
h = h.replace('int main(int argc,char **argv)', (ROOT/'tests/party_cooperation.inc').read_text(encoding='utf-8')+'\nint main(int argc,char **argv)', 1)
h = h.replace('    init_angband();', '    init_angband();\n    cooperation_tests();\n    return 0;', 1)
combat = (ROOT/'src/cmd1.c').read_text(encoding='euc_jp')
combat = combat.replace('static bool party_attack_once(', 'bool party_attack_once(', 1)
combat = combat.replace('static void py_attack_aux(', 'int test_attack_attempts;\nstatic void py_attack_aux(', 1)
combat = combat.replace('while ((num++ < num_blow) && !p_ptr->is_dead)\n\t{', 'while ((num++ < num_blow) && !p_ptr->is_dead)\n\t{\n        test_attack_attempts++;', 1)
with tempfile.TemporaryDirectory(prefix='toband-cooperation-') as directory:
    tmp = Path(directory)
    (tmp/'test.c').write_bytes(h.encode('euc_jp'))
    (tmp/'cmd1.c').write_bytes(combat.encode('euc_jp'))
    display = (ROOT/'src/files.c').read_text(encoding='euc_jp').replace('static int player_average_hit(', 'int player_average_hit(', 1)
    (tmp/'files.c').write_bytes(display.encode('euc_jp'))
    objects = [p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband','cmd1','files')]
    exe = tmp/'test.exe'
    subprocess.run([str(GCC),*FLAGS,str(tmp/'test.c'),str(tmp/'cmd1.c'),str(tmp/'files.c'),*map(str,objects),'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result = subprocess.run([str(exe),(BUILD/'lib').as_posix()+'/'],cwd=tmp,capture_output=True,timeout=40)
    print(result.stdout.decode('cp932',errors='replace'),end='')
    if result.returncode:
        print(result.stderr.decode('cp932',errors='replace'))
        raise SystemExit(result.returncode)
