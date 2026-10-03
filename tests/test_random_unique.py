"""Random uniques: procedural variety, placement, retained floors and save migration."""
from pathlib import Path
import subprocess, tempfile, shutil, re
ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT/'build/party'
GCC = ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS = ['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
# Recruitment lists match the referenced page, including its spelling/order.
import hashlib
name_header=(ROOT/"src/random-unique-data.h").read_bytes().decode("euc_jp")
names=re.findall(r'"([^"\n]+)"',name_header.split("static cptr ru_human_names[] = {")[1].split("};")[0])
assert len(names)==512
assert hashlib.sha256("\n".join(names).encode()).hexdigest()=="da41a47eb8ca6d803257b6b5c3b5ce5806912c7765a13bb68ec29887ff0d9e78"
names=re.findall(r'"([^"\n]+)"',name_header.split("static cptr ru_large_names[] = {")[1].split("};")[0])
assert len(names)==256
assert hashlib.sha256("\n".join(names).encode()).hexdigest()=="01651757037f70895c0691594de41a7d37b2c5bf771250fd02afa3170f5b3b87"
extra = r"""
#include "random-unique.c"
static int ru_test_bits(u32b v)
{
    int n=0;while(v){n++;v&=v-1;}return n;
}

static int ru_test_skills(const monster_race *r)
{
    return ru_test_bits(r->flags1)+ru_test_bits(r->flags2)+ru_test_bits(r->flags3)+
        ru_test_bits(r->flags4)+ru_test_bits(r->flags5)+ru_test_bits(r->flags6)+ru_test_bits(r->flagsr);
}

static void random_unique_power_tests(void)
{
    static const int powers[]={10,60,200,1000};
    long hp[4]={0},melee[4]={0},armor[4]={0},speed[4]={0},spells[4]={0},skills[4]={0};
    int i,j,k,si=0,max_added=0,baseline_count=0;
    random_unique_profile plain;
    monster_race r,baseline;
    WIPE(&plain,random_unique_profile);
    plain.hp_percent=plain.damage_percent=100;plain.sleep=-1;plain.spell_frequency=20;
    while(r_unique_symbol_table[si].mon_char!='O')si++;
    for(i=1;i<=128;i++) {
        Rand_value=i;clear_random_unique(MON_RANDOM_UNIQUE_1);baseline=r_info[MON_RANDOM_UNIQUE_1];
        random_unique_base(&baseline,&r_unique_symbol_table[si],&plain,87,10);
        random_unique_symbol_skills(&baseline,0,&plain);
        random_unique_apply_profile(&baseline,&plain);random_unique_resistances(&baseline,&plain);
        baseline_count=ru_test_skills(&baseline);
        for(j=0;j<4;j++) {
            Rand_value=i;clear_random_unique(MON_RANDOM_UNIQUE_1);r=r_info[MON_RANDOM_UNIQUE_1];
            random_unique_base(&r,&r_unique_symbol_table[si],&plain,87,powers[j]);
            random_unique_symbol_skills(&r,powers[j],&plain);
            random_unique_apply_profile(&r,&plain);random_unique_resistances(&r,&plain);
            random_unique_attack_count(&r,&plain);
            assert(r.hdice<=200 && r.ac<=500 && r.speed<=180 && r.freq_spell<=50);
            hp[j]+=(long)r.hdice*r.hside;armor[j]+=r.ac;speed[j]+=r.speed-110;spells[j]+=r.freq_spell;
            skills[j]+=ru_test_skills(&r);
            if(ru_test_skills(&r)-baseline_count>max_added)max_added=ru_test_skills(&r)-baseline_count;
            for(k=0;k<4;k++)melee[j]+=r.blow[k].d_dice*(r.blow[k].d_side+1);
        }
    }
    for(j=0;j<4;j++) {
        printf("Power %d / Lv87 O averages: HP %ld, AC %ld, speed +%ld, spell %ld%%, melee %ld, flags %ld\n",
            powers[j],hp[j]/128,armor[j]/128,speed[j]/128,spells[j]/128,melee[j]/256,skills[j]/128);
        if(j){assert(hp[j]>hp[j-1]);assert(armor[j]>armor[j-1]);assert(melee[j]>melee[j-1]);assert(speed[j]>speed[j-1]);assert(skills[j]>skills[j-1]);}
    }
    assert(max_added>12 && spells[3]>spells[0]);
    /* Upper-level falloff removes novice spells; species affinity still matters. */
    WIPE(&r,monster_race);r.d_char='p';r.level=100;
    for(j=0;j<RU_SKILL_COUNT;j++)if(ru_skills[j].group==5 && ru_skills[j].flag==RF5_MISSILE)
        assert(!ru_skill_weight(&r,&ru_skills[j],RU_ARCANE));
    /* Below Lv20 two blows, then three, then four; explicit titles win. */
    for(i=1;i<=40;i++) {
        r.level=i;
        for(j=0;j<4;j++){r.blow[j].method=RBM_HIT;r.blow[j].d_dice=2;r.blow[j].d_side=3;}
        random_unique_attack_count(&r,&plain);
        for(j=0;j<4;j++)assert(!!r.blow[j].method==(j<2+(i>=20)+(i>=40)));
    }
    plain.blows[3].method=RBM_HIT;r.level=1;
    for(j=0;j<4;j++)r.blow[j].method=RBM_HIT;
    random_unique_attack_count(&r,&plain);assert(r.blow[3].method);
    puts("Power allocation: scaling, high-power diversity, novice spell falloff and attack counts passed");
}

static void random_unique_ecology_tests(void)
{
    int i,j,k,level,dragon_elements=0,elemental_elements=0;
    monster_race r;
    random_unique_profile plain;
    WIPE(&plain,random_unique_profile);
    plain.hp_percent=plain.damage_percent=100;
    plain.sleep=-1;plain.spell_frequency=20;
    for(i=0;i<RU_SYMBOL_NUM;i++)for(level=1;level<=127;level+=21)for(k=0;k<12;k++)
    {
        WIPE(&r,monster_race);
        r.level=level;r.d_char=r_unique_symbol_table[i].mon_char;
        r.hdice=20;r.hside=80;r.speed=110;r.ac=50;r.r_elem=NO_ELEM;
        r.flags1=RF1_UNIQUE|RF1_ONLY_ITEM;r.flags3=RF3_NO_SLEEP;
        for(j=0;j<4;j++){r.blow[j].method=RBM_HIT;r.blow[j].effect=RBE_HURT;r.blow[j].d_dice=8;r.blow[j].d_side=10;}
        random_unique_symbol_skills(&r,60,&plain);
        random_unique_apply_profile(&r,&plain);
        random_unique_resistances(&r,&plain);
        assert(!(r.flags6&RF6_SPECIAL));
        assert(r.freq_spell<=50);
        assert(!(r.flags2&RF2_KILL_BODY) || !(r.flags2&RF2_MOVE_BODY));
        assert(!(r.flags2&RF2_KILL_WALL) || !(r.flags2&RF2_PASS_WALL));
        if(r.d_char=='O') {
            assert(!(r.flags3&RF3_ORC));
            if(level>=60) {
                assert(r.flags3&RF3_GIANT);
                assert(r.flags4&RF4_BR_DARK);
                assert((r.flagsr&(RFR_RES_FIRE|RFR_RES_NETH|RFR_RES_DARK|RFR_RES_POIS|RFR_RES_STON))==(RFR_RES_FIRE|RFR_RES_NETH|RFR_RES_DARK|RFR_RES_POIS|RFR_RES_STON));
                assert(r.blow[0].effect==RBE_HELL && r.blow[2].effect==RBE_SUPERHURT);
                if(level>=87)assert((r.flags3&(RF3_RES_TELE|RF3_NO_STUN))==(RF3_RES_TELE|RF3_NO_STUN) && (r.flagsa&RFA_BR_PURE_ELEM));
                if(level>=93)assert(r.flags2&RF2_POWERFUL);
            } else assert(!(r.flags4&RF4_BR_DARK) && !(r.flags3&RF3_GIANT));
        }
        if(r.d_char=='A')assert((r.flags3&RF3_GOOD) && (r.flags6&RF6_HEAL) && r.blow[1].effect==RBE_HOLY && (r.flagsr&RFR_RES_LITE));
        if(r.d_char=='D' || r.d_char=='d') {
            assert(r.flags3&RF3_DRAGON);
            assert(r.flags4&(RF4_BR_ACID|RF4_BR_ELEC|RF4_BR_FIRE|RF4_BR_COLD|RF4_BR_POIS));
            if(r.flags4&RF4_BR_ACID){assert(r.flagsr&RFR_RES_ACID);dragon_elements|=1;}
            if(r.flags4&RF4_BR_ELEC){assert(r.flagsr&RFR_RES_ELEC);dragon_elements|=2;}
            if(r.flags4&RF4_BR_FIRE){assert(r.flagsr&RFR_RES_FIRE);dragon_elements|=4;}
            if(r.flags4&RF4_BR_COLD){assert(r.flagsr&RFR_RES_COLD);dragon_elements|=8;}
            if(r.flags4&RF4_BR_POIS){assert(r.flagsr&RFR_RES_POIS);dragon_elements|=16;}
        }
        if(r.d_char=='E') {
            assert(r.r_elem>=0 && r.r_elem<ELEM_NUM);
            elemental_elements|=1<<r.r_elem;
            if(r.r_elem==ELEM_FIRE)assert((r.flagsr&RFR_RES_FIRE) && r.blow[0].effect==RBE_FIRE);
            if(r.r_elem==ELEM_AQUA)assert((r.flagsr&RFR_RES_WATE) && r.blow[0].effect==RBE_COLD);
        }
        if(r.d_char=='V')assert((r.flags3&RF3_UNDEAD) && r.blow[3].effect==RBE_EXP_VAMP);
        if(r.d_char=='L')assert((r.flags2&RF2_SMART) && (r.flags5&RF5_BO_NETH) && r.freq_spell>=35);
        if(r.d_char=='S')assert((r.flags3&RF3_ANIMAL) && r.blow[0].effect==RBE_POISON);
        if(r.d_char=='K')assert(r.blow[0].effect==RBE_ACID);
        if(r.d_char=='T')assert((r.flags2&RF2_REGENERATE) && (r.flags3&RF3_TROLL));
    }
    assert(dragon_elements==31 && elemental_elements==15);
    /* Actual weight calculation: same fire skill is 3x as likely for a dragon.
     * No level-1 symbol may roll mana storm, nor a fire-only theme nether magic. */
    WIPE(&r,monster_race);r.level=60;r.d_char='p';
    j=ru_skill_weight(&r,&ru_skills[0],RU_FIRE);r.d_char='D';
    assert(ru_skill_weight(&r,&ru_skills[0],RU_FIRE)==3*j);
    assert(!ru_skill_weight(&r,&ru_skills[0],RU_WATER));
    for(i=0;i<RU_SYMBOL_NUM;i++) {
        r.d_char=r_unique_symbol_table[i].mon_char;r.level=1;
        for(j=0;j<RU_SKILL_COUNT;j++)if(ru_skills[j].flag==RF5_BA_MANA && ru_skills[j].group==5)
            assert(!ru_skill_weight(&r,&ru_skills[j],RU_ARCANE));
    }
    puts("54 symbol ecologies: traits, level gates, elemental coherence and weighted skills passed");
}

static void random_unique_profile_tests(void)
{
    int i,j,k,tier_count[3]={0};
    monster_race r;
    static cptr low_moves[]={"餓鬼","悲恋","女心","斜陽"};
    assert(RU_PROFILE_COUNT==113 && RU_HUMAN_NAME_COUNT==512 && RU_LARGE_NAME_COUNT==256);
    assert(random_unique_tier(1)==0 && random_unique_tier(39)==0);
    assert(random_unique_tier(40)==1 && random_unique_tier(79)==1);
    assert(random_unique_tier(80)==2 && random_unique_tier(127)==2);
    for(i=0;i<RU_PROFILE_COUNT;i++)
    {
        const random_unique_profile *p=&ru_profiles[i];
        assert(strlen(p->title)==4);
        tier_count[p->tier]++;
        for(j=0;j<i;j++)assert(strcmp(p->title,ru_profiles[j].title));
        for(j=0;j<4;j++)if(!strcmp(p->title,low_moves[j]))assert(p->tier==0);
        /* Apply every profile to every symbol, including opposing alignments,
         * fire-resistant demons, immobile races and empty-minded creatures. */
        for(j=0;j<RU_SYMBOL_NUM;j++)
        {
            WIPE(&r,monster_race);
            r.level=20+40*p->tier;r.d_char=r_unique_symbol_table[j].mon_char;
            r.hdice=10;r.hside=50;r.ac=50;r.speed=110;
            r.flags1=RF1_UNIQUE|RF1_ONLY_ITEM;r.flags3=RF3_NO_SLEEP;
            for(k=0;k<4;k++){r.blow[k].method=RBM_HIT;r.blow[k].effect=RBE_HURT;r.blow[k].d_dice=4;r.blow[k].d_side=6;}
            random_unique_traits(&r,30);
            random_unique_symbol_skills(&r,30,p);
            random_unique_apply_profile(&r,p);
            random_unique_resistances(&r,p);
            assert(!r.text && r.hdice && r.hside && r.ac>=0 && r.ac<=500);
            assert(r.speed>=100 && r.speed<=180);
            assert((r.flags3&(RF3_GOOD|RF3_EVIL))!=(RF3_GOOD|RF3_EVIL));
            assert(!(r.flags6&RF6_SPECIAL));
            assert(!(r.flags1&(RF1_FRIENDS|RF1_QUESTOR)));
            assert((r.flags1&p->flags1)==p->flags1);
            assert((r.flags2&p->flags2)==p->flags2);
            assert((r.flags3&p->flags3)==p->flags3);
            assert((r.flags4&p->flags4)==p->flags4);
            assert((r.flags5&p->flags5)==p->flags5);
            assert((r.flags6&p->flags6)==p->flags6);
            assert((r.flags7&p->flags7)==p->flags7);
            assert((r.flagsr&p->flagsr)==p->flagsr);
            if(p->flags1&RF1_ESCORT)assert(r.escort_char[0]==r.d_char);
            assert(!(r.flagsa & ~RFA_BR_PURE_ELEM));
            assert(!!r.freq_spell==!!(r.flags4|r.flags5|r.flags6|r.flagsa));
            if(p->flags3&RF3_HURT_FIRE)assert(!(r.flagsr&RFR_RES_FIRE));
            if(p->flags1&RF1_ONLY_GOLD)assert(!(r.flags1&RF1_ONLY_ITEM));
            if(p->flags2&RF2_WEIRD_MIND)assert(!(r.flags2&RF2_EMPTY_MIND));
            if(p->sleep>=0)assert(r.sleep==p->sleep);
            if(p->speed_bonus>0 || (p->flags1&RF1_RAND_25))assert(!(r.flags1&RF1_NEVER_MOVE));
            for(k=0;k<4;k++)
            {
                if(p->blows[k].method){assert(r.blow[k].method==p->blows[k].method);assert(r.blow[k].effect==p->blows[k].effect);}
                assert(r.blow[k].d_dice>0 && r.blow[k].d_side>0);
            }
        }
    }
    assert(tier_count[0]==28 && tier_count[1]==53 && tier_count[2]==32);
    /* Every epithet keeps its individually reviewed particle choice. */
    for(i=0;i<RU_PROFILE_COUNT;i++)
    {
        static cptr bare_titles[]={"武神","激神","餓鬼","獄卒","逆賊","逆臣","妖刀","魔剣","名槍","剛弓","鬼才","異才","厄神","獅子","猛禽","大蛇","荒鷲","関取","水神","幽鬼","酒豪","横綱","大関"};
        bool bare=FALSE;
        char prefix[80];
        for(j=0;j<sizeof(bare_titles)/sizeof(bare_titles[0]);j++)
            if(!strcmp(ru_profiles[i].title,bare_titles[j]))bare=TRUE;
        assert(ru_profiles[i].use_no==!bare);
        strnfmt(prefix,sizeof(prefix),"%s%s《",ru_profiles[i].title,bare?"":"の");
        random_unique_name(MON_RANDOM_UNIQUE_1,i);
        assert(!strncmp(r_name+r_info[MON_RANDOM_UNIQUE_1].name,prefix,strlen(prefix)));
    }

    /* Verify every byte-valued symbol against the exact case-sensitive mapping. */
    for(k=0;k<128*4;k++)
    {
        int symbol=k%128,gender_case=k/128;
        u32b gender=(gender_case&1?RF1_MALE:0)|(gender_case&2?RF1_FEMALE:0);
        bool human=(symbol && strchr("VoOpUuGFAWth",symbol));
        cptr *names=human?ru_human_names:ru_large_names;
        int count=human?RU_HUMAN_NAME_COUNT:RU_LARGE_NAME_COUNT;
        r_info[MON_RANDOM_UNIQUE_1].d_char=symbol;
        r_info[MON_RANDOM_UNIQUE_1].flags1=RF1_UNIQUE|gender;
        if(human && gender==RF1_MALE) count=256;
        if(human && gender==RF1_FEMALE) {names+=256;count=256;}
        for(i=0;i<30;i++)
        {
            char suffix[80];
            random_unique_name(MON_RANDOM_UNIQUE_1,0);
            for(j=0;j<count;j++) {
                strnfmt(suffix,sizeof(suffix),"《%s》",names[j]);
                if(strstr(r_name+r_info[MON_RANDOM_UNIQUE_1].name,suffix))break;
            }
            assert(j<count);
        }
    }
    for(k=0;k<4;k++)
    {
        cptr *names=k?ru_human_names:ru_large_names;
        int count=k==1?RU_HUMAN_NAME_COUNT:RU_LARGE_NAME_COUNT,seen[512]={0};
        r_info[MON_RANDOM_UNIQUE_1].flags1=RF1_UNIQUE|(k==2?RF1_MALE:k==3?RF1_FEMALE:0);
        if(k==3)names+=256;
        r_info[MON_RANDOM_UNIQUE_1].d_char=k?'O':'D';
        for(i=0;i<count;i++)
        {
            char suffix[80];
            assert(strlen(names[i])>0 && strlen(names[i])+12<RANDOM_UNIQUE_NAME_SIZE);
            strnfmt(suffix,sizeof(suffix),"《%s》",names[i]);
            strcpy(r_name+r_info[MON_RANDOM_UNIQUE_2].name,suffix);
            Rand_value=i+1;random_unique_name(MON_RANDOM_UNIQUE_1,0);
            assert(!strstr(r_name+r_info[MON_RANDOM_UNIQUE_1].name,suffix));
        }
        r_name[r_info[MON_RANDOM_UNIQUE_2].name]=0;
        r_name[r_info[MON_RANDOM_UNIQUE_3].name]=0;
        for(i=0;i<24000;i++)
        {
            random_unique_name(MON_RANDOM_UNIQUE_1,0);
            for(j=0;j<count;j++) {
                char suffix[80];strnfmt(suffix,sizeof(suffix),"《%s》",names[j]);
                if(strstr(r_name+r_info[MON_RANDOM_UNIQUE_1].name,suffix))seen[j]++;
            }
        }
        for(i=0;i<count;i++)assert(seen[i]);
    }
    puts("113 profiles / 54 symbols; 512 human + 256 L-size names, symbol/gender routing and duplicate exclusion passed");
}

static void random_unique_tests(void)
{
    int i,j,x,slot=MON_RANDOM_UNIQUE_1,seen[256]={0},profiles[RU_PROFILE_COUNT]={0},symbols=0,spells=0,counts=0;
    monster_race original, other;
    monster_type mon;
    char name[RANDOM_UNIQUE_NAME_SIZE], desc[256];
    s16b floor;
    bool fear=FALSE;
    static char spaces[30000];
    memset(spaces,32,sizeof(spaces)-1); keys=spaces;key_index=0;auto_more=TRUE;
    wipe_m_list();init_saved_floors(FALSE);random_unique_reset();
    Rand_quick=TRUE;Rand_value=987654321;
    random_unique_profile_tests();
    random_unique_ecology_tests();
    random_unique_power_tests();
    random_unique_reset();
    p_ptr->inside_arena=p_ptr->inside_quest=p_ptr->wild_mode=0;
    dungeon_type=DUNGEON_RUINS;dun_level=monster_level=60;
    cur_hgt=cur_wid=15;py=px=1;
    for(i=0;i<15;i++)for(x=0;x<15;x++)cave[i][x].feat=(i==0||x==0||i==14||x==14)?FEAT_PERM_SOLID:FEAT_FLOOR;

    /* Equal weights: a generated Lv40 unique must beat a Lv36 ordinary race
     * in harder-monster rerolls, regardless of its reserved Lv1 table slot. */
    {
        int n,hits=0;
        random_unique_reset();dungeon_type=DUNGEON_PALACE;dun_level=monster_level=40;
        get_mon_num_prep(get_monster_hook(),get_monster_hook2(5,5));
        r_info[slot].level=40;r_info[slot].max_num=1;r_info[slot].cur_num=0;
        r_info[571].cur_num=0;r_info[571].max_num=100;
        for(j=0;j<alloc_race_size;j++)
            alloc_race_table[j].prob2=(alloc_race_table[j].index==slot || alloc_race_table[j].index==571)?100:0;
        Rand_value=442211;
        for(n=0;n<10000;n++)if(get_mon_num(40)==slot)hits++;
        assert(hits>6000 && hits<7500);
        for(n=0;n<100;n++)assert(!get_mon_num(10));
        printf("Dynamic allocation level: %d / 10000 harder-monster choices; depth eligibility passed\n",hits);
        random_unique_reset();dun_level=monster_level=40;Rand_value=987123;hits=0;
        for(n=0;n<20000;n++) {
            get_mon_num_prep(get_monster_hook(),get_monster_hook2(5,5));
            j=get_mon_num(40);if(IS_RANDOM_UNIQUE(j))hits++;
        }
        printf("Palace 40 / free slots: %d / 20000 natural selection trials\n",hits);
        random_unique_reset();dungeon_type=DUNGEON_RUINS;dun_level=monster_level=60;
    }
    for (i=0;i<3000;i++)
    {
        int id=slot+i%3;
        monster_race *r=&r_info[id];
        monster_level=1+i%127;dun_level=MAX(10,monster_level);
        assert(random_unique_generate(id));
        assert(r->level>=1 && r->level<=monster_level);
        assert(r->flags1 & RF1_UNIQUE); assert(r->flags7 & RF7_UNIQUE2);
        assert(!(r->flags1 & (RF1_QUESTOR|RF1_FRIENDS)));
        assert(!(r->flags6 & RF6_SPECIAL));
        assert(r->hdice>0 && r->hside>0 && r->hdice*r->hside<=MAX_MAX_MAXHP);
        assert(r->speed>=100 && r->speed<=180 && r->ac<=500);
        assert(!r->text);
        assert(strlen(r_name+r->name)<RANDOM_UNIQUE_NAME_SIZE);
        assert(strstr(r_name+r->name,"《"));
        for(j=0;j<RU_PROFILE_COUNT;j++)
            if(!strncmp(r_name+r->name,ru_profiles[j].title,strlen(ru_profiles[j].title)))break;
        assert(j<RU_PROFILE_COUNT && ru_profiles[j].tier==random_unique_tier(r->level));
        profiles[j]++;
        if(!seen[r->d_char]++)symbols++;
        if(r->flags4||r->flags5||r->flags6)spells++;
        for(j=0;j<4;j++) { if(r->blow[j].method)assert(r->blow[j].d_dice && r->blow[j].d_side); else assert(!r->blow[j].d_dice && !r->blow[j].d_side); }
    }
    printf("variety: %d symbols, %d spell users / 3000\n",symbols,spells);fflush(stdout);assert(symbols>25 && spells>100);
    for(i=0;i<RU_PROFILE_COUNT;i++)assert(profiles[i]);
    dun_level=9;assert(!random_unique_generate(slot));
    dun_level=60;p_ptr->inside_arena=1;assert(!random_unique_generate(slot));p_ptr->inside_arena=0;
    p_ptr->inside_quest=QUEST_ARMORICA;assert(!random_unique_generate(slot));p_ptr->inside_quest=0;
    monster_level=60;
    for(i=0;i<5000;i++)
    {
        get_mon_num_prep(get_monster_hook(),get_monster_hook2(5,5));
        j=get_mon_num(monster_level);
        if(IS_RANDOM_UNIQUE(j))counts++;
    }
    assert(counts>0);printf("natural allocation: %d / 5000\n",counts);
    /* Retired named races cannot allocate or spawn even through direct placement. */
    for(i=968;i<=983;i++) {
        assert(r_info[i].flags1&RF1_RAND_U_NAME);
        assert(!r_info[i].rarity);
        assert(!place_monster_aux(0,6,6,i,PM_ALLOW_GROUP));
    }
    for(i=0;i<alloc_race_size;i++)
        assert(!(r_info[alloc_race_table[i].index].flags1&RF1_RAND_U_NAME) || !alloc_race_table[i].prob2);

    /* New escort profiles must really place escorts, not merely display a flag. */
    assert(random_unique_generate(slot));
    r_info[slot].d_char=r_info[slot].x_char='p';r_info[slot].level=60;
    r_info[slot].flags3 &= ~(RF3_GOOD|RF3_UNDEAD|RF3_DEMON);
    r_info[slot].flags3 |= RF3_EVIL;
    r_info[slot].flags7 &= ~(RF7_AQUATIC|RF7_CAN_FLY);
    r_info[slot].flags1 |= RF1_ESCORT;
    r_info[slot].flags1 &= ~RF1_ESCORTS;
    r_info[slot].escort_char[0]='p';
    assert(place_monster_aux(0,5,5,slot,PM_ALLOW_GROUP));
    counts=0;
    for(i=1;i<m_max;i++)if(m_list[i].r_idx && m_list[i].r_idx!=slot){assert(!(r_info[m_list[i].r_idx].flags1&RF1_UNIQUE));counts++;}
    assert(counts>0);puts("Actual escort placement passed");
    wipe_m_list();
    /* Ordinary placement, display and no duplicate on the current floor. */
    for(i=0;i<100;i++)
    {
        assert(random_unique_generate(slot));
        if(place_monster_aux(0,5,5,slot,0))break;
    }
    assert(i<100);j=cave[5][5].m_idx;
    m_list[j].ml=TRUE;monster_desc(desc,&m_list[j],0);
    strcpy(name,r_name+r_info[slot].name);assert(strstr(desc,name));
    assert(!place_monster_aux(0,6,5,slot,0));
    original=r_info[slot];
    get_mon_num_prep(get_monster_hook(),get_monster_hook2(6,5));
    assert(!memcmp(&original,&r_info[slot],sizeof(original)));
    assert(!strcmp(name,r_name+r_info[slot].name));
    /* Full save/load restores parameters before monster lore and floor entities. */
    r_info[slot].r_flags3=r_info[slot].flags3;
    r_info[slot].r_sights=7;
    original=r_info[slot];mon=m_list[j];random_unique_kills=12;
    p_ptr->is_dead=0;strcpy(savefile,"random-unique.sav");
    /* Simulate a pre-retirement named Sirene and a separately named ordinary pet. */
    assert(place_monster_aux(0,7,7,571,0));
    m_list[cave[7][7].m_idx].r_idx=981;
    m_list[cave[7][7].m_idx].ap_r_idx=981;
    m_list[cave[7][7].m_idx].nickname=quark_add("Legacy Sirene");
    m_list[cave[7][7].m_idx].hp=123;
    assert(place_monster_aux(0,8,8,571,0));
    m_list[cave[8][8].m_idx].nickname=quark_add("My pet");
    assert(save_player());
    random_unique_reset();
    assert(load_player());
    Rand_quick=TRUE;Rand_value=11223344;
    assert(m_list[cave[7][7].m_idx].r_idx==981);
    assert(m_list[cave[7][7].m_idx].hp==123);
    assert(!m_list[cave[7][7].m_idx].nickname);
    assert(!strcmp(quark_str(m_list[cave[8][8].m_idx].nickname),"My pet"));
    puts("Retired named variants: no new spawns; old bodies and pet names preserved");

#define RANDOM_UNIQUE_FIELD(type, field) assert(r_info[slot].field==original.field);
#include "random-unique-fields.h"
#undef RANDOM_UNIQUE_FIELD
    assert(!memcmp(r_info[slot].blow,original.blow,sizeof(original.blow)));
    assert(random_unique_kills==12);
    assert(!r_info[slot].text);
    assert(!memcmp(r_info[slot].escort_char,original.escort_char,sizeof(original.escort_char)));
    assert(r_info[slot].r_flags3==original.r_flags3 && r_info[slot].r_sights==7);
    assert(!strcmp(name,r_name+r_info[slot].name));
    assert(cave[5][5].m_idx && m_list[cave[5][5].m_idx].hp==mon.hp);
    /* Element and extra breath flags survive the real reader, without changing
     * the on-disk format; neutral ogres must not gain a random racial element. */
    {
        static const u32b elem_flags[]={RF3_ELEM_FIRE,RF3_ELEM_AQUA,RF3_ELEM_EARTH,RF3_ELEM_WIND};
        int e;
        for(e=0;e<=4;e++) {
            r_info[slot].flags3=(r_info[slot].flags3&~RF3_ELEM_MASK)|(e<4?elem_flags[e]:0);
            r_info[slot].r_elem=e<4?e:NO_ELEM;
            r_info[slot].flagsa|=RFA_BR_PURE_ELEM;
            assert(save_player());random_unique_reset();assert(load_player());
            assert(r_info[slot].r_elem==(e<4?e:NO_ELEM));
            assert(r_info[slot].flagsa&RFA_BR_PURE_ELEM);
        }
    }
    puts("Four elemental affinities and neutral pure-element breath persisted");
    /* Existing generated names and arbitrary pre-redesign abilities are data,
     * not re-derived from the new title dictionary when a save is read. */
    strcpy(name,"猛毒の支配者《アガルオン》");
    strcpy(r_name+r_info[slot].name,name);
    r_info[slot].flags5|=RF5_BA_NETH;
    r_info[slot].flags1|=RF1_ESCORT;
    r_info[slot].escort_char[0]=r_info[slot].d_char;
    original=r_info[slot];
    assert(save_player());random_unique_reset();assert(load_player());
    assert(!strcmp(name,r_name+r_info[slot].name));
    assert(r_info[slot].flags5==original.flags5 && r_info[slot].flags1==original.flags1);
    assert(r_info[slot].escort_char[0]==r_info[slot].d_char && !r_info[slot].text);
    assert(!memcmp(r_info[slot].blow,original.blow,sizeof(original.blow)));
    puts("Old-style individual and new escort persisted without regeneration");
    /* Another retained floor owns this slot even when its live counter is zero. */
    floor=get_new_floor_id();r_info[slot].floor_id=floor;
    get_sf_ptr(floor)->dun_level=dun_level;
    assert(save_floor(get_sf_ptr(floor),0));
    wipe_m_list();
    assert(!random_unique_generate(slot));
    assert(load_floor(get_sf_ptr(floor),0));
    assert(!strcmp(name,r_name+r_info[slot].name));
    assert(cave[5][5].m_idx && m_list[cave[5][5].m_idx].hp==mon.hp);
    wipe_m_list();
    original=r_info[slot];assert(!random_unique_generate(slot));
    assert(!memcmp(&original,&r_info[slot],sizeof(original)));
    get_sf_ptr(floor)->floor_id=0;assert(random_unique_generate(slot));
    /* Killing the individual permits a newly named one; fixed uniques unchanged. */
    r_info[slot].floor_id=0;
    for(i=0;i<100;i++){assert(random_unique_generate(slot));if(place_monster_aux(0,5,5,slot,0))break;}
    assert(i<100);j=cave[5][5].m_idx;
    other=r_info[MON_ASMODE];
    r_info[slot].mexp=0;
    mon_take_hit(j,m_list[j].hp+1,&fear,NULL,FALSE);assert(!cave[5][5].m_idx);
    assert(r_info[slot].max_num==0 && random_unique_kills==13);assert(random_unique_generate(slot));
    assert(random_unique_kills==13);
    assert(!memcmp(&other,&r_info[MON_ASMODE],sizeof(other)));
    /* The demon realm permits only evil races, including generated ones. */
    wipe_m_list();random_unique_reset();dungeon_type=DUNGEON_DEMON;dun_level=950;monster_level=100;
    for(i=0;i<150;i++)
    {
        get_mon_num_prep(get_monster_hook(),get_monster_hook2(5,5));
        for(j=0;j<alloc_race_size;j++)
            if(IS_RANDOM_UNIQUE(alloc_race_table[j].index) && alloc_race_table[j].prob2)
                assert(r_info[alloc_race_table[j].index].flags3 & RF3_EVIL);
    }
    puts("Random uniques: variety, natural spawn, display, duplicate/floor protection, save/load, death and demon filter passed");
}
"""
h=(ROOT/'tests/party_integration.c').read_bytes().decode('euc_jp')
h=h.replace('int main(int argc,char **argv)',extra+'\nint main(int argc,char **argv)',1)
h=h.replace('    display_regressions();','    random_unique_tests();\n    return 0;\n    display_regressions();',1)
with tempfile.TemporaryDirectory(prefix='toband-random-unique-') as d:
    tmp=Path(d)
    shutil.copytree(BUILD/'lib',tmp/'lib',ignore=shutil.ignore_patterns('*.raw','save'))
    for p in (ROOT/'lib/edit').glob('*.txt'):
        (tmp/'lib/edit'/p.name).write_bytes(p.read_bytes().decode('euc_jp').encode('cp932'))
    (tmp/'lib/save').mkdir(exist_ok=True)
    (tmp/'test.c').write_bytes(h.encode('euc_jp'))
    objs=[str(p) for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    testobjs=[p for p in objs if Path(p).stem!='random-unique']
    subprocess.run([str(GCC),*FLAGS,str(tmp/'test.c'),*testobjs,'-o',str(tmp/'test.exe'),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(tmp/'test.exe'),(tmp/'lib').as_posix()+'/'],cwd=tmp,capture_output=True,timeout=90)
    print(result.stdout.decode('cp932',errors='replace'))
    if result.returncode:
        print(result.stderr.decode('cp932',errors='replace'));raise SystemExit(result.returncode)
    # Previous 0.10.0.1 save writer and previous race count (including runeweapon remap).
    old=(ROOT/'src/save.c').read_bytes().replace(b'    wr_random_uniques();', b'')
    old=old.replace(b'wr_byte(T_VER_EXTRA);',b'wr_byte(1);')
    (tmp/'old-save.c').write_bytes(old)
    subprocess.run([str(GCC),*FLAGS,'-c',str(tmp/'old-save.c'),'-o',str(tmp/'old-save.o')],check=True)
    old_h=(ROOT/'tests/party_integration.c').read_bytes().decode('euc_jp')
    start=old_h.index('    /* Exercise the whole live save,')
    end=old_h.index('    if(argc==3 && !strcmp(argv[2],"legacy"))',start)
    old_h=old_h[:start]+r"""
    cur_hgt=cur_wid=5;py=px=1;p_ptr->is_dead=0;
    for(i=0;i<5;i++){int x;for(x=0;x<5;x++)cave[i][x].feat=FEAT_FLOOR;}
    max_r_idx=989;
    assert(save_player());max_r_idx=992;
    assert(load_player());
    assert(party_count==2 && !strcmp(player_name,"Original"));
    for(i=MON_RANDOM_UNIQUE_1;i<=MON_RANDOM_UNIQUE_3;i++)assert(r_info[i].extra==0 && r_info[i].max_num>0);
    dun_level=monster_level=60;assert(random_unique_generate(MON_RANDOM_UNIQUE_1));
    puts("Previous-version save migrated; random uniques available");
    return 0;
"""+old_h[end:]
    (tmp/'old-test.c').write_bytes(old_h.encode('euc_jp'))
    oldobjs=[str(tmp/'old-save.o') if Path(p).stem=='save' else p for p in objs]
    subprocess.run([str(GCC),*FLAGS,str(tmp/'old-test.c'),*oldobjs,'-o',str(tmp/'old-test.exe'),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(tmp/'old-test.exe'),(tmp/'lib').as_posix()+'/'],cwd=tmp,capture_output=True,timeout=90)
    print(result.stdout.decode('cp932',errors='replace'))
    if result.returncode:
        print(result.stderr.decode('cp932',errors='replace'));raise SystemExit(result.returncode)
