static int waiting_messages;
void __real_msg_print(cptr msg);
void __wrap_msg_print(cptr msg)
{
    if (msg && *msg) waiting_messages++;
    __real_msg_print(msg);
}

static int support_realm, support_spell;
static void cast_support(int item)
{
    assert(party_active == 1);
    assert(do_spell(support_realm, support_spell, SPELL_CAST));
    assert(party_active == 1);
    p_ptr->csp -= 2;
    energy_use = 100;
}

static void private_condition_regressions(void)
{
    int i, j, hp, mp, reserve_mp, original;
    object_type weapon;
#define PARTY_EFFECT(type, name) p_ptr->name = 0;
#include "party-effects.h"
#undef PARTY_EFFECT
    character_generated = FALSE; character_dungeon = FALSE;
    p_ptr->playing = TRUE; p_ptr->is_dead = 0; p_ptr->leaving = FALSE;
    p_ptr->inside_arena = FALSE; astral_mode = FALSE;
    p_ptr->pclass = CLASS_SOLDIER; p_ptr->prace = RACE_HUMAN;
    p_ptr->paralyzed = p_ptr->stun = p_ptr->blind = p_ptr->confused = p_ptr->shero = 0;
    p_ptr->poisoned = p_ptr->cut = p_ptr->stoning = p_ptr->invuln = 0;
    p_ptr->riding = 0; p_ptr->lev = p_ptr->max_plv = 30;
    p_ptr->cexp_info[CLASS_SOLDIER].clev = 10;
    p_ptr->town_num = 0; dun_level = 1;
    for (i = 0; i < INVEN_TOTAL; i++) object_wipe(&inventory[i]);
    py = px = 5; cur_hgt = cur_wid = 20;
    for (i = 0; i < 20; i++) for (j = 0; j < 20; j++) {
        cave[i][j].feat = (i == 0 || j == 0 || i == 19 || j == 19) ? FEAT_PERM_SOLID : FEAT_FLOOR;
        cave[i][j].info = CAVE_GLOW | CAVE_MARK | CAVE_VIEW;
    }
    p_ptr->race_hp[0] = 500; p_ptr->race_sp[0] = 100;
    p_ptr->update = PU_BONUS | PU_HP | PU_MANA; update_stuff();
    p_ptr->chp = 20; p_ptr->csp = 7;
    party_rebind(); party_reset(); party_capture(); party_count = 3;
    for (i = 1; i < 3; i++) {
        party_members[i] = party_members[0];
        sprintf(party_members[i].name, "Reserve%d", i);
        party_members[i].player.pclass = CLASS_CLERIC;
        party_members[i].player.cexp_info[CLASS_CLERIC].clev = 30;
        party_members[i].player.magic_exp[REALM_HOLY] = 500;
        party_members[i].player.magic_exp[REALM_WIND] = 500;
        party_members[i].player.magic_exp[REALM_DEATH] = 500;
        party_members[i].player.chp = 100; party_members[i].player.csp = 50;
    }
    auto_more = TRUE; keys = "                         "; key_index = 0;
    Rand_quick = TRUE; Rand_value = 12345;
    /* Reserve cloak effects stay enabled without restarting each world tick. */
    {
        int k, tick;
        int egos[] = {EGO_OPPOSE_ACID,EGO_OPPOSE_ELEC,EGO_OPPOSE_FIRE,EGO_OPPOSE_COLD,EGO_NO_ELEM};
        object_type *cloak = &party_members[1].equipment[INVEN_OUTER-INVEN_RARM];
        for(k=0;k<5;k++) {
            object_prep(cloak, lookup_kind(TV_CLOAK, SV_CLOAK));
            cloak->name2=egos[k];
            party_process_reserves();
            waiting_messages=0;
            for(tick=0;tick<5;tick++) party_process_reserves();
            assert(waiting_messages==0);
            assert(party_active==0 && !p_ptr->oppose_fire && !p_ptr->no_elem);
            if(k==0) assert(party_members[1].player.oppose_acid==1);
            if(k==1) assert(party_members[1].player.oppose_elec==1);
            if(k==2) assert(party_members[1].player.oppose_fire==1);
            if(k==3) assert(party_members[1].player.oppose_cold==1);
            if(k==4) assert(party_members[1].player.no_elem==1);
            object_wipe(cloak);
            party_process_reserves();
            assert(!party_members[1].player.oppose_acid && !party_members[1].player.oppose_elec);
            assert(!party_members[1].player.oppose_fire && !party_members[1].player.oppose_cold);
            assert(!party_members[1].player.no_elem);
        }
        puts("Reserve cloaks: five maintained effects, no repeated activation, removal expires passed");
    }
    p_ptr->poisoned = 20; p_ptr->cut = 20; p_ptr->oppose_cold = 44;
    msg_flag = FALSE; assert(party_switch(1));
    assert(!p_ptr->poisoned && !p_ptr->cut && !p_ptr->oppose_cold);
    p_ptr->blind = 12; p_ptr->fast = 25;
    msg_flag = FALSE; assert(party_switch(0));
    assert(p_ptr->poisoned == 20 && p_ptr->cut == 20 && p_ptr->oppose_cold == 44);
    assert(!p_ptr->blind && !p_ptr->fast);
    support_realm = REALM_HOLY; support_spell = 12;
    mp = p_ptr->csp; reserve_mp = party_members[1].player.csp;
    msg_flag = FALSE; party_cast_book(1, 0, cast_support);
    assert(!p_ptr->poisoned && !p_ptr->cut && p_ptr->csp == mp);
    assert(party_members[1].player.blind == 12 && party_members[1].player.csp == reserve_mp - 2);
    support_spell = 3; hp = p_ptr->chp;
    msg_flag = FALSE; party_cast_book(1, 0, cast_support);
    assert(p_ptr->chp > hp && party_members[1].player.chp == 100 && energy_use == 200);
    support_realm = REALM_WIND; support_spell = 5;
    msg_flag = FALSE; party_cast_book(1, 0, cast_support);
    assert(p_ptr->fast > 0 && party_members[1].player.fast == 25);
    object_prep(&inventory[INVEN_RARM], lookup_kind(TV_SWORD, SV_DAGGER));
    weapon = inventory[INVEN_RARM];
    support_realm = REALM_DEATH; support_spell = 20;
    msg_flag = FALSE; party_cast_book(1, 0, cast_support);
    assert(p_ptr->evil_weapon > 0 && !party_members[1].player.evil_weapon);
    assert(inventory[INVEN_RARM].to_d == weapon.to_d + 15);
    assert(!party_members[1].equipment[0].k_idx);
    party_members[1].player.poisoned = 10; party_members[1].player.cut = 15;
    party_members[1].player.oppose_fire = 12;
    hp = p_ptr->chp; mp = p_ptr->csp; original = party_active;
    msg_flag = FALSE; party_process_reserves();
    assert(party_active == original && p_ptr->chp == hp && p_ptr->csp == mp);
    assert(party_members[1].player.chp < 100 && party_members[1].player.poisoned < 10);
    assert(party_members[1].player.blind < 12 && party_members[1].player.oppose_fire == 11);
    can_save = TRUE; assert(save_player()); party_reset(); assert(load_player());
#ifdef TEST_PARTY_V4
    assert(!party_members[1].player.poisoned && !party_members[1].player.oppose_fire);
#else
    assert(party_members[1].player.poisoned > 0 && party_members[1].player.oppose_fire == 11);
#endif
    assert(p_ptr->oppose_cold == 44);
    Rand_quick = TRUE; Rand_value = 12345;
    party_members[1].player.poisoned = 10;
    party_members[1].player.chp = 0;
    party_members[1].revived = FALSE;
    object_prep(&party_members[1].equipment[0], lookup_kind(TV_SWORD, SV_DAGGER));
    keys = "aaaaaaaaaaaaaaaaaaaaaaaaaa"; key_index = 0; msg_flag = FALSE;
    hp = p_ptr->chp; mp = p_ptr->csp;
    party_process_reserves();
    assert(party_active == 0 && !p_ptr->is_dead && p_ptr->chp == hp && p_ptr->csp == mp);
    assert(party_members[1].dead && !party_members[1].equipment[0].k_idx);
    assert(key_index >= 1); /* Tarot selected for the reserve, no replacement selection. */
    assert(!party_switch(1));
    msg_flag = FALSE; assert(party_revive_member(1));
    assert(!party_members[1].dead && !party_members[1].player.poisoned);
    assert(!party_members[1].player.blind && !party_members[1].player.oppose_fire);
    puts("Private conditions: switching, active-target healing/buffs/weapon, caster MP/power, ticking, damage, reserve death, revival and saves passed");
}

