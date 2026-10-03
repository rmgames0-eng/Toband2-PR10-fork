"""Heaven quest placement and real old/new save round trips."""
from pathlib import Path
import subprocess,tempfile,shutil
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build/party';GCC=ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h=(ROOT/'tests/party_integration.c').read_bytes().decode('euc_jp')
extra=r"""
static void heaven_tests(void)
{
    int i, pass, k, old_inside=p_ptr->inside_quest;
    int old_depths[]={1000,1024,1025,1050,1075,1100};
    int new_depths[]={950,969,970,980,990,1000};
    assert(d_info[DUNGEON_HEAVEN].mindepth==950);
    assert(d_info[DUNGEON_HEAVEN].maxdepth==1000);
    for(i=QUEST_FELLANA;i<=QUEST_FILARHH;i++) {
        init_flags=INIT_ASSIGN; p_ptr->inside_quest=i;
        assert(!process_dungeon_file("q_info.txt",0,0,0,0));
        assert(quest[i].level==970+(i-QUEST_FELLANA)*10);
        assert(quest[i].r_idx==922+i-QUEST_FELLANA);
        quest[i].status=QUEST_STATUS_TAKEN;
    }
    p_ptr->inside_quest=old_inside;
    dungeon_type=DUNGEON_HEAVEN;
    for(i=QUEST_FELLANA;i<=QUEST_FILARHH;i++)
        assert(quest_number(970+(i-QUEST_FELLANA)*10)==i);
    assert(!quest_number(975) && !quest_number(985) && !quest_number(995));
    character_generated=FALSE;
    p_ptr->is_dead=0; p_ptr->chp=100; p_ptr->mhp=200;
    for(pass=0;pass<2;pass++) for(k=0;k<6;k++) {
        saved_floor_type *sf;
        s16b previous=0;
        init_saved_floors(TRUE);
        dungeon_type=DUNGEON_HEAVEN;
        p_ptr->floor_id=0;
        for(i=QUEST_FELLANA;i<=QUEST_FILARHH;i++) {
            quest[i].level=1025+(i-QUEST_FELLANA)*25;
            quest[i].status=i==QUEST_FELLANA ? QUEST_STATUS_FINISHED : QUEST_STATUS_TAKEN;
        }
        if(pass) {
            dun_level=base_level=1025;
            previous=get_new_floor_id();sf=get_sf_ptr(previous);
            sf->dun_level=1025;
            assert(save_floor(sf,SLF_SECOND));
        }
        dun_level=base_level=max_dlv[DUNGEON_HEAVEN]=old_depths[k];
        if(pass) {
            p_ptr->floor_id=get_new_floor_id();sf=get_sf_ptr(p_ptr->floor_id);
            sf->dun_level=dun_level;sf->upper_floor_id=previous;
        }
        assert(save_player()); init_saved_floors(TRUE); assert(load_player());
        assert(dun_level==new_depths[k] && base_level==new_depths[k]);
        assert(max_dlv[DUNGEON_HEAVEN]==new_depths[k]);
        for(i=QUEST_FELLANA;i<=QUEST_FILARHH;i++)
            assert(quest[i].level==970+(i-QUEST_FELLANA)*10);
        assert(quest[QUEST_FELLANA].status==QUEST_STATUS_FINISHED);
        assert(quest[QUEST_HOLP].status==QUEST_STATUS_TAKEN);
        if(pass) {
            assert(get_sf_ptr(p_ptr->floor_id)->dun_level==new_depths[k]);
            assert(get_sf_ptr(previous)->dun_level==970);
            assert(get_sf_ptr(p_ptr->floor_id)->upper_floor_id==previous);
        }
        assert(save_player()); init_saved_floors(TRUE); assert(load_player());
        assert(dun_level==new_depths[k] && max_dlv[DUNGEON_HEAVEN]==new_depths[k]);
    }
    /* A character outside Heaven still keeps converted progress. */
    init_saved_floors(TRUE);p_ptr->floor_id=0;
    dungeon_type=DUNGEON_ARMORICA;dun_level=base_level=3;
    max_dlv[DUNGEON_HEAVEN]=1050;quest[QUEST_FILARHH].level=1100;
    assert(save_player());init_saved_floors(TRUE);assert(load_player());
    assert(dun_level==3 && max_dlv[DUNGEON_HEAVEN]==980);
    puts("Heaven: 950-1000, four quest floors, old/current multi-floor saves and preserved quest status passed");
}
"""
h=h.replace('int main(int argc,char **argv)',extra+'\nint main(int argc,char **argv)',1)
h=h.replace('    display_regressions();','    heaven_tests();\n    return 0;\n    display_regressions();',1)
with tempfile.TemporaryDirectory(prefix='toband-heaven-') as d:
    tmp=Path(d)
    shutil.copytree(BUILD/'lib',tmp/'lib',ignore=shutil.ignore_patterns('*.raw','save'))
    for name in ('d_info.txt','q_heaven.txt'):
        (tmp/'lib/edit'/name).write_bytes((ROOT/'lib/edit'/name).read_bytes().decode('euc_jp').encode('cp932'))
    (tmp/'lib/save').mkdir(exist_ok=True)
    (tmp/'test.c').write_bytes(h.encode('euc_jp'))
    objs=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    subprocess.run([str(GCC),*FLAGS,str(tmp/'test.c'),*map(str,objs),'-o',str(tmp/'test.exe'),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    r=subprocess.run([str(tmp/'test.exe'),(tmp/'lib').as_posix()+'/'],cwd=tmp,capture_output=True,timeout=60)
    print(r.stdout.decode('cp932',errors='replace'))
    if r.returncode:print(r.stderr.decode('cp932',errors='replace'));raise SystemExit(r.returncode)
