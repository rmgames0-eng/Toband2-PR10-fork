/* Headless integration test using the real data, birth UI and savefile code. */
#include "angband.h"
#include "party.h"
#include <assert.h>

static term test_term;
static int clear_requests;
static const char *keys;
static int key_index;
static const char party_label[]="\x8c\xf0\x91\xe3";
static int power_step;
static char power_target = 'b';
static bool auto_power;
static bool inspect_party_menu;
static bool auto_recruit;
static int auto_count = -1;
static bool home_test, home_started;
static bool death_input;
static int death_phase, chosen_card;
static int home_target;

static errr test_xtra(int n, int v)
{
    if (n == TERM_XTRA_CLEAR) clear_requests++;
    if (n == TERM_XTRA_EVENT && v)
    {
        if (death_input) {
            int i, j, found[5]; char row[81];
            if (death_phase == 0) {
                for(i=0;i<5;i++) {
                    memcpy(row,Term->scr->c[i+2],80); row[80]=0; found[i]=-1;
                    for(j=0;j<45;j++) if(strstr(row,tarot_info[j].name)) { found[i]=j; break; }
                    assert(found[i]>=0 && (found[i]==0 || found[i]%2==1));
                    for(j=0;j<i;j++) assert(found[i]!=found[j]);
                }
                chosen_card=found[4]; death_phase++; Term_keypress('e'); return 0;
            }
            death_input=FALSE; Term_keypress('c'); return 0;
        }
        if (party_training) {
            char prompt[81]; int x;
            for(x=0;x<80;x++) prompt[x]=Term->scr->c[0][x]; prompt[80]=0;
            Term_keypress(strstr(prompt,"[y/n]") ? 'y' : 'a'); return 0;
        }
        if (inspect_party_menu) {
            int n;
            assert(Term->scr->c[23][75]=='#');
            for(n=0;n<party_count;n++) {
                int elem=party_members[n].player.pelem;
                assert((Term->scr->a[n+2][66] & 0x0f)==elem_attr(elem));
                assert(Term->scr->c[n+2][59]=='-');
            }
            inspect_party_menu=FALSE; Term_keypress(ESCAPE); return 0;
        }
        if(auto_power) {
            int y,x;
            char row[81];
            if(power_step==1) {
                /* The roster must not present a reserve's historical HP as current. */
                for(y=2;y<party_count+2;++y) {
                    for(x=0;x<80;++x) row[x]=Term->scr->c[y][x];
                    row[80]=0; assert(!strstr(row,"HP "));
                }
                auto_power=FALSE; Term_keypress(power_target); return 0;
            }
            for(y=1;y<24;++y) {
                for(x=0;x<80;++x) row[x]=Term->scr->c[y][x];
                row[80]=0;
                if(strstr(row,party_label)) {
                    if (!use_menu) assert(row[1]=='a');
                    power_step=1; Term_keypress(use_menu ? '\r' : row[1]); return 0;
                }
            }
            for(y=0;y<24;++y) { for(x=0;x<80;++x) fputc(Term->scr->c[y][x],stderr); fputc('\n',stderr); }
            fprintf(stderr,"Party entry missing from U menu\n"); exit(4);
        }
        if (auto_recruit && !party_creating) {
            if (home_test && party_count >= home_target) {
                char menu_row[81]; int mx;
                for(mx=0;mx<80;mx++) menu_row[mx]=Term->scr->c[19][mx]; menu_row[80]=0;
                assert(strstr(menu_row,"T)") != NULL);
                for(mx=0;mx<80;mx++) menu_row[mx]=Term->scr->c[21][mx]; menu_row[80]=0;
                assert(strstr(menu_row,"i/e)") != NULL);
                for(mx=0;mx<80;mx++) menu_row[mx]=Term->scr->c[23][mx]; menu_row[80]=0;
                assert(strstr(menu_row,"x)") && strstr(menu_row,"c)"));
                Term_keypress(ESCAPE); return 0;
            }
            if (home_test && !home_started) {
                home_started=TRUE; Term_keypress('a'); return 0;
            }
            Term_keypress(' '); return 0;
        }
        if (auto_recruit && auto_count != party_count) {
            auto_count=party_count;
            keys="Q=\r\r\r\rQ\r\033Recruit\rQ\r";
            key_index=0;
        }
        if (!keys || !keys[key_index])
        {
            int y,x;
            fprintf(stderr,"Input exhausted at %d\n",key_index);
            for(y=0;y<24;++y) {
                for(x=0;x<80;++x) fputc(Term->scr->c[y][x],stderr);
                fputc('\n',stderr);
            }
            exit(2);
        }
        Term_keypress(keys[key_index++]);
    }
    return 0;
}
static errr test_curs(int x,int y) { return 0; }
static errr test_wipe(int x,int y,int n) { return 0; }
static errr test_text(int x,int y,int n,byte a,cptr s) { return 0; }
static void fail_quit(cptr s) { if(s) fprintf(stderr,"quit: %s\n",s); exit(3); }

static long panel_number(int y, int x, int width)
{
    char buf[32]; int i;
    for(i=0;i<width;++i) buf[i]=Term->scr->c[y][x+i];
    buf[width]=0; return atol(buf);
}

static bool term_contains(term *t, const char *text)
{
    int y; char row[81];
    for(y=0;y<24;++y) {
        memcpy(row,t->scr->c[y],80); row[80]=0;
        if(strstr(row,text)) return TRUE;
    }
    return FALSE;
}

static void display_regressions(void)
{
    int mode, x, y, old_class=p_ptr->pclass;
    u32b realms;
    char saved[24][80];
    term sub;
    /* Cancel the U menu and the roster, and choose the current member,
     * with both keyboard-list and cursor-menu input styles. */
    for(mode=0;mode<2;++mode) {
        use_menu=mode; repeat_clear();
        keys="          "; key_index=0; msg_print(NULL);
        Term_clear(); Term_putstr(0,5,-1,TERM_WHITE,"original game screen");
        for(y=0;y<24;++y) memcpy(saved[y],Term->scr->c[y],80);
        keys="\033"; key_index=0;
        do_cmd_racial_power();
        assert(character_icky==0 && energy_use==0);
        for(y=0;y<24;++y) assert(!memcmp(saved[y],Term->scr->c[y],80));
        auto_power=TRUE; power_step=0; power_target=ESCAPE;
        do_cmd_racial_power();
        assert(character_icky==0 && energy_use==0);
        for(y=0;y<24;++y) assert(!memcmp(saved[y],Term->scr->c[y],80));
        auto_power=TRUE; power_step=0; power_target='a'+party_active;
        do_cmd_racial_power();
        assert(character_icky==0 && energy_use==0);
    }
    use_menu=FALSE;
    /* Partial MP refresh must erase the previous caster's MP for a gunner. */
    p_ptr->pclass=CLASS_WIZARD; party_rebind();
    p_ptr->csp=12; p_ptr->msp=34;
    p_ptr->redraw=PR_MANA; redraw_stuff();
    assert(panel_number(ROW_CURSP,COL_CURSP+2,5)==12);
    assert(panel_number(ROW_CURSP,COL_CURSP+8,4)==34);
    p_ptr->pclass=CLASS_GUNNER; party_rebind();
    p_ptr->redraw=PR_MANA; redraw_stuff();
    for(x=0;x<13;++x) assert(Term->scr->c[ROW_CURSP][COL_CURSP+x]==' ');
    p_ptr->pclass=old_class; party_rebind();

    /* Race, title, stats and armor refresh from the active character. */
    p_ptr->redraw=PR_MISC|PR_TITLE|PR_STATS|PR_ARMOR; redraw_stuff();
    for(mode=0;mode<A_MAX;++mode) {
        char stat[32];
        cnv_stat(p_ptr->stat_use[mode],stat);
        assert(!memcmp(&Term->scr->c[ROW_STAT+mode][COL_STAT+6],stat,strlen(stat)));
    }

    /* Render all realm indices, including 10 and 11, as the sole realm.
     * Every first spell belongs on the first row of its column. */
    realms=cp_ptr->realm_choices;
    for(mode=0;mode<MAX_REALM;++mode) {
        const char *name;
        cp_ptr->realm_choices=1L<<mode;
        display_spell_list();
        x=27*(mode%3);
        assert(Term->scr->c[0][x]=='a' && Term->scr->c[0][x+1]=='/');
        name=do_spell(mode+1,0,SPELL_NAME);
        if(mp_ptr->info[mode][0].slevel<99 && name && *name)
            assert(Term->scr->c[0][x+5]==*name);
    }
    cp_ptr->realm_choices=realms;

    /* A spell subwindow must clear text when changing to a non-caster;
     * equipment and inventory subwindows must refresh without changing Term. */
    term_init(&sub,80,24,256);
    sub.xtra_hook=test_xtra; sub.curs_hook=test_curs;
    sub.wipe_hook=test_wipe; sub.text_hook=test_text;
    angband_term[1]=&sub;
    for(mode=0;mode<3;++mode) {
        window_flag[1]=mode==0 ? PW_SPELL : mode==1 ? PW_EQUIP : PW_INVEN;
        Term_activate(&sub); Term_putstr(0,23,-1,TERM_WHITE,"stale window");
        Term_activate(&test_term);
        p_ptr->window=window_flag[1]; window_stuff();
        assert(Term==&test_term);
        for(x=0;x<12;++x) assert(sub.scr->c[23][x]==' ');
    }
    for(mode=0;mode<2;++mode) {
        char description[MAX_NLEN];
        keys="          "; key_index=0; msg_print(NULL);
        assert(party_switch(1-party_active));
        window_flag[1]=PW_PLAYER;
        p_ptr->window=PW_PLAYER; window_stuff();
        assert(term_contains(&sub,player_name));
        assert(term_contains(&sub,c_name+cp_ptr->name));
        window_flag[1]=PW_EQUIP;
        p_ptr->window=PW_EQUIP; window_stuff();
        object_desc(description,&inventory[INVEN_RARM],0);
        assert(!memcmp(&sub.scr->c[0][3],description,MIN(strlen(description),40)));
        assert(Term==&test_term);
    }
    angband_term[1]=NULL; window_flag[1]=0;
    term_nuke(&sub);
}

static void mechanics_regressions(void)
{
    int target=1-party_active, c;
    s32b gold=p_ptr->au[SV_GOLD_NOTE];
    object_type pack[INVEN_RARM];
    memcpy(pack,inventory,sizeof(pack));
    object_type armor;
    party_member saved=party_members[target];
    keys="                                                                "; key_index=0; msg_print(NULL);
    object_prep(&armor,lookup_kind(TV_SOFT_ARMOR,SV_ROBE));
    armor.number=1;
    add_flag(armor.art_flags,TR_ALIGN_LNC);
    add_flag(armor.art_flags,TR_ALIGN_GNE);
    armor.to_align[ALI_LNC]=4; armor.to_align[ALI_GNE]=-3;
    party_members[target].equipment[INVEN_BODY-INVEN_RARM]=armor;
    party_members[target].player.align_self[ALI_LNC]=10;
    party_members[target].player.align_self[ALI_GNE]=20;
    assert(party_switch(target));
    assert(p_ptr->align[ALI_LNC]==50+friend_align_lnc);
    assert(p_ptr->align[ALI_GNE]==-10+friend_align_gne);
    assert(party_switch(1-target));
    party_members[target]=saved;
    p_ptr->wraith_form=5;
    assert(party_switch(target));
    assert(!p_ptr->wraith_form);
    assert(party_switch(1-target));
    assert(p_ptr->wraith_form==5 && p_ptr->no_flowed);
    p_ptr->wraith_form=0;
    /* Real bonuses for every class must settle before returning from switch.
     * A normal update afterwards must not change HP/MP maxima, speed or armor. */
    for(c=0;c<max_c_idx;++c) {
        s32b hp,mp;
        int speed,ac;
        party_members[target]=saved;
        party_members[target].player.pclass=c;
        party_members[target].player.cexp_info[c].clev=1;
        party_members[target].player.cexp_info[c].max_clev=1;
        party_members[target].player.cexp_info[c].max_max_clev=1;
        keys="                                                                "; key_index=0; msg_print(NULL);
        assert(party_switch(target));
        hp=p_ptr->mhp; mp=p_ptr->msp; speed=p_ptr->pspeed; ac=p_ptr->to_a;
        p_ptr->update=PU_BONUS|PU_HP|PU_MANA;
        character_icky=TRUE; update_stuff(); character_icky=FALSE;
        assert(p_ptr->mhp==hp && p_ptr->msp==mp && p_ptr->pspeed==speed && p_ptr->to_a==ac);
        assert(p_ptr->chp>=1 && p_ptr->chp<=p_ptr->mhp && p_ptr->csp<=p_ptr->msp);
        assert(party_switch(1-target));
    }
    for(c=0;c<max_p_idx;++c) {
        s32b hp,mp;
        int speed,ac;
        party_members[target]=saved;
        party_members[target].player.prace=c;
        keys="                                                                "; key_index=0; msg_print(NULL);
        assert(party_switch(target));
        hp=p_ptr->mhp; mp=p_ptr->msp; speed=p_ptr->pspeed; ac=p_ptr->to_a;
        p_ptr->update=PU_BONUS|PU_HP|PU_MANA;
        character_icky=TRUE; update_stuff(); character_icky=FALSE;
        assert(p_ptr->mhp==hp && p_ptr->msp==mp && p_ptr->pspeed==speed && p_ptr->to_a==ac);
        assert(p_ptr->chp>=1 && p_ptr->chp<=p_ptr->mhp && p_ptr->csp<=p_ptr->msp);
        assert(party_switch(1-target));
    }
    assert(p_ptr->au[SV_GOLD_NOTE]==gold && !memcmp(pack,inventory,sizeof(pack)));
    assert(p_ptr->poisoned==7 && p_ptr->oppose_cold==30);
    party_members[target]=saved;
}


static void side_effect_regressions(void)
{
    static party_member saved[MAX_PARTY_MEMBERS];
    player_type original=*p_ptr;
    object_type items[INVEN_TOTAL], unidentified;
    monster_type monster=m_list[1];
    s16b old_mmax=m_max,old_energy=energy_use;
    byte active=party_active;
    int target=1-party_active,c;
    u32b realms[MAX_CLASS];
    char name[32];
    memcpy(saved,party_members,sizeof(saved)); memcpy(items,inventory,sizeof(items));
    strcpy(name,player_name);
    for(c=0;c<max_c_idx;++c) realms[c]=class_info[c].realm_choices;
    keys="                                                                "; key_index=0; msg_print(NULL);
    p_ptr->pclass=CLASS_MEDIUM; p_ptr->realm_medium=CH_AQUA;
    p_ptr->pelem=p_ptr->celem=ELEM_AQUA;
    party_rebind(); init_realm_table();
    party_members[target].player.pclass=CLASS_MEDIUM;
    party_members[target].player.realm_medium=CH_FIRE|CH_HOLY;
    party_members[target].player.pelem=party_members[target].player.celem=ELEM_FIRE;
    assert(party_switch(target));
    assert(cp_ptr->realm_choices==(CH_FIRE|CH_HOLY));
    assert(party_switch(active));
    assert(cp_ptr->realm_choices==CH_AQUA);
    m_max=2; m_list[1].r_idx=1; m_list[1].silent_song=TRUE;
    m_list[1].ml=FALSE;
    p_ptr->singing=MUSIC_SILENT;
    assert(party_switch(target));
    assert(!p_ptr->singing && !m_list[1].silent_song);
    object_prep(&unidentified,lookup_kind(TV_SOFT_ARMOR,SV_ROBE));
    unidentified.number=1; unidentified.ident=0;
    party_members[active].equipment[INVEN_BODY-INVEN_RARM]=unidentified;
    party_identify_reserve_equipment();
    assert(party_members[active].equipment[INVEN_BODY-INVEN_RARM].ident & IDENT_KNOWN);
    assert(party_active==target);
    *p_ptr=original; memcpy(inventory,items,sizeof(items));
    memcpy(party_members,saved,sizeof(saved)); party_active=active;
    strcpy(player_name,name); m_list[1]=monster; m_max=old_mmax; energy_use=old_energy;
    for(c=0;c<max_c_idx;++c) class_info[c].realm_choices=realms[c];
    party_rebind();
}

static void dump_regressions(void)
{
    static party_member members[MAX_PARTY_MEMBERS], initial[MAX_PARTY_MEMBERS];
    player_type player;
    object_type items[INVEN_TOTAL];
    int mode,i;
    byte count=party_count,active=party_active,rewards=party_rewards;
    s32b saved_turn=turn;
    u32b saved_random=Rand_value;
    s16b energy=energy_use;
    char name[32];
    memcpy(initial,party_members,sizeof(initial));
    for(mode=0;mode<3;++mode) {
        FILE *out;
        char *text;
        long size;
        if(mode==1) party_count=0; /* A legacy one-person save must stay unmodified. */
        if(mode==2) {
            party_count=MAX_PARTY_MEMBERS;
            for(i=count;i<MAX_PARTY_MEMBERS;++i) {
                party_members[i]=initial[1];
                sprintf(party_members[i].name,"Reserve%02d",i+1);
            }
            party_members[1].player.realm_medium=CH_FIRE|CH_WIND;
            party_members[1].player.essence_box[0]=9876;
        }
        player=*p_ptr; memcpy(items,inventory,sizeof(items));
        memcpy(members,party_members,sizeof(members)); strcpy(name,player_name);
        out=tmpfile(); assert(out);
        assert(!make_character_dump(out));
        fflush(out); size=ftell(out); rewind(out);
        text=malloc(size+1); assert(text);
        assert(fread(text,1,size,out)==size); text[size]=0; fclose(out);
        assert(strstr(text,name));
        assert(strstr(text,"\133\222\207\212\324\210\352\227\227\135"));
        assert(!strstr(strstr(text,"\133\222\207\212\324\210\352\227\227\135")+1,"\133\222\207\212\324\210\352\227\227\135"));
        assert(strstr(text,"\133\213\244\227\114\217\356\225\361\135"));
        assert(!strstr(strstr(text,"\133\213\244\227\114\217\356\225\361\135")+1,"\133\213\244\227\114\217\356\225\361\135"));
        assert(strstr(text,"\133\203\114\203\203\203\211\203\116\203\136\202\314\216\235\202\277\225\250\135"));
        assert(!strstr(strstr(text,"\133\203\114\203\203\203\211\203\116\203\136\202\314\216\235\202\277\225\250\135")+1,"\133\203\114\203\203\203\211\203\116\203\136\202\314\216\235\202\277\225\250\135"));
        if(mode!=1) {
            const char *reserve=strstr(text,"\133\220\154\225\250\040\062\072");
            const char *end=strstr(reserve,"\133\213\244\227\114\217\356\225\361\135");
            const char *hp=strstr(reserve,"HP");
            assert(reserve && end && hp && hp<end);
        }
        if(mode!=1) assert(strstr(text,party_members[1].name));
        if(mode==2) assert(strstr(text,"Reserve16"));
        assert(!memcmp(&player,p_ptr,sizeof(player)));
        assert(!memcmp(items,inventory,sizeof(items)));
        assert(!memcmp(members,party_members,sizeof(members)));
        assert(Rand_value==saved_random && turn==saved_turn && energy_use==energy && party_active==active && party_rewards==rewards);
        assert(party_count==(mode==0 ? count : mode==1 ? 0 : MAX_PARTY_MEMBERS));
        assert(!strcmp(name,player_name));
        free(text);
    }
    memcpy(party_members,initial,sizeof(initial)); party_count=count;
}


static void test_visible_monster_window(void)
{
    term sub;
    monster_type saved_monsters[7];
    monster_race saved_races[4];
    char *saved_names = r_name;
    char names[] = "\0Soldier\0Unique\0Secret";
    char row[81];
    int saved_max = m_max, x, i;
    u32b saved_window = p_ptr->window, saved_rng = Rand_value;
    memcpy(saved_monsters, m_list, sizeof(saved_monsters));
    memcpy(saved_races, r_info, sizeof(saved_races));
    memset(m_list, 0, sizeof(saved_monsters));
    memset(r_info, 0, sizeof(saved_races));
    r_name = names;
    r_info[1].name=1; r_info[1].d_char='p'; r_info[1].level=12; r_info[1].r_tkills=1;
    r_info[2].name=9; r_info[2].d_char='U'; r_info[2].flags1=RF1_UNIQUE;
    r_info[3].name=16; r_info[3].d_char='S';
    for(i=1;i<7;i++) { m_list[i].r_idx=m_list[i].ap_r_idx=1; m_list[i].ml=TRUE; }
    MON_CSLEEP(&m_list[2])=10;
    m_list[3].r_idx=m_list[3].ap_r_idx=2;
    m_list[4].r_idx=m_list[4].ap_r_idx=3; m_list[4].ml=FALSE;
    m_list[5].smart1=SM1_PET;
    m_list[6].r_idx=0;
    /* A disguised monster must not leak its real race. */
    m_list[1].r_idx=3;
    m_max=7;
    term_init(&sub,80,5,256);
    sub.xtra_hook=test_xtra; sub.curs_hook=test_curs;
    sub.wipe_hook=test_wipe; sub.text_hook=test_text;
    angband_term[1]=&sub; window_flag[1]=PW_MONLIST;
    p_ptr->window=0; window_stuff();
    assert(Term==&test_term);
    for(x=0;x<80;x++) row[x]=sub.scr->c[0][x]; row[80]=0;
    assert(strstr(row,"Unique") && strstr(row,"??"));
    for(x=0;x<80;x++) row[x]=sub.scr->c[1][x]; row[80]=0;
    assert(strstr(row,"Soldier") && strstr(row,"12"));
    assert(row[2]=='2' && row[7]=='1');
    i=clear_requests; window_stuff(); assert(clear_requests==i);
    MON_CSLEEP(&m_list[2])=0;
    window_stuff(); assert(sub.scr->c[1][7]=='2');
    m_list[1].ml=m_list[2].ml=FALSE;
    window_stuff(); assert(sub.scr->c[1][2]==' ');
    p_ptr->image=1; window_stuff();
    for(x=0;x<80;x++) row[x]=sub.scr->c[0][x]; row[80]=0;
    assert(!strstr(row,"Unique"));
    p_ptr->image=0;
    m_list[1].ml=m_list[2].ml=TRUE;
    Term_activate(&sub); Term_resize(8,1); Term_activate(&test_term);
    window_stuff(); assert(Term==&test_term);
    assert(Rand_value==saved_rng);
    angband_term[1]=NULL; window_flag[1]=0; term_nuke(&sub);
    memcpy(m_list,saved_monsters,sizeof(saved_monsters));
    memcpy(r_info,saved_races,sizeof(saved_races));
    r_name=saved_names; m_max=saved_max; p_ptr->window=saved_window;
}



/* A bulk XP award must give exactly the same growth as one level at a time. */
static void test_training_batch_growth(void)
{
    player_type old_player=*p_ptr;
    party_member old_member=party_members[1], baseline, bulk;
    u32b rng;
    int job, level, stat, target;
    bool old_more=auto_more;
    auto_more=TRUE;
    for(target=8;target<=30;target+=22) for(job=0;job<2;job++) {
        baseline=old_member;
        baseline.player.pclass=job ? CLASS_WITCH : CLASS_SOLDIER;
        memset(baseline.player.cexp_info,0,sizeof(baseline.player.cexp_info));
        baseline.player.cexp_info[baseline.player.pclass].clev=1;
        baseline.player.cexp_info[baseline.player.pclass].max_clev=1;
        baseline.player.cexp_info[baseline.player.pclass].max_max_clev=1;
        for(stat=0;stat<A_MAX;stat++) baseline.player.stat_cur[stat]=baseline.player.stat_max[stat]=16;
        p_ptr->lev=1;
        p_ptr->cexp_info[p_ptr->pclass].clev=target+1;
        party_members[1]=baseline;
        rng=Rand_value;
        assert(party_train_member(1));
        bulk=party_members[1];
        party_members[1]=baseline; Rand_value=rng;
        for(level=2;level<=target;level++) {
            p_ptr->cexp_info[p_ptr->pclass].clev=level+1;
            assert(party_train_member(1));
        }
        for(stat=0;stat<A_MAX;stat++) {
            if(bulk.player.stat_max[stat]!=party_members[1].player.stat_max[stat])
                fprintf(stderr,"Batch growth mismatch class %d stat %d: bulk %d / sequential %d\n",
                    baseline.player.pclass,stat,bulk.player.stat_max[stat],party_members[1].player.stat_max[stat]);
            assert(bulk.player.stat_max[stat]==party_members[1].player.stat_max[stat]);
        }
        assert(bulk.player.gx_thn==party_members[1].player.gx_thn);
        assert(!memcmp(bulk.player.weapon_exp,party_members[1].player.weapon_exp,sizeof(bulk.player.weapon_exp)));
        assert(!memcmp(bulk.player.class_hp,party_members[1].player.class_hp,sizeof(bulk.player.class_hp)));
    }
    *p_ptr=old_player; party_members[1]=old_member; auto_more=old_more; party_rebind();
}

static void test_party_training(void)
{
    player_type old_player=*p_ptr, before;
    party_member old_members[MAX_PARTY_MEMBERS];
    object_type old_inventory[INVEN_TOTAL];
    u32b rng;
    int old_energy=energy_use, old_depth=character_icky;
    bool old_more=auto_more;
    s32b old_turn=turn;
    memcpy(old_members,party_members,sizeof(old_members));
    memcpy(old_inventory,inventory,sizeof(old_inventory));
    auto_more=TRUE;
    p_ptr->lev=8; p_ptr->cexp_info[p_ptr->pclass].clev=5;
    before=*p_ptr;
    assert(party_train_member(1));
    assert(party_members[1].player.lev==7);
    assert(party_members[1].player.cexp_info[party_members[1].player.pclass].clev==4);
    assert(!memcmp(&before,p_ptr,sizeof(before)));
    assert(!memcmp(old_inventory,inventory,sizeof(old_inventory)));
    assert(party_members[1].player.chp==old_members[1].player.chp);
    assert(party_members[1].player.csp==old_members[1].player.csp);
    rng=Rand_value;
    assert(!party_train_member(1) && Rand_value==rng);
    assert(!party_train_member(party_active));
    p_ptr->lev=50; p_ptr->cexp_info[p_ptr->pclass].clev=50;
    before=*p_ptr;
    assert(party_train_member(1));
    assert(party_members[1].player.lev==30);
    assert(party_members[1].player.cexp_info[party_members[1].player.pclass].clev==30);
    assert(party_members[1].player.exp==player_exp[28]*party_members[1].player.expfact/100);
    assert(party_members[1].player.max_exp==party_members[1].player.exp);
    assert(party_members[1].player.max_plv==30);
    assert(party_members[1].player.race_hp[29]>=0);
    assert(!memcmp(&before,p_ptr,sizeof(before)));
    assert(!memcmp(old_inventory,inventory,sizeof(old_inventory)));
    rng=Rand_value; assert(!party_train_member(1) && Rand_value==rng);
    p_ptr->lev=1; p_ptr->cexp_info[p_ptr->pclass].clev=1;
    assert(!party_train_member(1));
    assert(party_members[1].player.lev==30);
    assert(turn==old_turn && energy_use==old_energy && character_icky==old_depth && !party_training);
    *p_ptr=old_player;
    memcpy(party_members,old_members,sizeof(old_members));
    auto_more=old_more; party_rebind();
}



static void test_training_keymap(void)
{
    bool old_rogue=rogue_like_commands;
    cptr old_map=keymap_act[KEYMAP_MODE_ROGUE]['T'];
    rogue_like_commands=TRUE;
    keymap_act[KEYMAP_MODE_ROGUE]['T']="t";
    keys="T"; key_index=0; request_command(TRUE); assert(command_cmd=='t');
    keys="\\T"; key_index=0; request_command(TRUE); assert(command_cmd=='T');
    keymap_act[KEYMAP_MODE_ROGUE]['T']=old_map;
    rogue_like_commands=old_rogue;
}

static void test_string_copy_boundaries(void)
{
    char out[16];
    const char short_source[]={'A',0,'B','C','D','E',0,0,0,0,0,0,0,0,0,0};
    const char japanese[]={ (char)0x82,(char)0xa0,'X',0 };
    memset(out,'!',sizeof(out));
    assert(my_strcpy(out,short_source,8)==1);
    assert(out[0]=='A' && out[1]==0 && out[2]=='!');
    assert(my_strcpy(out,"AB",0)==2 && out[2]=='!');
    assert(my_strcpy(out,"AB",1)==2 && out[0]==0);
    assert(my_strcpy(out,japanese,2)==3 && out[0]==0);
    assert(my_strcpy(out,japanese,3)==3 && out[2]==0);
}

static void death_regressions(void)
{
    int i, n, card_count, equip_count, objects;
    object_type gear[INVEN_TOTAL-INVEN_RARM];
    player_type *reserve;
    p_ptr->playing=TRUE; p_ptr->is_dead=0; p_ptr->inside_arena=FALSE;
    p_ptr->paralyzed=p_ptr->stun=p_ptr->stoning=p_ptr->infected=0;
    p_ptr->tim_resurrection=0; p_ptr->pclass=CLASS_SOLDIER;
    p_ptr->chp=20; p_ptr->mhp=100; p_ptr->csp=5;
    p_ptr->riding=0; p_ptr->update=p_ptr->redraw=p_ptr->window=0;
    can_save=TRUE; /* The test runner isolates the save path. */
    party_rebind(); party_reset(); party_capture();
    party_count=3;
    for(i=1;i<3;i++) {
        party_members[i]=party_members[0];
        sprintf(party_members[i].name,"Survivor%d",i);
        party_members[i].player.chp=10+i;
        party_members[i].player.csp=2+i;
    }
    /* Arena commands cannot switch, and defeat stays with arena exit handling. */
    {
        party_member roster[MAX_PARTY_MEMBERS];
        object_type equipment[INVEN_TOTAL-INVEN_RARM];
        int old_objects=o_cnt;
        memcpy(roster,party_members,sizeof(roster));
        memcpy(equipment,&inventory[INVEN_RARM],sizeof(equipment));
        p_ptr->inside_arena=TRUE;
        assert(!party_switch(1) && party_active==0);
        p_ptr->chp=-1; p_ptr->is_dead=DEATH_DEAD;
        assert(!party_handle_death());
        assert(p_ptr->inside_arena && p_ptr->is_dead && party_active==0);
        assert(!memcmp(roster,party_members,sizeof(roster)));
        assert(!memcmp(equipment,&inventory[INVEN_RARM],sizeof(equipment)));
        assert(o_cnt==old_objects);
        p_ptr->inside_arena=FALSE; p_ptr->chp=20; p_ptr->is_dead=0;
        puts("Arena: switching rejected without a turn; defeat bypasses party death and drops");
    }
    /* Avoid resurrection artifacts and use ordinary cursed equipment. */
    for(i=INVEN_RARM;i<INVEN_TOTAL;i++) object_wipe(&inventory[i]);
    object_prep(&inventory[INVEN_RARM],lookup_kind(TV_SWORD,SV_DAGGER));
    inventory[INVEN_RARM].number=1; inventory[INVEN_RARM].curse_flags=TRC_CURSED;
    object_prep(&inventory[INVEN_BODY],lookup_kind(TV_SOFT_ARMOR,SV_ROBE));
    inventory[INVEN_BODY].number=1;
    memcpy(gear,&inventory[INVEN_RARM],sizeof(gear));
    wipe_o_list();
    take_hit(DAMAGE_LOSELIFE,1000,"death regression");
    assert(p_ptr->is_dead && party_active==0 && inventory[INVEN_RARM].k_idx);
    death_input=TRUE; death_phase=0;
    assert(party_handle_death());
    assert(!death_input && !character_icky && party_active==2 && !p_ptr->is_dead && !p_ptr->leaving);
    assert(p_ptr->chp==12 && p_ptr->csp==4);
    assert(party_members[0].dead && !party_switch(0) && !party_train_member(0));
    for(i=0;i<INVEN_TOTAL-INVEN_RARM;i++) assert(!party_members[0].equipment[i].k_idx);
    card_count=equip_count=0;
    for(i=1;i<o_max;i++) if(o_list[i].k_idx) {
        if(o_list[i].tval==TV_TAROT) { card_count++; assert(o_list[i].pval==chosen_card && o_list[i].number==1); }
        else {
            for(n=0;n<INVEN_TOTAL-INVEN_RARM;n++) if(gear[n].k_idx==o_list[i].k_idx) break;
            assert(n<INVEN_TOTAL-INVEN_RARM); assert(o_list[i].curse_flags==gear[n].curse_flags); equip_count++;
        }
    }
    assert(card_count==1 && equip_count==2 && inventory[0].number==7);
    reserve=&party_members[1].player;
    reserve->chp=1; reserve->csp=0;
    reserve->poisoned=reserve->cut=0;
    for(i=0;i<100;i++) party_regenerate();
    assert(reserve->chp>1 && party_members[0].player.chp<0);
    assert(save_player()); party_reset(); assert(load_player());
    assert(party_members[0].dead && party_active==2 && !party_switch(0));
    /* One survivor is selected automatically; terminal death never switches. */
    p_ptr->playing=TRUE; p_ptr->is_dead=DEATH_DEAD; p_ptr->chp=-1;
    death_input=TRUE; death_phase=0; assert(party_handle_death()); death_input=FALSE;
    assert(party_active==1 && party_members[2].dead && !p_ptr->is_dead);
    p_ptr->is_dead=DEATH_DEAD; p_ptr->chp=-1;
    death_input=TRUE; death_phase=0; assert(!party_handle_death()); death_input=FALSE;
    assert(death_phase==0); /* Final death must not request tarot input. */
    card_count=0;
    for(i=1;i<o_max;i++) if(o_list[i].k_idx && o_list[i].tval==TV_TAROT) card_count+=o_list[i].number;
    assert(card_count==2); /* Only the two deaths with successors drop cards. */
    assert(party_members[1].dead && !party_has_successor());
    objects=o_cnt; assert(!party_handle_death()); assert(o_cnt==objects);
    assert(save_player()); party_reset(); assert(load_player());
    assert(party_members[0].dead && party_members[1].dead && party_members[2].dead);
    /* Resurrection targets dead reserves, never the caster or living members. */
    p_ptr->playing=TRUE; p_ptr->is_dead=0; p_ptr->chp=20;
    party_capture();
    assert(party_has_dead_member());
    assert(!party_revive_member(party_active));
    keys="\033"; key_index=0;
    assert(!do_spell(REALM_HOLY,23,SPELL_CAST));
    assert(party_members[0].dead && party_active==1 && p_ptr->chp==20);
    keys="a"; key_index=0;
    assert(do_spell(REALM_HOLY,23,SPELL_CAST));
    assert(!party_members[0].dead && party_members[2].dead);
    assert(party_active==1 && p_ptr->chp==20 && !p_ptr->tim_resurrection);
    assert(party_members[0].player.chp==party_members[0].player.mhp);
    assert(party_members[0].player.csp==0 && !party_members[0].player.is_dead);
    for(i=0;i<INVEN_TOTAL-INVEN_RARM;i++) assert(!party_members[0].equipment[i].k_idx);
    assert(!party_revive_member(0));
    assert(save_player()); party_reset(); assert(load_player());
    assert(!party_members[0].dead && party_members[2].dead);
    p_ptr->playing=TRUE;
    assert(party_switch(0));
    assert(p_ptr->chp==p_ptr->mhp && p_ptr->csp==0);
    assert(party_revive_member(2) && !party_has_dead_member());
    assert(!party_resurrect());
    assert(party_members[0].revived && party_members[2].revived);
    assert(save_player()); party_reset(); assert(load_player());
    assert(party_members[0].revived && party_members[2].revived);
    /* Leave one survivor so any tarot input would be an error. */
    party_members[1].dead=TRUE;
    p_ptr->playing=TRUE; p_ptr->is_dead=DEATH_DEAD; p_ptr->chp=-1;
    keys=""; key_index=0;
    card_count=0;
    for(i=1;i<o_max;i++) if(o_list[i].k_idx && o_list[i].tval==TV_TAROT) card_count+=o_list[i].number;
    assert(party_handle_death() && party_active==2);
    n=0;
    for(i=1;i<o_max;i++) if(o_list[i].k_idx && o_list[i].tval==TV_TAROT) n+=o_list[i].number;
    assert(n==card_count && party_members[0].dead && party_members[0].revived);
    assert(party_revive_member(0));
    assert(party_switch(0));
    p_ptr->is_dead=DEATH_DEAD; p_ptr->chp=-1;
    assert(party_handle_death() && party_active==2);
    n=0;
    for(i=1;i<o_max;i++) if(o_list[i].k_idx && o_list[i].tval==TV_TAROT) n+=o_list[i].number;
    assert(n==card_count);
    puts("Revived members never generate another tarot, including after save/load and repeated revival");
    /* Necromancy changes only the selected dead member, with both outcomes. */
    {
        int seen = 0, trial, race;
        player_type caster = *p_ptr;
        player_type target = party_members[0].player;
        keys="\033"; key_index=0;
        assert(!do_spell(REALM_DEATH,25,SPELL_CAST));
        assert(party_members[0].dead && party_members[0].player.prace==target.prace);
        assert(fake_spell_flags[REALM_DEATH-1][1] & (1UL<<25));
        assert(m_info[CLASS_LICH].info[REALM_DEATH-1][25].slevel==50);
        assert(m_info[CLASS_LICH].info[REALM_DEATH-1][25].smana==150);
        for (trial=0; trial<64; trial++)
        {
            Rand_quick = TRUE; Rand_value = 12345 + trial * 7919;
            party_members[0].dead=TRUE;
            keys="a"; key_index=0;
            assert(do_spell(REALM_DEATH,25,SPELL_CAST));
            race=party_members[0].player.prace;
            assert(race==RACE_SKELETON || race==RACE_GHOST);
            seen |= race==RACE_SKELETON ? 1 : 2;
            assert(!party_members[0].dead && party_members[0].revived);
            assert(party_members[0].player.chp==party_members[0].player.mhp);
            assert(party_members[0].player.csp==0);
            assert(party_members[0].player.exp==target.exp);
            assert(party_members[0].player.lev==target.lev);
            assert(party_members[0].player.pclass==target.pclass);
            assert(!memcmp(party_members[0].player.cexp_info,target.cexp_info,sizeof(target.cexp_info)));
            assert(!memcmp(party_members[0].player.magic_exp,target.magic_exp,sizeof(target.magic_exp)));
            assert(party_active==2 && p_ptr->prace==caster.prace);
            assert(p_ptr->chp==caster.chp && p_ptr->csp==caster.csp);
            assert(rp_ptr==&race_info[caster.prace]);
        }
        assert(seen==3);
        race=party_members[0].player.prace;
        assert(save_player()); party_reset(); assert(load_player());
        assert(party_members[0].player.prace==race && party_members[0].revived);
        p_ptr->playing=TRUE;
        assert(party_switch(0));
        assert(p_ptr->prace==race && p_ptr->chp==p_ptr->mhp && p_ptr->csp==0);
        assert(party_revive_member(1));
        assert(!do_spell(REALM_DEATH,25,SPELL_CAST));
        puts("Necromancy: cancellation, both races, private state, spell access, save/load and switching passed");
    }
    /* Reincarnation is a targeted transformation, not self-reincarnation. */
    {
        player_type caster = *p_ptr;
        party_member target = party_members[1];
        object_type pack[INVEN_PACK];
        int race;
        memcpy(pack, inventory, sizeof(pack));
        party_members[2].dead=TRUE;
        party_members[1].player.prace=RACE_HUMAN;
        assert(!party_has_reincarnation_target()); /* Undead caster excluded. */
        assert(!do_spell(REALM_DRAKONITE,6,SPELL_CAST));
        for (race=RACE_SKELETON; race<=RACE_GHOST; race++)
        {
            party_members[1]=target;
            party_members[1].player.prace=race;
            party_members[1].dead=TRUE;
            assert(!party_has_reincarnation_target());
            party_members[1].dead=FALSE;
            assert(party_has_reincarnation_target());
            keys="\033"; key_index=0;
            assert(!do_spell(REALM_DRAKONITE,6,SPELL_CAST));
            assert(party_members[1].player.prace==race);
            keys="bn"; key_index=0;
            assert(!do_spell(REALM_DRAKONITE,6,SPELL_CAST));
            assert(party_members[1].player.prace==race);
            keys="by"; key_index=0;
            assert(do_spell(REALM_DRAKONITE,6,SPELL_CAST));
            assert(party_members[1].player.prace==RACE_HUMAN);
            assert(party_members[1].player.lev==1 && party_members[1].player.exp==0);
            assert(party_members[1].player.reincarnate_cnt==target.player.reincarnate_cnt+1);
            assert(party_members[1].revived==target.revived);
            assert(!memcmp(party_members[1].equipment,target.equipment,sizeof(target.equipment)));
            assert(!memcmp(pack,inventory,sizeof(pack)));
            assert(!memcmp(&caster,p_ptr,sizeof(caster)));
            assert(rp_ptr==&race_info[caster.prace]);
            assert(!party_has_reincarnation_target());
        }
        assert(save_player()); party_reset(); assert(load_player());
        assert(party_members[1].player.prace==RACE_HUMAN && party_members[1].player.lev==1);
        p_ptr->playing=TRUE;
        assert(party_switch(1));
        assert(p_ptr->prace==RACE_HUMAN && p_ptr->lev==1);
        puts("Reincarnation: eligible races, dead/self rejection, cancellation, private transformation and persistence passed");
    }
    /* The class also requires one of its entry artifacts. */
    object_prep(&inventory[INVEN_HEAD], lookup_kind(a_info[41].tval, a_info[41].sval));
    inventory[INVEN_HEAD].name1 = 41;
    /* Relics Knight requires no living companion, not an empty roster. */
    p_ptr->prace=RACE_GOBLIN; p_ptr->pclass=CLASS_SOLDIER;
    p_ptr->psex=SEX_MALE;
    p_ptr->align[ALI_GNE]=p_ptr->align_self[ALI_GNE]=-300;
    p_ptr->align[ALI_LNC]=p_ptr->align_self[ALI_LNC]=-300;
    for(i=0;i<A_MAX;i++) p_ptr->stat_max[i]=STAT_MAX_MAX;
    memset(p_ptr->cexp_info,0,sizeof(p_ptr->cexp_info));
    party_rebind();
    party_active=0; party_count=1;
    assert(can_choose_class(CLASS_RELICSKNIGHT,CLASS_CHOOSE_MODE_NORMAL));
    party_count=3;
    party_members[1].dead=party_members[2].dead=TRUE;
    assert(can_choose_class(CLASS_RELICSKNIGHT,CLASS_CHOOSE_MODE_NORMAL));
    party_members[2].dead=FALSE; party_members[2].player.chp=0;
    assert(!can_choose_class(CLASS_RELICSKNIGHT,CLASS_CHOOSE_MODE_NORMAL));
    party_members[2].player.chp=100;
    assert(!can_choose_class(CLASS_RELICSKNIGHT,CLASS_CHOOSE_MODE_NORMAL));
    puts("Relics Knight: solo/all-dead eligible, living reserve including zero HP rejected");
    party_count=1; party_rewards=0;
    quest[QUEST_ARMORICA].status=QUEST_STATUS_FINISHED;
    p_ptr->is_dead=0; p_ptr->chp=20;
    p_ptr->pclass=CLASS_RELICSKNIGHT; party_rebind();
    assert(!party_can_recruit());
    do_cmd_party_recruit();
    assert(party_count==1 && party_rewards==0 && !party_creating);
    p_ptr->pclass=CLASS_SOLDIER; party_rebind();
    assert(party_can_recruit());
    puts("Relics Knight: home recruitment blocked without consuming an earned slot; other classes eligible");

    puts("Companion resurrection: cancellation, full HP/zero MP, equipment, save/load and switching passed");
    puts("Deferred death, selection, equipment/tarot drops, regeneration and death persistence passed");
}

int main(int argc,char **argv)
{
    char path[1024];
    player_type original;
    object_type original_pack[INVEN_TOTAL];
    party_member first_recruit;
    s32b original_turn;
    u32b original_realms[MAX_CLASS];
    int i;
    assert(argc==2 || argc==3);
    quit_aux=fail_quit;
    term_init(&test_term,80,24,256);
    test_term.xtra_hook=test_xtra;
    test_term.curs_hook=test_curs;
    test_term.wipe_hook=test_wipe;
    test_term.text_hook=test_text;
    Term_activate(&test_term);
    angband_term[0]=&test_term;
    strcpy(path,argv[1]);
    init_file_paths(path);
    init_angband();
    test_string_copy_boundaries();
    test_training_keymap();
    Rand_quick=TRUE; Rand_value=12345;
    test_visible_monster_window();
    p_ptr->lev=p_ptr->max_plv=p_ptr->max_max_plv=1;
    p_ptr->race_hp[0]=p_ptr->mhp=100; p_ptr->chp=37;
    p_ptr->race_sp[0]=p_ptr->msp=50; p_ptr->csp=13;
    p_ptr->food=7000; p_ptr->au[SV_GOLD_NOTE]=12345;
    p_ptr->poisoned=7; p_ptr->oppose_cold=30;
    for(i=0;i<A_MAX;++i) p_ptr->stat_cur[i]=p_ptr->stat_max[i]=15;
    p_ptr->expfact=race_info[0].r_exp;
    for(i=0;i<MAX_CLASS;++i) p_ptr->cexpfact[i]=class_info[i].c_exp;
    p_ptr->cexp_info[0].clev=p_ptr->cexp_info[0].max_clev=p_ptr->cexp_info[0].max_max_clev=1;
    p_ptr->energy_need=-27;
    strcpy(player_name,"Original");
    strcpy(savefile,"party-integration.sav");
    party_rebind();
    object_prep(&inventory[0],lookup_kind(TV_FOOD,SV_FOOD_RATION));
    inventory[0].number=7;
    inven_cnt=1;
    turn=9876; original_turn=turn;
    original=*p_ptr;
    memcpy(original_pack,inventory,sizeof(original_pack));
    party_capture();
    /* Q and '=' must be ignored, then choose sex/race/class/element, stats,
     * finish background editing, enter a name and accept recruitment. */
    keys="Q=\r\r\r\rQ\r\033Recruit\rQ\r"; key_index=0;
    for(i=0;i<max_c_idx;++i) original_realms[i]=class_info[i].realm_choices;
    party_birth_member();
    for(i=0;i<max_c_idx;++i) assert(original_realms[i]==class_info[i].realm_choices);
    assert(party_count==2 && party_active==0 && !party_creating);
    assert(character_icky==0);
    original.redraw=p_ptr->redraw; original.window=p_ptr->window;
    assert(!memcmp(&original,p_ptr,sizeof(original)));
    assert(!memcmp(original_pack,inventory,sizeof(original_pack)));
    assert(turn==original_turn && p_ptr->energy_need==-27);
    assert(!strcmp(player_name,"Original") && !strcmp(savefile,"party-integration.sav"));
    assert(p_ptr->poisoned==7 && p_ptr->oppose_cold==30);
    assert(party_members[1].player.lev==1);
    assert(!strcmp(party_members[1].name,"Recruit"));
    test_training_batch_growth();
    test_party_training();
    first_recruit=party_members[1];
    Term_putch(75,23,TERM_YELLOW,'#');
    for(i=MIN_ELEM;i<ELEM_NUM;i++) {
        party_members[1].player.pelem=i;
        inspect_party_menu=TRUE;
        do_cmd_party();
        assert(!inspect_party_menu && party_active==0 && energy_use==0);
        assert(Term->scr->c[23][75]=='#' && character_icky==0);
        assert(turn==original_turn);
    }
    party_members[1]=first_recruit;
    py=px=1; cave[py][px].feat=FEAT_FLOOR;
    always_show_list=TRUE; use_menu=FALSE;
    auto_power=TRUE; power_step=0;
    do_cmd_racial_power();
    assert(party_active==1 && energy_use==100);
    assert(p_ptr->chp==first_recruit.player.chp);
    assert(p_ptr->csp==first_recruit.player.csp);
    assert(!p_ptr->poisoned && !p_ptr->oppose_cold);
    assert(p_ptr->au[SV_GOLD_NOTE]==12345 && inventory[0].number==7);
    assert(party_switch(0));
    for(i=0;i<INVEN_TOTAL-INVEN_RARM;++i)
        assert(!memcmp(&party_members[1].equipment[i],&first_recruit.equipment[i],sizeof(object_type)));
    /* Exercise the whole live save, including the current floor/checksum. */
    cur_hgt=cur_wid=5;
    for(i=0;i<cur_hgt;++i) {
        int x;
        for(x=0;x<cur_wid;++x) cave[i][x].feat=FEAT_FLOOR;
    }
    p_ptr->is_dead=0;
    party_rewards=3;
    assert(save_player());
    party_reset();
    assert(load_player());
    if(argc==3 && !strcmp(argv[2],"legacy")) {
        /* Legacy files have no party block. Claim both earned slots once. */
        assert(party_count==0 && party_rewards==0);
        quest[QUEST_ARMORICA].status=QUEST_STATUS_FINISHED;
        quest[QUEST_BARMAMUTHA_L].status=QUEST_STATUS_COMPLETED;
        quest[QUEST_BARMAMUTHA_C].status=QUEST_STATUS_FINISHED;
        assert(party_can_recruit() && party_count==0 && party_rewards==0);
        /* Visiting and leaving home must not force character creation. */
        p_ptr->town_num=1; dun_level=0;
        town[1].ethnic=NO_ETHNICITY;
        cave[py][px].feat=FEAT_SHOP_HEAD+STORE_HOME;
        keys="\033"; key_index=0;
        do_cmd_store();
        assert(character_icky==0);
        assert(party_count==0 && party_rewards==0 && party_can_recruit());
        auto_recruit=TRUE; home_test=TRUE; home_started=FALSE; home_target=2;
        do_cmd_store();
        assert(character_icky==0);
        assert(party_count==2 && party_rewards==1 && party_can_recruit());
        home_started=FALSE; home_target=3;
        do_cmd_store();
        assert(character_icky==0);
        assert(party_count==3 && party_rewards==3 && !party_can_recruit());
        do_cmd_party_recruit();
        assert(party_count==3);
        puts("Legacy full save migration and both quest rewards passed");
        return 0;
    }
    if(argc==3 && !strcmp(argv[2],"party-v1")) {
        assert(party_count==2 && party_rewards==3 && party_active==0);
        assert(!party_members[1].dead);
        assert(party_members[1].player.chp==MAX(1,(s32b)((u64b)p_ptr->chp*party_members[1].player.mhp/p_ptr->mhp)));
        assert(party_members[1].player.csp==(p_ptr->msp ? (s32b)((u64b)p_ptr->csp*party_members[1].player.msp/p_ptr->msp) : 0));
        puts("Previous party-version full save conversion passed");
        return 0;
    }
    assert(party_count==2 && party_rewards==3 && party_active==0);
    assert(!strcmp(party_members[1].name,"Recruit"));
    assert(party_members[1].player.lev==1);
    assert(py==1 && px==1 && cave[py][px].feat==FEAT_FLOOR);
    /* Render real HP after repeated U switches and real damage handling.
     * Prior tests ran with character_generated false and missed frozen UI. */
    character_generated=TRUE;
    character_dungeon=FALSE;
    auto_more=TRUE;
    for(i=0;i<12;++i) {
        int x;
        char displayed[6];
        p_ptr->chp=p_ptr->mhp;
        keys="          "; key_index=0; msg_print(NULL);
        use_menu=i%2;
        power_target=party_active ? 'a' : 'b';
        auto_power=TRUE; power_step=0;
        do_cmd_racial_power();
        assert(character_icky==0 && !p_ptr->is_dead && p_ptr->chp>1);
        /* This fixture has no town/wilderness UI; render the HP panel only. */
        p_ptr->update=0; p_ptr->redraw=PR_HP; p_ptr->window=0;
        take_hit(DAMAGE_GENO,1,"party display regression");
        for(x=0;x<5;++x) displayed[x]=Term->scr->c[ROW_CURHP][COL_CURHP+2+x];
        displayed[5]=0;
        assert(atol(displayed)==p_ptr->chp);
    }
    display_regressions();
    mechanics_regressions();
    side_effect_regressions();
    dump_regressions();
    character_generated=FALSE;
    p_ptr->is_dead=DEATH_DEAD;
    assert(save_player()); party_reset(); assert(load_player());
    assert(party_count==2 && party_rewards==3);
    dump_regressions();
    death_regressions();
    puts("Real-data birth, switch, party dumps and complete save/load passed");
    return 0;
}
