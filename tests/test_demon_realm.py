"""Real fixed-map generation, gate travel, guardian exclusion and legacy saves."""
from pathlib import Path
import tempfile,subprocess,shutil
ROOT=Path(__file__).resolve().parents[1];BUILD=ROOT/'build/party';GCC=ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h=(ROOT/'tests/party_integration.c').read_bytes().decode('euc_jp')
extra=r"""
extern bool activate_chaos_gate(void);
static int gate_great_objects;
void __real_place_object(int y, int x, u32b flags);
void __wrap_place_object(int y, int x, u32b flags)
{
    if(dungeon_type==DUNGEON_RUINS && dun_level==127 &&
       (flags & (AMF_GOOD|AMF_GREAT))==(AMF_GOOD|AMF_GREAT)) gate_great_objects++;
    __real_place_object(y,x,flags);
}
static void gate_answer(char *s) { keys="          "; key_index=0; msg_print(NULL); keys=s; key_index=0; }
static void demon_tests(void)
{
    int i,y,x,count;
    static char spaces[20000];
    memset(spaces,32,sizeof(spaces)-1);

    monster_race *boss=&r_info[MON_DEMUNZA];
    assert(d_info[DUNGEON_DEMON].mindepth==950 && d_info[DUNGEON_DEMON].maxdepth==1000);
    assert(d_info[DUNGEON_DEMON].final_guardian==MON_DEMUNZA);
    assert(d_info[DUNGEON_DEMON].flags1 & DF1_CLOSED);
    assert(boss->flags7 & RF7_GUARDIAN);
    boss->max_num=1;boss->cur_num=0;
    Rand_quick=TRUE;Rand_value=917325;
    character_generated=FALSE; p_ptr->inside_quest=0;
    p_ptr->oldpx=p_ptr->oldpy=0;p_ptr->riding=0;
    dungeon_type=DUNGEON_RUINS;dun_level=127;
    wipe_o_list();wipe_m_list();gate_great_objects=0;generate_cave();
    assert(gate_great_objects>=18);
    assert(cur_hgt==44 && cur_wid==88);
    assert(IS_DEMON_GATE(&cave[21][44]));
    assert(cave_perma_bold(21,44) && !cave_valid_bold(21,44));
    assert(cave_floor_bold(21,44));
    assert(cave[py][px].feat==FEAT_LESS);
    count=0;for(y=0;y<cur_hgt;y++) for(x=0;x<cur_wid;x++) if(IS_DEMON_GATE(&cave[y][x]))count++;
    assert(count==1 && (cave[21][44].info & CAVE_ICKY));
    /* Flood fill checks that the gate is walkable from the entrance. */
    {
        int queue[4000],n=0,k=0;bool seen[44][88]={0};
        queue[n++]=py*88+px;seen[py][px]=TRUE;
        while(k<n) {
            int cell=queue[k++],dy,dx;
            for(dy=-1;dy<=1;dy++)for(dx=-1;dx<=1;dx++) {
                y=cell/88+dy;x=cell%88+dx;
                if(y<1||y>=43||x<1||x>=87||seen[y][x]||!cave_floor_bold(y,x))continue;
                seen[y][x]=TRUE;queue[n++]=y*88+x;
            }
        }
        assert(seen[21][44]);
        for(y=1;y<43;y++)for(x=1;x<87;x++) {
            if(cave_floor_bold(y,x)) assert(seen[y][x]);
            else assert(cave_perma_bold(y,x));
        }
    }
    {
        int monsters=0, objects=0;
        for(i=1;i<m_max;i++)if(m_list[i].r_idx)monsters++;
        for(i=1;i<o_max;i++)if(o_list[i].k_idx)objects++;
        assert(monsters>=100 && objects>=75);
        /* Circular sanctum wall, permanent pillars and four cardinal portals. */
        assert(cave_perma_bold(17,40) && cave_perma_bold(25,40));
        assert(cave_perma_bold(19,41) && cave_perma_bold(23,47));
        assert(cave_floor_bold(16,44) && cave_floor_bold(26,44));
        assert(cave_floor_bold(21,34) && cave_floor_bold(21,54));
        printf("Gate vault: %d monsters, %d objects; all rooms reachable, all walls permanent\n",monsters,objects);
    }
    p_ptr->leaving=FALSE;
    assert(!activate_chaos_gate() && !p_ptr->leaving);
    py=21;px=44;
    gate_answer("n");assert(!activate_chaos_gate() && dungeon_type==DUNGEON_RUINS);
    gate_answer("y");assert(activate_chaos_gate());
    assert(dungeon_type==DUNGEON_DEMON && dun_level==950 && p_ptr->leaving);
    leave_floor();wipe_o_list();wipe_m_list();keys=spaces;key_index=0;change_floor();
    assert(dun_level==950 && dungeon_type==DUNGEON_DEMON && cave_floor_bold(py,px));
    /* Recall must work before the guardian is killed. */
    max_dlv[DUNGEON_DEMON]=950;p_ptr->word_recall=0;
    gate_answer(" ");assert(recall_player(12));assert(p_ptr->word_recall>0);p_ptr->word_recall=0;
    assert(boss->max_num>0);
    gate_answer("y");assert(activate_chaos_gate());
    assert(dungeon_type==DUNGEON_RUINS && dun_level==127);
    leave_floor();wipe_o_list();wipe_m_list();keys=spaces;key_index=0;change_floor();
    assert(IS_DEMON_GATE(&cave[21][44]));
    /* Heaven also returns before Filarhh is defeated. */
    dungeon_type=DUNGEON_HEAVEN;dun_level=970;max_dlv[DUNGEON_HEAVEN]=970;
    quest[QUEST_FILARHH].status=QUEST_STATUS_TAKEN;
    gate_answer(" ");assert(recall_player(12));assert(p_ptr->word_recall>0);p_ptr->word_recall=0;
    gate_answer("y");assert(activate_chaos_gate());
    assert(dungeon_type==DUNGEON_HEAVEN_WAY && dun_level==585);
    back_from_heaven=FALSE;
    /* No ordinary allocation or direct placement away from the boss floor. */
    wipe_o_list();wipe_m_list();cur_hgt=cur_wid=7;py=px=1;
    for(y=0;y<7;y++)for(x=0;x<7;x++)cave[y][x].feat=FEAT_FLOOR;
    dungeon_type=DUNGEON_RUINS;dun_level=127;
    boss->max_num=1;boss->cur_num=0;
    assert(!place_monster_aux(0,3,3,MON_DEMUNZA,PM_IGNORE_TERRAIN));
    dungeon_type=DUNGEON_DEMON;dun_level=999;
    assert(!place_monster_aux(0,3,3,MON_DEMUNZA,PM_IGNORE_TERRAIN));
    dun_level=1000;get_mon_num_prep(NULL,NULL);
    for(i=0;i<alloc_race_size;i++)if(alloc_race_table[i].index==MON_DEMUNZA)assert(!alloc_race_table[i].prob2);
    assert(alloc_guardian(PM_IN_GENERATE));
    assert(boss->cur_num==1);
    wipe_m_list();
    /* Evil-only allocation, realm-local Ogre rarity, and fixed intermediate bosses. */
    {
        int j, id, seen=0;
        int ids[3]={MON_DAGDA,MON_ASMODE,MON_DIABLO};
        dungeon_type=DUNGEON_DEMON;dun_level=960;
        get_mon_num_prep(NULL,NULL);
        for(j=0;j<alloc_race_size;j++) {
            alloc_entry *entry=&alloc_race_table[j];
            monster_race *race=&r_info[entry->index];
            if(!(race->flags3 & RF3_EVIL)) assert(entry->prob2==0);
            if(!IS_RANDOM_UNIQUE(entry->index) && race->d_char=='O' && race->level>=60 && !(race->flags1 & RF1_QUESTOR) && !(race->flags7 & RF7_GUARDIAN)) {
                assert(race->rarity==20 && entry->prob1==5);
                assert(race->flagsr & RFR_RES_FIRE);
                assert(entry->prob2==100);seen++;
            }
        }
        assert(seen>=7);
        (void)summon_specific(0,3,3,100,SUMMON_ANGEL,PM_ALLOW_GROUP);
        for(j=0;j<alloc_race_size;j++)
            if(!(r_info[alloc_race_table[j].index].flags3 & RF3_EVIL))assert(!alloc_race_table[j].prob2);

        /* Direct placement cannot bypass the alignment restriction. */
        for(id=1;id<max_r_idx;id++)if(r_info[id].name && !(r_info[id].flags3 & RF3_EVIL))break;
        assert(id<max_r_idx && !place_monster_aux(0,3,3,id,PM_IGNORE_TERRAIN));
        dungeon_type=DUNGEON_RUINS;dun_level=127;get_mon_num_prep(NULL,NULL);
        for(j=0;j<alloc_race_size;j++) {
            alloc_entry *entry=&alloc_race_table[j];
            monster_race *race=&r_info[entry->index];
            if(!IS_RANDOM_UNIQUE(entry->index) && race->d_char=='O' && race->level>=60 && !(race->flags1 & RF1_QUESTOR) && !(race->flags7 & RF7_GUARDIAN))
                assert(entry->prob2==entry->prob1);
        }
        for(j=0;j<3;j++) {
            wipe_o_list();wipe_m_list();
            r_info[ids[j]].max_num=1;r_info[ids[j]].cur_num=0;
            dungeon_type=DUNGEON_DEMON;dun_level=970+j*10;
            Rand_value=75193+j*321;keys=spaces;key_index=0;generate_cave();
            count=0;
            for(x=1;x<m_max;x++)if(m_list[x].r_idx) {
                assert(r_info[m_list[x].r_idx].flags3 & RF3_EVIL);
                if(m_list[x].r_idx==ids[j])count++;
            }
            assert(count==1);
            wipe_m_list();r_info[ids[j]].max_num=0;
            assert(!alloc_guardian(PM_IN_GENERATE)); /* Defeated uniques stay dead. */
            r_info[ids[j]].max_num=1;
            dun_level=969+j*10;
            assert(!place_monster_aux(0,py,px,ids[j],PM_IGNORE_TERRAIN));
        }
    }
    /* Every fixed boss is excluded from all other dungeons and depths. */
    {
        int ids[4]={MON_DAGDA,MON_ASMODE,MON_DIABLO,MON_DEMUNZA};
        int depths[4]={970,980,990,1000};
        int d,j,k;
        wipe_o_list();wipe_m_list();cur_hgt=cur_wid=9;py=px=1;
        for(y=0;y<9;y++)for(x=0;x<9;x++)cave[y][x].feat=FEAT_FLOOR;
        for(d=0;d<max_d_idx;d++)for(j=0;j<4;j++) {
            dungeon_type=d;dun_level=depths[j];
            r_info[ids[j]].max_num=1;r_info[ids[j]].cur_num=0;
            get_mon_num_prep(NULL,NULL);
            for(k=0;k<alloc_race_size;k++)for(x=0;x<4;x++)
                if(alloc_race_table[k].index==ids[x])assert(!alloc_race_table[k].prob2);
            if(d!=DUNGEON_DEMON)
                assert(!place_monster_aux(0,4,4,ids[j],PM_IGNORE_TERRAIN|PM_IGNORE_AMGRID));
            else {
                dun_level=depths[j]-1;
                assert(!place_monster_aux(0,4,4,ids[j],PM_IGNORE_TERRAIN|PM_IGNORE_AMGRID));
                dun_level=depths[j];
                assert(place_monster_aux(0,4,4,ids[j],PM_IGNORE_TERRAIN|PM_IGNORE_AMGRID));
                wipe_m_list();
            }
        }
    }
    /* Initial groups work on lava and on ordinary floors outside the realm. */
    {
        int pass,j,leader;
        assert(r_info[685].flags1 & RF1_FRIENDS);
        assert(r_info[MON_OGRE_DARK_LORD].flags1 & RF1_ESCORT);
        for(pass=0;pass<4;pass++) {
            wipe_o_list();wipe_m_list();cur_hgt=cur_wid=31;py=px=1;
            dungeon_type=pass<2?DUNGEON_DEMON:DUNGEON_RUINS;dun_level=950;
            for(y=0;y<31;y++)for(x=0;x<31;x++)cave[y][x].feat=pass<2?FEAT_SHAL_LAVA:FEAT_FLOOR;
            leader=(pass%2)?MON_OGRE_DARK_LORD:685;
            Rand_value=123456+pass;
            assert(place_monster_aux(0,15,15,leader,PM_ALLOW_GROUP|PM_IGNORE_AMGRID));
            count=0;
            for(j=1;j<m_max;j++)if(m_list[j].r_idx) {
                monster_race *r=&r_info[m_list[j].r_idx];
                assert(r->d_char=='O' && r->level>=60 && !(r->flags1 & RF1_UNIQUE));
                if(leader==685)assert(m_list[j].r_idx==685);
                else assert(r->level<=r_info[leader].level);
                count++;
            }
            assert(count>1);
        }
    }
    /* Real population check, including terrain-specific allocation hooks. */
    {
        int seed, monsters=0, ogres=0;
        dungeon_type=DUNGEON_DEMON;dun_level=950;
        cave[3][3].feat=FEAT_SHAL_LAVA;
        get_mon_num_prep(get_monster_hook(),get_monster_hook2(3,3));
        for(x=0;x<alloc_race_size;x++)if(alloc_race_table[x].index==685)
            assert(alloc_race_table[x].prob2==100);
        cave[3][3].feat=FEAT_FLOOR;
        get_mon_num_prep(get_monster_hook(),get_monster_hook2(3,3));
        for(x=0;x<alloc_race_size;x++)if(alloc_race_table[x].index==685)
            assert(alloc_race_table[x].prob2==100);
        for(seed=0;seed<5;seed++) {
            wipe_o_list();wipe_m_list();Rand_value=76131+seed*173;
            keys=spaces;key_index=0;generate_cave();
            for(x=1;x<m_max;x++)if(m_list[x].r_idx) {
                monster_race *r=&r_info[m_list[x].r_idx];monsters++;
                if(r->d_char=='O' && r->level>=60)ogres++;
            }
        }
        assert(monsters>0 && ogres>0);
        printf("Five real demon floors: %d monsters, %d Ogres\n",monsters,ogres);
    }
    /* Generate the real final floor and find its unique guardian. */
    for(i=0;i<3;i++) {
        wipe_o_list();wipe_m_list();boss->max_num=1;boss->cur_num=0;
        dungeon_type=DUNGEON_DEMON;dun_level=1000;
        Rand_value=917325+i*327;keys=spaces;key_index=0;generate_cave();
        count=0;for(x=1;x<m_max;x++)if(m_list[x].r_idx==MON_DEMUNZA)count++;
        assert(count==1 && cave_floor_bold(py,px));
    }
    wipe_o_list();wipe_m_list();cur_hgt=cur_wid=7;py=px=1;
    for(y=0;y<7;y++)for(x=0;x<7;x++)cave[y][x].feat=FEAT_FLOOR;
    /* Old Hell Way floor and recall progress migrate once, preserving objects. */
    dungeon_type=DUNGEON_DEMON;dun_level=base_level=681;max_dlv[DUNGEON_DEMON]=696;
    p_ptr->floor_id=0;p_ptr->is_dead=0;p_ptr->leaving=FALSE;p_ptr->word_recall=0;
    assert(save_player());init_saved_floors(TRUE);assert(load_player());
    assert(dun_level==975 && base_level==975 && max_dlv[DUNGEON_DEMON]==1000);
    assert(save_player());init_saved_floors(TRUE);assert(load_player());assert(dun_level==975);
    /* Old visited Ruins maps acquire a gate without replacing the floor. */
    dungeon_type=DUNGEON_RUINS;dun_level=base_level=127;
    for(y=0;y<7;y++)for(x=0;x<7;x++){cave[y][x].feat=FEAT_FLOOR;cave[y][x].mimic=0;cave[y][x].special=0;}
    cave[2][2].feat=FEAT_LESS;
    assert(save_player());init_saved_floors(TRUE);assert(load_player());
    assert(IS_DEMON_GATE(&cave[3][3]) && cave[2][2].feat==FEAT_LESS);
    dungeon_type=0;dun_level=base_level=0;
    cave[2][2].feat=FEAT_ENTRANCE;cave[2][2].special=DUNGEON_DEMON;
    assert(save_player());init_saved_floors(TRUE);assert(load_player());
    assert(cave[2][2].feat==FEAT_DIRT && !cave[2][2].special);
    puts("Demon realm: fixed star vault, walkable gate, travel/recall, exclusive guardian, old saves passed");
}
"""
h=h.replace('int main(int argc,char **argv)',extra+'\nint main(int argc,char **argv)',1)
h=h.replace('    display_regressions();','    demon_tests();\n    return 0;\n    display_regressions();',1)
with tempfile.TemporaryDirectory(prefix='toband-demon-') as d:
    tmp=Path(d);shutil.copytree(BUILD/'lib',tmp/'lib',ignore=shutil.ignore_patterns('*.raw','save'))
    for name in ('d_info.txt','f_info.txt','d_gate.txt','q_heaven.txt','r_info.txt'):
        (tmp/'lib/edit'/name).write_bytes((ROOT/'lib/edit'/name).read_bytes().decode('euc_jp').encode('cp932'))
    (tmp/'lib/save').mkdir(exist_ok=True)
    (tmp/'test.c').write_bytes(h.encode('euc_jp'))
    c=(ROOT/'src/cmd6.c').read_bytes().decode('euc_jp').replace('static bool activate_chaos_gate(', 'bool activate_chaos_gate(',1)
    (tmp/'cmd6.c').write_bytes(c.encode('euc_jp'))
    objs=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband','cmd6')]
    subprocess.run([str(GCC),*FLAGS,str(tmp/'test.c'),str(tmp/'cmd6.c'),*map(str,objs),'-o',str(tmp/'test.exe'),'-Wl,--wrap=place_object','-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(tmp/'test.exe'),(tmp/'lib').as_posix()+'/'],cwd=tmp,capture_output=True,timeout=60)
    print(result.stdout.decode('cp932',errors='replace'))
    if result.returncode:print(result.returncode);print(result.stderr.decode('cp932',errors='replace'));raise SystemExit(result.returncode)
