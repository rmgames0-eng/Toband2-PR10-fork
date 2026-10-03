/* Multiple protagonists: shared world, private character sheets/equipment. */
#include "angband.h"
#include "party.h"

party_member party_members[MAX_PARTY_MEMBERS];
byte party_count = 0, party_active = 0, party_rewards = 0;
bool party_creating = FALSE;
bool party_training = FALSE;
bool party_casting = FALSE;
static int party_cast_original = -1;
static int party_effect_caster = -1;
static bool party_waiting = FALSE;
static byte party_target_death = 0;

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
    if (!p_ptr->is_dead && p_ptr->chp >= 0) member->dead = FALSE;
    if (p_ptr->resurrection_cnt) member->revived = TRUE;
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
#define PARTY_EFFECT(type, name) p_ptr->name = member->player.name;
#include "party-effects.h"
#undef PARTY_EFFECT
    strcpy(p_ptr->temple_tech_name, member->player.temple_tech_name);
    p_ptr->temple_tech_effects = member->player.temple_tech_effects;
    memcpy(p_ptr->history, member->player.history, sizeof(p_ptr->history));
    memcpy(p_ptr->cexp_info, member->player.cexp_info, sizeof(p_ptr->cexp_info));
    memcpy(&inventory[INVEN_RARM], member->equipment, sizeof(member->equipment));
    strcpy(player_name, member->name);
    mw_old_weight = member->weapon_weight;
    mw_diff_to_melee = member->weapon_melee;
    p_ptr->magical_weapon = member->player.magical_weapon;
    p_ptr->evil_weapon = member->player.evil_weapon;
    p_ptr->special_attack = (p_ptr->special_attack & ~PARTY_WEAPON_BRANDS) |
        (member->player.special_attack & PARTY_WEAPON_BRANDS);
    party_rebind();
}

static void party_recalculate(void);

/* Each living reserve supplies the bonus for its own class role. */
int party_class_role(int pclass)
{
    switch (pclass)
    {
    case CLASS_ARCHER: case CLASS_GUNNER: case CLASS_CRESCENT:
    case CLASS_MEDIUM: case CLASS_RANGER: return PARTY_ROLE_SHOOT;
    case CLASS_WIZARD: case CLASS_WARLOCK: case CLASS_EXORCIST:
    case CLASS_WITCH: case CLASS_SIRENE: case CLASS_CLERIC: case CLASS_PRIEST:
    case CLASS_LICH: case CLASS_HIGHWITCH: case CLASS_ARCHMAGE:
    case CLASS_SUCCUBUS: case CLASS_ELEMENTALER:
    case CLASS_DARK_ELEMENT: return PARTY_ROLE_MAGIC;
    default: return PARTY_ROLE_MELEE;
    }
}

int party_cooperation_count(int role)
{
    int i, count = 0;
    if (astral_mode || party_creating || party_training || p_ptr->inside_arena ||
        p_ptr->is_dead || p_ptr->chp < 0) return 0;
    for (i = 0; i < party_count; i++)
        if (i != party_active && !party_members[i].dead &&
            party_members[i].player.chp >= 0 &&
            party_class_role(party_members[i].player.pclass) == role) count++;
    return count;
}

int party_magic_cost(int cost)
{
    int percent;
    if (cost <= 0) return cost;
    percent = MAX(10, 100 - 10 * party_cooperation_count(PARTY_ROLE_MAGIC));
    return MAX(1, (cost * percent + 99) / 100);
}

/* Read-only combat previews: never clamp or otherwise change stored members. */
int party_support_damage(int (*average)(int), int *count)
{
    player_type *original = p_ptr, preview;
    object_type *old_inventory = inventory, equipment[INVEN_TOTAL];
    int active = party_active, i, damage = 0;
    int old_inven = inven_cnt, old_equip = equip_cnt;
    s16b old_weight = mw_old_weight, old_melee = mw_diff_to_melee;
    bool old_monk = monk_armour_aux, old_notify = monk_notify_aux;
    char name[32];
    *count = 0;
    if (party_casting || !party_cooperation_count(PARTY_ROLE_MELEE)) return 0;
    strcpy(name, player_name);
    for (i = 0; i < party_count; i++)
    {
        player_type *reserve = &party_members[i].player;
        if (i == active || party_members[i].dead || reserve->chp < 0 || party_class_role(reserve->pclass) != PARTY_ROLE_MELEE ||
            reserve->paralyzed || reserve->confused || reserve->afraid || reserve->stoning >= 250) continue;
        preview = *original;
        memcpy(equipment, old_inventory, sizeof(equipment));
        p_ptr = &preview; inventory = equipment;
        party_restore(&party_members[i]); party_active = i;
        p_ptr->action = ACTION_NONE;
        p_ptr->singing = p_ptr->restart_singing = p_ptr->song_start = 0;
        p_ptr->special_attack &= PARTY_WEAPON_BRANDS;
        party_recalculate();
        if (p_ptr->migite || p_ptr->hidarite)
        {
            damage += average(p_ptr->migite ? 0 : 1);
            (*count)++;
        }
    }
    p_ptr = original; inventory = old_inventory; party_active = active;
    strcpy(player_name, name);
    mw_old_weight = old_weight; mw_diff_to_melee = old_melee;
    inven_cnt = old_inven; equip_cnt = old_equip;
    monk_armour_aux = old_monk; monk_notify_aux = old_notify;
    party_rebind();
    return damage;
}

/* Each reserve lends exactly one attack, with its own complete combat sheet.
 * Do not enter the public attack command (prompts, action costs or recursion). */
bool party_followup_attacks(int y, int x, bool (*attack)(int, int))
{
    int original = party_active, offset, member, target, old_energy = energy_use;
    int old_penet = penet_ac;
    bool killed = FALSE, old_save = can_save, old_autosave = autosave_l;
    bool old_monk = monk_armour_aux, old_notify = monk_notify_aux;
    player_type before;
    u32b realms[MAX_CLASS];
    int j;
    if (party_casting || party_waiting || p_ptr->leaving ||
        !party_cooperation_count(PARTY_ROLE_MELEE)) return FALSE;
    target = cave[y][x].m_idx;
    if (!target) return FALSE;
    party_capture(); before = *p_ptr;
    for (j = 0; j < max_c_idx; j++) realms[j] = class_info[j].realm_choices;
    can_save = autosave_l = FALSE;
    party_casting = party_waiting = TRUE;
    for (offset = 1; offset < party_count; offset++)
    {
        player_type *reserve;
        member = (original + offset) % party_count;
        reserve = &party_members[member].player;
        if (party_members[member].dead || reserve->chp < 0 || party_class_role(reserve->pclass) != PARTY_ROLE_MELEE ||
            reserve->paralyzed || reserve->confused || reserve->afraid || reserve->stoning >= 250) continue;
        if (cave[y][x].m_idx != target || !m_list[target].r_idx) break;
        party_restore(&party_members[member]); party_active = member;
        p_ptr->action = ACTION_NONE;
        p_ptr->singing = p_ptr->restart_singing = p_ptr->song_start = 0;
        p_ptr->special_attack &= PARTY_WEAPON_BRANDS;
        penet_ac = 0;
        init_realm_table(); party_recalculate();
        {
            char enemy_name[80];
            monster_desc(enemy_name, &m_list[target], 0);
            msg_format("%sが%sを攻撃した。", player_name, enemy_name);
        }
        killed = attack(y, x);
        if (p_ptr->is_dead) (void)party_handle_death();
        else party_capture();
        p_ptr->is_dead = before.is_dead;
        p_ptr->leaving = before.leaving;
        if (killed) break;
    }
    party_restore(&party_members[original]); party_active = original;
    p_ptr->action = before.action;
    p_ptr->singing = before.singing; p_ptr->restart_singing = before.restart_singing;
    p_ptr->song_start = before.song_start; p_ptr->special_attack = before.special_attack;
    p_ptr->is_dead = before.is_dead; p_ptr->leaving = before.leaving;
    for (j = 0; j < max_c_idx; j++) class_info[j].realm_choices = realms[j];
    init_realm_table(); party_recalculate();
    party_casting = party_waiting = FALSE;
    can_save = old_save; autosave_l = old_autosave;
    monk_armour_aux = old_monk; monk_notify_aux = old_notify;
    energy_use = old_energy; penet_ac = old_penet;
    p_ptr->redraw |= PR_BASIC | PR_EXTRA | PR_EQUIPPY;
    p_ptr->window |= PW_PLAYER | PW_INVEN | PW_EQUIP | PW_SPELL;
    return killed;
}

/* Clear special actions/powers when restoring a dead member to life. */
static void party_clear_effects(player_type *player)
{
    player->opposite_pelem = player->no_elem = 0;
    player->celem = player->pelem;
    player->protevil = player->hero = player->shero = 0;
    player->shield = player->blessed = 0;
    /* Temporary infravision is a shared timed effect. */
    player->tim_invis = player->tim_esp = 0;
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

/* Old saves kept the timers only on the active character. Clear orphaned
 * reserve modifications whose original duration cannot be recovered. */
void party_migrate_weapons(void)
{
    int i;
    for (i = 0; i < party_count; i++)
    {
        party_member *m = &party_members[i];
        if (i == party_active)
        {
            m->player.magical_weapon = p_ptr->magical_weapon;
            m->player.evil_weapon = p_ptr->evil_weapon;
            m->player.special_attack = p_ptr->special_attack & PARTY_WEAPON_BRANDS;
            continue;
        }
        party_clear_weapon(&m->equipment[0], &m->weapon_weight, &m->weapon_melee);
        m->player.magical_weapon = m->player.evil_weapon = 0;
        m->player.special_attack &= ~PARTY_WEAPON_BRANDS;
    }
}

/* Tick at the same point as the active member's weapon timers. No character
 * switch or effect messages are needed to expire a reserve's own equipment. */
void party_timeout_weapons(void)
{
    int i;
    if (astral_mode || party_creating || party_training || party_casting) return;
    for (i = 0; i < party_count; i++)
    {
        party_member *m = &party_members[i];
        player_type *p = &m->player;
        if (i == party_active || m->dead || p->chp < 0) continue;
        if (p->magical_weapon > 0 && !--p->magical_weapon)
        {
            p->special_attack &= ~PARTY_ELEMENTAL_BRANDS;
            if (m->weapon_weight)
            {
                m->equipment[0].weight = m->weapon_weight;
                m->weapon_weight = 0;
            }
        }
        if (p->evil_weapon > 0 && !--p->evil_weapon)
            p->special_attack &= ~ATTACK_EVIL;
        if (!p->magical_weapon && !p->evil_weapon)
            party_clear_weapon(&m->equipment[0], &m->weapon_weight, &m->weapon_melee);
    }
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

/* Voluntary switches require dismounting; reject before touching party state. */
static bool party_switch_blocked_by_riding(void)
{
    if (!p_ptr->riding) return FALSE;
    energy_use = 0;
#ifdef JP
    msg_print("騎乗中は仲間と交代できません。");
#else
    msg_print("You cannot switch companions while riding.");
#endif
    return TRUE;
}

static bool party_switch_aux(int member, bool death)
{
    player_type before;
    int old_inven, old_equip, c, landing_y = py, landing_x = px;
    u32b old_realms[MAX_CLASS];
    bool old_monk = monk_armour_aux, old_notify = monk_notify_aux;
    bool legal;
    if (p_ptr->inside_arena || astral_mode || party_creating || member < 0 || member >= party_count || member == party_active)
        return FALSE;
    if (!death && party_switch_blocked_by_riding()) return FALSE;
    if (party_members[member].dead || party_members[member].player.chp < 0) return FALSE;
    if (!death && (p_ptr->is_dead || p_ptr->chp < 0 || p_ptr->paralyzed || stun_level(p_ptr->stun) >= 4))
        return FALSE;
    party_capture();
    before = *p_ptr;
    for (c = 0; c < max_c_idx; ++c) old_realms[c] = class_info[c].realm_choices;
    old_inven = inven_cnt; old_equip = equip_cnt;
    party_restore(&party_members[member]);
    if (death)
    {
        p_ptr->is_dead = 0;
        if (p_ptr->riding)
        {
            p_ptr->riding = 0;
            p_ptr->pet_extra_flags &= ~PF_RYOUTE;
            p_ptr->riding_ryoute = p_ptr->old_riding_ryoute = FALSE;
        }
    }
    p_ptr->singing = p_ptr->restart_singing = p_ptr->song_start = 0;
    p_ptr->action = ACTION_NONE;
    init_realm_table();
    party_recalculate();
    legal = p_ptr->wild_mode || player_can_enter(cave[py][px].feat);
    if (!p_ptr->wild_mode && (cave[py][px].feat == FEAT_AIR || cave[py][px].feat == FEAT_DARK_PIT))
        legal = legal && p_ptr->levitation;
    if (!p_ptr->wild_mode && cave[py][px].feat == FEAT_DEEP_WATER)
        legal = legal && (p_ptr->levitation || p_ptr->can_swim);
    /* Leave the pet on the death square. Find a place the successor can
     * occupy on foot, including when all adjacent squares are blocked. */
    if (death && before.riding)
    {
        int y, x, best = 0x7fffffff;
        legal = FALSE;
        for (y = 1; y < cur_hgt - 1; y++) for (x = 1; x < cur_wid - 1; x++)
        {
            int feat = cave[y][x].feat, dist;
            if (!cave_empty_bold(y, x) || !player_can_enter(feat)) continue;
            if ((feat == FEAT_AIR || feat == FEAT_DARK_PIT) && !p_ptr->levitation) continue;
            if (feat == FEAT_DEEP_WATER && !p_ptr->levitation && !p_ptr->can_swim) continue;
            dist = distance(py, px, y, x);
            if (dist < best) { best = dist; landing_y = y; landing_x = x; legal = TRUE; }
        }
    }
    if (!legal && (!death || before.riding))
    {
        party_restore(&party_members[party_active]);
        *p_ptr = before;
        for (c = 0; c < max_c_idx; ++c) class_info[c].realm_choices = old_realms[c];
        party_rebind();
        inven_cnt = old_inven; equip_cnt = old_equip;
        monk_armour_aux = old_monk; monk_notify_aux = old_notify;
        msg_print("この場所ではその仲間に交代できません。");
        return FALSE;
    }
    if (death && before.riding)
    {
        int oy = py, ox = px;
        py = landing_y; px = landing_x;
        set_mermaid_in_water();
        lite_spot(oy, ox); lite_spot(py, px);
        verify_panel();
        p_ptr->update |= PU_FLOW | PU_DISTANCE | PU_MON_LITE;
        p_ptr->redraw |= PR_UHEALTH;
    }
    /* End effects on other creatures only after the switch is accepted. */
    if (before.singing == MUSIC_SILENT) song_of_silence(0);
    party_members[party_active].player.singing = 0;
    party_members[party_active].player.action = ACTION_NONE;
    /* Cancel effects outside player_type only after the switch has succeeded. */
    if (stop_the_time_player && p_ptr->energy_need < 0) p_ptr->energy_need = 0;
    stop_the_time_player = FALSE;
    fool_effect_status &= ~FOOL_STATUS_PLAYER;
    resting = running = command_rep = 0;
    repeat_clear();
    party_active = (byte)member;
    energy_use = death ? 0 : 100;
    if (death) { p_ptr->leaving = FALSE; strcpy(p_ptr->died_from, "(alive and well)"); }
    p_ptr->update |= PU_BONUS | PU_HP | PU_MANA | PU_TORCH | PU_UN_VIEW | PU_UN_LITE | PU_VIEW | PU_LITE | PU_MONSTERS;
    p_ptr->redraw |= PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY;
    p_ptr->window |= PW_INVEN | PW_EQUIP | PW_PLAYER | PW_SPELL;
    msg_format("%sと交代した。", player_name);
    return TRUE;
}

bool party_switch(int member)
{
    return party_switch_aux(member, FALSE);
}

/* Use the active member first, then cycle through the living reserves.
 * Realm restrictions depend on the member's class and current element. */
int party_book_caster(const object_type *book)
{
    int offset, member, count = MAX(1, party_count), result = -1;
    player_type *original = p_ptr, preview;
    player_class *old_class = cp_ptr;
    if (book->tval < TV_MAGERY_BOOK || book->tval >= TV_MAGERY_BOOK + MAX_REALM) return -1;
    for (offset = 0; offset < count; offset++)
    {
        u32b realms;
        member = (party_active + offset) % count;
        if (offset)
        {
            if (astral_mode || original->inside_arena || party_creating || party_casting) break;
            if (party_members[member].dead || party_members[member].player.chp < 0) continue;
            preview = *original;
#define PARTY_FIELD(type, name, count) memcpy(&preview.name, &party_members[member].player.name, sizeof(preview.name));
#include "party-fields.h"
#undef PARTY_FIELD
#define PARTY_EFFECT(type, name) preview.name = party_members[member].player.name;
#include "party-effects.h"
#undef PARTY_EFFECT
            p_ptr = &preview;
        }
        cp_ptr = &class_info[p_ptr->pclass];
        realms = cp_ptr->realm_choices;
        init_realm_table();
        if (cp_ptr->realm_choices && check_book_realm(book->tval, book->sval)) result = member;
        cp_ptr->realm_choices = realms;
        p_ptr = original;
        if (result >= 0) break;
    }
    cp_ptr = old_class;
    return result;
}

/* Borrow the caster's private sheet and equipment only for this command.
 * Successful self spells temporarily target the controlled member; costs,
 * failure consequences and growth remain with the caster. */
void party_cast_book(int member, int item, void (*cast)(int))
{
    int original = party_active, i, spent;
    bool old_save = can_save, old_autosave = autosave_l;
    bool old_monk = monk_armour_aux, old_notify = monk_notify_aux;
    u32b realms[MAX_CLASS];
    if (member == original) { cast(item); return; }
    if (party_casting || astral_mode || p_ptr->inside_arena || party_creating ||
        p_ptr->is_dead || member < 0 || member >= party_count ||
        party_members[member].dead || party_members[member].player.chp < 0) return;
    party_capture();
    for (i = 0; i < max_c_idx; i++) realms[i] = class_info[i].realm_choices;
    party_target_death = 0;
    party_cast_original = original;
    party_casting = TRUE;
    can_save = autosave_l = FALSE;
    party_restore(&party_members[member]);
    party_active = member;
    init_realm_table();
    party_recalculate();
    cast(item);
    spent = energy_use;
    if (p_ptr->is_dead) (void)party_handle_death();
    else party_capture();
    if (party_active != original)
    {
        party_restore(&party_members[original]);
        party_active = original;
    }
    for (i = 0; i < max_c_idx; i++) class_info[i].realm_choices = realms[i];
    init_realm_table();
    party_recalculate();
    p_ptr->update |= PU_BONUS | PU_HP | PU_MANA | PU_TORCH | PU_UN_VIEW | PU_UN_LITE | PU_VIEW | PU_LITE | PU_MONSTERS;
    p_ptr->redraw |= PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY;
    p_ptr->window |= PW_INVEN | PW_EQUIP | PW_PLAYER | PW_SPELL;
    monk_armour_aux = old_monk; monk_notify_aux = old_notify;
    can_save = old_save; autosave_l = old_autosave;
    /* Reserve casting takes twice the caster's normal action time. */
    energy_use = spent * 2;
    if (party_target_death) { p_ptr->is_dead = party_target_death; p_ptr->leaving = TRUE; }
    party_target_death = 0;
    party_casting = FALSE;
    party_cast_original = -1;
}

/* Scope only successful self-targeted spell effects, never mana costs or failure. */
const player_type *party_spell_caster(void)
{
    return party_effect_caster >= 0 ? &party_members[party_effect_caster].player : p_ptr;
}

int party_spell_target_begin(void)
{
    int caster;
    if (!party_casting || party_cast_original < 0 || party_effect_caster >= 0) return -1;
    caster = party_active;
    party_capture();
    party_effect_caster = caster;
    party_restore(&party_members[party_cast_original]);
    party_active = party_cast_original;
    init_realm_table();
    party_recalculate();
    return caster;
}

void party_spell_target_end(int caster)
{
    if (caster < 0) return;
    party_capture();
    party_target_death = p_ptr->is_dead;
    p_ptr->is_dead = 0;
    if (party_target_death) p_ptr->leaving = FALSE;
    party_restore(&party_members[caster]);
    party_active = caster;
    party_effect_caster = -1;
    init_realm_table();
    party_recalculate();
}

/* Terminal classes restrict all other living members, including when the
 * terminal-class owner is waiting. Stored active sheets may be stale. */
int party_level_limit(void)
{
    if (quest[QUEST_FILARHH].status == QUEST_STATUS_FINISHED || r_info[MON_FILARHH].r_pkills > 0) return 50;
    if (quest[QUEST_DOLGARUA].status == QUEST_STATUS_FINISHED || r_info[MON_DOLGARUA].r_pkills > 0) return 45;
    return 35;
}

static const player_type *party_sheet(int member)
{
    return member == party_active ? p_ptr : &party_members[member].player;
}

static bool party_living(int member)
{
    const player_type *p = party_sheet(member);
    return member == party_active ? !p->is_dead && p->chp >= 0 :
        !party_members[member].dead && !p->is_dead && p->chp >= 0;
}

static bool party_above_limit(const player_type *p, int limit)
{
    int c;
    if (MAX(p->lev, p->max_plv) > limit) return TRUE;
    for (c = 0; c < max_c_idx; c++)
        if (MAX(p->cexp_info[c].clev, p->cexp_info[c].max_clev) > limit) return TRUE;
    return FALSE;
}

/* Terminal classes normally use party limits; these two knights are exempt. */
static bool party_class_is_advanced(int pclass)
{
    if (pclass == CLASS_TEMPLEKNIGHT || pclass == CLASS_WHITEKNIGHT) return FALSE;
    return (class_info[pclass].c_flags & PCF_NO_CHANGE) != 0;
}

int party_exp_level_limit(void)
{
    int i;
    if (astral_mode || party_creating || party_training) return 0;
    for (i = 0; i < party_count; i++)
        if (i != party_active && party_living(i) &&
            party_class_is_advanced(party_sheet(i)->pclass))
            return party_level_limit();
    return 0;
}

/* The same milestones unlock one, two, then three living terminal classes. */
static int party_advanced_limit(void)
{
    int level = party_level_limit();
    return level >= 50 ? 3 : level >= 45 ? 2 : 1;
}

bool party_can_dark_contract(void)
{
    int i, living = 0;
    if (astral_mode || party_creating || party_training || party_casting ||
        p_ptr->is_dead || p_ptr->chp < 0 || p_ptr->inside_arena ||
        dungeon_type != DUNGEON_DEMON || !dun_level ||
        cave[py][px].feat != FEAT_PURGATORY_ALTAR ||
        pclass_is_(CLASS_DARK_ELEMENT)) return FALSE;
    /* The current game has exactly two recruitable companions. */
    if (party_count != 3) return FALSE;
    for (i = 0; i < party_count; i++)
        if (i != party_active && party_living(i)) living++;
    return living == 2;
}

bool party_can_enter_class(int new_class)
{
    int i, limit, advanced = 1;
    if (astral_mode || party_creating || !party_class_is_advanced(new_class)) return TRUE;
    limit = party_level_limit();
    for (i = 0; i < party_count; i++)
    {
        if (i == party_active || !party_living(i)) continue;
        if (party_above_limit(party_sheet(i), limit)) return FALSE;
        if (party_class_is_advanced(party_sheet(i)->pclass)) advanced++;
    }
    return advanced <= party_advanced_limit();
}

/* Validate the entire hypothetical living roster, not just the dead target.
 * A returning terminal-class owner must not bypass the entry restriction. */
bool party_can_revive(int member)
{
    int i, j, advanced = 0, limit = party_level_limit();
    if (astral_mode || member < 0 || member >= party_count || member == party_active ||
        !party_members[member].dead) return FALSE;
    for (i = 0; i < party_count; i++)
    {
        if (i != member && !party_living(i)) continue;
        if (!party_class_is_advanced(party_sheet(i)->pclass)) continue;
        if (++advanced > party_advanced_limit()) return FALSE;
        for (j = 0; j < party_count; j++)
            if (j != i && (j == member || party_living(j)) && party_above_limit(party_sheet(j), limit)) return FALSE;
    }
    return TRUE;
}

bool party_has_successor(void)
{
    int i;
    if (astral_mode) return FALSE;
    for (i = 0; i < party_count; i++)
        if (i != party_active && !party_members[i].dead && party_members[i].player.chp >= 0)
            return TRUE;
    return FALSE;
}

/* Recompute personal flags instead of relying on derived fields, which are
 * not serialized for reserves. This read-only preview never switches actors. */
static void party_regen_rates(const party_member *member, int *hp, int *mp)
{
    player_type preview = member->player, *original = p_ptr;
    player_race *old_race = rp_ptr;
    player_class *old_class = cp_ptr;
    object_type equipment[INVEN_TOTAL], *old_inventory = inventory;
    u32b flags[TR_FLAG_SIZE];
    bool regen, mana;
    int i;

    memset(equipment, 0, sizeof(equipment));
    memcpy(&equipment[INVEN_RARM], member->equipment, sizeof(member->equipment));
    /* Timed powers belong to this reserve as well. */
    p_ptr = &preview;
    rp_ptr = &race_info[preview.prace];
    cp_ptr = &class_info[preview.pclass];
    inventory = equipment;
    player_flags(flags);
    regen = have_flag(flags, TR_REGEN) != 0;
    mana = have_flag(flags, TR_REGEN_MANA) != 0;
    for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
    {
        if (!equipment[i].k_idx) continue;
        object_flags(&equipment[i], flags);
        if (have_flag(flags, TR_REGEN)) regen = TRUE;
        if (have_flag(flags, TR_REGEN_MANA)) mana = TRUE;
    }
    inventory = old_inventory;
    p_ptr = original;
    rp_ptr = old_race;
    cp_ptr = old_class;
    *hp = PY_REGEN_NORMAL * (regen ? 2 : 1);
    *mp = *hp * (mana ? 2 : 1);
}

/* Use the normal natural-regeneration base rate, once per world tick.
 * A reserve does not borrow the active character's rest or equipment bonuses. */
static void party_regen_value(s32b *current, s32b maximum, u32b *fraction, int base, int rate)
{
    u64b value;
    if (maximum <= 0 || *current >= maximum) { *current = MAX(0, maximum); *fraction = 0; return; }
    value = (u64b)MAX(0, *current) * 65536 + (*fraction & 65535) +
        (u64b)maximum * rate + base;
    if (value >= (u64b)maximum * 65536) { *current = maximum; *fraction = 0; }
    else { *current = value / 65536; *fraction = value & 65535; }
}

bool party_reserves_recovered(void)
{
    int i;
    if (astral_mode) return TRUE;
    for (i = 0; i < party_count; i++)
    {
        const party_member *m = &party_members[i];
        const player_type *p = &m->player;
        if (i == party_active || m->dead || p->chp < 0) continue;
        if (p->chp < p->mhp || p->csp < p->msp || p->poisoned || p->cut ||
            p->blind || p->confused || p->stun || p->afraid || p->paralyzed || p->stoning) return FALSE;
    }
    return TRUE;
}

void party_regenerate(void)
{
    int i;
    if (astral_mode || party_creating || party_training || p_ptr->is_dead) return;
    for (i = 0; i < party_count; i++)
    {
        player_type *p = &party_members[i].player;
        int hp_rate, mp_rate;
        if (i == party_active || party_members[i].dead || p->chp < 0) continue;
        party_regen_rates(&party_members[i], &hp_rate, &mp_rate);
        if (!p->poisoned && !p->cut)
            party_regen_value(&p->chp, p->mhp, &p->chp_frac, PY_REGEN_HPBASE, hp_rate);
        party_regen_value(&p->csp, p->msp, &p->csp_frac, PY_REGEN_MNBASE, mp_rate);
    }
}

/* Reserves experience ailments at the same world-tick cadence as the actor.
 * Borrow their sheet to recompute CON and handle death drops, then restore the
 * actor exactly. Do not run active-only movement, recall or environmental harm. */
void party_process_reserves(void)
{
    player_type before;
    int original, i, j, adjust, damage, level, cloak_ego, old_energy = energy_use;
    bool old_save = can_save, old_monk = monk_armour_aux, old_notify = monk_notify_aux;
    u32b realms[MAX_CLASS];
    if (astral_mode || party_creating || party_training || party_casting || p_ptr->is_dead) return;
    if (party_count < 2) return;
    party_capture();
    before = *p_ptr; original = party_active;
    for (j = 0; j < max_c_idx; j++) realms[j] = class_info[j].realm_choices;
    can_save = FALSE; party_waiting = TRUE;
    for (i = 0; i < party_count; i++)
    {
        if (i == original || party_members[i].dead || party_members[i].player.chp < 0) continue;
        party_restore(&party_members[i]); party_active = i;
        init_realm_table(); party_recalculate();
        damage = 0;
        if (!p_ptr->invuln)
        {
            static const int cut_damage[] = {0, 1, 3, 7, 16, 32, 80, 200};
            if (p_ptr->poisoned) damage++;
            if (p_ptr->cut)
            {
                level = cut_level(p_ptr->cut);
                damage += cut_damage[MIN(7, MAX(0, level))];
            }
        }
        p_ptr->chp -= damage;
        if (p_ptr->chp < 0) p_ptr->is_dead = DEATH_DEAD;
        if (p_ptr->stoning)
        {
            if (p_ptr->stoning >= 250) p_ptr->is_dead |= DEATH_STONED;
            else p_ptr->stoning++;
        }
        if (p_ptr->is_dead)
        {
            msg_format("控えの%sが%sで死亡した。", player_name,
                (p_ptr->is_dead & DEATH_STONED) ? "石化" : p_ptr->poisoned ? "毒・出血" : "出血");
            (void)party_handle_death();
        }
        else
        {
            /* Equipment-maintained effects must not expire and restart in calc_bonuses. */
            cloak_ego = inventory[INVEN_OUTER].k_idx ? inventory[INVEN_OUTER].name2 : 0;
            adjust = adj_con_fix[p_ptr->stat_ind[A_CON]] + 1;
            p_ptr->poisoned = MAX(0, p_ptr->poisoned - adjust);
            p_ptr->stun = MAX(0, p_ptr->stun - adjust);
            if (cut_level(p_ptr->cut) < 7) p_ptr->cut = MAX(0, p_ptr->cut - adjust);
            if (p_ptr->fast) p_ptr->fast = MAX(0, p_ptr->fast - 1);
            if (p_ptr->slow) p_ptr->slow = MAX(0, p_ptr->slow - (easy_band ? 2 : 1));
            if (p_ptr->blind) p_ptr->blind = MAX(0, p_ptr->blind - (easy_band ? 2 : 1));
            if (p_ptr->paralyzed) p_ptr->paralyzed = MAX(0, p_ptr->paralyzed - (easy_band ? 2 : 1));
            if (p_ptr->confused) p_ptr->confused = MAX(0, p_ptr->confused - (easy_band ? 2 : 1));
            if (p_ptr->afraid) p_ptr->afraid = MAX(0, p_ptr->afraid - (easy_band ? 2 : 1));
            if (p_ptr->image) p_ptr->image = MAX(0, p_ptr->image - (easy_band ? 2 : 1));
            if (p_ptr->opposite_pelem) p_ptr->opposite_pelem = MAX(0, p_ptr->opposite_pelem - 1);
            if (p_ptr->no_elem) p_ptr->no_elem = MAX(cloak_ego == EGO_NO_ELEM ? 1 : 0, p_ptr->no_elem - 1);
            if (p_ptr->protevil) p_ptr->protevil = MAX(0, p_ptr->protevil - 1);
            if (p_ptr->invuln) p_ptr->invuln = MAX(0, p_ptr->invuln - 1);
            if (p_ptr->hero) p_ptr->hero = MAX(0, p_ptr->hero - 1);
            if (p_ptr->shero) p_ptr->shero = MAX(0, p_ptr->shero - 1);
            if (p_ptr->shield) p_ptr->shield = MAX(0, p_ptr->shield - 1);
            if (p_ptr->blessed) p_ptr->blessed = MAX(0, p_ptr->blessed - 1);
            if (p_ptr->tim_invis) p_ptr->tim_invis = MAX(0, p_ptr->tim_invis - 1);
            if (p_ptr->tim_infra) p_ptr->tim_infra = MAX(0, p_ptr->tim_infra - 1);
            if (p_ptr->oppose_acid) p_ptr->oppose_acid = MAX(cloak_ego == EGO_OPPOSE_ACID ? 1 : 0, p_ptr->oppose_acid - 1);
            if (p_ptr->oppose_elec) p_ptr->oppose_elec = MAX(cloak_ego == EGO_OPPOSE_ELEC ? 1 : 0, p_ptr->oppose_elec - 1);
            if (p_ptr->oppose_fire) p_ptr->oppose_fire = MAX(cloak_ego == EGO_OPPOSE_FIRE ? 1 : 0, p_ptr->oppose_fire - 1);
            if (p_ptr->oppose_cold) p_ptr->oppose_cold = MAX(cloak_ego == EGO_OPPOSE_COLD ? 1 : 0, p_ptr->oppose_cold - 1);
            if (p_ptr->oppose_pois) p_ptr->oppose_pois = MAX(0, p_ptr->oppose_pois - 1);
            if (p_ptr->tim_esp) p_ptr->tim_esp = MAX(0, p_ptr->tim_esp - 1);
            if (p_ptr->wraith_form) p_ptr->wraith_form = MAX(0, p_ptr->wraith_form - 1);
            if (p_ptr->chargespell) p_ptr->chargespell = MAX(0, p_ptr->chargespell - 1);
            if (p_ptr->magicdef) p_ptr->magicdef = MAX(0, p_ptr->magicdef - 1);
            if (p_ptr->tim_res_time) p_ptr->tim_res_time = MAX(0, p_ptr->tim_res_time - 1);
            if (p_ptr->tim_sh_fire) p_ptr->tim_sh_fire = MAX(0, p_ptr->tim_sh_fire - 1);
            if (p_ptr->tim_sh_elec) p_ptr->tim_sh_elec = MAX(0, p_ptr->tim_sh_elec - 1);
            if (p_ptr->tim_sh_cold) p_ptr->tim_sh_cold = MAX(0, p_ptr->tim_sh_cold - 1);
            if (p_ptr->tim_sh_holy) p_ptr->tim_sh_holy = MAX(0, p_ptr->tim_sh_holy - 1);
            if (p_ptr->tim_sh_aura) p_ptr->tim_sh_aura = MAX(0, p_ptr->tim_sh_aura - 1);
            if (p_ptr->tim_eyeeye) p_ptr->tim_eyeeye = MAX(0, p_ptr->tim_eyeeye - 1);
            if (p_ptr->tim_inc_blow) p_ptr->tim_inc_blow = MAX(0, p_ptr->tim_inc_blow - 1);
            if (p_ptr->tim_dec_blow) p_ptr->tim_dec_blow = MAX(0, p_ptr->tim_dec_blow - 1);
            if (p_ptr->zoshonel_protect) p_ptr->zoshonel_protect = MAX(0, p_ptr->zoshonel_protect - 1);
            if (p_ptr->earth_spike) p_ptr->earth_spike = MAX(0, p_ptr->earth_spike - 1);
            if (p_ptr->wind_guard) p_ptr->wind_guard = MAX(0, p_ptr->wind_guard - 1);
            if (p_ptr->tim_resurrection) p_ptr->tim_resurrection = MAX(0, p_ptr->tim_resurrection - 1);
            if (p_ptr->multishadow) p_ptr->multishadow = MAX(0, p_ptr->multishadow - 1);
            if (p_ptr->dustrobe) p_ptr->dustrobe = MAX(0, p_ptr->dustrobe - 1);
            if (p_ptr->inhibit_flood) p_ptr->inhibit_flood = MAX(0, p_ptr->inhibit_flood - 1);
            party_recalculate();
            party_capture();
        }
        p_ptr->is_dead = before.is_dead; p_ptr->leaving = before.leaving;
    }
    party_restore(&party_members[original]); party_active = original;
    *p_ptr = before;
    for (j = 0; j < max_c_idx; j++) class_info[j].realm_choices = realms[j];
    party_rebind(); party_recalculate();
    party_waiting = FALSE; can_save = old_save;
    monk_armour_aux = old_monk; monk_notify_aux = old_notify;
    energy_use = old_energy;
    p_ptr->window |= PW_INVEN | PW_EQUIP | PW_PLAYER;
}

/* Scatter near the corpse; a wall/deep-water death may require the nearest
 * open floor beyond the ordinary three-square drop radius. */
static void party_drop_object(object_type *object)
{
    int y, x, by = py, bx = px, best = 0x7fffffff;
    for (y = 1; y < cur_hgt - 1; y++)
        for (x = 1; x < cur_wid - 1; x++)
            if (cave_clean_bold(y, x))
            {
                int d = distance(py, px, y, x);
                if (d < best) { best = d; by = y; bx = x; }
            }
    (void)drop_near(object, -1, by, bx);
}

/* A voluntary sacrifice is not a combat death: no succession or tarot draw. */
bool party_dark_sacrifice(void)
{
    int i, j, victims[2], n = 0;
    char prompt[160];
    if (!party_can_dark_contract()) return FALSE;
    for (i = 0; i < party_count; i++) if (i != party_active) victims[n++] = i;
#ifdef JP
    strnfmt(prompt, sizeof(prompt), "%sと%sを生贄に捧げます。仲間は消失し、二度と加入できません。よろしいですか？",
        party_members[victims[0]].name, party_members[victims[1]].name);
#else
    strnfmt(prompt, sizeof(prompt), "Sacrifice %s and %s. Are you sure? ",
        party_members[victims[0]].name, party_members[victims[1]].name);
#endif
    if (!get_check(prompt)) return FALSE;
    party_capture();
    if (o_cnt + 2 * INVEN_TOTAL >= max_o_idx) compact_objects(2 * INVEN_TOTAL);
    for (i = 0; i < 2; i++)
    {
        party_member *m = &party_members[victims[i]];
        party_clear_weapon(&m->equipment[0], &m->weapon_weight, &m->weapon_melee);
        for (j = 0; j < INVEN_TOTAL - INVEN_RARM; j++)
        {
            if (!m->equipment[j].k_idx) continue;
            party_drop_object(&m->equipment[j]);
            object_wipe(&m->equipment[j]);
        }
        memset(m, 0, sizeof(*m));
    }
    /* Keep the survivor, even when the controlled character was slot 1 or 2.
     * The seal lives in the already serialized reward flags, and never clears
     * on a later class change. */
    party_members[0] = party_members[party_active];
    memset(&party_members[1], 0, sizeof(party_members) - sizeof(party_members[0]));
    party_active = 0;
    party_count = 1;
    party_rewards |= PARTY_RECRUIT_SEALED;
    msg_print("生贄に捧げた仲間は消失した。");
    p_ptr->update |= PU_BONUS;
    p_ptr->window |= PW_PLAYER;
    return TRUE;
}

/* Called only after the current player/monster/world action has unwound.
 * Never switch characters halfway through a blow or spell. */
bool party_handle_death(void)
{
    int i, j, count = 0, selected = -1, pool[23], cards[5];
    char ch, buf[160];
    object_type card;
    bool old_save;
    if (astral_mode || !p_ptr->playing || !p_ptr->is_dead ||
        (p_ptr->inside_arena && !party_waiting && !(p_ptr->is_dead & DEATH_SNAP_DRAGON))) return FALSE;
    party_capture();
    old_save = can_save;
    can_save = FALSE;
    if (!party_members[party_active].dead)
    {
        party_members[party_active].dead = TRUE;
        if (o_cnt + INVEN_TOTAL >= max_o_idx) compact_objects(64);
        party_clear_weapon(&inventory[INVEN_RARM], &mw_old_weight, &mw_diff_to_melee);
        p_ptr->magical_weapon = p_ptr->evil_weapon = 0;
        p_ptr->special_attack &= ~PARTY_WEAPON_BRANDS;
        for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
        {
            if (!inventory[i].k_idx) continue;
            party_drop_object(&inventory[i]);
            p_ptr->total_weight -= inventory[i].weight * inventory[i].number;
            if (equip_cnt > 0) equip_cnt--;
            object_wipe(&inventory[i]);
        }
        /* A final death goes straight to game over without a tarot choice. */
        if (party_has_successor() && !party_members[party_active].revived)
        {
            for (i = 0; i < 23; i++) pool[i] = i ? i * 2 - 1 : 0;
            for (i = 0; i < 5; i++)
            {
                j = i + randint0(23 - i);
                cards[i] = pool[j]; pool[j] = pool[i];
            }
            screen_save();
            prt("残すタロットカードを1枚選んでください。", 1, 0);
            for (i = 0; i < 5; i++)
            {
                strnfmt(buf, sizeof(buf), "%c) %s", I2A(i), tarot_info[cards[i]].name);
                prt(buf, i + 2, 0);
            }
            do { ch = inkey(); i = A2I(ch); } while (i < 0 || i >= 5);
            screen_load();
            object_prep(&card, lookup_kind(TV_TAROT, 0));
            card.number = 1; card.pval = cards[i];
            object_aware(&card); object_known(&card);
            party_drop_object(&card);
        }
        party_capture();
    }
    if (party_waiting) { can_save = old_save; return TRUE; }
    for (i = 0; i < party_count; i++)
        if (i != party_active && !party_members[i].dead && party_members[i].player.chp >= 0)
        { count++; selected = i; }
    if (!count) { can_save = old_save; return FALSE; }
    if (party_cast_original >= 0) selected = party_cast_original;
    else if (count > 1)
    {
        screen_save();
        prt("交代する仲間を選んでください。", 1, 0);
        for (i = 0; i < party_count; i++)
        {
            party_member *m = &party_members[i];
            prt("", i + 2, 0);
            if (i == party_active || m->dead || m->player.chp < 0) continue;
            strnfmt(buf, sizeof(buf), "%c) %s  HP %ld/%ld  MP %ld/%ld", I2A(i), m->name,
                (long)m->player.chp, (long)m->player.mhp, (long)m->player.csp, (long)m->player.msp);
            prt(buf, i + 2, 0);
        }
        do { ch = inkey(); selected = A2I(ch); }
        while (selected < 0 || selected >= party_count || selected == party_active ||
            party_members[selected].dead || party_members[selected].player.chp < 0);
        screen_load();
    }
    if (!party_switch_aux(selected, TRUE)) { can_save = old_save; return FALSE; }
    can_save = old_save;
    if (can_save && !cheat_save && !save_player()) msg_print("セーブ失敗！");
    return TRUE;
}

/* Compute the roster alignment from personal equipment and current shared pets.
 * Do not switch players or recalculate bonuses just to draw the menu. */
static void party_alignment(const party_member *member, char *buf, size_t size)
{
    const player_type *p = &member->player;
    s32b lnc = p->align_self[ALI_LNC] + friend_align_lnc;
    s32b gne = p->align_self[ALI_GNE] + friend_align_gne;
    u32b race_flags = race_info[p->prace].r_flags;
    u32b class_flags = class_info[p->pclass].c_flags;
    cptr good, law;
    int i;
    for (i = 0; i < INVEN_TOTAL - INVEN_RARM; i++)
    {
        object_type item = member->equipment[i];
        u32b flags[TR_FLAG_SIZE];
        object_flags(&item, flags);
        if (have_flag(flags, TR_ALIGN_LNC)) lnc += item.to_align[ALI_LNC] * 10;
        if (have_flag(flags, TR_ALIGN_GNE)) gne += item.to_align[ALI_GNE] * 10;
    }
    if (race_flags & PRF_ALIGN_GOOD) gne = 50;
    else if (race_flags & PRF_ALIGN_EVIL) gne = -50;
    else if (class_flags & PCF_ALIGN_GOOD) gne = 50;
    else if (class_flags & PCF_ALIGN_EVIL) gne = -50;
    else if (!r_info[MON_FILARHH].max_num) gne = -50;
#ifdef JP
    good = gne >= 50 ? "善良" : gne <= -50 ? "邪悪" : "中立";
    law = lnc >= 50 ? "秩序" : lnc <= -50 ? "混沌" : "中庸";
#else
    good = gne >= 50 ? "G" : gne <= -50 ? "E" : "N";
    law = lnc >= 50 ? "L" : lnc <= -50 ? "C" : "N";
#endif
    strnfmt(buf, size, "%s-%s", good, law);
}

static void party_menu_field(int row, int col, int width, byte color, cptr text)
{
    int len = 0, w, h;
    Term_get_size(&w, &h);
    if (row >= h || col >= w) return;
    if (width > w - col) width = w - col;
    while (text[len])
    {
        int step = 1;
#ifdef JP
        if (iskanji(text[len])) step = 2;
#endif
        if (len + step > width || (step == 2 && !text[len + 1])) break;
        len += step;
    }
    if (len) Term_putstr(col, row, len, color, text);
}

void do_cmd_party_next(void)
{
    int offset, member;
    energy_use = 0;
    if (party_switch_blocked_by_riding()) return;
    if (p_ptr->inside_arena)
    {
        msg_print("闘技場では仲間と交代できません。");
        return;
    }
    if (astral_mode || party_count < 2) return;
    for (offset = 1; offset < party_count; offset++)
    {
        member = (party_active + offset) % party_count;
        if (party_members[member].dead || party_members[member].player.chp < 0) continue;
        (void)party_switch(member);
        return;
    }
}

void do_cmd_party(void)
{
    int i;
    char ch, buf[160];
    energy_use = 0;
    if (party_switch_blocked_by_riding()) return;
    if (p_ptr->inside_arena)
    {
        msg_print("闘技場では仲間と交代できません。");
        return;
    }
    if (astral_mode || party_count < 2) return;
    party_capture();
    screen_save();
    prt("   名前         職業         Lv  HP         MP         アライン   属性", 1, 0);
    for (i = 0; i < party_count; ++i)
    {
        party_member *m = &party_members[i];
        int row = i + 2, elem = m->player.pelem;
        prt("", row, 0);
        strnfmt(buf, sizeof(buf), "%c)%c", I2A(i), m->dead ? 'X' : i == party_active ? '*' : ' ');
        party_menu_field(row, 0, 3, TERM_WHITE, buf);
        party_menu_field(row, 3, 12, m->dead ? TERM_L_DARK : TERM_WHITE, m->name);
        party_menu_field(row, 16, 12, TERM_WHITE, c_name + class_info[m->player.pclass].name);
        strnfmt(buf, sizeof(buf), "%ld", (long)m->player.lev);
        party_menu_field(row, 29, 3, TERM_WHITE, buf);
        if (m->dead) strcpy(buf, "死亡");
        else strnfmt(buf, sizeof(buf), "%ld/%ld", (long)m->player.chp, (long)m->player.mhp);
        party_menu_field(row, 33, 10, m->dead ? TERM_L_DARK : TERM_WHITE, buf);
        strnfmt(buf, sizeof(buf), "%ld/%ld", (long)m->player.csp, (long)m->player.msp);
        party_menu_field(row, 44, 10, TERM_WHITE, m->dead ? "-" : buf);
        party_alignment(m, buf, sizeof(buf));
        party_menu_field(row, 55, 9, TERM_WHITE, buf);
        party_menu_field(row, 66, 5, elem_attr(elem),
            elem >= MIN_ELEM && elem < ELEM_NUM ? elem_names[elem] : "-");
    }
    strnfmt(buf, sizeof(buf), "現在: HP %ld/%ld  MP %ld/%ld",
        (long)p_ptr->chp, (long)p_ptr->mhp, (long)p_ptr->csp, (long)p_ptr->msp);
    prt(buf, party_count + 2, 0);
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
    if ((party_rewards & PARTY_RECRUIT_SEALED) || pclass_is_(CLASS_RELICSKNIGHT) || astral_mode || party_creating || p_ptr->is_dead || p_ptr->chp < 0 ||
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

/* Match the normal end-of-game identification without changing protagonists. */
void party_identify_reserve_equipment(void)
{
    int m, i;
    for (m = 0; m < party_count; ++m)
    {
        if (m == party_active) continue;
        for (i = 0; i < INVEN_TOTAL - INVEN_RARM; ++i)
        {
            object_type *object = &party_members[m].equipment[i];
            if (!object->k_idx) continue;
            object_aware(object);
            object_known(object);
        }
    }
}

/* Training borrows the normal level-up routines in an isolated character context. */
bool party_train_member(int member)
{
    player_type *original = p_ptr, *trainee;
    object_type *original_inventory = inventory, *training_inventory;
    party_member *m;
    int race_limit, class_limit, i;
    int old_inven = inven_cnt, old_equip = equip_cnt, old_energy = energy_use;
    bool old_xtra = character_xtra, old_save = can_save;
    bool old_monk = monk_armour_aux, old_notify = monk_notify_aux;
    s16b old_weight = mw_old_weight, old_melee = mw_diff_to_melee;
    char old_name[32];
    u32b old_realms[MAX_CLASS];
    s32b needed;
    if (astral_mode || party_training || party_creating || p_ptr->is_dead || p_ptr->chp < 0 ||
        member < 0 || member >= party_count || member == party_active) return FALSE;
    race_limit = MIN(30, p_ptr->lev - 1);
    class_limit = MIN(30, p_ptr->cexp_info[p_ptr->pclass].clev - 1);
    m = &party_members[member];
    if (m->dead || m->player.chp < 0) return FALSE;
    if (m->player.lev >= race_limit &&
        m->player.cexp_info[m->player.pclass].clev >= class_limit) return FALSE;
    strcpy(old_name, player_name);
    for (i = 0; i < max_c_idx; i++) old_realms[i] = class_info[i].realm_choices;
    C_MAKE(trainee, 1, player_type);
    C_MAKE(training_inventory, INVEN_TOTAL, object_type);
    *trainee = *original;
    memcpy(training_inventory, original_inventory, sizeof(object_type) * INVEN_TOTAL);
    screen_save();
    party_training = TRUE;
    can_save = FALSE;
    character_xtra = TRUE;
    p_ptr = trainee;
    inventory = training_inventory;
    party_restore(m);
    init_realm_table();
    party_recalculate();
    msg_format("%sのトレーニングを開始します。", player_name);
    if (p_ptr->cexp_info[p_ptr->pclass].clev < class_limit)
    {
        needed = (s32b)((u64b)player_exp[class_limit - 2] * p_ptr->cexpfact[p_ptr->pclass] / 100);
        if (needed > p_ptr->cexp_info[p_ptr->pclass].cexp)
            gain_class_exp(needed - p_ptr->cexp_info[p_ptr->pclass].cexp);
    }
    if (p_ptr->lev < race_limit)
    {
        needed = (s32b)((u64b)player_exp[race_limit - 2] * p_ptr->expfact / 100);
        if (needed > p_ptr->exp) gain_racial_exp(needed - p_ptr->exp);
    }
    party_recalculate();
    /* Training itself does not heal; reserves recover only as world time passes. */
    p_ptr->chp = m->player.chp; p_ptr->chp_frac = m->player.chp_frac;
    p_ptr->csp = m->player.csp; p_ptr->csp_frac = m->player.csp_frac;
    party_store(m);
    msg_format("%sのトレーニングが完了しました。種族レベル%d、職業レベル%d。",
        player_name, p_ptr->lev, p_ptr->cexp_info[p_ptr->pclass].clev);
    msg_print(NULL);
    p_ptr = original;
    inventory = original_inventory;
    strcpy(player_name, old_name);
    mw_old_weight = old_weight; mw_diff_to_melee = old_melee;
    inven_cnt = old_inven; equip_cnt = old_equip;
    energy_use = old_energy;
    for (i = 0; i < max_c_idx; i++) class_info[i].realm_choices = old_realms[i];
    party_rebind();
    character_xtra = old_xtra;
    can_save = old_save;
    monk_armour_aux = old_monk; monk_notify_aux = old_notify;
    party_training = FALSE;
    C_KILL(trainee, 1, player_type);
    C_KILL(training_inventory, INVEN_TOTAL, object_type);
    screen_load();
    return TRUE;
}

static bool party_can_reincarnate(int member)
{
    party_member *m;
    if (astral_mode || member < 0 || member >= party_count || member == party_active) return FALSE;
    m = &party_members[member];
    return !m->dead && !m->player.is_dead && m->player.chp >= 0 &&
        (m->player.prace == RACE_SKELETON || m->player.prace == RACE_GHOST);
}

/* mode: 0 resurrection, 1 necromancy, 2 reincarnation. */
static bool party_restore_life(int member, int mode)
{
    player_type *original = p_ptr, *trainee;
    object_type *original_inventory = inventory, *training_inventory;
    party_member *m;
    int i;
    int old_inven = inven_cnt, old_equip = equip_cnt, old_energy = energy_use;
    bool old_xtra = character_xtra, old_save = can_save;
    bool old_monk = monk_armour_aux, old_notify = monk_notify_aux;
    s16b old_weight = mw_old_weight, old_melee = mw_diff_to_melee;
    char old_name[32];
    u32b old_realms[MAX_CLASS];
    if (astral_mode || party_training || party_creating || p_ptr->is_dead || p_ptr->chp < 0 ||
        member < 0 || member >= party_count || member == party_active) return FALSE;
    m = &party_members[member];
    /* Mode 3 is Martyrize: its complete post-sacrifice roster was checked
     * before restoring anyone; the caster is still alive until casting ends. */
    if (mode == 3 ? !m->dead : (mode == 2 ? !party_can_reincarnate(member) : !party_can_revive(member))) return FALSE;
    strcpy(old_name, player_name);
    for (i = 0; i < max_c_idx; i++) old_realms[i] = class_info[i].realm_choices;
    C_MAKE(trainee, 1, player_type);
    C_MAKE(training_inventory, INVEN_TOTAL, object_type);
    *trainee = *original;
    memcpy(training_inventory, original_inventory, sizeof(object_type) * INVEN_TOTAL);
    screen_save();
    party_training = TRUE;
    can_save = FALSE;
    character_xtra = TRUE;
    p_ptr = trainee;
    inventory = training_inventory;
    party_restore(m);
    if (mode == 1)
    {
        int j;
        s32b gain;
        p_ptr->prace = one_in_(2) ? RACE_SKELETON : RACE_GHOST;
        party_rebind();
        /* As with evolution(), rebuild racial HP/MP for the new body.
         * Retain earned levels, experience, stats and class growth. */
        memset(p_ptr->race_hp, 0, sizeof(p_ptr->race_hp));
        memset(p_ptr->race_sp, 0, sizeof(p_ptr->race_sp));
        for (i = 0; i < p_ptr->max_plv; i++)
        {
            for (j = 0; j < (i == 0 ? 3 : 1); j++)
            {
                gain = rand_spread(rp_ptr->r_mhp, 1);
                p_ptr->race_hp[i] += MAX(gain, 0);
                gain = rand_spread(rp_ptr->r_msp, 1);
                p_ptr->race_sp[i] += MAX(gain, 0);
            }
        }
    }
    if (mode == 2) reincarnation();
    init_realm_table();
    party_recalculate();
    if (mode != 2)
    {
#define PARTY_EFFECT(type, name) p_ptr->name = 0;
#include "party-effects.h"
#undef PARTY_EFFECT
    party_clear_effects(p_ptr);
    /* Recompute maxima without the equipment dropped at death. */
    p_ptr->is_dead = 0;
    p_ptr->chp = MAX(1, p_ptr->mhp); p_ptr->chp_frac = 0;
    p_ptr->csp = 0; p_ptr->csp_frac = 0;
    strcpy(p_ptr->died_from, "(alive and well)");
    m->revived = TRUE;
    }
    else
    {
        p_ptr->chp = MIN(p_ptr->chp, p_ptr->mhp);
        p_ptr->csp = MIN(p_ptr->csp, p_ptr->msp);
        p_ptr->chp_frac = p_ptr->csp_frac = 0;
    }
    party_store(m);
    p_ptr = original;
    inventory = original_inventory;
    strcpy(player_name, old_name);
    mw_old_weight = old_weight; mw_diff_to_melee = old_melee;
    inven_cnt = old_inven; equip_cnt = old_equip;
    energy_use = old_energy;
    for (i = 0; i < max_c_idx; i++) class_info[i].realm_choices = old_realms[i];
    party_rebind();
    character_xtra = old_xtra;
    can_save = old_save;
    monk_armour_aux = old_monk; monk_notify_aux = old_notify;
    party_training = FALSE;
    C_KILL(trainee, 1, player_type);
    C_KILL(training_inventory, INVEN_TOTAL, object_type);
    screen_load();
    return TRUE;
}

/* This spell is caster-only, never redirected to the active spell target. */
bool party_martyrize(void)
{
    int i, j, dead = 0, advanced = 0, limit = party_level_limit();
    bool legal = TRUE;
    if (party_casting)
    {
        msg_print("マーティライズは操作中のキャラクターしか唱えられません。");
        return FALSE;
    }
    if (astral_mode || party_creating || party_training || p_ptr->inside_arena || p_ptr->is_dead || p_ptr->chp < 0) return FALSE;
    /* Every companion will be alive and the caster dead. Validate that final
     * roster atomically, rather than picking winners by party slot order. */
    for (i = 0; i < party_count; i++)
    {
        if (i == party_active) continue;
        if (party_members[i].dead) dead++;
        if (!party_class_is_advanced(party_members[i].player.pclass)) continue;
        if (++advanced > party_advanced_limit()) legal = FALSE;
        for (j = 0; j < party_count; j++)
            if (j != i && j != party_active && party_above_limit(&party_members[j].player, limit)) legal = FALSE;
    }
    if (!dead) { msg_print("蘇生する仲間がいません。"); return FALSE; }
    if (!legal) { msg_print("仲間全員を蘇生する条件を満たしていません。"); return FALSE; }
    if (!get_check("自分の命を犠牲にして仲間全員を蘇生しますか？")) return FALSE;
    for (i = 0; i < party_count; i++)
    {
        if (i == party_active || !party_members[i].dead) continue;
        if (!party_restore_life(i, 3)) return FALSE;
        msg_format("%sが生き返った。", party_members[i].name);
    }
    /* Resolve equipment drops, tarot and succession after the spell action,
     * so its MP and energy costs are charged to the caster, not a survivor. */
    msg_format("%sは仲間のために命を捧げた。", player_name);
    p_ptr->chp = -1;
    p_ptr->chp_frac = 0;
    p_ptr->is_dead = DEATH_DEAD;
    p_ptr->leaving = TRUE;
    strcpy(p_ptr->died_from, "マーティライズによる自己犠牲");
    p_ptr->redraw |= PR_HP;
    p_ptr->window |= PW_PLAYER;
    return TRUE;
}

bool party_revive_member(int member)
{
    return party_restore_life(member, FALSE);
}

bool party_has_dead_member(void)
{
    int i;
    if (astral_mode) return FALSE;
    for (i = 0; i < party_count; i++)
        if (party_can_revive(i)) return TRUE;
    return FALSE;
}

static bool party_resurrect_aux(bool necromancy)
{
    int i;
    char ch, buf[160];
    if (!party_has_dead_member())
    {
        msg_print("蘇生できる仲間がいません。");
        return FALSE;
    }
    screen_save();
    prt("蘇生する仲間を選んでください。(ESCで中止)", 1, 0);
    for (i = 0; i < party_count; i++)
    {
        prt("", i + 2, 0);
        if (!party_can_revive(i)) continue;
        strnfmt(buf, sizeof(buf), "%c) %s", I2A(i), party_members[i].name);
        prt(buf, i + 2, 0);
    }
    while (TRUE)
    {
        ch = inkey();
        if (ch == ESCAPE) { screen_load(); return FALSE; }
        i = A2I(ch);
        if (party_can_revive(i)) break;
        bell();
    }
    screen_load();
    if (!party_restore_life(i, necromancy)) return FALSE;
    if (necromancy)
        msg_format("%sは%sとして生き返った！", party_members[i].name,
            p_name + race_info[party_members[i].player.prace].name);
    else
        msg_format("%sが生き返った！", party_members[i].name);
    return TRUE;
}

bool party_resurrect(void)
{
    return party_resurrect_aux(FALSE);
}

bool party_necromancy(void)
{
    return party_resurrect_aux(TRUE);
}

bool party_has_reincarnation_target(void)
{
    int i;
    for (i = 0; i < party_count; i++)
        if (party_can_reincarnate(i)) return TRUE;
    return FALSE;
}

bool party_reincarnate(void)
{
    int i;
    char ch, buf[160];
    if (!party_has_reincarnation_target())
    {
        msg_print("転生できるスケルトンかゴーストの仲間がいません。");
        return FALSE;
    }
    screen_save();
    prt("転生する仲間を選んでください。(ESCで中止)", 1, 0);
    for (i = 0; i < party_count; i++)
    {
        prt("", i + 2, 0);
        if (!party_can_reincarnate(i)) continue;
        strnfmt(buf, sizeof(buf), "%c) %s (%s)", I2A(i), party_members[i].name,
            p_name + race_info[party_members[i].player.prace].name);
        prt(buf, i + 2, 0);
    }
    while (TRUE)
    {
        ch = inkey();
        if (ch == ESCAPE) { screen_load(); return FALSE; }
        i = A2I(ch);
        if (party_can_reincarnate(i)) break;
        bell();
    }
    screen_load();
    strnfmt(buf, sizeof(buf), "%sをレベル1の人間に転生させます。よろしいですか？ ", party_members[i].name);
    if (!get_check(buf)) return FALSE;
    if (!party_restore_life(i, 2)) return FALSE;
    msg_format("%sは人間に転生した！", party_members[i].name);
    return TRUE;
}

void do_cmd_party_train(void)
{
    int i;
    char ch, buf[160];
    if (astral_mode || party_count < 2) return;
    screen_save();
    strnfmt(buf, sizeof(buf), "トレーニング上限: 種族Lv%d / 職業Lv%d",
        MAX(1, MIN(30, p_ptr->lev - 1)),
        MAX(1, MIN(30, p_ptr->cexp_info[p_ptr->pclass].clev - 1)));
    prt(buf, 1, 0);
    for (i = 0; i < party_count; i++)
    {
        party_member *m = &party_members[i];
        if (i == party_active || m->dead) { prt("", i + 2, 0); continue; }
        strnfmt(buf, sizeof(buf), "%c) %s  種族Lv%d / %s Lv%d", I2A(i), m->name,
            m->player.lev, c_name + class_info[m->player.pclass].name,
            m->player.cexp_info[m->player.pclass].clev);
        prt(buf, i + 2, 0);
    }
    while (get_com("誰をトレーニングしますか？（ESCで中断）", &ch, FALSE))
    {
        i = A2I(ch);
        if (i < 0 || i >= party_count || i == party_active || party_members[i].dead) continue;
        screen_load();
        if (!party_train_member(i)) msg_print("その仲間はトレーニングの上限に達しています。");
        return;
    }
    screen_load();
}
