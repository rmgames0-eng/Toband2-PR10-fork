"""Check deferred item descriptions in the real screen renderer (optimized build)."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build/party'
GCC = ROOT / 'tools/mingw32/bin/gcc.exe'
FLAGS = ['-std=gnu89', '-O2', '-DWIN32', '-DJP', '-DSJIS',
         '-finput-charset=EUC-JP', '-fexec-charset=CP932', '-I', str(ROOT / 'src')]
h = (ROOT / 'tests/party_integration.c').read_text(encoding='euc_jp')
h = h.replace('static term test_term;', 'static term test_term;\nstatic int description_seen;')
h = h.replace('        if (death_input) {', r'''
        {
            int y, count = 0;
            char row[81];
            for (y = 1; y < 24; y++) {
                memcpy(row, Term->scr->c[y], 80); row[80] = 0;
                if (strstr(row, "武器タイプ: ")) count++;
            }
            assert(count == 1);
            description_seen++;
            Term_keypress(' ');
            return 0;
        }
        if (death_input) {''', 1)
h = h.replace('    init_angband();', r'''
    init_angband();
    {
        object_type o;
        object_prep(&o, lookup_kind(TV_DIGGING, SV_PICK));
        o.ident = IDENT_KNOWN | IDENT_MENTAL;
        assert(screen_object(&o, NULL, TRUE));
        assert(screen_object(&o, NULL, FALSE));
        assert(description_seen == 2);
        puts("Item description: weapon type survives deferred rendering in both display modes");
        return 0;
    }''', 1)
with tempfile.TemporaryDirectory(prefix='toband-description-') as directory:
    tmp = Path(directory)
    source = tmp / 'test.c'
    source.write_bytes(h.encode('euc_jp'))
    objects = [p for p in BUILD.glob('*.o') if p.stem not in ('main-win', 'angband')]
    exe = tmp / 'test.exe'
    subprocess.run([str(GCC), *FLAGS, str(source), *map(str, objects), '-o', str(exe),
                    '-lwinmm', '-lcomdlg32', '-lgdi32'], check=True)
    result = subprocess.run([str(exe), (BUILD / 'lib').as_posix() + '/'],
                            cwd=tmp, capture_output=True, timeout=30)
    print(result.stdout.decode('cp932', errors='replace'), end='')
    if result.returncode:
        print(result.stderr.decode('cp932', errors='replace'))
        raise SystemExit(result.returncode)
