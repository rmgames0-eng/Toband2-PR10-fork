"""Purgatory altar generation, atomic sacrifice, class entry and save/revival."""
from pathlib import Path
import subprocess,tempfile,shutil
ROOT=Path(__file__).resolve().parents[1]; BUILD=ROOT/'build/party'; GCC=ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h=(ROOT/'tests/party_integration.c').read_bytes().decode('euc_jp')
extra=r"""
extern bool change_class(int cmd);
extern void alloc_purgatory_altar(void);
extern bool party_restore_life(int member,int mode);
static void altar_keys(char *s) {keys="                    ";key_index=0;msg_print(NULL);keys=s;key_index=0;}
static void altar_tests(void)
{
    int i,j,x,y,pass,total=0,count;
    party_member snapshot[3];
    object_type sword;
    bool old_save;
    static char spaces[20000];
    memset(spaces,32,sizeof(spaces)-1);keys=spaces;key_index=0;
    Rand_quick=TRUE;Rand_value=321713;
    character_generated=FALSE;character_dungeon=TRUE;
    wipe_o_list();wipe_m_list();cur_hgt=cur_wid=15;py=px=1;
    /* Probability on a viable lava floor, uniqueness, and realm exclusion. */
    for(pass=0;pass<2500;pass++) {
        dungeon_type=DUNGEON_DEMON;dun_level=950;
        for(y=0;y<15;y++)for(x=0;x<15;x++) {
            memset(&cave[y][x],0,sizeof(cave_type));cave[y][x].feat=FEAT_SHAL_LAVA;
        }
        alloc_purgatory_altar();count=0;
        for(y=1;y<14;y++)for(x=1;x<14;x++)if(cave[y][x].feat==FEAT_PURGATORY_ALTAR) {
            int dy,dx;count++;
            for(dy=-1;dy<=1;dy++)for(dx=-1;dx<=1;dx++)if(dy||dx)
                assert(cave[y+dy][x+dx].feat==FEAT_FLOOR);
        }
        assert(count<=1);total+=count;
        if(count) {
            for(j=0;j<100;j++)alloc_purgatory_altar();
            count=0;for(y=1;y<14;y++)for(x=1;x<14;x++)if(cave[y][x].feat==FEAT_PURGATORY_ALTAR)count++;
            assert(count==1);
        }
    }
    assert(total>=60 && total<=145);
    printf("Altar generation: %d / 2500, one per floor\n",total);
    for(y=0;y<15;y++)for(x=0;x<15;x++)cave[y][x].feat=FEAT_FLOOR;
    dungeon_type=DUNGEON_HEAVEN;
    for(pass=0;pass<500;pass++)alloc_purgatory_altar();
    for(y=0;y<15;y++)for(x=0;x<15;x++)assert(cave[y][x].feat!=FEAT_PURGATORY_ALTAR);
    /* Exercise the production cave generator, not just the allocation helper. */
    total=0;
    for(pass=0;pass<100;pass++) {
        wipe_o_list();wipe_m_list();dungeon_type=DUNGEON_DEMON;dun_level=950;
        p_ptr->inside_quest=0;p_ptr->riding=0;p_ptr->oldpy=p_ptr->oldpx=0;
        Rand_value=617171+pass*79;keys=spaces;key_index=0;
        generate_cave();count=0;
        for(y=0;y<cur_hgt;y++)for(x=0;x<cur_wid;x++)if(cave[y][x].feat==FEAT_PURGATORY_ALTAR)count++;
        assert(count<=1);total+=count;
    }
    assert(total>0);printf("Real demon floors with altar: %d / 100\n",total);
    wipe_o_list();wipe_m_list();cur_hgt=cur_wid=15;
    for(y=0;y<15;y++)for(x=0;x<15;x++){memset(&cave[y][x],0,sizeof(cave_type));cave[y][x].feat=FEAT_FLOOR;}
    init_flags=INIT_ONLY_BUILDINGS;
    assert(!process_dungeon_file("t0000000.txt",0,0,MAX_HGT,MAX_WID));
    assert(!strcmp(building[28].name,"煉獄の祭壇"));
    assert(building[28].actions[0]==BACT_DARK_CONTRACT && building[28].letters[0]=='c');
    init_flags=0;
    dungeon_type=DUNGEON_DEMON;dun_level=950;py=px=7;cave[7][7].feat=FEAT_PURGATORY_ALTAR;
    memset(inventory,0,sizeof(object_type)*INVEN_TOTAL);
    p_ptr->prace=RACE_HUMAN;p_ptr->pclass=CLASS_SOLDIER;p_ptr->psex=SEX_MALE;
    p_ptr->lev=p_ptr->max_plv=p_ptr->max_max_plv=20;
    p_ptr->chp=p_ptr->mhp=100;p_ptr->csp=p_ptr->msp=20;
    p_ptr->is_dead=0;p_ptr->inside_quest=p_ptr->inside_arena=0;
    p_ptr->playing=TRUE;p_ptr->leaving=FALSE;p_ptr->floor_id=0;
    memset(p_ptr->cexp_info,0,sizeof(p_ptr->cexp_info));
    p_ptr->cexp_info[CLASS_SOLDIER].clev=p_ptr->cexp_info[CLASS_SOLDIER].max_clev=p_ptr->cexp_info[CLASS_SOLDIER].max_max_clev=20;
    party_rebind();party_reset();party_capture();party_count=3;
    for(i=1;i<3;i++) {
        party_members[i]=party_members[0];
        party_members[i].player.pclass=i==1?CLASS_GUNNER:CLASS_WITCH;
        party_members[i].player.lev=party_members[i].player.max_plv=party_members[i].player.max_max_plv=50;
        party_members[i].player.exp=party_members[i].player.max_exp=party_members[i].player.max_max_exp=1234567;
        for(j=0;j<MAX_CLASS;j++) {
            cexp_info_type *c=&party_members[i].player.cexp_info[j];
            c->cexp=c->max_cexp=c->max_max_cexp=8765432;c->cexp_frac=123;
            c->clev=c->max_clev=c->max_max_clev=45;
        }
        strcpy(party_members[i].name,i==1?"First":"Second");
        object_prep(&sword,lookup_kind(TV_SWORD,SV_DAGGER));
        sword.to_h=i;party_members[i].equipment[0]=sword;
    }
    r_info[MON_FILARHH].r_pkills=r_info[MON_DOLGARUA].r_pkills=0;
    quest[QUEST_FILARHH].status=quest[QUEST_DOLGARUA].status=QUEST_STATUS_UNTAKEN;
    for(i=0;i<ETHNICITY_NUM;i++)chaos_frame[i]=100;
    assert(party_can_dark_contract());
    assert(can_choose_class(CLASS_DARK_ELEMENT,CLASS_CHOOSE_MODE_BLDGS));
    assert(!can_choose_class(CLASS_DARK_ELEMENT,CLASS_CHOOSE_MODE_NORMAL));

    {
        player_type before=*p_ptr;
        /* Every terminal class may sign the altar contract, except itself. */
        for(i=0;i<max_c_idx;i++)if(class_info[i].c_flags & PCF_NO_CHANGE) {
            p_ptr->pclass=i;party_rebind();
            assert(party_can_dark_contract()==(i!=CLASS_DARK_ELEMENT));
            assert(can_choose_class(CLASS_DARK_ELEMENT,CLASS_CHOOSE_MODE_BLDGS)==(i!=CLASS_DARK_ELEMENT));
            assert(!can_choose_class(CLASS_DARK_ELEMENT,CLASS_CHOOSE_MODE_NORMAL));
        }
        /* Succubus: Witch and terminal sources; preserve sex/race/alignment/party gates. */
        party_count=1;p_ptr->psex=SEX_FEMALE;p_ptr->align[ALI_LNC]=-100;
        for(i=0;i<max_c_idx;i++) {
            p_ptr->pclass=i;party_rebind();
            assert(can_choose_class(CLASS_SUCCUBUS,CLASS_CHOOSE_MODE_BLDGS)==
                (i!=CLASS_SUCCUBUS && (i==CLASS_WITCH || (class_info[i].c_flags & PCF_NO_CHANGE))));
            assert(!can_choose_class(CLASS_SUCCUBUS,CLASS_CHOOSE_MODE_NORMAL));
        }
        p_ptr->pclass=CLASS_RELICSKNIGHT;party_rebind();
        p_ptr->psex=SEX_MALE;assert(!can_choose_class(CLASS_SUCCUBUS,CLASS_CHOOSE_MODE_BLDGS));p_ptr->psex=SEX_FEMALE;
        p_ptr->prace=RACE_SKELETON;party_rebind();assert(!can_choose_class(CLASS_SUCCUBUS,CLASS_CHOOSE_MODE_BLDGS));
        p_ptr->prace=RACE_HUMAN;party_rebind();
        p_ptr->align[ALI_LNC]=100;assert(!can_choose_class(CLASS_SUCCUBUS,CLASS_CHOOSE_MODE_BLDGS));p_ptr->align[ALI_LNC]=-100;
        party_count=3;assert(!can_choose_class(CLASS_SUCCUBUS,CLASS_CHOOSE_MODE_BLDGS));
        assert(!can_choose_class(CLASS_WHITEKNIGHT,CLASS_CHOOSE_MODE_BLDGS));
        *p_ptr=before;party_rebind();
    }
    p_ptr->pclass=CLASS_RELICSKNIGHT;party_rebind();assert(party_can_dark_contract());
    p_ptr->pclass=CLASS_SOLDIER;party_rebind();
    party_members[1].dead=TRUE;assert(!party_can_dark_contract());party_members[1].dead=FALSE;
    party_count=2;assert(!party_can_dark_contract());party_count=3;
    dungeon_type=DUNGEON_RUINS;assert(!party_can_dark_contract());dungeon_type=DUNGEON_DEMON;
    memcpy(snapshot,party_members,sizeof(snapshot));old_save=can_save;
    altar_keys("n");assert(!change_class(BACT_DARK_CONTRACT));
    assert(!memcmp(snapshot,party_members,sizeof(snapshot)) && p_ptr->pclass==CLASS_SOLDIER && can_save==old_save);
    for(i=0;i<ETHNICITY_NUM;i++)assert(chaos_frame[i]==100);
    /* Contract while the original survivor occupies the last party slot. */
    { party_member tmp=party_members[0];party_members[0]=party_members[2];party_members[2]=tmp;party_active=2; }
    p_ptr->pclass=CLASS_RELICSKNIGHT;party_rebind();
    altar_keys("y                    ");assert(change_class(BACT_DARK_CONTRACT));
    assert(p_ptr->pclass==CLASS_DARK_ELEMENT && cp_ptr==&class_info[CLASS_DARK_ELEMENT] && can_save==old_save);
    for(i=0;i<ETHNICITY_NUM;i++)assert(chaos_frame[i]==-200);
    assert(!party_can_dark_contract());
    assert(party_count==1 && party_active==0 && (party_rewards & PARTY_RECRUIT_SEALED));
    for(i=1;i<MAX_PARTY_MEMBERS;i++) assert(!party_members[i].name[0] && !party_members[i].equipment[0].k_idx);
    count=0;for(i=1;i<o_max;i++)if(o_list[i].k_idx) {assert(o_list[i].tval!=TV_TAROT);if(o_list[i].tval==TV_SWORD)count++;}
    assert(count==2);
    assert(!party_can_revive(1) && !party_can_revive(2) && !party_can_recruit());
    assert(save_player());init_saved_floors(TRUE);party_reset();assert(load_player());
    assert(p_ptr->pclass==CLASS_DARK_ELEMENT && party_count==1 && party_active==0);
    assert(party_rewards & PARTY_RECRUIT_SEALED);
    assert(!party_can_revive(1) && !party_can_revive(2));
    /* Even after a different class and unlocked recruitment quests, no recruits. */
    p_ptr->pclass=CLASS_SOLDIER;party_rebind();
    quest[QUEST_ARMORICA].status=QUEST_STATUS_FINISHED;
    quest[QUEST_BARMAMUTHA_L].status=QUEST_STATUS_FINISHED;
    assert(!party_can_recruit());do_cmd_party_recruit();assert(party_count==1);
    puts("Altar: cancellation, companion disappearance, equipment/no tarot, permanent recruitment seal and save/load passed");
}
"""
h=h.replace('int main(int argc,char **argv)',extra+'\nint main(int argc,char **argv)',1)
h=h.replace('    display_regressions();','    altar_tests();\n    return 0;\n    display_regressions();',1)
with tempfile.TemporaryDirectory(prefix='toband-altar-') as d:
    tmp=Path(d);shutil.copytree(BUILD/'lib',tmp/'lib',ignore=shutil.ignore_patterns('*.raw','save'))
    for name in ('t0000000.txt','f_info.txt','p_class.txt','misc.txt','s_info.txt','m_info.txt'):
        (tmp/'lib/edit'/name).write_bytes((ROOT/'lib/edit'/name).read_bytes().decode('euc_jp').encode('cp932'))
    (tmp/'lib/save').mkdir(exist_ok=True);(tmp/'test.c').write_bytes(h.encode('euc_jp'))
    sources=[]
    for name,old,new in [('bldg','static bool change_class(','bool change_class('),('generate','static void alloc_purgatory_altar(','void alloc_purgatory_altar('),('party','static bool party_restore_life(','bool party_restore_life(')]:
        s=(ROOT/'src'/f'{name}.c').read_bytes().decode('euc_jp');assert old in s
        p=tmp/f'{name}.c';p.write_bytes(s.replace(old,new,1).encode('euc_jp'));sources.append(str(p))
    objs=[str(p) for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband','bldg','generate','party')]
    subprocess.run([str(GCC),*FLAGS,str(tmp/'test.c'),*sources,*objs,'-o',str(tmp/'test.exe'),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(tmp/'test.exe'),(tmp/'lib').as_posix()+'/'],cwd=tmp,capture_output=True,timeout=60)
    print(result.stdout.decode('cp932',errors='replace'))
    if result.returncode:print(result.stderr.decode('cp932',errors='replace'));raise SystemExit(result.returncode)
