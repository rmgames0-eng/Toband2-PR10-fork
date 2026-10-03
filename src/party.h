/* Party members own only the fields enumerated in party-fields.h and equipment. */
#ifndef INCLUDED_PARTY_H
#define INCLUDED_PARTY_H
#define PARTY_ROLE_MELEE 0
#define PARTY_ROLE_SHOOT 1
#define PARTY_ROLE_MAGIC 2
extern int party_class_role(int pclass);
extern int party_cooperation_count(int role);
extern int party_magic_cost(int cost);
extern int party_support_damage(int (*average)(int), int *count);
extern bool party_followup_attacks(int y, int x, bool (*attack)(int, int));
#define MAX_PARTY_MEMBERS 16
#define PARTY_POWER 1000
#define PARTY_SAVE_VERSION 7
#define PARTY_RECRUIT_SEALED 0x04
#define TEMPLE_COMMAND_POWER 1004
#define TEMPLE_EFFECT_COUNT 18
#define TEMPLE_KNOCKBACK (1UL << (TEMPLE_EFFECT_COUNT - 1))
extern bool temple_command_eligible(void);
extern void temple_command_learn(void);
extern void temple_command_describe(u32b effects, char *buf, int len);
extern bool temple_command_name_valid(cptr name);
extern void temple_command_flags(u32b effects, u32b *flags);
extern bool temple_command_attack(int y, int x);
extern void temple_command_power(void);
#define PARTY_ELEMENTAL_BRANDS (ATTACK_ACID | ATTACK_ELEC | ATTACK_FIRE | ATTACK_COLD | ATTACK_POIS)
#define PARTY_WEAPON_BRANDS (PARTY_ELEMENTAL_BRANDS | ATTACK_EVIL)

typedef struct party_member {
    byte dead;
    byte revived; /* Permanent: no tarot on subsequent deaths. */
    player_type player;
    object_type equipment[INVEN_TOTAL - INVEN_RARM];
    char name[32];
    s16b weapon_weight, weapon_melee;
} party_member;
extern party_member party_members[MAX_PARTY_MEMBERS];
extern byte party_count, party_active, party_rewards;
extern bool party_creating;
extern bool party_training;
extern bool party_casting;
extern int party_book_caster(const object_type *book);
extern void party_cast_book(int member, int item, void (*cast)(int));
extern bool party_train_member(int member);
extern void do_cmd_party_train(void);
extern bool party_has_successor(void);
extern bool party_has_dead_member(void);
extern int party_level_limit(void);
extern int party_exp_level_limit(void);
extern bool party_can_enter_class(int new_class);
extern bool party_can_dark_contract(void);
extern bool party_dark_sacrifice(void);
extern bool party_can_revive(int member);
extern bool party_revive_member(int member);
extern bool party_resurrect(void);
extern bool party_martyrize(void);
extern bool party_necromancy(void);
extern bool party_reincarnate(void);
extern bool party_has_reincarnation_target(void);
extern bool party_handle_death(void);
extern void party_regenerate(void);
extern bool party_reserves_recovered(void);
extern void party_timeout_weapons(void);
extern void party_process_reserves(void);
extern int party_spell_target_begin(void);
extern void party_spell_target_end(int caster);
extern const player_type *party_spell_caster(void);
extern void party_migrate_weapons(void);
extern void party_reset(void);
extern void party_capture(void);
extern bool party_can_recruit(void);
extern void do_cmd_party_recruit(void);
extern bool party_switch(int member);
extern void do_cmd_party(void);
extern void do_cmd_party_next(void);
extern void party_birth_member(void);
extern void party_rebind(void);
extern void party_identify_reserve_equipment(void);
#endif
