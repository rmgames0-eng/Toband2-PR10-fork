"""Exercise reserve spell casting with real books, magic and save files."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build/party'
GCC=ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h=(ROOT/'tests/party_integration.c').read_text(encoding='euc_jp')
extra=(ROOT/'tests/party_weapon.inc').read_text(encoding='utf-8')+'\n'+(ROOT/'tests/party_magic.inc').read_text(encoding='utf-8')
h=h.replace('int main(int argc,char **argv)',extra+'\nint main(int argc,char **argv)',1)
h=h.replace('    death_regressions();','    death_regressions();\n    reserve_magic_regressions();',1)
with tempfile.TemporaryDirectory(prefix='toband-party-magic-') as d:
    d=Path(d);c=d/'test.c';exe=d/'test.exe';c.write_bytes(h.encode('euc_jp'))
    objects=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    # Exercise the previous writer as well as the current one.
    old=(ROOT/'src/save.c').read_bytes().decode('euc_jp')
    old=old.replace('wr_byte(PARTY_SAVE_VERSION);','wr_byte(3);')
    old=old.replace('    wr_u16b(MAX_CLASS); /* Party v6: serialized class count. */', '')
    start_party=old.index('static void wr_party(void)')
    end_party=old.index('\n}\n', start_party)+3
    old=old[:start_party]+old[start_party:end_party].replace('i < (count)', 'i < ((count) == MAX_CLASS ? 39 : ((count) == MAX_CLASS * PY_MAX_LEVEL ? 39 * PY_MAX_LEVEL : (count)))').replace('j < MAX_CLASS', 'j < 39')+old[end_party:]
    start=old.index('        /* Party v5:')
    end=old.index('#undef PARTY_EFFECT',start)+len('#undef PARTY_EFFECT')
    old=old[:start]+old[end:]
    old=old.replace('        /* Party v4: private weapon timers and brands. */\n        wr_s16b(member->player.magical_weapon);\n        wr_s16b(member->player.evil_weapon);\n        wr_u32b(member->player.special_attack & PARTY_WEAPON_BRANDS);','')
    old_c=d/'save_v3.c'; old_o=d/'save_v3.o'; old_c.write_bytes(old.encode('euc_jp'))
    subprocess.run([str(GCC),*FLAGS,'-c',str(old_c),'-o',str(old_o)],check=True)
    for legacy in (False,True):
        linked=[old_o if legacy and p.stem=='save' else p for p in objects]
        subprocess.run([str(GCC),*FLAGS,*(['-DTEST_PARTY_V3'] if legacy else []),str(c),*map(str,linked),'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
        result=subprocess.run([str(exe),(BUILD/'lib').as_posix()+'/'],cwd=d,capture_output=True,timeout=30)
        print(result.stdout.decode('cp932',errors='replace'),end='')
        if result.returncode:
            print(result.stderr.decode('cp932',errors='replace'))
            raise SystemExit(result.returncode)
