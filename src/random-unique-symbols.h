/* Symbol ecology for TOband-R3 random uniques.
 * Inspired by gensoband's symbol suitability, weighted skills and final
 * resistance pass. Symbols refer to THIS game's bestiary, not Gensokyo's.
 * Included only by random-unique.c. No new attacks or persistent fields. */
#define RU_FIRE   0x0001UL
#define RU_COLD   0x0002UL
#define RU_ELEC   0x0004UL
#define RU_ACID   0x0008UL
#define RU_POIS   0x0010UL
#define RU_WATER  0x0020UL
#define RU_DARK   0x0040UL
#define RU_DEATH  0x0080UL
#define RU_HOLY   0x0100UL
#define RU_MIND   0x0200UL
#define RU_SPACE  0x0400UL
#define RU_ARCANE 0x0800UL
#define RU_WAR    0x1000UL
#define RU_KIN    0x2000UL

typedef struct {
    byte group, min_level, weight;
    u32b flag, themes;
    cptr suitable, unsuitable;
    byte max_level;
} ru_skill;

/* Suitable symbols lower the preferred minimum by ten. Below that level
 * weights taper to zero at half; novice skills taper out above their preferred
 * maximum. Each acquired skill consumes power and leaves the candidate pool. */
static const ru_skill ru_skills[] = {
    {5,10,30,RF5_BO_FIRE,RU_FIRE,"dDEuvU","",60},
    {5,25,22,RF5_BA_FIRE,RU_FIRE,"dDEuU",""},
    {4,30,12,RF4_BR_FIRE,RU_FIRE,"dDvZH","hptAF"},
    {2,30,12,RF2_AURA_FIRE,RU_FIRE,"EvU",""},
    {5,10,30,RF5_BO_COLD,RU_COLD,"dDE","",60},
    {5,25,22,RF5_BA_COLD,RU_COLD,"dDEL",""},
    {5,50,12,RF5_BO_ICEE,RU_COLD,"DL",""},
    {4,30,12,RF4_BR_COLD,RU_COLD,"dDvZ","hptAF"},
    {3,35,10,RF3_AURA_COLD,RU_COLD,"EvGW",""},
    {5,10,30,RF5_BO_ELEC,RU_ELEC,"dDE","",60},
    {5,25,22,RF5_BA_ELEC,RU_ELEC,"dDEP",""},
    {4,30,12,RF4_BR_ELEC,RU_ELEC,"dDvZ","hptAF"},
    {2,35,10,RF2_AURA_ELEC,RU_ELEC,"Ev",""},
    {5,10,30,RF5_BO_ACID,RU_ACID,"dDij","",60},
    {5,25,22,RF5_BA_ACID,RU_ACID,"dDij",""},
    {4,30,12,RF4_BR_ACID,RU_ACID,"dDvZ","hptAF"},
    {5,15,25,RF5_BA_POIS,RU_POIS,"ijnmS",""},
    {4,25,12,RF4_BR_POIS,RU_POIS,"dDZMJ","hptAF"},
    {5,20,25,RF5_BO_WATE,RU_WATER,"lnEM","",65},
    {5,40,20,RF5_BA_WATE,RU_WATER,"lnEM",""},
    {5,35,25,RF5_BA_DARK,RU_DARK,"OUVLW",""},
    {4,45,10,RF4_BR_DARK,RU_DARK,"ODZ","hptAF"},
    {6,10,22,RF6_DARKNESS,RU_DARK|RU_DEATH,"GVWL",""},
    {5,25,25,RF5_BO_NETH,RU_DEATH,"GLVW",""},
    {5,55,16,RF5_BA_NETH,RU_DEATH,"LVWU",""},
    {4,65,10,RF4_BR_NETH,RU_DEATH,"DWZ","hptAF"},
    {6,20,20,RF6_S_UNDEAD,RU_DEATH,"LVW",""},
    {6,80,8,RF6_S_HI_UNDEAD,RU_DEATH,"LVW",""},
    {5,30,24,RF5_BA_LITE,RU_HOLY,"A",""},
    {6,30,24,RF6_HEAL,RU_HOLY|RU_ARCANE,"AOL",""},
    {6,65,12,RF6_GODLY_SPEAR,RU_HOLY,"A",""},
    {6,65,8,RF6_S_ANGEL,RU_HOLY,"A",""},
    {5,5,25,RF5_SCARE,RU_MIND|RU_DARK,"eGLVW","",65},
    {5,15,25,RF5_CONF,RU_MIND,"enL",""},
    {5,15,20,RF5_BLIND,RU_MIND|RU_DARK,"enO",""},
    {5,25,14,RF5_HOLD,RU_MIND,"enL",""},
    {5,20,20,RF5_MIND_BLAST,RU_MIND,"eL",""},
    {5,65,8,RF5_BRAIN_SMASH,RU_MIND,"eL",""},
    {5,35,18,RF5_DRAIN_MANA,RU_MIND|RU_ARCANE,"eLU",""},
    {6,30,14,RF6_FORGET,RU_MIND,"eLV",""},
    {6,5,24,RF6_BLINK,RU_SPACE,"pFQGS",""},
    {6,30,18,RF6_TPORT,RU_SPACE,"pFQL",""},
    {6,40,16,RF6_TELE_TO,RU_SPACE|RU_MIND,"QeL",""},
    {6,40,16,RF6_TELE_AWAY,RU_SPACE,"QL",""},
    {6,30,14,RF6_TRAPS,RU_SPACE,"pQ",""},
    {5,5,30,RF5_MISSILE,RU_ARCANE,"hpy","",30},
    {5,35,16,RF5_BO_MANA,RU_ARCANE,"pLO",""},
    {5,85,7,RF5_BA_MANA,RU_ARCANE,"LO",""},
    {4,55,12,RF4_DISPEL,RU_ARCANE|RU_MIND,"pLU",""},
    {5,15,22,RF5_CAUSE_2,RU_DEATH|RU_MIND,"pLW","",45},
    {5,40,16,RF5_CAUSE_3,RU_DEATH|RU_MIND,"pLW","",75},
    {5,70,9,RF5_CAUSE_4,RU_DEATH|RU_MIND,"pLW",""},
    {6,30,16,RF6_HASTE,RU_WAR|RU_ARCANE,"pOLT",""},
    {2,25,16,RF2_SMART,RU_ARCANE|RU_MIND|RU_HOLY,"hpALO","gijmv,"},
    {3,20,18,RF3_NO_FEAR,RU_WAR,"OPTg",""},
    {3,50,12,RF3_NO_STUN,RU_WAR,"OPg",""},
    {2,30,14,RF2_REGENERATE,RU_WAR|RU_DEATH,"TMVz",""},
    {2,35,12,RF2_MOVE_BODY,RU_WAR,"OPTg",""},
    {2,60,6,RF2_KILL_WALL,RU_WAR,"gPX","O"},
    {6,20,18,RF6_S_KIN,RU_KIN,"aCIOSTZ",""},
    {6,25,18,RF6_S_MONSTER,RU_KIN,"Qhy",""},
    {6,55,10,RF6_S_MONSTERS,RU_KIN,"Q",""},
    {6,40,12,RF6_S_DEMON,RU_DARK,"uU",""},
    {6,45,12,RF6_S_DRAGON,RU_KIN,"dDQ",""},
    {6,85,6,RF6_S_HI_DRAGON,RU_KIN,"DQ",""},
    {1,20,10,RF1_ESCORT,RU_KIN,"aCIOSTZ",""},
    {1,65,4,RF1_ESCORT|RF1_ESCORTS,RU_KIN,"aCOTZ",""},
    {2,45,8,RF2_REFLECTING,RU_ARCANE|RU_WAR,"gAL","ijmw,"},
    {2,25,12,RF2_INVISIBLE,RU_SPACE|RU_DEATH,"GVW","OPT"},
    {2,35,10,RF2_PASS_WALL,RU_SPACE|RU_DEATH,"GVW#","OP"},
    {3,65,12,RF3_RES_TELE,RU_SPACE|RU_ARCANE,"LOUQ",""},
    {4,50,10,RF4_BR_SOUN,RU_WAR,"DZP","hpt"},
    {4,55,10,RF4_BR_SHAR,RU_COLD|RU_WAR,"DZ","hpt"},
    {4,50,10,RF4_BR_WALL,RU_SPACE|RU_WAR,"DvZ","hpt"},
    {4,65,8,RF4_BR_CHAO,RU_ARCANE,"DZ","hptA"},
    {4,85,5,RF4_BR_TIME,RU_SPACE,"DZ","hpt"},
    {4,70,8,RF4_BR_GRAV,RU_SPACE,"DvZ","hpt"},
    {4,40,10,RF4_BR_INER,RU_COLD|RU_SPACE,"DvZ","hpt"},
    {4,90,4,RF4_BR_MANA,RU_ARCANE,"DZ*","hpt"},
    {4,95,3,RF4_BR_DISI,RU_ARCANE,"DZ","hptA"},
    {4,75,6,RF4_BR_NUKE,RU_POIS|RU_FIRE,"DZ","hptA"},
    {5,45,18,RF5_BO_PLAS,RU_FIRE|RU_ELEC,"UED",""},
    {6,20,15,RF6_S_ANT,RU_KIN,"a",""},
    {6,25,15,RF6_S_SPIDER,RU_KIN,"S",""},
    {6,35,15,RF6_S_HOUND,RU_KIN,"CZ",""},
    {6,30,12,RF6_S_BEAST,RU_KIN,"qCf",""},
    {8,15,18,RFR_RES_ACID,RU_ACID|RU_WAR,"gDij",""},
    {8,15,18,RFR_RES_ELEC,RU_ELEC|RU_WAR,"gDE",""},
    {8,15,18,RFR_RES_FIRE,RU_FIRE|RU_WAR,"gDOU",""},
    {8,15,18,RFR_RES_COLD,RU_COLD|RU_WAR,"gDGL",""},
    {8,20,18,RFR_RES_POIS,RU_POIS|RU_WAR,"gijmszGLV",""},
    {8,40,12,RFR_RES_SOUN,RU_WAR,"gOP",""},
    {8,45,12,RFR_RES_SHAR,RU_WAR|RU_COLD,"gD",""},
    {8,55,10,RFR_RES_CHAO,RU_ARCANE,"UL",""},
    {8,50,10,RFR_RES_DISE,RU_ARCANE|RU_WAR,"gL",""},
    {8,75,6,RFR_RES_TIME,RU_SPACE,"Q",""},
    {8,65,8,RFR_RES_GRAV,RU_SPACE,"v",""},
    {8,45,10,RFR_RES_STON,RU_WAR,"gO",""}
};
#define RU_SKILL_COUNT (sizeof(ru_skills) / sizeof(ru_skills[0]))

static u32b *ru_flag_group(monster_race *r, int group)
{
    switch (group) {
    case 1: return &r->flags1;
    case 2: return &r->flags2;
    case 3: return &r->flags3;
    case 4: return &r->flags4;
    case 5: return &r->flags5;
    case 6: return &r->flags6;
    default: return &r->flagsr;
    }
}

static u32b ru_profile_flag(const random_unique_profile *p,int group)
{
    switch(group) {
    case 1:return p->flags1;
    case 2:return p->flags2;
    case 3:return p->flags3;
    case 4:return p->flags4;
    case 5:return p->flags5;
    case 6:return p->flags6;
    default:return p->flagsr;
    }
}

/* An elemental body/breath has one coherent element, chosen per individual. */
static u32b ru_element(monster_race *r, bool breath, bool body)
{
    static const u32b themes[] = {RU_ACID,RU_ELEC,RU_FIRE,RU_COLD,RU_POIS};
    static const u32b breaths[] = {RF4_BR_ACID,RF4_BR_ELEC,RF4_BR_FIRE,RF4_BR_COLD,RF4_BR_POIS};
    static const u32b resists[] = {RFR_RES_ACID,RFR_RES_ELEC,RFR_RES_FIRE,RFR_RES_COLD,RFR_RES_POIS};
    static const byte effects[] = {RBE_ACID,RBE_ELEC,RBE_FIRE,RBE_COLD,RBE_POISON};
    int e = randint0(5);
    r->flagsr |= resists[e];
    if (breath) r->flags4 |= breaths[e];
    if (body) {
        int j;
        for (j=0;j<4;j++) r->blow[j].effect=effects[e];
        if (e==1) r->flags2|=RF2_AURA_ELEC;
        if (e==2) r->flags2|=RF2_AURA_FIRE;
        if (e==3) r->flags3|=RF3_AURA_COLD;
    }
    return themes[e];
}

/* Return the creature's spell affinities. A symbol owns real baseline traits;
 * optional skills are layered on top, then the explicit epithet takes priority. */
static u32b random_unique_symbol_traits(monster_race *r)
{
    char c=r->d_char;
    int lev=r->level, hp=100, ac=0, speed=0, damage=100, j;
    u32b theme=0;
    r->flags2 &= ~(RF2_OPEN_DOOR|RF2_BASH_DOOR);
    if (strchr("hkopstyAOLPTUV",c)) r->flags2|=RF2_OPEN_DOOR|RF2_BASH_DOOR;
    if (strchr("abcfrCIJKSZql",c)) r->flags3|=RF3_ANIMAL;
    if (strchr("szGLVW",c)) {
        r->flags3|=RF3_UNDEAD|RF3_EVIL|RF3_NO_FEAR;
        r->flags2|=RF2_COLD_BLOOD;
        r->flagsr|=RFR_RES_POIS|RFR_RES_NETH;
    }
    if (strchr("uU",c)) {
        r->flags3|=RF3_DEMON|RF3_EVIL;
        r->flagsr|=RFR_RES_FIRE|RFR_RES_DARK|RFR_RES_POIS;
    }
    if (strchr("gijmv&*,",c)) r->flags2|=RF2_EMPTY_MIND|RF2_COLD_BLOOD;
    if (strchr("gEv&*",c)) r->flags3|=RF3_NONLIVING;
    if (strchr("bBevAFG",c)) r->flags7|=RF7_CAN_FLY;
    if (strchr("Qm&,",c) && !one_in_(20)) r->flags1|=RF1_NEVER_MOVE;
    switch (c) {
    case 'a': ac=15; theme=RU_POIS|RU_KIN; r->blow[1].effect=RBE_POISON; break;
    case 'b': hp=80; speed=5; theme=RU_DARK; if(lev>=25)r->blow[3].effect=RBE_EXP_VAMP; break;
    case 'c': ac=10; theme=RU_POIS; r->blow[0].effect=RBE_POISON; break;
    case 'd': case 'D':
        hp=c=='D'?140:115; ac=c=='D'?30:15; damage=120;
        r->flags3|=RF3_DRAGON; r->flags7|=RF7_CAN_FLY;
        theme=ru_element(r,TRUE,FALSE)|RU_KIN;
        if(c=='D' && lev>=65)r->flags2|=RF2_POWERFUL;
        break;
    case 'e': hp=80; theme=RU_MIND; r->flags5|=RF5_CONF; r->blow[0].effect=RBE_PARALYZE; break;
    case 'f': hp=90; speed=8; damage=125; theme=RU_WAR; r->sleep=5; break;
    case 'g': hp=150; ac=40; speed=-5; damage=130; theme=RU_WAR;
        r->flags3|=RF3_NO_STUN|RF3_NO_CONF|RF3_NO_FEAR;
        r->flagsr|=RFR_RES_POIS|RFR_RES_STON; r->flags2|=RF2_MOVE_BODY;
        if(lev>=60)r->blow[0].effect=RBE_SHATTER;
        break;
    case 'h': case 'p':
        if(one_in_(2)){theme=RU_ARCANE|RU_MIND; r->flags2|=RF2_SMART; hp=85; r->freq_spell=30;}
        else {theme=RU_WAR|RU_KIN; ac=20; damage=120;}
        break;
    case 'i': case 'j':
        hp=120; speed=-5; theme=ru_element(r,FALSE,TRUE);
        r->flagsr|=RFR_RES_POIS; break;
    case 'k': theme=RU_POIS|RU_KIN; r->flags3|=RF3_EVIL; r->blow[1].effect=RBE_POISON; break;
    case 'l': hp=120; theme=RU_WATER; r->flags7|=RF7_CAN_SWIM; r->flagsr|=RFR_RES_WATE; break;
    case 'm': case ',': hp=130; theme=RU_POIS|RU_MIND; r->blow[0].effect=RBE_CONFUSE;
        r->flagsr|=RFR_RES_POIS; break;
    case 'n': ac=20; theme=RU_POIS|RU_WATER|RU_MIND; r->flags7|=RF7_CAN_SWIM;
        r->blow[0].effect=RBE_POISON; if(lev>=50)r->blow[1].effect=RBE_STONE; break;
    case 'o': ac=10; damage=115; theme=RU_WAR|RU_KIN; r->flags3|=RF3_ORC|RF3_EVIL; break;
    case 'q': hp=140; damage=135; theme=RU_WAR; break;
    case 'r': hp=75; speed=5; theme=RU_POIS; r->blow[0].effect=RBE_DISEASE; break;
    case 's': ac=20; theme=RU_DEATH|RU_WAR; r->flagsr|=RFR_RES_COLD; break;
    case 't': hp=80; theme=RU_SPACE|RU_MIND; r->flags2|=RF2_TAKE_ITEM;
        r->blow[0].effect=RBE_EAT_GOLD; break;
    case 'u': theme=RU_FIRE|RU_DARK|RU_SPACE; r->blow[0].effect=RBE_FIRE; break;
    case 'v': hp=85; speed=5; theme=ru_element(r,TRUE,TRUE); r->flagsr|=RFR_RES_POIS; break;
    case 'w': hp=130; speed=-5; theme=RU_POIS; r->blow[0].effect=RBE_POISON; break;
    case 'y': hp=80; theme=RU_ARCANE|RU_KIN; r->flags3|=RF3_EVIL; break;
    case 'z': hp=150; speed=-5; theme=RU_DEATH; r->blow[0].effect=RBE_DISEASE;
        r->flags2|=RF2_REGENERATE; break;
    case 'A': hp=120; ac=30; theme=RU_HOLY|RU_ARCANE;
        r->flags3|=RF3_GOOD|RF3_NO_FEAR; r->flags2|=RF2_SMART; r->freq_spell=30;
        r->flagsr|=RFR_RES_LITE; r->flags6|=RF6_HEAL;
        r->blow[1].effect=RBE_HOLY; if(lev>=75)r->flags5|=RF5_BA_LITE;
        break;
    case 'B': speed=10; hp=85; damage=115; theme=RU_WAR; r->blow[1].effect=RBE_BLIND; break;
    case 'C': speed=5; theme=RU_KIN|RU_WAR; r->sleep=5; r->aaf=MAX(r->aaf,40); break;
    case 'E':
        hp=115; r->flags3|=RF3_NO_FEAR|RF3_NO_CONF;
        r->r_elem=randint0(ELEM_NUM);
        if(r->r_elem==ELEM_FIRE){
            theme=RU_FIRE; r->flags3|=RF3_ELEM_FIRE; r->flags2|=RF2_AURA_FIRE;
            r->flags5|=RF5_BO_FIRE; r->flagsr|=RFR_RES_FIRE;
            for(j=0;j<4;j++)r->blow[j].effect=RBE_FIRE;
        } else if(r->r_elem==ELEM_AQUA){
            theme=RU_WATER|RU_COLD; r->flags3|=RF3_ELEM_AQUA; r->flags7|=RF7_CAN_SWIM;
            r->flags5|=RF5_BO_WATE; r->flagsr|=RFR_RES_WATE|RFR_RES_COLD;
            for(j=0;j<4;j++)r->blow[j].effect=RBE_COLD;
        } else if(r->r_elem==ELEM_EARTH){
            theme=RU_WAR; ac=40; r->flags3|=RF3_ELEM_EARTH; r->flags2|=RF2_PASS_WALL;
            r->flagsr|=RFR_RES_SHAR; r->blow[0].effect=RBE_SUPERHURT;
        } else {
            theme=RU_ELEC|RU_SPACE; speed=8; r->flags3|=RF3_ELEM_WIND; r->flags7|=RF7_CAN_FLY;
            r->flags5|=RF5_BO_ELEC; r->flagsr|=RFR_RES_ELEC;
            for(j=0;j<4;j++)r->blow[j].effect=RBE_ELEC;
        }
        break;
    case 'F': hp=75; speed=8; theme=RU_SPACE|RU_MIND; r->flags6|=RF6_BLINK; break;
    case 'G': hp=85; theme=RU_DEATH|RU_MIND|RU_SPACE;
        r->flags2|=RF2_INVISIBLE; if(!one_in_(10))r->flags2|=RF2_PASS_WALL;
        r->blow[0].effect=RBE_EXP_20; r->flagsr|=RFR_RES_COLD; break;
    case 'H': hp=130; damage=120; theme=ru_element(r,lev>=30,FALSE)|RU_WAR;
        if(one_in_(2))r->flags7|=RF7_CAN_FLY; break;
    case 'I': hp=80; speed=8; theme=RU_POIS|RU_KIN; r->flags7|=RF7_CAN_FLY;
        r->blow[0].effect=RBE_POISON; break;
    case 'J': theme=RU_POIS; r->blow[0].effect=RBE_POISON;
        r->flags2|=RF2_COLD_BLOOD; r->flags7|=RF7_CAN_SWIM; break;
    case 'K': hp=120; ac=40; theme=RU_ACID|RU_WAR;
        r->blow[0].effect=RBE_ACID; r->flagsr|=RFR_RES_ACID; break;
    case 'L': hp=90; theme=RU_DEATH|RU_ARCANE|RU_MIND|RU_SPACE;
        r->flags2|=RF2_SMART; r->flags5|=RF5_BO_NETH; r->freq_spell=35;
        r->blow[0].effect=RBE_DR_MANA; r->flagsr|=RFR_RES_COLD; break;
    case 'M': hp=150; damage=125; theme=RU_POIS|RU_FIRE|RU_WATER;
        r->flags2|=RF2_REGENERATE; r->flags7|=RF7_CAN_SWIM;
        r->blow[0].effect=RBE_POISON; if(lev>=40)r->flags4|=RF4_BR_POIS; break;
    case 'O':
        r->flags3|=RF3_EVIL; theme=RU_WAR|RU_KIN;
        if(lev<60){hp=120; damage=120; break;}
        /* Valerian ogres, not Gensokyo oni nor lowercase orcs. */
        hp=150; ac=20; damage=140; theme|=RU_DARK|RU_ARCANE; r->freq_spell=25;
        r->flags3|=RF3_GIANT|RF3_NO_CONF|RF3_NO_FEAR|RF3_HURT_LITE;
        r->flagsr|=RFR_RES_FIRE|RFR_RES_DARK|RFR_RES_NETH|RFR_RES_POIS|RFR_RES_STON;
        r->flags4|=RF4_BR_DARK; r->flags2|=RF2_BASH_DOOR|RF2_MOVE_BODY;
        for(j=0;j<4;j++){r->blow[j].method=RBM_CRUSH; r->blow[j].effect=j<2?RBE_HELL:RBE_SUPERHURT;}
        if(lev>=78)r->flags6|=RF6_S_KIN;
        if(lev>=87){r->flags3|=RF3_RES_TELE|RF3_NO_STUN; r->flags2|=RF2_SMART; r->flagsa|=RFA_BR_PURE_ELEM;}
        if(lev>=93)r->flags2|=RF2_POWERFUL;
        break;
    case 'P': hp=160; damage=150; ac=20; theme=RU_WAR;
        r->flags3|=RF3_GIANT; r->flags2|=RF2_MOVE_BODY;
        r->blow[1].effect=RBE_CONFUSE; if(lev>=60)r->blow[0].effect=RBE_SHATTER; break;
    case 'Q': hp=80; theme=RU_KIN|RU_SPACE; r->flags6|=RF6_S_MONSTER; r->freq_spell=35; break;
    case 'R': hp=110; ac=15; theme=RU_POIS|RU_FIRE; r->flags2|=RF2_COLD_BLOOD; break;
    case 'S': speed=5; theme=RU_POIS|RU_SPACE|RU_KIN;
        r->blow[0].effect=RBE_POISON; r->flagsr|=RFR_RES_POIS;
        if(lev>=25)r->flags6|=RF6_BLINK; break;
    case 'T': hp=160; damage=125; theme=RU_WAR|RU_KIN;
        r->flags3|=RF3_TROLL|RF3_EVIL; r->flags2|=RF2_REGENERATE; break;
    case 'U': hp=130; damage=120; ac=20; theme=RU_FIRE|RU_DARK|RU_DEATH|RU_ARCANE;
        r->blow[0].effect=RBE_HELL; r->flags2|=RF2_SMART; r->freq_spell=30; break;
    case 'V': theme=RU_DEATH|RU_DARK|RU_MIND|RU_SPACE;
        r->blow[3].effect=RBE_EXP_VAMP; r->flags2|=RF2_REGENERATE;
        r->flags3|=RF3_HURT_LITE; r->flagsr|=RFR_RES_DARK|RFR_RES_COLD; break;
    case 'W': theme=RU_DEATH|RU_DARK; r->blow[0].effect=RBE_EXP_40;
        r->flags2|=RF2_INVISIBLE; r->flagsr|=RFR_RES_COLD; break;
    case 'X': hp=130; ac=30; theme=RU_WAR|RU_MIND; r->flags2|=RF2_KILL_WALL;
        r->blow[0].effect=RBE_CONFUSE; break;
    case 'Y': hp=130; damage=130; theme=RU_COLD|RU_WAR;
        r->flagsr|=RFR_RES_COLD; r->blow[0].effect=RBE_COLD; break;
    case 'Z': speed=5; theme=ru_element(r,TRUE,FALSE)|RU_KIN; r->sleep=0; break;
    case '#': hp=140; ac=35; theme=RU_WAR|RU_POIS; r->flags2|=RF2_PASS_WALL;
        r->blow[0].effect=RBE_PARALYZE; break;
    case '&': ac=25; theme=RU_MIND|RU_SPACE; r->blow[0].effect=RBE_PARALYZE; break;
    case '*': hp=80; theme=ru_element(r,TRUE,TRUE)|RU_ARCANE;
        r->flags7|=RF7_CAN_FLY; break;
    }
    if(r->flags2&RF2_EMPTY_MIND)r->flags3|=RF3_NO_FEAR|RF3_NO_CONF;
    else if(lev>25 && !one_in_(7))r->flags3|=RF3_NO_CONF;
    r->hside=MAX(1,MIN(65535L,(long)r->hside*hp/100));
    r->ac=MAX(0,MIN(500,r->ac+ac));
    r->speed=MAX(100,MIN(180,r->speed+speed));
    for(j=0;j<4;j++)r->blow[j].d_side=MAX(1,MIN(255,r->blow[j].d_side*damage/100));
    return theme;
}

static u32b ru_profile_themes(const random_unique_profile *p)
{
    u32b themes=0;
    size_t i;
    /* Existing profile magic participates in the same ecology. */
    for(i=0;i<RU_SKILL_COUNT;i++) {
        const ru_skill *s=&ru_skills[i];
        u32b f=s->group==4?p->flags4:s->group==5?p->flags5:s->group==6?p->flags6:0;
        if(f&s->flag)themes|=s->themes;
    }
    return themes;
}

static int ru_skill_weight(const monster_race *r,const ru_skill *s,u32b themes)
{
    int minlev=s->min_level, maxlev=s->max_level?s->max_level:127, weight=s->weight;
    if(!(themes&s->themes))return 0;
    if(strchr(s->suitable,r->d_char)){weight*=3;minlev-=10;}
    if(strchr(s->unsuitable,r->d_char)){weight=MAX(1,weight/3);minlev+=10;maxlev+=10;}
    minlev=MAX(1,minlev);
    if(r->level<minlev)weight=weight*MAX(0,r->level-minlev/2)/MAX(1,minlev-minlev/2);
    if(r->level>maxlev)weight=weight*MAX(0,2*maxlev-r->level)/MAX(1,r->level);
    return weight;
}

enum { RU_GROW_HP, RU_GROW_DAMAGE, RU_GROW_AC, RU_GROW_SPEED,
       RU_GROW_SPELL, RU_GROW_SENSE, RU_GROW_COUNT };

/* These are independent weights, not a replacement classification for the
 * 113 titles. A tough, fast caster can favor several growth axes at once. */
static void ru_growth_weights(const monster_race *r,const random_unique_profile *p,int *w)
{
    char c=r->d_char;
    w[RU_GROW_HP]=90;w[RU_GROW_DAMAGE]=95;w[RU_GROW_AC]=45;
    w[RU_GROW_SPEED]=18;w[RU_GROW_SPELL]=15;w[RU_GROW_SENSE]=12;
    if(strchr("gqzDMOPTXY#",c)){w[RU_GROW_HP]+=60;w[RU_GROW_DAMAGE]+=50;}
    if(strchr("gKOPX#",c))w[RU_GROW_AC]+=40;
    if(strchr("bfBFCISZ",c)){w[RU_GROW_SPEED]+=35;w[RU_GROW_HP]-=20;}
    if(strchr("eALQU",c)){w[RU_GROW_SPELL]+=65;w[RU_GROW_DAMAGE]-=35;}
    if(strchr("phty",c) && (r->flags2&RF2_SMART)){w[RU_GROW_SPELL]+=55;w[RU_GROW_DAMAGE]-=20;}
    if(p->hp_percent>100)w[RU_GROW_HP]+=p->hp_percent-100;
    if(p->damage_percent>100)w[RU_GROW_DAMAGE]+=2*(p->damage_percent-100);
    if(p->ac_bonus>0)w[RU_GROW_AC]+=2*p->ac_bonus;
    if(p->speed_bonus>0)w[RU_GROW_SPEED]+=5*p->speed_bonus;
    if(p->flags4|p->flags5|p->flags6)w[RU_GROW_SPELL]+=30;
    if(p->flags7&RF7_LONG_RANGE)w[RU_GROW_SENSE]+=45;
}

static bool ru_has_magic(const monster_race *r,const random_unique_profile *p)
{
    return (r->flags4|r->flags5|r->flags6|r->flagsa|p->flags4|p->flags5|p->flags6)!=0;
}

static void ru_grow_stat(monster_race *r,const random_unique_profile *p,int kind)
{
    int j,lev=r->level,inc,cap;
    switch(kind) {
    case RU_GROW_HP:
        inc=1+lev/30;
        if(r->hdice>=150)inc=MAX(1,inc/4);
        else if(r->hdice>=100)inc=MAX(1,inc/2);
        r->hdice=MIN(200,r->hdice+inc);
        break;
    case RU_GROW_DAMAGE:
        cap=2+lev/6+(p->damage_percent>110?2:0);
        for(j=0;j<4;j++) {
            if(r->blow[j].d_dice<cap && one_in_(3))r->blow[j].d_dice++;
            if(r->blow[j].d_side<5+lev/6 && one_in_(3))r->blow[j].d_side++;
        }
        break;
    case RU_GROW_AC:
        inc=randint1(5)+lev/10;
        if(r->ac>300)inc=MAX(1,inc/4);
        else if(r->ac>150)inc=MAX(1,inc/2);
        r->ac=MIN(500,r->ac+inc);
        break;
    case RU_GROW_SPEED:
        cap=110+lev/4+MAX(0,p->speed_bonus);
        if(r->speed<cap || one_in_(3)){inc=randint1(2)+lev/40;r->speed=MIN(170,r->speed+inc);}
        break;
    case RU_GROW_SPELL:
        if(ru_has_magic(r,p))r->freq_spell=MIN(50,MAX(r->freq_spell,p->spell_frequency)+5);
        break;
    case RU_GROW_SENSE:
        r->aaf=MIN(100,r->aaf+10);r->sleep=MAX(0,r->sleep-5);
        break;
    }
}

/* Every power point participates in ONE weighted growth pool. There is no
 * fixed skill-count ceiling and no second full-power stat growth pass.
 * Advanced skills cost more power; learned skills leave the pool. */
static void random_unique_symbol_skills(monster_race *r,int power,const random_unique_profile *p)
{
    u32b themes=random_unique_symbol_traits(r)|ru_profile_themes(p);
    int remaining=MAX(0,MIN(1000,power)),learned=0,base[RU_GROW_COUNT];
    ru_growth_weights(r,p,base);
    while(remaining>0) {
        size_t i;
        int j,total=0,weights[RU_SKILL_COUNT],stats[RU_GROW_COUNT],pick;
        for(j=0;j<RU_GROW_COUNT;j++) {
            stats[j]=base[j];
            if(j==RU_GROW_HP && r->hdice>=200)stats[j]=0;
            if(j==RU_GROW_AC && r->ac>=500)stats[j]=0;
            if(j==RU_GROW_SPEED && r->speed>=170)stats[j]=0;
            if(j==RU_GROW_SENSE && r->aaf>=100)stats[j]=0;
            if(j==RU_GROW_SPELL && (!ru_has_magic(r,p) || r->freq_spell>=50))stats[j]=0;
            total+=stats[j];
        }
        for(i=0;i<RU_SKILL_COUNT;i++) {
            const ru_skill *s=&ru_skills[i];
            u32b owned=*ru_flag_group(r,s->group)|ru_profile_flag(p,s->group);
            weights[i]=(owned&s->flag)==s->flag?0:ru_skill_weight(r,s,themes);
            /* A broad spell list has diminishing appeal, never a hard cap. */
            weights[i]=weights[i]*100/(100+learned*15);
            if(remaining<1+s->min_level/40)weights[i]=0;
            if(s->group==2 && s->flag==RF2_SMART && ((r->flags2|p->flags2)&RF2_EMPTY_MIND))weights[i]=0;
            total+=weights[i];
        }
        if(!total)break;
        pick=randint0(total);
        for(j=0;j<RU_GROW_COUNT;j++)if((pick-=stats[j])<0)break;
        if(j<RU_GROW_COUNT){ru_grow_stat(r,p,j);remaining--;continue;}
        for(i=0;i<RU_SKILL_COUNT;i++)if((pick-=weights[i])<0){
            *ru_flag_group(r,ru_skills[i].group)|=ru_skills[i].flag;
            remaining-=1+ru_skills[i].min_level/40;learned++;break;
        }
    }
}

/* Offensive elements should have matching defenses, but an explicit epithet
 * weakness wins. This runs after both the symbol and title are applied. */
static void random_unique_resistances(monster_race *r,const random_unique_profile *p)
{
    int j;
    if(r->flags4&RF4_BR_ACID || r->flags5&(RF5_BO_ACID|RF5_BA_ACID))r->flagsr|=RFR_RES_ACID;
    if(r->flags4&RF4_BR_ELEC || r->flags5&(RF5_BO_ELEC|RF5_BA_ELEC) || r->flags2&RF2_AURA_ELEC)r->flagsr|=RFR_RES_ELEC;
    if(r->flags4&RF4_BR_FIRE || r->flags5&(RF5_BO_FIRE|RF5_BA_FIRE) || r->flags2&RF2_AURA_FIRE)r->flagsr|=RFR_RES_FIRE;
    if(r->flags4&RF4_BR_COLD || r->flags5&(RF5_BO_COLD|RF5_BA_COLD|RF5_BO_ICEE) || r->flags3&RF3_AURA_COLD)r->flagsr|=RFR_RES_COLD;
    if(r->flags4&RF4_BR_POIS || r->flags5&RF5_BA_POIS)r->flagsr|=RFR_RES_POIS;
    if(r->flags4&RF4_BR_DARK || r->flags5&RF5_BA_DARK)r->flagsr|=RFR_RES_DARK;
    if(r->flags4&RF4_BR_NETH || r->flags5&(RF5_BO_NETH|RF5_BA_NETH))r->flagsr|=RFR_RES_NETH;
    if(r->flags4&RF4_BR_LITE || r->flags5&RF5_BA_LITE)r->flagsr|=RFR_RES_LITE;
    if(r->flags5&(RF5_BO_WATE|RF5_BA_WATE))r->flagsr|=RFR_RES_WATE;
    if(r->flags4&RF4_BR_SOUN)r->flagsr|=RFR_RES_SOUN;
    if(r->flags4&RF4_BR_SHAR)r->flagsr|=RFR_RES_SHAR;
    if(r->flags4&RF4_BR_WALL)r->flagsr|=RFR_RES_WALL;
    if(r->flags4&RF4_BR_CHAO)r->flagsr|=RFR_RES_CHAO;
    if(r->flags4&RF4_BR_TIME)r->flagsr|=RFR_RES_TIME;
    if(r->flags4&RF4_BR_GRAV)r->flagsr|=RFR_RES_GRAV;
    if(r->flags4&RF4_BR_INER)r->flagsr|=RFR_RES_INER;
    if(r->flags4&RF4_BR_CONF)r->flagsr|=RFR_RES_CONF;
    if(r->flags4&RF4_BR_DISE)r->flagsr|=RFR_RES_DISE;
    if(r->flags4&RF4_BR_STON)r->flagsr|=RFR_RES_STON;
    if(r->flags4&RF4_BR_PLAS || r->flags5&RF5_BO_PLAS)r->flagsr|=RFR_RES_PLAS;
    /* Inherited light vulnerability yields to an explicit light-resist title. */
    if(p->flagsr&RFR_RES_LITE)r->flags3&=~RF3_HURT_LITE;
    if(r->flags3&RF3_HURT_LITE)r->flagsr&=~RFR_RES_LITE;
    if(p->flags3&RF3_HURT_FIRE){r->flagsr&=~RFR_RES_FIRE;r->flags2&=~RF2_AURA_FIRE;}
    if(r->flags2&RF2_KILL_BODY)r->flags2&=~RF2_MOVE_BODY;
    if(r->flags2&RF2_KILL_WALL)r->flags2&=~RF2_PASS_WALL;
    if(r->flags2&RF2_SMART && !(p->flags2&RF2_EMPTY_MIND))r->flags2&=~RF2_EMPTY_MIND;
    if(r->flags2&RF2_EMPTY_MIND)r->flags2&=~RF2_WEIRD_MIND;
    /* Powerful non-AC damage needs restraint; preserve deliberately specified
     * title attacks, while reducing only inherited life/stat-drain blows. */
    for(j=0;j<4;j++)if(!p->blows[j].method &&
        (r->blow[j].effect==RBE_EXP_20 || r->blow[j].effect==RBE_EXP_40 || r->blow[j].effect==RBE_DR_MANA))
        r->blow[j].d_side=MAX(1,r->blow[j].d_side*2/3);
}
