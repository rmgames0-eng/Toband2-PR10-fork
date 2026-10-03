"""Check Brainstorm using actual projection and status processing."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build/party'
GCC=ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h=(ROOT/'tests/party_integration.c').read_text(encoding='euc_jp')
marker='    death_regressions();'
assert marker in h
h=h.replace(marker,r"""
    {
        player_type saved=*p_ptr;
        bool old_easy=easy_band;
        int caster, target, pass, mana_damage;
        monster_type *m;
        wipe_m_list();
        caster=m_pop(); m=&m_list[caster];
        m->r_idx=m->ap_r_idx=MON_DAGDA; m->fy=3; m->fx=3;
        m->hp=m->maxhp=m->max_maxhp=40000; cave[3][3].m_idx=caster;
        easy_band=FALSE;
        p_ptr->chp=p_ptr->mhp=3000;
        p_ptr->invuln=p_ptr->wraith_form=p_ptr->multishadow=0;
        p_ptr->infected=0; p_ptr->is_dead=0;
        p_ptr->update=p_ptr->redraw=p_ptr->window=0;
        p_ptr->confused=0;
        project(caster,4,py,px,500,GF_MANA,PROJECT_KILL|PROJECT_PLAYER|PROJECT_JUMP,MODIFY_ELEM_MODE_NONE);
        mana_damage=3000-p_ptr->chp;
        assert(mana_damage>0 && !p_ptr->confused);
        for(pass=0;pass<2;pass++) {
            p_ptr->chp=3000; p_ptr->confused=0; p_ptr->resist_conf=pass;
            project(caster,4,py,px,500,GF_BRAINSTORM,PROJECT_KILL|PROJECT_PLAYER|PROJECT_JUMP,MODIFY_ELEM_MODE_NONE);
            assert(3000-p_ptr->chp==mana_damage);
            assert(p_ptr->confused>=4 && p_ptr->confused<=7);
        }
        target=m_pop(); m=&m_list[target];
        m->r_idx=m->ap_r_idx=MON_DAGDA; m->fy=2; m->fx=3;
        m->hp=m->maxhp=m->max_maxhp=40000; cave[2][3].m_idx=target;
        project(caster,0,2,3,500,GF_BRAINSTORM,PROJECT_KILL|PROJECT_JUMP,MODIFY_ELEM_MODE_NONE);
        assert(m->hp<40000 && MON_CONFUSED(m)>0);
        wipe_m_list(); *p_ptr=saved; easy_band=old_easy;
        puts("Brainstorm: mana-equivalent damage and confusion bypass for player and monster passed");
    }
    death_regressions();
""",1)
# Both caster paths must use the same mana-storm formula and radius.
for n in (1,2):
    s=(ROOT/f'src/mspells{n}.c').read_text(encoding='euc_jp')
    a=s.index('case MON_DAGDA:'); b=s.index('break;',a);block=s[a:b]
    assert '(rlev * 4) + 50 + damroll(10, 10)' in block
    assert 'GF_BRAINSTORM, dam, 4, FALSE, FALSE' in block
with tempfile.TemporaryDirectory(prefix='toband-dagda-') as d:
    d=Path(d);c=d/'test.c';exe=d/'test.exe';c.write_bytes(h.encode('euc_jp'))
    objects=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    subprocess.run([str(GCC),*FLAGS,str(c),*map(str,objects),'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(exe),(BUILD/'lib').as_posix()+'/'],cwd=d,capture_output=True,timeout=30)
    print(result.stdout.decode('cp932',errors='replace'),end='')
    if result.returncode:
        print(result.stderr.decode('cp932',errors='replace'))
        raise SystemExit(result.returncode)
