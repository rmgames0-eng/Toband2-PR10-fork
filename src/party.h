/* Party members own only the fields enumerated in party-fields.h and equipment. */
#ifndef INCLUDED_PARTY_H
#define INCLUDED_PARTY_H
#define MAX_PARTY_MEMBERS 16
#define PARTY_POWER 1000
#define PARTY_SAVE_VERSION 1

typedef struct party_member {
    player_type player;
    object_type equipment[INVEN_TOTAL - INVEN_RARM];
    char name[32];
    s16b weapon_weight, weapon_melee;
} party_member;
extern party_member party_members[MAX_PARTY_MEMBERS];
extern byte party_count, party_active, party_rewards;
extern bool party_creating;
extern void party_reset(void);
extern void party_capture(void);
extern bool party_can_recruit(void);
extern void do_cmd_party_recruit(void);
extern bool party_switch(int member);
extern void do_cmd_party(void);
extern void party_birth_member(void);
extern void party_rebind(void);
extern void party_identify_reserve_equipment(void);
#endif
