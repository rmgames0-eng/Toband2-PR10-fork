"""After build-mingw.py --build-dir build/party, exercise the real game core.

Runs real birth screens with scripted input and actual live/dead savefiles.
Also links the previous release's save writer to verify legacy migration.
All savefiles are isolated in temporary directories.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build/party'
GCC = ROOT / 'tools/mingw32/bin/gcc.exe'
FLAGS = ['-std=gnu89', '-O2', '-DWIN32', '-DJP', '-DSJIS',
         '-finput-charset=EUC-JP', '-fexec-charset=CP932', '-I', str(ROOT/'src')]
objects = [p for p in BUILD.glob('*.o') if p.stem not in ('main-win', 'angband')]
assert any(p.stem == 'party' for p in objects), 'Build the game in build/party first'
with tempfile.TemporaryDirectory(prefix='toband-party-test-') as tmp:
    tmp = Path(tmp)
    legacy = subprocess.check_output(['git', '-c', f'safe.directory={ROOT.as_posix()}',
                                      '-C', str(ROOT), 'show',
                                      '2fad94006a35c4f34a77e9e37d65805e93903a6a:src/save.c'])
    legacy = legacy.replace(b'wr_byte(T_VER_EXTRA);', b'wr_byte(0);')
    old_c, old_o = tmp/'legacy_save.c', tmp/'legacy_save.o'
    old_c.write_bytes(legacy)
    subprocess.run([str(GCC), *FLAGS, '-c', str(old_c), '-o', str(old_o)], check=True)
    v1_c, v1_o = tmp/'party_v1_save.c', tmp/'party_v1_save.o'
    v1_c.write_bytes((ROOT/'src/save.c').read_bytes().replace(b'        /* Party v5: individual ailments and temporary effects. */\n#define PARTY_EFFECT(type, name) wr_##type(member->player.name);\n#include "party-effects.h"\n#undef PARTY_EFFECT', b'').replace(b'        /* Party v4: private weapon timers and brands. */\n        wr_s16b(member->player.magical_weapon);\n        wr_s16b(member->player.evil_weapon);\n        wr_u32b(member->player.special_attack & PARTY_WEAPON_BRANDS);', b'').replace(b'wr_byte(PARTY_SAVE_VERSION);', b'wr_byte(1);').replace(b'        wr_byte(member->dead);', b'').replace(b'        wr_byte(member->revived);', b''))
    subprocess.run([str(GCC), *FLAGS, '-c', str(v1_c), '-o', str(v1_o)], check=True)
    for mode in ('current', 'legacy', 'party-v1'):
        old = mode == 'legacy'
        linked = [(old_o if old else v1_o) if mode != 'current' and p.stem == 'save' else p for p in objects]
        exe = tmp / ('legacy.exe' if old else 'party.exe')
        subprocess.run([str(GCC), *FLAGS, str(ROOT/'tests/party_integration.c'),
                        *map(str, linked), '-o', str(exe), '-lwinmm', '-lcomdlg32',
                        '-lgdi32'], check=True)
        result = subprocess.run([str(exe), (BUILD/'lib').as_posix()+'/',
                                 *([mode] if mode != 'current' else [])], cwd=tmp,
                                capture_output=True, timeout=30)
        print(result.stdout.decode('cp932', errors='replace'), end='')
        if result.returncode:
            print(result.stderr.decode('cp932', errors='replace'))
            raise SystemExit(f'Integration test failed: {result.returncode}')
