"""Check Relics Knight wall excavation through real movement."""
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
    death_regressions();
    {
        int kind, feats[3]={FEAT_WALL_EXTRA,FEAT_MAGMA,FEAT_QUARTZ};
        wipe_m_list(); wipe_o_list();
        p_ptr->pclass=CLASS_RELICSKNIGHT; p_ptr->prace=RACE_GOBLIN;
        p_ptr->lev=p_ptr->max_plv=10;
        p_ptr->cexp_info[CLASS_RELICSKNIGHT].clev=10;
        p_ptr->cexp_info[CLASS_RELICSKNIGHT].max_clev=10;
        p_ptr->riding=0; p_ptr->wraith_form=0; p_ptr->wild_mode=FALSE;
        p_ptr->confused=p_ptr->stun=p_ptr->blind=0;
        p_ptr->inside_arena=FALSE; p_ptr->inside_quest=0;
        p_ptr->is_dead=0; p_ptr->playing=TRUE;
        dun_level=1;
        party_rebind(); calc_bonuses();
        assert(p_ptr->kill_wall && p_ptr->pass_wall);
        for(kind=0;kind<3;kind++) {
            py=2; px=2; cave[2][2].feat=FEAT_FLOOR;
            cave[2][3].feat=feats[kind]; cave[2][3].info=CAVE_MARK;
            move_player(6,FALSE);
            assert(py==2 && px==3 && cave_floor_bold(2,3));
        }
        {
            int trial, success=0, failure=0, expected;
            for(trial=0;trial<200;trial++) {
                py=2; px=2;
                cave[2][3].feat=FEAT_PERM_EXTRA + trial % 4;
                cave[2][3].info=CAVE_MARK;
                Rand_quick=TRUE; Rand_value=12345+7919*trial;
                expected=one_in_(20);
                Rand_value=12345+7919*trial;
                energy_use=0; msg_flag=FALSE;
                move_player(6,FALSE);
                assert(energy_use==100);
                if(expected) {
                    assert(px==3 && cave_floor_bold(2,3)); success++;
                } else {
                    assert(px==2 && cave[2][3].feat==FEAT_PERM_EXTRA+trial%4); failure++;
                }
            }
            assert(success>0 && failure>0);
            py=2; px=1; cave[2][0].feat=FEAT_PERM_SOLID;
            cave[2][0].info=CAVE_MARK;
            for(trial=0;trial<100;trial++) {
                msg_flag=FALSE; move_player(4,FALSE);
                assert(px==1 && cave[2][0].feat==FEAT_PERM_SOLID);
            }
            py=2; px=2;
        }
        p_ptr->pclass=CLASS_SOLDIER;
        p_ptr->cexp_info[CLASS_SOLDIER].clev=p_ptr->cexp_info[CLASS_SOLDIER].max_clev=10;
        party_rebind(); calc_bonuses();
        assert(!p_ptr->kill_wall && !p_ptr->pass_wall);
        cave[2][3].feat=FEAT_WALL_EXTRA;
        msg_flag=FALSE; move_player(6,FALSE);
        assert(px==2 && cave[2][3].feat==FEAT_WALL_EXTRA);
        puts("Relics Knight: walls excavated; interior permanent walls use 1-in-20 attempts and consume turns; borders protected");
    }
""",1)
with tempfile.TemporaryDirectory(prefix='toband-relics-wall-') as d:
    d=Path(d);c=d/'test.c';exe=d/'test.exe';c.write_bytes(h.encode('euc_jp'))
    objects=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    subprocess.run([str(GCC),*FLAGS,str(c),*map(str,objects),'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(exe),(BUILD/'lib').as_posix()+'/'],cwd=d,capture_output=True,timeout=30)
    print(result.stdout.decode('cp932',errors='replace'),end='')
    if result.returncode:
        print(result.stderr.decode('cp932',errors='replace'))
        raise SystemExit(result.returncode)
