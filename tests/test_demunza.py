"""Load Demunza through the real parser and verify spawned HP above 65535."""
from pathlib import Path
import subprocess, tempfile, shutil
ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build/party'
h = (ROOT/'tests/party_integration.c').read_bytes().decode('euc_jp')
h = h.replace('    init_angband();', r"""
    init_angband();
    {
        monster_race *r = &r_info[988];
        monster_type *m;
        assert(max_r_idx > 988 && r->level == 127);
        assert(r->hdice == 246 && r->hside == 271);
        assert(r->flags1 & RF1_FORCE_MAXHP);
        assert(r->flags2 & RF2_AURA_FIRE);
        assert(!(r->flags1 & RF1_QUESTOR));
        assert(r->blow[0].effect == RBE_HELL && r->blow[2].effect == RBE_SHATTER);
        r->max_num = 1; r->cur_num = 0; m_max = 1;
        for (i = 0; i < 3; ++i) assert(r->blow[i].d_dice == 25 && r->blow[i].d_side == 15);
        assert(r->blow[1].effect == RBE_UN_BONUS);
        assert(r->blow[3].effect == RBE_FIRE && r->blow[3].d_dice == 50 && r->blow[3].d_side == 50);
        cur_hgt = cur_wid = 7; py = px = 1;
        dungeon_type = DUNGEON_DEMON; dun_level = 1000;
        for (i = 0; i < 7; ++i) {
            int x;
            for (x = 0; x < 7; ++x) cave[i][x].feat = FEAT_FLOOR;
        }
        assert(place_monster_aux(0, 3, 3, 988, PM_IGNORE_TERRAIN | PM_IGNORE_AMGRID));
        m = &m_list[cave[3][3].m_idx];
        assert(m->hp == 66666 && m->maxhp == 66666 && m->max_maxhp == 66666);
        assert(!place_monster_aux(0, 4, 4, 988, PM_IGNORE_TERRAIN | PM_IGNORE_AMGRID));
        puts("Demunza: parsed, level 127, real spawn HP 66666 and unique restriction passed");
    }
    return 0;
""", 1)
with tempfile.TemporaryDirectory(prefix='toband-demunza-') as directory:
    tmp = Path(directory)
    shutil.copytree(BUILD/'lib', tmp/'lib', ignore=shutil.ignore_patterns('*.raw', 'save'))
    for name in ('r_info.txt', 'misc.txt', 'd_info.txt'):
        (tmp/'lib/edit'/name).write_bytes((ROOT/'lib/edit'/name).read_bytes().decode('euc_jp').encode('cp932'))
    (tmp/'lib/save').mkdir(exist_ok=True)
    c = tmp/'test.c'; c.write_bytes(h.encode('euc_jp'))
    exe = tmp/'test.exe'
    flags = ['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
    objects = [str(p) for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    subprocess.run([str(ROOT/'tools/mingw32/bin/gcc.exe'),*flags,str(c),*objects,'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result = subprocess.run([str(exe),(tmp/'lib').as_posix()+'/'],cwd=tmp,capture_output=True,timeout=45)
    print(result.stdout.decode('cp932',errors='replace'),end='')
    if result.returncode:
        print(result.stderr.decode('cp932',errors='replace'))
        raise SystemExit(result.returncode)
