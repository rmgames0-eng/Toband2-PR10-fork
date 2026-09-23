"""Run production party switching, recruitment and save codecs on Win32.

UI/world services and derived-stat calculation are test doubles. The player
types, party logic, item codec and versioned party codec are production code.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def source(name):
    return (ROOT / 'src' / name).read_bytes().decode('euc_jp')


def function(text, signature):
    start = text.index(signature)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


program = r'''
#include "angband.h"
#include "party.h"
#include <assert.h>
#include <stdarg.h>

static player_type body;
player_type *p_ptr = &body;
static object_type items[INVEN_TOTAL];
object_type *inventory = items;
player_sex sex_info[MAX_SEXES], *sp_ptr;
static player_race races[4];
player_race *race_info = races, *rp_ptr;
static player_class classes[MAX_CLASS];
player_class *class_info = classes, *cp_ptr;
static player_magic magic[MAX_CLASS];
player_magic *m_info = magic, *mp_ptr;
static skill_table skills[MAX_CLASS];
skill_table *s_info = skills;
static quest_type quests[100];
quest_type *quest = quests;
static cave_type grid[MAX_WID];
cave_type *cave[MAX_HGT];
static monster_type monsters[3];
monster_type *m_list = monsters;
static monster_race monsters_race[3];
monster_race *r_info = monsters_race;
static object_kind kinds[10];
object_kind *k_info = kinds;
u16b max_p_idx=4, max_c_idx=MAX_CLASS;
char player_name[32];
char *p_name="race", *c_name="class";
s16b mw_old_weight, mw_diff_to_melee, inven_cnt, equip_cnt;
s16b energy_use, running, resting, command_rep;
int py, px;
static byte kanji_code=3;
void euc2sjis(char *s) {}
byte codeconv(char *s) { return 3; }
bool astral_mode, character_xtra, character_icky, stop_the_time_player;
bool monk_armour_aux, monk_notify_aux;
byte fool_effect_status;
int mutant_regenerate_mod;
static int recruits, messages;

int calc_mutant_regenerate_mod(void) { return 100; }
int stun_level(int value) { return value >= 100 ? 4 : 0; }
void msg_print(cptr s) { if(s) ++messages; }
void msg_format(cptr s, ...) { ++messages; }
void repeat_clear(void) {}
void screen_save(void) {}
void screen_load(void) {}
errr Term_clear(void) { return 0; }
void prt(cptr s, int y, int x) {}
bool get_com(cptr s, char *ch, bool z) { return FALSE; }
size_t strnfmt(char *s, size_t max, cptr fmt, ...) { s[0]=0; return 0; }
bool player_can_enter(byte feat) { return feat != FEAT_WALL_EXTRA || p_ptr->pass_wall; }
void update_stuff(void) {
    p_ptr->mhp=p_ptr->race_hp[0]+p_ptr->player_ghp+inventory[INVEN_BODY].to_a;
    p_ptr->msp=p_ptr->race_sp[0];
    p_ptr->levitation=p_ptr->prace==1;
    p_ptr->can_swim=p_ptr->prace==2;
    p_ptr->pass_wall=p_ptr->wraith_form!=0;
    p_ptr->pspeed=110+p_ptr->prace;
    if(p_ptr->chp>p_ptr->mhp)p_ptr->chp=p_ptr->mhp;
    if(p_ptr->csp>p_ptr->msp)p_ptr->csp=p_ptr->msp;
    p_ptr->update=0;
}
void party_birth_member(void) {
    assert(!party_creating);
    memset(&party_members[party_count],0,sizeof(party_members[0]));
    party_members[party_count].player.lev=1;
    ++party_count; ++recruits;
}
cptr quark_str(s16b q) { return q==1?"inscription":"artifact"; }
s16b quark_add(cptr s) { return !strcmp(s,"inscription")?1:2; }
static bool t_older_than(byte a,byte b,byte c,byte d) { return FALSE; }
static void convert_object(object_type *o) {}
static FILE *fff;
static byte xor_byte;
static u32b v_stamp,x_stamp,v_check,x_check;
'''

save, load = source('save.c'), source('load.c')
for name in ('sf_put', 'wr_byte', 'wr_u16b', 'wr_s16b', 'wr_u32b', 'wr_s32b', 'wr_string', 'wr_item'):
    program += function(save, 'static void ' + name + '(')
program += function(load, 'static byte sf_get(')
for name in ('rd_byte', 'rd_u16b', 'rd_s16b', 'rd_u32b', 'rd_s32b', 'rd_string', 'rd_item'):
    program += function(load, 'static void ' + name + '(')
program += '#include "party.c"\n'
program += function(save, 'static void wr_party(')
program += function(load, 'static errr rd_party(')
program += r'''
static void reset(void) {
    memset(&body,0,sizeof(body)); memset(items,0,sizeof(items));
    memset(quests,0,sizeof(quests)); memset(grid,0,sizeof(grid));
    party_reset(); astral_mode=FALSE; py=px=0; cave[0]=grid;
    strcpy(player_name,"First"); body.lev=10;
    body.race_hp[0]=body.mhp=100; body.chp=33;
    body.race_sp[0]=body.msp=50; body.csp=17;
    body.energy_need=-17; body.food=6000; body.au[1]=123456;
    body.poisoned=99; body.oppose_cold=40; body.fast=10;
    body.wraith_form=9; body.invuln=8; body.multishadow=7;
    body.hero=11; body.no_elem=3; body.singing=2;
    body.special_blow=123; body.exp=765; body.skill_exp[0]=42;
    body.weapon_exp[0]=555; body.cexp_info[0].cexp=123456;
    body.history[0][0]='A';
    items[0].k_idx=1; items[0].number=7; items[0].weight=10;
    items[INVEN_RARM].k_idx=2; items[INVEN_RARM].number=1;
    items[INVEN_RARM].weight=20; items[INVEN_RARM].curse_flags=TRC_CURSED;
    party_rebind(); update_stuff(); party_capture();
    party_count=2; party_members[1]=party_members[0];
    strcpy(party_members[1].name,"Second");
    party_members[1].player.prace=3; party_members[1].player.pclass=1;
    party_members[1].player.exp=10; party_members[1].player.skill_exp[0]=2;
    party_members[1].player.special_blow=0;
    party_members[1].player.race_hp[0]=79; party_members[1].player.race_sp[0]=20;
    party_members[1].equipment[0].k_idx=3; party_members[1].equipment[0].weight=35;
    energy_use=0;
}
static void switching(void) {
    player_type before;
    party_member dormant;
    reset(); assert(party_switch(1));
    assert(!strcmp(player_name,"Second") && p_ptr->exp==10 && p_ptr->skill_exp[0]==2);
    assert(p_ptr->chp==26 && p_ptr->csp==6 && energy_use==100);
    assert(p_ptr->energy_need==-17 && p_ptr->au[1]==123456 && p_ptr->food==6000);
    assert(p_ptr->poisoned==99 && p_ptr->oppose_cold==40 && p_ptr->fast==10);
    assert(p_ptr->wraith_form==9 && p_ptr->invuln==8 && p_ptr->multishadow==7);
    assert(!p_ptr->hero && !p_ptr->no_elem && !p_ptr->singing);
    assert(items[0].number==7 && items[INVEN_RARM].k_idx==3 && p_ptr->total_weight==105);
    dormant=party_members[0]; p_ptr->exp+=500; p_ptr->chp=9;
    assert(!memcmp(&dormant,&party_members[0],sizeof(dormant)));
    assert(party_switch(0));
    assert(p_ptr->chp==11 && p_ptr->exp==765 && p_ptr->weapon_exp[0]==555);
    assert(p_ptr->special_blow==123 && !p_ptr->hero && items[INVEN_RARM].curse_flags==TRC_CURSED);
    p_ptr->msp=0; p_ptr->csp=0; assert(party_switch(1)); assert(p_ptr->csp==0);
    assert(party_fraction(1,30000,2,TRUE)==1);
    assert(party_fraction(2000000000,2000000000,2000000000,FALSE)==2000000000);
    reset(); p_ptr->paralyzed=1; assert(!party_switch(1));
    p_ptr->paralyzed=0; p_ptr->stun=100; assert(!party_switch(1));
    p_ptr->stun=0; p_ptr->is_dead=DEATH_DEAD; assert(!party_switch(1));
    p_ptr->is_dead=0; astral_mode=TRUE; assert(!party_switch(1));
    reset(); p_ptr->confused=6; assert(party_switch(1)); assert(p_ptr->confused==6);
    reset(); p_ptr->wraith_form=0; update_stuff(); grid[0].feat=FEAT_WALL_EXTRA;
    before=*p_ptr; assert(!party_switch(1)); assert(!memcmp(&before,p_ptr,sizeof(before)));
    assert(items[INVEN_RARM].k_idx==2 && party_active==0 && !energy_use);
    grid[0].feat=FEAT_AIR; assert(!party_switch(1));
    grid[0].feat=FEAT_DEEP_WATER; assert(!party_switch(1));
    party_members[1].player.prace=2; assert(party_switch(1));
    reset(); p_ptr->riding=1; monsters[1].r_idx=1; assert(!party_switch(1));
    monsters_race[1].flags7=RF7_RIDING; assert(party_switch(1)); assert(p_ptr->riding==1);
    reset(); mw_old_weight=20; mw_diff_to_melee=5;
    inventory[INVEN_RARM].weight=1; inventory[INVEN_RARM].to_h=5; inventory[INVEN_RARM].to_d=5;
    p_ptr->magical_weapon=10; p_ptr->wraith_form=0;
    grid[0].feat=FEAT_WALL_EXTRA; assert(!party_switch(1));
    assert(mw_old_weight==20 && inventory[INVEN_RARM].weight==1 && p_ptr->magical_weapon==10);
    grid[0].feat=FEAT_FLOOR; assert(party_switch(1)); assert(party_switch(0));
    assert(inventory[INVEN_RARM].weight==20 && inventory[INVEN_RARM].to_h==0 && inventory[INVEN_RARM].to_d==0);
    assert(!mw_old_weight && !mw_diff_to_melee && !p_ptr->magical_weapon);
    reset(); stop_the_time_player=TRUE; p_ptr->energy_need=-2000; assert(party_switch(1));
    assert(!stop_the_time_player && p_ptr->energy_need==0 && energy_use==100);
    reset(); do_cmd_party(); assert(party_active==0 && !energy_use);
}
static void recruitment(void) {
    reset(); party_count=1; recruits=0;
    quests[QUEST_ARMORICA].status=QUEST_STATUS_FAILED;
    quests[QUEST_BARMAMUTHA_L].status=QUEST_STATUS_TAKEN;
    do_cmd_party_recruit(); assert(recruits==0);
    quests[QUEST_ARMORICA].status=QUEST_STATUS_COMPLETED;
    assert(party_can_recruit() && recruits==0);
    do_cmd_party_recruit(); assert(recruits==1 && party_count==2 && party_rewards==1);
    quests[QUEST_BARMAMUTHA_C].status=QUEST_STATUS_FINISHED;
    do_cmd_party_recruit(); assert(recruits==2 && party_count==3 && party_rewards==3);
    quests[QUEST_BARMAMUTHA_L].status=QUEST_STATUS_REWARDED;
    do_cmd_party_recruit(); assert(recruits==2);
    reset(); party_count=1; recruits=0; astral_mode=TRUE;
    quests[QUEST_ARMORICA].status=QUEST_STATUS_FINISHED;
    do_cmd_party_recruit(); assert(recruits==0);
}
static void serialization(void) {
    static party_member expected[MAX_PARTY_MEMBERS];
    int i,j,m;
    reset(); party_rewards=3;
    kinds[2].tval=TV_SWORD; kinds[2].sval=1;
    items[INVEN_RARM].tval=TV_SWORD; items[INVEN_RARM].sval=1;
    items[INVEN_RARM].inscription=1; items[INVEN_RARM].art_name=2;
    items[INVEN_RARM].timeout=57; items[INVEN_RARM].to_stat[2]=7;
    party_capture();
    for(i=1;i<MAX_PARTY_MEMBERS;++i) {
        party_members[i]=party_members[0];
        party_members[i].player.exp=i*10000;
        party_members[i].player.class_hp[2][77]=i*3;
        party_members[i].player.cexp_info[5].cexp_frac=0xf0000000U+i;
        party_members[i].player.essence_box[100]=i*5;
        party_members[i].weapon_weight=i+10;
    }
    party_count=MAX_PARTY_MEMBERS;
    memcpy(expected,party_members,sizeof(expected));
    fff=tmpfile(); assert(fff); xor_byte=0; wr_party(); fflush(fff); rewind(fff); xor_byte=0;
    party_reset(); assert(!rd_party());
    assert(party_count==MAX_PARTY_MEMBERS && party_active==0 && party_rewards==3);
    for(m=0;m<MAX_PARTY_MEMBERS;++m) {
#define PARTY_FIELD(type,name,count) assert(!memcmp(&party_members[m].player.name,&expected[m].player.name,sizeof(expected[m].player.name)));
#include "party-fields.h"
#undef PARTY_FIELD
        assert(!memcmp(party_members[m].player.cexp_info,expected[m].player.cexp_info,sizeof(expected[m].player.cexp_info)));
        assert(!strcmp(party_members[m].name,expected[m].name));
        assert(!memcmp(party_members[m].player.history,expected[m].player.history,sizeof(expected[m].player.history)));
        assert(party_members[m].weapon_weight==expected[m].weapon_weight);
        assert(party_members[m].equipment[0].inscription==1);
        assert(party_members[m].equipment[0].art_name==2);
        assert(party_members[m].equipment[0].timeout==57);
        assert(party_members[m].equipment[0].to_stat[2]==7);
        assert(party_members[m].equipment[0].curse_flags==TRC_CURSED);
    }
    fclose(fff);
    fff=tmpfile(); xor_byte=0; wr_byte(PARTY_SAVE_VERSION); wr_byte(MAX_PARTY_MEMBERS+1); wr_byte(0); wr_byte(0);
    fflush(fff); rewind(fff); xor_byte=0; party_reset(); assert(rd_party()); fclose(fff);
    party_reset(); assert(!party_count && !party_rewards); /* legacy initialization */
}
int main(void) {
    switching(); recruitment(); serialization();
    puts("Party switching, quest rewards and 16-member save round trip passed");
    return 0;
}
'''

with tempfile.TemporaryDirectory() as tmp:
    c = Path(tmp) / 'party_test.c'
    c.write_bytes(program.encode('euc_jp'))
    for opt in ('-O0', '-O2'):
        exe = Path(tmp) / 'party_test.exe'
        subprocess.run([str(ROOT/'tools/mingw32/bin/gcc.exe'), '-std=gnu89', opt,
                        '-DWIN32', '-DJP', '-DSJIS', '-finput-charset=EUC-JP',
                        '-fexec-charset=CP932', '-I', str(ROOT/'src'), str(c),
                        '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
