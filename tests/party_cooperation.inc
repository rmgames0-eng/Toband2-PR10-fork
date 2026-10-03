extern bool party_attack_once(int y, int x);
extern int test_attack_attempts;
extern int player_average_hit(int hand);
static int cooperating_calls;
static bool check_cooperating_actor(int y, int x)
{
    assert(party_active == 1 || party_active == 2);
    assert(!strcmp(player_name, party_members[party_active].name));
    assert(inventory[INVEN_RARM].to_d == 10 * party_active);
    assert(!party_followup_attacks(y, x, check_cooperating_actor));
    p_ptr->csp--;
    energy_use = 777;
    cooperating_calls++;
    return FALSE;
}

static void cooperation_tests(void)
{
    int i, j, idx, hp, mp, base, damage;
    monster_type *m;
    object_type weapon;
    character_generated = character_dungeon = FALSE;
    astral_mode = FALSE; auto_more = TRUE;
    keys = "                                     "; key_index = 0;
    Rand_quick = TRUE; Rand_value = 12345;
    p_ptr->playing = TRUE; p_ptr->is_dead = 0; p_ptr->leaving = FALSE;
    p_ptr->inside_arena = FALSE; p_ptr->prace = RACE_HUMAN;
    p_ptr->pclass = CLASS_SOLDIER; p_ptr->psex = SEX_MALE;
    p_ptr->lev = p_ptr->max_plv = 1;
    p_ptr->race_hp[0] = 500; p_ptr->race_sp[0] = 100;
    p_ptr->chp = 100; p_ptr->csp = 30;
    for (i = 0; i < A_MAX; i++) p_ptr->stat_cur[i] = p_ptr->stat_max[i] = 100;
    p_ptr->cexp_info[CLASS_SOLDIER].clev = 1;
    memset(inventory, 0, sizeof(object_type) * INVEN_TOTAL);
    py = px = 5; cur_hgt = cur_wid = 20; dun_level = 1;
    for (i = 0; i < 20; i++) for (j = 0; j < 20; j++) {
        cave[i][j].feat = (i == 0 || j == 0 || i == 19 || j == 19) ? FEAT_PERM_SOLID : FEAT_FLOOR;
        cave[i][j].info = CAVE_GLOW | CAVE_MARK | CAVE_VIEW;
    }
    party_rebind(); p_ptr->update = PU_BONUS | PU_HP | PU_MANA; update_stuff();
    p_ptr->chp = 100; p_ptr->csp = 30;
    strcpy(player_name, "Active"); party_reset(); party_capture(); party_count = 3;
    object_prep(&weapon, lookup_kind(TV_SWORD, SV_DAGGER));
    weapon.dd = weapon.ds = 1; weapon.to_h = 100;
    for (i = 1; i < 3; i++) {
        party_members[i] = party_members[0];
        sprintf(party_members[i].name, "Reserve%d", i);
        weapon.to_d = i * 10;
        party_members[i].equipment[0] = weapon;
    }
    assert(party_cooperation_count(PARTY_ROLE_MELEE) == 2);
    party_members[1].dead = TRUE; assert(party_cooperation_count(PARTY_ROLE_MELEE) == 1);
    party_members[1].dead = FALSE;
    party_members[1].player.pclass = CLASS_KNIGHT; assert(party_cooperation_count(PARTY_ROLE_MELEE) == 2);
    /* Keep mixed classes for preview, real attacks, aura and death checks. */
    p_ptr->inside_arena = TRUE; assert(!party_cooperation_count(PARTY_ROLE_MELEE)); p_ptr->inside_arena = FALSE;

    idx = m_pop(); m = &m_list[idx];
    m->r_idx = m->ap_r_idx = 1; m->fy = 5; m->fx = 6; m->ml = TRUE;
    m->hp = m->maxhp = m->max_maxhp = 100000; cave[5][6].m_idx = idx;
    r_info[1].level = 52; r_info[1].ac = 0;
    r_info[1].flags1 = r_info[1].flags3 = r_info[1].flags7 = 0;
    r_info[1].flags2 = RF2_AURA_FIRE;
    /* The supporter determines the effect, including Soldier/Witch reversal. */
    {
        int count;
        party_members[1].player.pclass = CLASS_WITCH;
        party_members[2].dead = TRUE;
        assert(!party_cooperation_count(PARTY_ROLE_MELEE));
        assert(party_magic_cost(100)==90);
        assert(!party_support_damage(player_average_hit,&count) && !count);
        cooperating_calls=0;
        assert(!party_followup_attacks(5,6,check_cooperating_actor));
        assert(cooperating_calls==0);
        p_ptr->pclass=CLASS_WITCH;party_rebind();
        party_members[1].player.pclass=CLASS_SOLDIER;
        assert(party_cooperation_count(PARTY_ROLE_MELEE)==1);
        assert(party_magic_cost(100)==100);
        (void)party_support_damage(player_average_hit,&count);assert(count==1);
        assert(!party_followup_attacks(5,6,check_cooperating_actor));
        assert(cooperating_calls==1);
        party_members[1].player.csp=p_ptr->csp;
        p_ptr->pclass=CLASS_SOLDIER;party_rebind();
        party_members[1].player.pclass=CLASS_KNIGHT;
        party_members[2].dead=FALSE;cooperating_calls=0;
    }
    hp = p_ptr->chp; mp = p_ptr->csp; energy_use = 100;
    assert(!party_followup_attacks(5, 6, check_cooperating_actor));
    assert(cooperating_calls == 2 && energy_use == 100);
    assert(party_active == 0 && !strcmp(player_name, "Active"));
    assert(p_ptr->chp == hp && p_ptr->csp == mp);
    assert(party_members[1].player.csp == mp - 1 && party_members[2].player.csp == mp - 1);
    cooperating_calls = 0; party_members[1].player.paralyzed = 10;
    assert(!party_followup_attacks(5, 6, check_cooperating_actor));
    assert(cooperating_calls == 1); party_members[1].player.paralyzed = 0;


    {
        player_type saved = *p_ptr;
        party_member first = party_members[1], second = party_members[2];
        object_type items[INVEN_TOTAL];
        int total, count, increased, y, found_blows = 0, found_damage = 0;
        char row[81], expected[64];
        memcpy(items, inventory, sizeof(items));
        total = party_support_damage(player_average_hit, &count);
        assert(count == 2 && total >= 0);
        assert(!memcmp(&saved, p_ptr, sizeof(saved)));
        assert(!memcmp(&first, &party_members[1], sizeof(first)));
        assert(!memcmp(&second, &party_members[2], sizeof(second)));
        assert(!memcmp(items, inventory, sizeof(items)));
        assert(party_active == 0 && !strcmp(player_name, "Active"));
        party_members[1].equipment[0].ident = IDENT_KNOWN | IDENT_MENTAL;
        total = party_support_damage(player_average_hit, &count);
        party_members[1].equipment[0].to_d += 50;
        increased = party_support_damage(player_average_hit, &count);
        assert(increased == total + 5000);
        party_members[1] = first;
        p_ptr->migite = TRUE; p_ptr->hidarite = FALSE;
        p_ptr->num_blow[0] = 2; p_ptr->num_blow[1] = 0;
        total = party_support_damage(player_average_hit, &count);
        sprintf(expected, "%d+%d", 2 * player_average_hit(0) / 100, total / 100);
        display_player(0);
        for (y = 0; y < 24; y++) {
            memcpy(row, Term->scr->c[y], 80); row[80] = 0;
            if (strstr(row, "2 (+2援)")) found_blows++;
            if (strstr(row, expected)) found_damage++;
            assert(!strstr(row, "2+0 (+2援)"));
        }
        assert(found_blows == 1 && found_damage >= 1);
        *p_ptr = saved; party_rebind();
        puts("Combat display: zero left hand omitted, reserve one-hit damage added, preview state unchanged");
    }
    test_attack_attempts = 0; Rand_value = 4321;
    assert(!party_followup_attacks(5, 6, party_attack_once));
    assert(test_attack_attempts == 2);
    assert(m->hp < 100000);
    assert(p_ptr->chp == hp && p_ptr->csp == mp && energy_use == 100);
    assert(party_members[1].player.chp < hp && party_members[2].player.chp < hp);
    damage = 100000 - m->hp; assert(damage > 20);
    /* The normal attack command dispatches these blows once, not once per hand. */
    r_info[1].flags2 = 0;
    p_ptr->migite = TRUE; p_ptr->hidarite = FALSE; p_ptr->num_blow[0] = 1;
    p_ptr->update = 0; test_attack_attempts = 0; Rand_value = 4321;
    assert(!py_attack(5, 6, 0));
    assert(test_attack_attempts == 3 && party_active == 0 && energy_use == 100);
    r_info[1].flags2 = RF2_AURA_FIRE;
    /* Fire immunity belongs to the reserve, not the controlled character. */
    add_flag(party_members[1].equipment[0].art_flags, TR_IM_FIRE);
    party_members[1].player.chp = hp;
    Rand_value = 4321; party_followup_attacks(5, 6, party_attack_once);
    assert(party_members[1].player.chp == hp && p_ptr->chp == hp);
    remove_flag(party_members[1].equipment[0].art_flags, TR_IM_FIRE);
    party_members[1].player.chp = 0; party_members[1].revived = TRUE;
    Rand_value = 4321; party_followup_attacks(5, 6, party_attack_once);
    assert(party_members[1].dead && !party_members[1].equipment[0].k_idx);
    assert(party_active == 0 && !p_ptr->is_dead && !p_ptr->leaving && p_ptr->chp == hp);
    assert(!party_casting && energy_use == 100);
    puts("Cooperation melee: one blow per reserve, private stats/MP/aura/immunity/death, drops, no recursion or switch passed");

    /* Shooting and magic bonuses use reserve roles, even for a different actor role. */
    for (i = 1; i < 3; i++) { party_members[i].dead = FALSE; party_members[i].player.chp = 100; }
    p_ptr->pclass = CLASS_SOLDIER; party_rebind();
    for(i=1;i<3;i++) party_members[i].player.pclass=CLASS_ARCHER;
    object_prep(&inventory[INVEN_BOW], lookup_kind(TV_BOW, SV_LONG_BOW));
    for (i = 1; i < 3; i++) party_members[i].dead = TRUE;
    p_ptr->update = PU_BONUS; update_stuff(); base = p_ptr->num_fire;
    party_members[1].dead = FALSE;
    p_ptr->update = PU_BONUS; update_stuff(); assert(p_ptr->num_fire == base + 50);
    party_members[2].dead = FALSE;
    p_ptr->update = PU_BONUS; update_stuff(); assert(p_ptr->num_fire == base + 100);
    assert(party_magic_cost(100) == 100);
    p_ptr->pclass = CLASS_ARCHER; party_rebind();
    party_members[1].player.pclass = CLASS_WITCH;
    party_members[2].player.pclass = CLASS_WIZARD;
    assert(party_magic_cost(100) == 80 && party_magic_cost(1) == 1 && !party_magic_cost(0));
    party_members[2].dead = TRUE; assert(party_magic_cost(100) == 90);
    base = calc_use_mana(15, REALM_FIRE);
    party_members[1].dead = TRUE;
    assert(base == (calc_use_mana(15, REALM_FIRE) * 90 + 99) / 100);
    for (i = 1; i < MAX_PARTY_MEMBERS; i++) {
        party_members[i] = party_members[0]; party_members[i].dead = FALSE;
        party_members[i].player.chp = 100; party_members[i].player.pclass = CLASS_WIZARD;
    }
    party_count = MAX_PARTY_MEMBERS; assert(party_magic_cost(100) == 10);
    puts("Cooperation bonuses: living reserve roles, arena exclusion, shooting +0.5/member, mana/energy -10% and limits passed");
}
