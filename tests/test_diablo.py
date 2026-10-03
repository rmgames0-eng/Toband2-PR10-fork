"""Exercise Diablo's lethal spell and ogre summons against the real core."""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build/party'
GCC=ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h=(ROOT/'tests/party_integration.c').read_text(encoding='euc_jp')
h=h.replace('    take_hit(DAMAGE_LOSELIFE,1000,"death regression");', r"""
    /* Half-health boundary, odd max HP, and protection bypass. */
    p_ptr->mhp=100; p_ptr->chp=51;
    assert(!diablo_death("Diablo") && p_ptr->chp==51 && !p_ptr->is_dead);
    p_ptr->mhp=99; p_ptr->chp=50;
    assert(!diablo_death("Diablo") && p_ptr->chp==50);
    p_ptr->mhp=100; p_ptr->chp=50;
    easy_band=TRUE; p_ptr->invuln=100; p_ptr->wraith_form=100; p_ptr->multishadow=100;
    assert(diablo_death("Diablo"));
    assert(p_ptr->chp==-1 && p_ptr->is_dead && party_active==0);
    assert(!diablo_death("Diablo"));
    easy_band=FALSE; p_ptr->invuln=p_ptr->wraith_form=p_ptr->multishadow=0;
""",1)
# Run actual summon selection on a cleared floor, before the death tests.
marker='    death_regressions();'
assert marker in h
h=h.replace(marker,r"""
    {
        int attempt, count=0;
        wipe_m_list();
        for(attempt=0;attempt<40;attempt++)
            summon_specific(0,2,2,125,SUMMON_OGRES,PM_ALLOW_GROUP);
        for(attempt=1;attempt<m_max;attempt++) if(m_list[attempt].r_idx) {
            monster_race *r=&r_info[m_list[attempt].r_idx];
            assert(r->d_char=='O' && r->level>=60);
            assert(!(r->flags1&RF1_UNIQUE) && !(r->flags7&RF7_UNIQUE2));
            count++;
        }
        assert(count>=2);
        wipe_m_list();
    }
    death_regressions();
    puts("Diablo: ogre-only summons, HP threshold, protection bypass and deferred party death passed");
""",1)
for n in (1,2):
    s=(ROOT/f'src/mspells{n}.c').read_text(encoding='euc_jp')
    block=s[s.index('/* RF6_SUMMON_KIN */'):]
    block=block[block.index('case MON_DAGDA:'):block.index('case MON_THORONDOR:')]
    assert all('case '+name+':' in block for name in ('MON_DAGDA','MON_ASMODE','MON_DIABLO','MON_DEMUNZA'))
    assert 'k < 4' in block and 'SUMMON_OGRES' in block
with tempfile.TemporaryDirectory(prefix='toband-diablo-') as d:
    d=Path(d); c=d/'test.c'; exe=d/'test.exe'; c.write_bytes(h.encode('euc_jp'))
    objects=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    subprocess.run([str(GCC),*FLAGS,str(c),*map(str,objects),'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(exe),(BUILD/'lib').as_posix()+'/'],cwd=d,capture_output=True,timeout=30)
    print(result.stdout.decode('cp932',errors='replace'),end='')
    if result.returncode:
        print(result.stderr.decode('cp932',errors='replace'))
        raise SystemExit(result.returncode)
