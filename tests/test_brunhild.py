"""Build first with build-mingw.py.
Exercise real quest acceptance, castle population, artifact reservation and saves.
"""
from pathlib import Path
import subprocess, tempfile, shutil
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build/party'
GCC=ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
EXTRA=r"""
extern void brun_test_castle(void);
extern void brun_test_movement(void);
extern bool brun_test_make(object_type *o);
static void brunhild_tests(void)
{
    int i, j, found, item=0, counts[993]={0};
    int ids[]={716,225,240,287,293,372,636,737,518,658,805,699,772,992};
    int expected[]={4,20,10,10,10,10,10,4,4,4,2,100,1,1};
    object_type forge;
    static char answers[20000];
    memset(answers,'y',sizeof(answers)-1); answers[0]='y';
    keys=answers;key_index=0;auto_more=TRUE;
    wipe_m_list();wipe_o_list();
    r_info[MON_MAN_LOOK_SEA].max_num=1;
    r_info[MON_ZENOBIA_LANCELOT].max_num=1;
    p_ptr->inside_quest=QUEST_BRUNHILD;
    quest[QUEST_BRUNHILD].status=QUEST_STATUS_UNTAKEN;
    init_flags=INIT_ASSIGN;
    assert(!process_dungeon_file("q_info.txt",0,0,0,0));
    assert(quest[QUEST_BRUNHILD].level==95);
    assert(quest[QUEST_BRUNHILD].flags & QUEST_FLAG_NO_RECALL);
    /* Old saves can still contain the previous quest flags. */
    quest[QUEST_BRUNHILD].flags &= ~QUEST_FLAG_NO_RECALL;
    p_ptr->word_recall=0;assert(recall_player(15));assert(!p_ptr->word_recall);
    p_ptr->word_recall=1;p_ptr->leaving=FALSE;
    brun_test_movement();assert(!p_ptr->word_recall && !p_ptr->leaving);
    quest[QUEST_BRUNHILD].flags |= QUEST_FLAG_NO_RECALL;
    puts("Brunhild: recall blocked and pending recall cancelled with legacy flags");
    assert(quest[QUEST_BRUNHILD].type==QUEST_TYPE_FIND_ARTIFACT);
    assert(quest[QUEST_BRUNHILD].k_idx==ART_BRUNHILD);
    assert(!(a_info[ART_BRUNHILD].gen_flags & TRG_QUESTITEM));
    assert(r_info[992].level==80 && (r_info[992].flags1 & RF1_UNIQUE));
    for(i=0;i<4;i++) {
        assert(r_info[992].blow[i].method==RBM_SLASH);
        assert(r_info[992].blow[i].effect==RBE_SUPERHURT);
        assert(r_info[992].blow[i].d_dice==20 && r_info[992].blow[i].d_side==15);
    }
    assert(r_info[992].speed==r_info[MON_LANCELOT].speed);
    assert(r_info[992].d_char=='p' && r_info[992].d_attr==TERM_WHITE);
    p_ptr->inside_quest=0;init_flags=0;
    assert(!place_monster_aux(0,2,2,992,0));
    py=px=1;cave[py][px].special=QUEST_BRUNHILD;
    a_info[ART_BRUNHILD].cur_num=1;
    brun_test_castle();
    assert(quest[QUEST_BRUNHILD].status==QUEST_STATUS_UNTAKEN);
    a_info[ART_BRUNHILD].cur_num=0;keys=answers;key_index=0;
    brun_test_castle();
    assert(quest[QUEST_BRUNHILD].status==QUEST_STATUS_TAKEN);
    assert(artifact_is_reserved(ART_BRUNHILD));
    assert(!create_named_art(ART_BRUNHILD,2,2));
    /* Isolate the sword's natural generation from other artifacts. */
    for(i=1;i<max_a_idx;i++) if(i!=ART_BRUNHILD) a_info[i].cur_num=1;
    dun_level=object_level=95;
    a_info[ART_BRUNHILD].level=0;a_info[ART_BRUNHILD].rarity=1;
    for(i=0;i<100;i++) {
        object_prep(&forge,lookup_kind(a_info[ART_BRUNHILD].tval,a_info[ART_BRUNHILD].sval));
        assert(!brun_test_make(&forge) && !forge.name1);
    }
    assert(a_info[ART_BRUNHILD].cur_num==0);
    /* Active reservation survives an actual save/load. */
    p_ptr->inside_quest=0;p_ptr->playing=TRUE;p_ptr->is_dead=0;
    assert(save_player());party_reset();assert(load_player());
    assert(quest[QUEST_BRUNHILD].status==QUEST_STATUS_TAKEN);
    assert(artifact_is_reserved(ART_BRUNHILD));
    assert(!(a_info[ART_BRUNHILD].gen_flags & TRG_QUESTITEM));
    assert(!create_named_art(ART_BRUNHILD,2,2));
    {
        int variant,yy,xx,mi,oy,ox;
        int artifact_spots[4][2]={{3,5},{3,59},{15,5},{15,59}};
        int guard_spots[4][2]={{4,6},{4,60},{14,6},{14,60}};
        for(variant=0;variant<16;variant++) {
            wipe_m_list();wipe_o_list();memset(counts,0,sizeof(counts));
            Rand_quick=TRUE;Rand_value=12345+variant*7919;seed_town=variant;
            p_ptr->inside_quest=QUEST_BRUNHILD;p_ptr->pclass=CLASS_WHITEKNIGHT;
            dun_level=base_level=monster_level=object_level=95;
            cur_hgt=MAX_HGT;cur_wid=MAX_WID;
            for(yy=0;yy<cur_hgt;yy++)for(xx=0;xx<cur_wid;xx++) {
                cave[yy][xx].feat=FEAT_PERM_SOLID;cave[yy][xx].m_idx=0;cave[yy][xx].o_idx=0;
            }
            for(mi=1;mi<max_r_idx;mi++)r_info[mi].cur_num=0;
            a_info[ART_BRUNHILD].cur_num=0;
            init_flags=INIT_CREATE_DUNGEON|INIT_ASSIGN;
            assert(!process_dungeon_file("q_info.txt",0,0,MAX_HGT,MAX_WID));
            found=0;
            for(mi=1;mi<m_max;mi++)if(m_list[mi].r_idx) {
                counts[m_list[mi].r_idx]++;found++;
                assert(is_hostile(&m_list[mi]) && (m_list[mi].mflag2 & MFLAG2_NOPET));
            }
            assert(found==190);
            for(mi=0;mi<14;mi++)assert(counts[ids[mi]]==expected[mi]);
            assert(r_info[MON_FIRE_BRASS].flags7 & RF7_UNIQUE2);
            yy=guard_spots[variant/4][0];xx=guard_spots[variant/4][1];
            assert(m_list[cave[yy][xx].m_idx].r_idx==MON_ZENOBIA_LANCELOT);
            found=0;
            for(mi=1;mi<o_max;mi++)if(o_list[mi].name1==ART_BRUNHILD) {
                found++;item=mi;
                assert(o_list[mi].iy==artifact_spots[variant%4][0] && o_list[mi].ix==artifact_spots[variant%4][1]);
            }
            assert(found==1 && a_info[ART_BRUNHILD].cur_num==1);
            /* Four corners always contain one sword: one real, three decoys. */
            for(j=0;j<4;j++) {
                int sword=cave[artifact_spots[j][0]][artifact_spots[j][1]].o_idx;
                assert(sword && !o_list[sword].next_o_idx);
                assert(o_list[sword].k_idx==lookup_kind(a_info[ART_BRUNHILD].tval,a_info[ART_BRUNHILD].sval));
                assert(o_list[sword].name1==(j==variant%4 ? ART_BRUNHILD : 0));
            }

            assert(o_cnt>=45);assert(py==9 && px==3);
            for(yy=0;yy<19;yy++)for(xx=0;xx<65;xx++)assert(cave[yy][xx].info & CAVE_ICKY);
            oy=py;ox=px;keys=answers;key_index=0;
            teleport_player(100);assert(py==oy && px==ox);
            teleport_player_to(10,20,FALSE,FALSE);assert(py==oy && px==ox);
            assert(!teleport_swap(6,50));
            mi=cave[guard_spots[variant/4][0]][guard_spots[variant/4][1]].m_idx;
            yy=m_list[mi].fy;xx=m_list[mi].fx;
            assert(!teleport_away(mi,100));teleport_to(mi,py,px,20,100,FALSE);
            assert(m_list[mi].fy==yy && m_list[mi].fx==xx);
        }
        /* A decoy neither completes the quest nor triggers the alarm. */
        for(mi=1;mi<m_max;mi++)if(m_list[mi].r_idx)m_list[mi].mtimed[MTIMED_CSLEEP]=100;
        py=artifact_spots[0][0];px=artifact_spots[0][1];
        keys=answers;key_index=0;record_fix_quest=FALSE;
        py_pickup_aux(cave[py][px].o_idx);
        assert(quest[QUEST_BRUNHILD].status==QUEST_STATUS_TAKEN);
        for(mi=1;mi<m_max;mi++)if(m_list[mi].r_idx)assert(MON_CSLEEP(&m_list[mi])==100);
        /* Every defender wakes when the real pickup completes the quest. */
        for(mi=1;mi<m_max;mi++)if(m_list[mi].r_idx)m_list[mi].mtimed[MTIMED_CSLEEP]=100;
        py=o_list[item].iy;px=o_list[item].ix;keys=answers;key_index=0;record_fix_quest=FALSE;
        py_pickup_aux(item);
        for(mi=1;mi<m_max;mi++)if(m_list[mi].r_idx)assert(!MON_CSLEEP(&m_list[mi]));
        /* Teleportation remains blocked after the alarm/completion. */
        oy=py;ox=px;teleport_player(20);assert(py==oy && px==ox);
    }
    assert(quest[QUEST_BRUNHILD].status==QUEST_STATUS_COMPLETED);
    assert(!artifact_is_reserved(ART_BRUNHILD));
    a_info[ART_BRUNHILD].cur_num=0;
    /* Untaken and failed quests release the sword for ordinary generation. */
    p_ptr->inside_quest=0;init_flags=0;
    for(i=0;i<2;i++) {
        quest[QUEST_BRUNHILD].status=i ? QUEST_STATUS_FAILED : QUEST_STATUS_UNTAKEN;
        object_prep(&forge,lookup_kind(a_info[ART_BRUNHILD].tval,a_info[ART_BRUNHILD].sval));
        assert(brun_test_make(&forge) && forge.name1==ART_BRUNHILD);
    }
    /* Both defeat orders, fixed-map resurrection, allocation and save/load. */
    for(i=0;i<2;i++) {
        int defeated=i ? MON_ZENOBIA_LANCELOT : MON_MAN_LOOK_SEA;
        int blocked=i ? MON_MAN_LOOK_SEA : MON_ZENOBIA_LANCELOT;
        int k,guardians=0;
        wipe_m_list();wipe_o_list();
        r_info[MON_MAN_LOOK_SEA].max_num=1;
        r_info[MON_ZENOBIA_LANCELOT].max_num=1;
        r_info[defeated].max_num=0;
        assert(hamilton_form_is_defeated(blocked));
        p_ptr->inside_quest=blocked==MON_ZENOBIA_LANCELOT ? QUEST_BRUNHILD : 0;
        dungeon_type=0;dun_level=base_level=monster_level=95;
        cave[2][2].feat=FEAT_FLOOR;
        assert(!place_monster_aux(0,2,2,blocked,0));
        get_mon_num_prep(NULL,NULL);
        for(k=0;k<alloc_race_size;k++) if(alloc_race_table[k].index==blocked)
            assert(!alloc_race_table[k].prob2);
        /* A scripted unique must not undo the shared defeat state. */
        p_ptr->inside_quest=QUEST_BRUNHILD;
        quest[QUEST_BRUNHILD].status=QUEST_STATUS_TAKEN;
        a_info[ART_BRUNHILD].cur_num=0;
        init_flags=INIT_CREATE_DUNGEON|INIT_ASSIGN;
        assert(!process_dungeon_file("q_info.txt",0,0,MAX_HGT,MAX_WID));
        for(k=1;k<m_max;k++)if(m_list[k].r_idx==MON_ZENOBIA_LANCELOT)guardians++;
        assert(!guardians && r_info[defeated].max_num==0);
        p_ptr->inside_quest=0;p_ptr->playing=TRUE;p_ptr->is_dead=0;
        keys=answers;key_index=0;
        assert(save_player());party_reset();assert(load_player());
        Rand_quick=TRUE;Rand_value=12345+i;
        assert(hamilton_form_is_defeated(blocked));
        p_ptr->inside_quest=blocked==MON_ZENOBIA_LANCELOT ? QUEST_BRUNHILD : 0;
        cave[2][2].feat=FEAT_FLOOR;
        assert(!place_monster_aux(0,2,2,blocked,0));
    }
    puts("Hamilton: both defeat orders exclude the other form in fixed maps, allocation and saved games");
    puts("Brunhild: acceptance gate, reservation/save, 190 themed defenders, 16 independent tower combinations, three harmless decoys, loot, alarm and no teleport passed");
}
"""
h=(ROOT/'tests/party_integration.c').read_text(encoding='euc_jp')
h=h.replace('int main(int argc,char **argv)',EXTRA+'\nint main(int argc,char **argv)',1)
h=h.replace('    display_regressions();','    brunhild_tests();\n    return 0;',1)
with tempfile.TemporaryDirectory(prefix='toband-brunhild-') as d:
    tmp=Path(d)
    shutil.copytree(BUILD/'lib',tmp/'lib',ignore=shutil.ignore_patterns('*.raw','save'))
    for f in (ROOT/'lib/edit').glob('*.txt'):
        (tmp/'lib/edit'/f.name).write_bytes(f.read_bytes().decode('euc_jp').encode('cp932'))
    (tmp/'lib/save').mkdir(exist_ok=True)
    (tmp/'test.c').write_bytes(h.encode('euc_jp'))
    objects=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband','bldg','object2','dungeon')]
    for name,wrapper in [('dungeon','void brun_test_movement(void) { process_world_aux_movement(); }'),('bldg','void brun_test_castle(void) { castle_quest(); }'),('object2','bool brun_test_make(object_type *o) { return make_artifact(o); }')]:
        source=tmp/(name+'.c');obj=tmp/(name+'.o')
        source.write_bytes((ROOT/'src'/(name+'.c')).read_bytes()+('\n'+wrapper+'\n').encode('ascii'))
        subprocess.run([str(GCC),*FLAGS,'-c',str(source),'-o',str(obj)],check=True)
        objects.append(obj)
    subprocess.run([str(GCC),*FLAGS,str(tmp/'test.c'),*map(str,objects),'-o',str(tmp/'test.exe'),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(tmp/'test.exe'),(tmp/'lib').as_posix()+'/'],cwd=tmp,capture_output=True,timeout=30)
    print(result.stdout.decode('cp932',errors='replace'),end='')
    if result.returncode:
        print(result.stderr.decode("cp932",errors="replace"))
        raise SystemExit(result.returncode)

