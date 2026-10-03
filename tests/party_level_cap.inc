static void party_level_cap_regressions(void)
{
    int i, c, limit, step, terminal_count = 0;
    s32b race_xp, class_xp;
    p_ptr->is_dead = 0; p_ptr->playing = TRUE; p_ptr->chp = 100;
    astral_mode = FALSE; character_generated = FALSE; character_dungeon = FALSE;
    p_ptr->pclass = CLASS_SOLDIER; p_ptr->lev = p_ptr->max_plv = 1;
    p_ptr->expfact = 100; p_ptr->exp = p_ptr->max_exp = 0;
    memset(p_ptr->cexp_info, 0, sizeof(p_ptr->cexp_info));
    p_ptr->cexp_info[CLASS_SOLDIER].clev = p_ptr->cexp_info[CLASS_SOLDIER].max_clev = 1;
    p_ptr->cexpfact[CLASS_SOLDIER] = 100;
    party_rebind(); party_reset(); party_capture(); party_count = 3;
    for (i = 1; i < 3; i++) {
        party_members[i] = party_members[0];
        party_members[i].player.lev = party_members[i].player.max_plv = 35;
        party_members[i].player.cexp_info[CLASS_SOLDIER].clev = 35;
        party_members[i].player.cexp_info[CLASS_SOLDIER].max_clev = 35;
    }
    quest[QUEST_DOLGARUA].status = quest[QUEST_FILARHH].status = QUEST_STATUS_TAKEN;
    r_info[MON_DOLGARUA].r_pkills = r_info[MON_FILARHH].r_pkills = 0;
    assert(party_level_limit() == 35 && !party_exp_level_limit());
    for (c = 0; c < max_c_idx; c++) {
        if (!(class_info[c].c_flags & PCF_NO_CHANGE)) continue;
        terminal_count++;
        assert(party_can_enter_class(c));
        party_members[1].player.max_plv = 36;
        assert(!party_can_enter_class(c));
        party_members[1].dead = TRUE;
        assert(party_can_enter_class(c));
        party_members[1].dead = FALSE; party_members[1].player.max_plv = 35;
        party_members[1].player.cexp_info[CLASS_KNIGHT].max_clev = 36;
        assert(!party_can_enter_class(c));
        party_members[1].player.cexp_info[CLASS_KNIGHT].max_clev = 0;
    }
    assert(terminal_count == 20);
    /* Both the normal gate and death/transformation gate reject over-limit allies. */
    party_members[1].player.max_plv = 36;
    assert(!can_choose_class(CLASS_LICH, CLASS_CHOOSE_MODE_DEATH));
    assert(!can_choose_class(CLASS_ANGELKNIGHT, CLASS_CHOOSE_MODE_DEATH));
    party_members[1].player.max_plv = 35;
    party_members[1].player.pclass = CLASS_LICH;
    assert(party_exp_level_limit() == 35);
    party_members[1].dead = TRUE; assert(!party_exp_level_limit());
    party_members[1].dead = FALSE;
    auto_more = TRUE; keys = "                                        "; key_index = 0;
    Rand_quick = TRUE; Rand_value = 12345;
    for (step = 0; step < 3; step++) {
        if (step == 1) quest[QUEST_DOLGARUA].status = QUEST_STATUS_FINISHED;
        if (step == 2) quest[QUEST_FILARHH].status = QUEST_STATUS_FINISHED;
        limit = step == 0 ? 35 : step == 1 ? 45 : 50;
        assert(party_level_limit() == limit);
        /* Historical maxima skip irrelevant stat-choice prompts during large awards. */
        p_ptr->max_plv = 50; p_ptr->cexp_info[CLASS_SOLDIER].max_clev = 50;
        p_ptr->update = p_ptr->redraw = p_ptr->window = 0;
        msg_flag = FALSE; gain_racial_exp(PY_MAX_EXP); gain_class_exp(PY_MAX_EXP);
        assert(p_ptr->lev == limit && p_ptr->cexp_info[CLASS_SOLDIER].clev == limit);
        assert(p_ptr->exp == player_exp[limit - 2]);
        assert(p_ptr->cexp_info[CLASS_SOLDIER].cexp == player_exp[limit - 2]);
        race_xp = p_ptr->exp; class_xp = p_ptr->cexp_info[CLASS_SOLDIER].cexp;
        gain_exp(PY_MAX_EXP);
        assert(p_ptr->exp == race_xp && p_ptr->cexp_info[CLASS_SOLDIER].cexp == class_xp);
    }
    quest[QUEST_DOLGARUA].status = quest[QUEST_FILARHH].status = QUEST_STATUS_TAKEN;
    /* Race at cap must not stop a lower current class from gaining. */
    p_ptr->lev = p_ptr->max_plv = 35; p_ptr->exp = player_exp[33];
    p_ptr->cexp_info[CLASS_SOLDIER].clev = 1;
    p_ptr->cexp_info[CLASS_SOLDIER].cexp = 0;
    gain_exp(10);
    assert(p_ptr->exp == player_exp[33] && p_ptr->cexp_info[CLASS_SOLDIER].cexp == 10);
    /* Resurrection must validate other terminal owners and the returning owner. */
    p_ptr->max_plv = p_ptr->lev = 35;
    p_ptr->cexp_info[CLASS_SOLDIER].max_clev = 35;
    party_members[2].dead = TRUE;
    party_members[2].player.max_plv = 36;
    assert(!party_can_revive(2) && !party_revive_member(2));
    party_members[2].player.max_plv = 35;
    assert(party_can_revive(2));
    party_members[1].dead = TRUE; party_members[2].dead = FALSE;
    party_members[2].player.max_plv = 36;
    assert(!party_can_revive(1));
    quest[QUEST_DOLGARUA].status = QUEST_STATUS_FINISHED;
    assert(party_can_revive(1));
    /* Count includes the controlled actor and all living reserves. */
    party_capture(); party_count = 4;
    for (i = 1; i < 4; i++) {
        party_members[i] = party_members[0];
        party_members[i].dead = FALSE;
        party_members[i].player.pclass = CLASS_LICH;
    }
    quest[QUEST_DOLGARUA].status = quest[QUEST_FILARHH].status = QUEST_STATUS_TAKEN;
    party_members[2].dead = party_members[3].dead = TRUE;
    assert(!party_can_enter_class(CLASS_GENERAL));
    assert(!party_can_revive(2));
    party_members[1].dead = TRUE;
    assert(party_can_enter_class(CLASS_GENERAL) && party_can_revive(2));
    party_members[1].dead = FALSE;
    quest[QUEST_DOLGARUA].status = QUEST_STATUS_FINISHED;
    assert(party_can_enter_class(CLASS_GENERAL) && party_can_revive(2));
    party_members[2].dead = FALSE;
    assert(!party_can_enter_class(CLASS_GENERAL) && !party_can_revive(3));
    quest[QUEST_FILARHH].status = QUEST_STATUS_FINISHED;
    assert(party_can_enter_class(CLASS_GENERAL) && party_can_revive(3));
    party_members[3].dead = FALSE;
    assert(!party_can_enter_class(CLASS_GENERAL));
    party_members[3].dead = TRUE;
    p_ptr->pclass = CLASS_GENERAL; /* Live sheet overrides the old Soldier snapshot. */
    assert(!party_can_revive(3));
    assert(party_can_enter_class(CLASS_LICH)); /* Replacement is not an extra member. */
    party_members[3].player.pclass = CLASS_SOLDIER;
    assert(party_can_revive(3));
    puts("Advanced-class slots: 1/2/3, active actor, dead exclusion, replacement and resurrection passed");
    puts("20 terminal classes: entry/death gates, historical classes, independent 35/45/50 XP limits, large awards, dead exclusion and revival restrictions passed");
}
