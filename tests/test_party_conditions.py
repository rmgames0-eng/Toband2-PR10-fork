"""Private effects, reserve spell targets, timed death, v5/v4 save migration."""
from pathlib import Path
import subprocess, tempfile, sys
ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build/party'
GCC = ROOT / 'tools/mingw32/bin/gcc.exe'
FLAGS = ['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h = (ROOT/'tests/party_integration.c').read_text(encoding='euc_jp')
extra = (ROOT/'tests/party_conditions.inc').read_text(encoding='utf-8')
h = h.replace('int main(int argc,char **argv)', extra+'\nint main(int argc,char **argv)',1)
# Use the existing birth/save fixture, then run the new specification tests.
h = h.replace('    display_regressions();', '    private_condition_regressions();\n    return 0;\n    display_regressions();',1)
with tempfile.TemporaryDirectory(prefix='toband-conditions-') as directory:
    tmp=Path(directory); source=tmp/'test.c'; source.write_bytes(h.encode('euc_jp'))
    objects=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    old=(ROOT/'src/save.c').read_bytes().decode('euc_jp').replace('wr_byte(PARTY_SAVE_VERSION);','wr_byte(4);')
    old=old.replace('    wr_u16b(MAX_CLASS); /* Party v6: serialized class count. */', '')
    start_party=old.index('static void wr_party(void)')
    end_party=old.index('\n}\n', start_party)+3
    old=old[:start_party]+old[start_party:end_party].replace('i < (count)', 'i < ((count) == MAX_CLASS ? 39 : ((count) == MAX_CLASS * PY_MAX_LEVEL ? 39 * PY_MAX_LEVEL : (count)))').replace('j < MAX_CLASS', 'j < 39')+old[end_party:]
    start=old.index('        /* Party v5:')
    end=old.index('#undef PARTY_EFFECT',start)+len('#undef PARTY_EFFECT')
    old=old[:start]+old[end:]
    oldc=tmp/'save_v4.c'; oldo=tmp/'save_v4.o'; oldc.write_bytes(old.encode('euc_jp'))
    subprocess.run([str(GCC),*FLAGS,'-c',str(oldc),'-o',str(oldo)],check=True)
    for legacy in ((False,) if "--current-only" in sys.argv else (False,True)):
        exe=tmp/'test.exe'
        linked=[oldo if legacy and p.stem=='save' else p for p in objects]
        subprocess.run([str(GCC),*FLAGS,*(['-DTEST_PARTY_V4'] if legacy else []),str(source),*map(str,linked),'-o',str(exe),'-Wl,--wrap=msg_print','-lwinmm','-lcomdlg32','-lgdi32'],check=True)
        result=subprocess.run([str(exe),(BUILD/'lib').as_posix()+'/'],cwd=tmp,capture_output=True,timeout=45)
        print(result.stdout.decode('cp932',errors='replace'),end='')
        if result.returncode:
            print('EXIT', result.returncode, result.stderr.decode('cp932',errors='replace'))
            raise SystemExit(result.returncode)
