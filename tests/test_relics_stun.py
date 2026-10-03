"""Check Relics Knight impact with production logic and monster status processing."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build/party'
GCC=ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h=(ROOT/'tests/party_integration.c').read_text(encoding='euc_jp')
marker='    death_regressions();'
assert marker in h
source=(ROOT/'src/cmd1.c').read_text(encoding='euc_jp')
a=source.index('static void relics_knight_stun(')
b=source.index('\n}\n',a)+3
h=h.replace('static void death_regressions(void)',source[a:b]+'\nstatic void death_regressions(void)',1)
assert 'relics_knight_stun(c_ptr->m_idx, k);' in source
h=h.replace(marker,r"""
    death_regressions();
    {
        player_type saved=*p_ptr;
        monster_race saved_race=r_info[1];
        int idx, trial, hits[3]={0,0,0}, pass, old_stun;
        monster_type *m;
        wipe_m_list();
        idx=m_pop(); m=&m_list[idx];
        m->r_idx=m->ap_r_idx=1; m->fy=2; m->fx=3;
        m->hp=m->maxhp=m->max_maxhp=1000; cave[2][3].m_idx=idx;
        r_info[1].level=10; r_info[1].flags1=0; r_info[1].flags3=0; r_info[1].flags7=0;
        p_ptr->pclass=CLASS_RELICSKNIGHT; p_ptr->lev=50;
        for(pass=0;pass<3;pass++) {
            r_info[1].flags7=pass==2 ? RF7_CHAOTIC : 0;
            r_info[1].flags3=pass==1 ? RF3_NO_STUN|RF3_NO_CONF|RF3_NO_SLEEP|RF3_UNDEAD : 0;
            r_info[1].flags1=pass==1 ? RF1_UNIQUE : 0;
            for(trial=0;trial<256;trial++) {
                set_monster_stunned(idx,0);
                Rand_quick=TRUE; Rand_value=trial*7919+12345;
                msg_flag=FALSE;
                relics_knight_stun(idx,1);
                if(MON_STUNNED(m)) {
                    hits[pass]++;
                    assert(MON_STUNNED(m)>=21 && MON_STUNNED(m)<=35);
                }
            }
        }
        assert(hits[0]==256 && hits[1]>0 && hits[1]<hits[0] && hits[2]==0);
        r_info[1].flags7=r_info[1].flags3=r_info[1].flags1=0;
        set_monster_stunned(idx,0);
        relics_knight_stun(idx,0); assert(!MON_STUNNED(m));
        relics_knight_stun(idx,1000); assert(!MON_STUNNED(m));
        p_ptr->pclass=CLASS_SOLDIER;
        relics_knight_stun(idx,1); assert(!MON_STUNNED(m));
        p_ptr->pclass=CLASS_RELICSKNIGHT;
        msg_flag=FALSE; relics_knight_stun(idx,1); old_stun=MON_STUNNED(m);
        msg_flag=FALSE; relics_knight_stun(idx,1); assert(MON_STUNNED(m)>old_stun);
        msg_flag=FALSE;
        wipe_m_list(); r_info[1]=saved_race; *p_ptr=saved; party_rebind();
        puts("Relics stun: chaos exclusion, martial resistance, duration, stacking and harmless/lethal hits passed");
    }
""",1)
with tempfile.TemporaryDirectory(prefix='toband-relics-stun-') as d:
    d=Path(d);c=d/'test.c';exe=d/'test.exe';c.write_bytes(h.encode('euc_jp'))
    objects=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    subprocess.run([str(GCC),*FLAGS,str(c),*map(str,objects),'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(exe),(BUILD/'lib').as_posix()+'/'],cwd=d,capture_output=True,timeout=30)
    print(result.stdout.decode('cp932',errors='replace'),end='')
    if result.returncode:
        print(result.stderr.decode('cp932',errors='replace'))
        raise SystemExit(result.returncode)
