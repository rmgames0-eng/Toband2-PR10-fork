/* Random unique monster generation, adapted from asamayim/gensoband
 * src/random_unique_monster.c, revision fc4418078a62db9fae2915c8b6ab24a063609093.
 * Symbol selection and base power formulas are adapted from that implementation.
 * TOband-R3 uses independent epithet profiles and symbol/gender-specific name pools.
 *
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */
#include "angband.h"

u32b random_unique_kills = 0;

#include "random-unique-data.h"
#include "random-unique-symbols.h"

struct random_unique_symboltype
{
	char	mon_char;
	int	freq;
	int 	base_level_min;
	int	base_level_max;
	int	hd_sides;
	int	power_rate;
	int rbm_60;
	int rbm_30;
	int rbm_10;
};
typedef struct random_unique_symboltype random_unique_symboltype;
#define RU_SYMBOL_NUM	54
static const random_unique_symboltype 	r_unique_symbol_table[RU_SYMBOL_NUM] ={
	{'a',20,0,40,43,75,RBM_BITE,RBM_CLAW,RBM_SPIT},
	{'b',20,0,40,49,75,RBM_CLAW,RBM_BITE,RBM_WAIL},
	{'c',40,0,50,42,85,RBM_BITE,RBM_STING,RBM_CRAWL},
	{'d',70,20,50,71,100,RBM_BITE,RBM_CLAW,RBM_CRUSH},
	{'e',30,10,80,55,90,RBM_GAZE,RBM_GAZE,RBM_CRUSH},
	{'f',60,0,70,61,90,RBM_CLAW,RBM_BITE,RBM_KICK},
	{'g',70,20,80,113,80,RBM_PUNCH,RBM_CRUSH,RBM_CRUSH},
	{'h',120,5,90,61,100,RBM_PUNCH,RBM_SLASH,RBM_KICK},
	{'i',20,0,25,41,50,RBM_CRAWL,RBM_SPIT,RBM_HIT},
	{'j',40,0,90,81,90,RBM_ENGULF,RBM_DROOL,RBM_HIT},
	{'k',50,0,70,77,85,RBM_CLAW,RBM_KICK,RBM_WAIL},
	{'l',40,16,60,73,90,RBM_CRUSH,RBM_CRUSH,RBM_HIT},
	{'m',20,0,40,55,75,RBM_SPORE,RBM_ENGULF,RBM_HIT},
	{'n',30,15,60,61,95,RBM_HIT,RBM_CRUSH,RBM_PUNCH},
	{'o',40,0,35,73,75,RBM_PUNCH,RBM_CRUSH,RBM_SLASH},
	{'p',150,0,110,75,100,RBM_SLASH,RBM_PUNCH,RBM_HIT},
	{'q',20,0,90,81,90,RBM_KICK,RBM_CRUSH,RBM_WAIL},
	{'r',20,0,40,39,75,RBM_BITE,RBM_BITE,RBM_CRUSH},
	{'s',40,5,60,53,95,RBM_PUNCH,RBM_SLASH,RBM_WAIL},
	{'t',25,40,80,84,110,RBM_KICK,RBM_SLASH,RBM_CRUSH},
	{'u',30,20,50,52,85,RBM_CLAW,RBM_STING,RBM_BITE},
	{'v',25,10,80,33,65,RBM_ENGULF,RBM_TOUCH,RBM_GAZE},
	{'w',20,0,70,70,75,RBM_CRUSH, RBM_BITE, RBM_ENGULF},
	{'y',70,0,50,52,80,RBM_CLAW,RBM_PUNCH,RBM_HIT},
	{'z',30,10,50,87,85,RBM_BITE,RBM_CLAW,RBM_WAIL},
	{'A',25,40,120,91,120, RBM_SLASH, RBM_GAZE, RBM_WAIL},
	{'B',30,0,90,56,85,RBM_CLAW,RBM_CRUSH,RBM_CLAW},
	{'C',40,10,80,90,85,RBM_CRUSH,RBM_BITE,RBM_BITE},
	{'D',60,50,120,100,135,RBM_HIT,RBM_BITE,RBM_CRUSH},
	{'E',50,20,90,61,100,RBM_TOUCH,RBM_HIT,RBM_HIT},
	{'F',30,0,40,43,70,RBM_HIT,RBM_KICK,RBM_WAIL},
	{'G',50,20,80,57,95,RBM_TOUCH,RBM_WAIL,RBM_GAZE},
	{'H',20,40,90,93,85,RBM_CRUSH,RBM_CLAW,RBM_BITE},
	{'I',30,20,70,61,75,RBM_BITE,RBM_STING,RBM_SLASH},
	{'J',25,10,50,64,80,RBM_BITE,RBM_CRUSH,RBM_CRUSH},
	{'K',25,10,90,80,90,RBM_BITE,RBM_CLAW,RBM_CRUSH},
	{'L',30,60,120,70,110,RBM_TOUCH,RBM_GAZE,RBM_GAZE},
	{'M',15,50,100,97,80,RBM_BITE,RBM_CRUSH,RBM_CRUSH},
	{'O',30,13,127,100,115,RBM_CRUSH,RBM_HIT,RBM_KICK},
	{'P',25,50,100,100,110,RBM_PUNCH,RBM_CRUSH,RBM_CRUSH},
	{'Q',20,40,100,49,75,RBM_CRAWL,RBM_ENGULF,RBM_GAZE},
	{'R',25,10,90,73,85,RBM_BITE,RBM_CLAW,RBM_ENGULF},
	{'S',35,1,90,60,85,RBM_BITE,RBM_STING,RBM_CLAW},
	{'T',30,30,70,94,75,RBM_PUNCH,RBM_CRUSH,RBM_SPIT},
	{'U',35,60,120,93,120,RBM_SLASH,RBM_BUTT,RBM_CRUSH},
	{'V',25,50,100,96,110,RBM_SLASH,RBM_BITE,RBM_BITE},
	{'W',20,40,100,63,100,RBM_TOUCH,RBM_SLASH,RBM_HIT},
	{'X',5,50,100,66,100,RBM_CRUSH,RBM_CRUSH,RBM_CRUSH},
	{'Y',50,40,100,83,120,RBM_SLASH,RBM_TOUCH,RBM_GAZE},
	{'Z',15,30,70,43,75,RBM_CLAW,RBM_BITE,RBM_CRUSH},
	{'#',10,20,80,73,85,RBM_ENGULF,RBM_CRUSH,RBM_CRUSH},
	{'&',5,30,70,80,80,RBM_ENGULF,RBM_BITE,RBM_TOUCH},
	{'*',15,20,100,47,90,RBM_CRUSH,RBM_GAZE,RBM_ENGULF},
	{',',20,10,50,41,60,RBM_CRUSH,RBM_SPORE,RBM_SPORE},
};
/* Slots are reusable only after their individual has died or its floor expired.
 * A live monster on a saved floor must never acquire a different race record. */
bool random_unique_available(int r_idx)
{
    monster_race *r;
    if (!IS_RANDOM_UNIQUE(r_idx) || r_idx >= max_r_idx) return FALSE;
    r = &r_info[r_idx];
    if (r->cur_num) return FALSE;
    return !r->max_num || !r->floor_id || !get_sf_ptr(r->floor_id);
}

static void clear_random_unique(int r_idx)
{
    monster_race *r = &r_info[r_idx];
    u32b name = r->name;
#ifdef JP
    u32b ename = r->E_name;
#endif
    WIPE(r, monster_race);
    r->name = name; r->text = 0;
#ifdef JP
    r->E_name = ename;
#endif
    r->hdice = r->hside = 1;
    r->speed = 110; r->ac = 10; r->aaf = 20;
    r->level = 1; r->rarity = r_idx == MON_RANDOM_UNIQUE_3 ? 3 : 5;
    r->max_num = 1;
    r->r_elem = NO_ELEM;
    r->flags1 = RF1_UNIQUE | RF1_FORCE_MAXHP;
    r->flags7 = RF7_UNIQUE2;
    if (r_idx == MON_RANDOM_UNIQUE_3) r->flags7 |= RF7_AQUATIC | RF7_CAN_SWIM;
    r->d_char = r->x_char = '?';
    r->d_attr = r->x_attr = TERM_WHITE;
    my_strcpy(r_name + name, "ランダムユニーク", RANDOM_UNIQUE_NAME_SIZE);
}

void random_unique_reset(void)
{
    int i;
    random_unique_kills = 0;
    for (i = MON_RANDOM_UNIQUE_1; i <= MON_RANDOM_UNIQUE_3 && i < max_r_idx; i++)
        clear_random_unique(i);
}

static int random_unique_symbol(int r_idx, int lev)
{
    int i, total = 0, weights[RU_SYMBOL_NUM];
    for (i = 0; i < RU_SYMBOL_NUM; i++)
    {
        const random_unique_symboltype *s = &r_unique_symbol_table[i];
        int w = s->freq;
        if (lev < s->base_level_min)
            w = w * MAX(0, lev - s->base_level_min / 2) / MAX(1, s->base_level_min / 2);
        if (lev > s->base_level_max)
            w = w * MAX(0, s->base_level_max - lev / 2) / MAX(1, lev / 2);
        if (r_idx == MON_RANDOM_UNIQUE_3)
        {
            if (strchr("ijlnKM", s->mon_char)) w *= 5;
            if (strchr("adfkoqtCDFOW", s->mon_char)) w /= 5;
            if (strchr("bpABV", s->mon_char)) w = 0;
        }
        weights[i] = w; total += w;
    }
    total = randint1(MAX(1, total));
    for (i = 0; i < RU_SYMBOL_NUM - 1; i++)
        if ((total -= weights[i]) <= 0) break;
    return i;
}


/* Tier is based only on the generated monster level, never on its bias/power. */
static int random_unique_tier(int level)
{
    return level < 40 ? 0 : level < 80 ? 1 : 2;
}

static int random_unique_profile_index(int level)
{
    int i, count = 0, pick, tier = random_unique_tier(level);
    for (i = 0; i < RU_PROFILE_COUNT; i++)
        if (ru_profiles[i].tier == tier) count++;
    pick = randint0(count);
    for (i = 0; i < RU_PROFILE_COUNT; i++)
        if (ru_profiles[i].tier == tier && pick-- == 0) return i;
    return 0;
}

static void random_unique_traits(monster_race *r, int power)
{
    int i, lev = r->level;
    if (power > lev / 3) r->flags1 |= RF1_DROP_GOOD;
    if (lev > 19 && randint0(MAX(1, power * 3 / 2)) > randint0(lev)) r->flags1 |= RF1_DROP_GREAT;
    i = MIN(20, power / ((r->flags1 & RF1_DROP_GREAT) ? 12 : 6));
    if (i >= 8) {r->flags1 |= RF1_DROP_4D2; i -= 8;}
    if (i >= 6) {r->flags1 |= RF1_DROP_3D2; i -= 6;}
    if (i >= 4) {r->flags1 |= RF1_DROP_2D2; i -= 4;}
    if (i >= 2) r->flags1 |= RF1_DROP_1D2;
}

static void random_unique_apply_profile(monster_race *r, const random_unique_profile *p)
{
    int i, frequency;
    r->flags1 |= p->flags1;
    r->flags2 |= p->flags2;
    r->flags3 |= p->flags3;
    r->flags4 |= p->flags4;
    r->flags5 |= p->flags5;
    r->flags6 |= p->flags6;
    r->flags7 |= p->flags7;
    r->flagsr |= p->flagsr;
    if (r->flags1 & RF1_ESCORT) r->escort_char[0] = r->d_char;
    if (p->flags3 & RF3_GOOD) r->flags3 &= ~RF3_EVIL;
    if (p->flags3 & RF3_EVIL) r->flags3 &= ~RF3_GOOD;
    if (p->flags1 & RF1_ONLY_GOLD) r->flags1 &= ~RF1_ONLY_ITEM;
    if (p->flags2 & RF2_WEIRD_MIND) r->flags2 &= ~RF2_EMPTY_MIND;
    if (p->flags2 & RF2_MOVE_BODY) r->flags2 &= ~RF2_KILL_BODY;
    if (p->flags2 & RF2_PASS_WALL) r->flags2 &= ~RF2_KILL_WALL;
    if (p->flags2 & RF2_KILL_BODY) r->flags2 &= ~RF2_MOVE_BODY;
    if (p->flags2 & RF2_KILL_WALL) r->flags2 &= ~RF2_PASS_WALL;
    if (p->flags3 & RF3_HURT_FIRE)
    {
        r->flagsr &= ~RFR_RES_FIRE;
        r->flags2 &= ~RF2_AURA_FIRE;
    }
    /* An explicitly mobile profile must not inherit an immobile symbol. */
    if (p->speed_bonus > 0 || (p->flags1 & (RF1_RAND_25 | RF1_RAND_50)))
        r->flags1 &= ~RF1_NEVER_MOVE;
    if (p->sleep >= 0) r->sleep = p->sleep;
    r->hside = MAX(1, MIN(65535L, (long)r->hside * p->hp_percent / 100));
    r->ac = MAX(0, MIN(500, r->ac + p->ac_bonus));
    r->speed = MAX(100, MIN(180, r->speed + p->speed_bonus));
    for (i = 0; i < 4; i++)
    {
        if (p->blows[i].method)
        {
            r->blow[i].method = p->blows[i].method;
            r->blow[i].effect = p->blows[i].effect;
        }
        r->blow[i].d_side = MAX(1, MIN(255, r->blow[i].d_side * p->damage_percent / 100));
    }
    r->freq_spell = (r->flags4 || r->flags5 || r->flags6 || r->flagsa) ? MAX(r->freq_spell, p->spell_frequency) : 0;
    /* Symbol magic remains available even for a physical epithet. */
    if (r->freq_spell) {
        frequency = r->freq_spell + randint0(3) * 5;
        r->freq_spell = MIN(50, frequency);
    }
    r->d_attr = r->x_attr = p->color;
    r->text = 0;
}

static void random_unique_name(int r_idx, int profile)
{
    char name[RANDOM_UNIQUE_NAME_SIZE], suffix[RANDOM_UNIQUE_NAME_SIZE];
    bool human = (r_info[r_idx].d_char && strchr("VoOpUuGFAWth", r_info[r_idx].d_char));
    cptr *names = human ? ru_human_names : ru_large_names;
    int count = human ? RU_HUMAN_NAME_COUNT : RU_LARGE_NAME_COUNT;
    int i, offset, first;
    u32b gender = r_info[r_idx].flags1 & (RF1_MALE | RF1_FEMALE);
    if (human && gender == RF1_MALE) count = RU_MALE_NAME_COUNT;
    else if (human && gender == RF1_FEMALE)
    {
        names += RU_MALE_NAME_COUNT;
        count = RU_FEMALE_NAME_COUNT;
    }
    /* No gender (or conflicting flags): use the complete human pool. */
    first = randint0(count);
    /* At most two other live/retained slots: scanning the whole pool guarantees
     * distinct personal names without an unbounded retry or numeric suffix. */
    for (offset = 0; offset < count; offset++)
    {
        int n = (first + offset) % count;
        strnfmt(suffix, sizeof(suffix), "《%s》", names[n]);
        for (i = MON_RANDOM_UNIQUE_1; i <= MON_RANDOM_UNIQUE_3; i++)
            if (i != r_idx && strstr(r_name + r_info[i].name, suffix)) break;
        if (i > MON_RANDOM_UNIQUE_3) break;
    }
    strnfmt(name, sizeof(name), "%s%s%s", ru_profiles[profile].title,
        ru_profiles[profile].use_no ? "の" : "", suffix);
    my_strcpy(r_name + r_info[r_idx].name, name, RANDOM_UNIQUE_NAME_SIZE);
}

/* Base rolls vary inside a symbol and respond to the title's independent
 * aptitudes. Later ecology/profile multipliers still apply exactly once. */
static void random_unique_base(monster_race *r,const random_unique_symboltype *s,
    const random_unique_profile *p,int lev,int power)
{
    int i,dice,w[RU_GROW_COUNT],spread=MAX(2,s->hd_sides/10);
    r->level=lev;r->extra=1;r->d_char=r->x_char=s->mon_char;
    ru_growth_weights(r,p,w);
    r->hdice=2+lev/6+randint1(MAX(2,lev/10));
    r->hside=MAX(1,s->hd_sides+randint0(2*spread+1)-spread);
    if(w[RU_GROW_HP]>=150)r->hdice+=1+lev/25;
    r->ac=12+lev/2+randint0(8+lev/3);
    if(w[RU_GROW_AC]>=80)r->ac+=10+lev/10;
    r->speed=110+MAX(0,(lev-20)/6)+randint0(3);
    if(w[RU_GROW_SPEED]>=45)r->speed+=3+lev/30;
    else if(w[RU_GROW_HP]>=150)r->speed-=2;
    r->sleep=strchr("jmvQZ&*",s->mon_char)?0:30+randint1(70);
    r->aaf=15+lev/4+randint0(MAX(1,lev/3));
    if(w[RU_GROW_SENSE]>=40)r->aaf=MIN(100,r->aaf+15);
    r->mexp=(power+randint1(3))*lev*(250+randint1(250))/100;
    r->flags1|=RF1_FORCE_SLEEP|RF1_ONLY_ITEM|RF1_DROP_90;
    r->flags2=RF2_OPEN_DOOR|RF2_BASH_DOOR;r->flags3=RF3_NO_SLEEP;
    dice=1+lev/20;
    if(w[RU_GROW_DAMAGE]>=140)dice++;
    if(w[RU_GROW_DAMAGE]>=220)dice++;
    for(i=0;i<4;i++) {
        int roll=randint0(10);
        r->blow[i].method=roll<6?s->rbm_60:roll<9?s->rbm_30:s->rbm_10;
        r->blow[i].effect=RBE_HURT;
        r->blow[i].d_dice=dice+randint0(MAX(1,dice/2));
        r->blow[i].d_side=dice+randint1(dice);
    }
}

static void random_unique_attack_count(monster_race *r,const random_unique_profile *p)
{
    int j,count=2+(r->level>=20)+(r->level>=40);
    if(r->d_char=='M' && r->level>=20)count=4;
    /* An explicitly specified fourth blow also keeps slots 1-3 active. */
    for(j=0;j<4;j++)if(p->blows[j].method)count=MAX(count,j+1);
    for(j=count;j<4;j++)memset(&r->blow[j],0,sizeof(r->blow[j]));
}

bool random_unique_generate(int r_idx)
{
    monster_race *r;
    const random_unique_symboltype *s;
    int i, lev, profile, power, level = MAX(1, MIN(127, monster_level));
    if (dun_level < 10 || p_ptr->inside_arena || p_ptr->inside_quest || p_ptr->wild_mode ||
        !random_unique_available(r_idx)) return FALSE;
    lev = randint1(level);
    for (i = 0; i < 2; i++) {int roll = randint1(level); lev = MAX(lev, roll);}
    profile = random_unique_profile_index(lev);
    s = &r_unique_symbol_table[random_unique_symbol(r_idx, lev)];
    power = s->power_rate / 10 + randint1(5);
    while (power < 1000 && !one_in_(7)) power++;
    if (lev >= 40 && one_in_(13)) power *= 2;
    if (lev >= 70 && one_in_(666)) power *= 5;
    power = power * lev / 30 * s->power_rate / 100 + randint0(MAX(1, lev / 5));
    power = MAX(1, MIN(1000, power));
    clear_random_unique(r_idx);
    r = &r_info[r_idx];
    random_unique_base(r, s, &ru_profiles[profile], lev, power);
    random_unique_traits(r, power);
    random_unique_symbol_skills(r, power, &ru_profiles[profile]);
    random_unique_apply_profile(r, &ru_profiles[profile]);
    random_unique_resistances(r, &ru_profiles[profile]);
    random_unique_attack_count(r, &ru_profiles[profile]);
    random_unique_name(r_idx, profile);
    return TRUE;
}
