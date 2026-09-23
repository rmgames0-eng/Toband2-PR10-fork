/* Multiple protagonists: shared world, private character sheets/equipment. */
#include "angband.h"
#include "party.h"

party_member party_members[MAX_PARTY_MEMBERS];
byte party_count = 0, party_active = 0, party_rewards = 0;
bool party_creating = FALSE;

void party_rebind(void)
{
    sp_ptr = &sex_info[p_ptr->psex];
    rp_ptr = &race_info[p_ptr->prace];
    cp_ptr = &class_info[p_ptr->pclass];
    mp_ptr = &m_info[p_ptr->pclass];
    p_ptr->s_ptr = &s_info[p_ptr->pclass];
    mutant_regenerate_mod = calc_mutant_regenerate_mod();
}

static void party_store(party_member *member)
{
    member->player = *p_ptr;
    memcpy(member->equipment, &inventory[INVEN_RARM], sizeof(member->equipment));
    strcpy(member->name, player_name);
    member->weapon_weight = mw_old_weight;
    member->weapon_melee = mw_diff_to_melee;
}

void party_reset(void)
{
    memset(party_members, 0, sizeof(party_members));
    party_count = party_active = party_rewards = 0;
    party_creating = FALSE;
}

void party_capture(void)
{
    if (party_creating) return;
    if (!party_count) { party_count = 1; party_active = 0; }
    party_store(&party_members[party_active]);
}

static void party_restore(const party_member *member)
{
#define PARTY_FIELD(type, name, count) memcpy(&p_ptr->name, &member->player.name, sizeof(p_ptr->name));
#include "party-fields.h"
#undef PARTY_FIELD
    memcpy(p_ptr->history, member->player.history, sizeof(p_ptr->history));
    memcpy(p_ptr->cexp_info, member->player.cexp_info, sizeof(p_ptr->cexp_info));
    memcpy(&inventory[INVEN_RARM], member->equipment, sizeof(member->equipment));
    strcpy(player_name, member->name);
    mw_old_weight = member->weapon_weight;
    mw_diff_to_melee = member->weapon_melee;
    party_rebind();
}

/* Ordinary ailments, speed, food and temporary resistances remain shared.
 * Special powers are dispelled; wraith_form, invuln and multishadow survive. */
static void party_clear_effects(player_type *player)
{
    player->opposite_pelem = player->no_elem = 0;
    player->celem = player->pelem;
    player->protevil = player->hero = player->shero = 0;
    player->shield = player->blessed = 0;
    player->tim_invis = player->tim_infra = player->tim_esp = 0;
    player->magical_weapon = player->evil_weapon = 0;
    player->chargespell = player->magicdef = 0;
    player->tim_sh_fire = player->tim_sh_elec = player->tim_sh_cold = 0;
    player->tim_sh_holy = player->tim_sh_aura = player->tim_eyeeye = 0;
    player->tim_inc_blow = player->tim_dec_blow = player->zoshonel_protect = 0;
    player->earth_spike = player->wind_guard = player->tim_resurrection = 0;
    player->dustrobe = 0;
    player->special_attack = 0;
    player->singing = player->restart_singing = player->song_start = 0;
    player->action = ACTION_NONE;
}

/* Magical/evil weapon modifies the object itself as well as a timer. */
static void party_clear_weapon(object_type *weapon, s16b *weight, s16b *bonus)
{
    if (*weight) weapon->weight = *weight;
    if (*bonus) { weapon->to_h -= *bonus; weapon->to_d -= *bonus; }
    *weight = *bonus = 0;
}

static void party_recalculate(void)
{
    int i;
    bool old_xtra = character_xtra;
    u32b pending = p_ptr->update;
    bool old_icky = character_icky;
    equip_cnt = inven_cnt = 0;
    p_ptr->total_weight = 0;
    for (i = 0; i < INVEN_TOTAL; ++i)
    {
        if (!inventory[i].k_idx) continue;
        p_ptr->total_weight += inventory[i].number * inventory[i].weight;
        if (i >= INVEN_RARM) ++equip_cnt; else ++inven_cnt;
    }
    character_xtra = TRUE;
    p_ptr->update = PU_BONUS | PU_HP | PU_MANA | PU_TORCH;
    /* Only character calculations, no view/monster processing during preview. */
    character_icky = TRUE;
    update_stuff();
    character_icky = old_icky;
    character_xtra = old_xtra;
    p_ptr->update |= pending;
}

static s32b party_fraction(s32b current, s32b maximum, s32b target, bool hp)
{
    u64b value = maximum > 0 && target > 0 ? (u64b)MAX(0, current) * target / maximum : 0;
    if (value > target) value = target;
    if (hp && value < 1) value = 1;
    return (s32b)value;
}

bool party_switch(int member)
{
    player_type before;
    int old_inven, old_equip;
    bool old_monk = monk_armour_aux, old_notify = monk_notify_aux;
    bool legal;
    if (astral_mode || party_creating || member < 0 || member >= party_count || member == party_active)
        return FALSE;
    if (p_ptr->is_dead || p_ptr->chp < 0 || p_ptr->paralyzed || stun_level(p_ptr->stun) >= 4)
        return FALSE;
    party_capture();
    before = *p_ptr;
    old_inven = inven_cnt; old_equip = equip_cnt;
    party_restore(&party_members[member]);
    party_clear_effects(p_ptr);
    party_clear_weapon(&inventory[INVEN_RARM], &mw_old_weight, &mw_diff_to_melee);
    party_recalculate();
    legal = p_ptr->wild_mode || player_can_enter(cave[py][px].feat);
    if (!p_ptr->wild_mode && (cave[py][px].feat == FEAT_AIR || cave[py][px].feat == FEAT_DARK_PIT))
        legal = legal && p_ptr->levitation;
    if (!p_ptr->wild_mode && cave[py][px].feat == FEAT_DEEP_WATER)
        legal = legal && (p_ptr->levitation || p_ptr->can_swim);
    /* All classes can ride in this game; retain the same eligible mount. */
    if (p_ptr->riding)
        legal = legal && (r_info[m_list[p_ptr->riding].r_idx].flags7 & RF7_RIDING);
    if (!legal)
    {
        party_restore(&party_members[party_active]);
        *p_ptr = before;
        party_rebind();
        inven_cnt = old_inven; equip_cnt = old_equip;
        monk_armour_aux = old_monk; monk_notify_aux = old_notify;
        msg_print("この場所ではその仲間に交代できません。");
        return FALSE;
    }
    /* Commit removal on the outgoing equipment only after validation. */
    party_clear_effects(&party_members[party_active].player);
    party_clear_weapon(&party_members[party_active].equipment[0],
        &party_members[party_active].weapon_weight, &party_members[party_active].weapon_melee);
    p_ptr->chp = party_fraction(before.chp, before.mhp, p_ptr->mhp, TRUE);
    p_ptr->csp = party_fraction(before.csp, before.msp, p_ptr->msp, FALSE);
    p_ptr->chp_frac = p_ptr->csp_frac = 0;
    /* Cancel effects outside player_type only after the switch has succeeded. */
    if (stop_the_time_player && p_ptr->energy_need < 0) p_ptr->energy_need = 0;
    stop_the_time_player = FALSE;
    fool_effect_status &= ~FOOL_STATUS_PLAYER;
    resting = running = command_rep = 0;
    repeat_clear();
    party_active = (byte)member;
    energy_use = 100;
    p_ptr->update |= PU_BONUS | PU_HP | PU_MANA | PU_TORCH | PU_UN_VIEW | PU_UN_LITE | PU_VIEW | PU_LITE | PU_MONSTERS;
    p_ptr->redraw |= PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY;
    p_ptr->window |= PW_INVEN | PW_EQUIP | PW_PLAYER | PW_SPELL;
    msg_format("%sに交代した。", player_name);
    return TRUE;
}

void do_cmd_party(void)
{
    int i;
    char ch, buf[160];
    energy_use = 0;
    if (astral_mode || party_count < 2) return;
    party_capture();
    screen_save();
    Term_clear();
    prt("交代する仲間を選んでください（ESCで中断）", 0, 0);
    for (i = 0; i < party_count; ++i)
    {
        party_member *m = &party_members[i];
        strnfmt(buf, sizeof(buf), "%c) %s%s %s / %s Lv%ld  HP %ld/%ld MP %ld/%ld",
            I2A(i), i == party_active ? "*" : "", m->name,
            p_name + race_info[m->player.prace].name, c_name + class_info[m->player.pclass].name,
            (long)m->player.lev, (long)m->player.chp, (long)m->player.mhp,
            (long)m->player.csp, (long)m->player.msp);
        prt(buf, i + 2, 0);
    }
    while (get_com("誰に交代しますか？ ", &ch, FALSE))
    {
        i = A2I(ch);
        if (i >= 0 && i < party_count) { screen_load(); (void)party_switch(i); return; }
    }
    screen_load();
}

static bool party_quest_complete(int quest_id)
{
    int status = quest[quest_id].status;
    return status == QUEST_STATUS_COMPLETED || status == QUEST_STATUS_REWARDED || status == QUEST_STATUS_FINISHED;
}

/* Return an earned, unclaimed slot without changing the roster. */
static int party_recruit_reward(void)
{
    int reward;
    if (astral_mode || party_creating || p_ptr->is_dead || p_ptr->chp < 0 ||
        party_count >= MAX_PARTY_MEMBERS) return -1;
    for (reward = 0; reward < 2; ++reward)
    {
        bool earned = reward == 0 ? party_quest_complete(QUEST_ARMORICA) :
            (party_quest_complete(QUEST_BARMAMUTHA_L) || party_quest_complete(QUEST_BARMAMUTHA_C));
        if (earned && !(party_rewards & (1 << reward))) return reward;
    }
    return -1;
}

bool party_can_recruit(void)
{
    return party_recruit_reward() >= 0;
}

/* Called only by the explicit home recruitment command. One member per choice. */
void do_cmd_party_recruit(void)
{
    int reward = party_recruit_reward();
    if (reward < 0) return;
    party_capture();
    msg_print("あなたの名声を聞きつけ、ヴァレリア島の戦士が仲間になりたがっています。");
    msg_print(NULL);
    party_birth_member();
    party_rewards |= (1 << reward);
}
