/* Headless integration test using the real data, birth UI and savefile code. */
#include "angband.h"
#include "party.h"
#include <assert.h>

static term test_term;
static const char *keys;
static int key_index;
static const char party_label[]="\x8c\xf0\x91\xe3";
static int power_step;
static char power_target = 'b';
static bool auto_power;
static bool auto_recruit;
static int auto_count = -1;
static bool home_test, home_started;
static int home_target;

static errr test_xtra(int n, int v)
{
    if (n == TERM_XTRA_EVENT && v)
    {
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
    assert(p_ptr->no_flowed);
    p_ptr->wraith_form=0;
    assert(party_switch(1-target));
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
            assert(reserve && end && (!hp || hp>=end));
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

int main(int argc,char **argv)
{
    char path[1024];
    player_type original;
    object_type original_pack[INVEN_TOTAL];
    party_member first_recruit;
    s32b original_turn;
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
    Rand_quick=TRUE; Rand_value=12345;
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
    party_birth_member();
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
    first_recruit=party_members[1];
    py=px=1; cave[py][px].feat=FEAT_FLOOR;
    always_show_list=TRUE; use_menu=FALSE;
    auto_power=TRUE; power_step=0;
    do_cmd_racial_power();
    assert(party_active==1 && energy_use==100);
    assert(p_ptr->chp==MAX(1,(37*p_ptr->mhp)/100));
    assert(p_ptr->csp==(13*p_ptr->msp)/50);
    assert(p_ptr->poisoned==7 && p_ptr->oppose_cold==30);
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
    if(argc==3) {
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
    dump_regressions();
    character_generated=FALSE;
    p_ptr->is_dead=DEATH_DEAD;
    assert(save_player()); party_reset(); assert(load_player());
    assert(party_count==2 && party_rewards==3);
    dump_regressions();
    puts("Real-data birth, switch, party dumps and complete save/load passed");
    return 0;
}
