/* Headless integration test using the real data, birth UI and savefile code. */
#include "angband.h"
#include "party.h"
#include <assert.h>

static term test_term;
static const char *keys;
static int key_index;
static const char party_label[]="\x8c\xf0\x91\xe3";
static int power_step;
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
            if(power_step==1) { auto_power=FALSE; Term_keypress('b'); return 0; }
            for(y=1;y<24;++y) {
                for(x=0;x<80;++x) row[x]=Term->scr->c[y][x];
                row[80]=0;
                if(strstr(row,party_label)) {
                    assert(row[1]=='a'); power_step=1; Term_keypress(row[1]); return 0;
                }
            }
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
        assert(party_count==0 && party_rewards==0 && party_can_recruit());
        auto_recruit=TRUE; home_test=TRUE; home_started=FALSE; home_target=2;
        do_cmd_store();
        assert(party_count==2 && party_rewards==1 && party_can_recruit());
        home_started=FALSE; home_target=3;
        do_cmd_store();
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
    p_ptr->is_dead=DEATH_DEAD;
    assert(save_player()); party_reset(); assert(load_player());
    assert(party_count==2 && party_rewards==3);
    puts("Real-data birth, switch and complete save/load passed");
    return 0;
}
