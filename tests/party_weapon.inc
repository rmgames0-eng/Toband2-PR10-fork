
static int weapon_magic_turns=3, weapon_evil_turns=5;
static void cast_weapon_enchantment(int item)
{
    msg_flag=FALSE;
    assert(set_magical_weapon(ATTACK_FIRE,weapon_magic_turns,INVEN_RARM,FALSE));
    msg_flag=FALSE;
    assert(set_evil_weapon(weapon_evil_turns,FALSE,INVEN_RARM,FALSE) || p_ptr->evil_weapon==weapon_evil_turns);
    energy_use=100;
}
static void weapon_regressions(void)
{
    object_type active_base, reserve_base;
    int i, bonus;
    object_prep(&inventory[INVEN_RARM],lookup_kind(TV_SWORD,SV_DAGGER));
    active_base=inventory[INVEN_RARM];
    object_prep(&party_members[1].equipment[0],lookup_kind(TV_SWORD,SV_DAGGER));
    reserve_base=party_members[1].equipment[0];
    p_ptr->magical_weapon=p_ptr->evil_weapon=0;
    p_ptr->special_attack &= ~PARTY_WEAPON_BRANDS;
    mw_old_weight=mw_diff_to_melee=0;
    party_members[1].weapon_weight=party_members[1].weapon_melee=0;
    party_members[1].player.magical_weapon=party_members[1].player.evil_weapon=0;
    party_members[1].player.special_attack &= ~PARTY_WEAPON_BRANDS;
    p_ptr->cexp_info[p_ptr->pclass].clev=30;
    keys="                                        "; key_index=0; msg_flag=FALSE;
    set_magical_weapon(ATTACK_COLD,7,INVEN_RARM,FALSE);
    bonus=party_members[1].player.cexp_info[party_members[1].player.pclass].clev/2;
    msg_flag=FALSE; party_cast_book(1,0,cast_weapon_enchantment);
    assert(p_ptr->magical_weapon==7 && !p_ptr->evil_weapon);
    assert((p_ptr->special_attack & PARTY_WEAPON_BRANDS)==ATTACK_COLD);
    assert(party_members[1].player.magical_weapon==3 && party_members[1].player.evil_weapon==5);
    assert((party_members[1].player.special_attack & PARTY_WEAPON_BRANDS)==(ATTACK_FIRE|ATTACK_EVIL));
    assert(party_members[1].equipment[0].to_h==reserve_base.to_h+bonus);
    assert(party_members[1].equipment[0].weight==1);
    /* Refreshing the spell must not add its weapon bonus twice. */
    msg_flag=FALSE; party_cast_book(1,0,cast_weapon_enchantment);
    assert(party_members[1].equipment[0].to_d==reserve_base.to_d+bonus);
    party_timeout_weapons();
    assert(party_members[1].player.magical_weapon==2 && party_members[1].player.evil_weapon==4);
    assert(p_ptr->magical_weapon==7);
    can_save=TRUE; assert(save_player()); party_reset(); assert(load_player());
    assert(p_ptr->magical_weapon==7 && (p_ptr->special_attack & PARTY_WEAPON_BRANDS)==ATTACK_COLD);
#ifdef TEST_PARTY_V3
    assert(!party_members[1].player.magical_weapon && !party_members[1].player.evil_weapon);
    assert(!party_members[1].weapon_weight && !party_members[1].weapon_melee);
    assert(party_members[1].equipment[0].weight==reserve_base.weight);
    assert(party_members[1].equipment[0].to_h==reserve_base.to_h);
    assert(party_members[1].equipment[0].to_d==reserve_base.to_d);
    puts("Previous v3 save: orphaned reserve enchantments removed, active enchantment preserved");
#else
    assert(party_members[1].player.magical_weapon==2 && party_members[1].player.evil_weapon==4);
    party_timeout_weapons(); party_timeout_weapons();
    assert(!party_members[1].player.magical_weapon && party_members[1].player.evil_weapon==2);
    assert(party_members[1].player.special_attack==ATTACK_EVIL);
    assert(party_members[1].equipment[0].weight==reserve_base.weight);
    assert(party_members[1].equipment[0].to_h==reserve_base.to_h+bonus);
    party_timeout_weapons(); party_timeout_weapons();
    assert(!party_members[1].player.evil_weapon && !party_members[1].player.special_attack);
    assert(party_members[1].equipment[0].to_h==reserve_base.to_h && party_members[1].equipment[0].to_d==reserve_base.to_d);
    /* The reverse expiry order retains the elemental weight and bonus. */
    weapon_magic_turns=4; weapon_evil_turns=2;
    msg_flag=FALSE; party_cast_book(1,0,cast_weapon_enchantment);
    party_timeout_weapons(); party_timeout_weapons();
    assert(party_members[1].player.magical_weapon==2 && !party_members[1].player.evil_weapon);
    assert(party_members[1].player.special_attack==ATTACK_FIRE && party_members[1].equipment[0].weight==1);
    assert(party_members[1].equipment[0].to_h==reserve_base.to_h+bonus);
    party_timeout_weapons(); party_timeout_weapons();
    assert(party_members[1].equipment[0].weight==reserve_base.weight);
    assert(party_members[1].equipment[0].to_d==reserve_base.to_d);
    puts("Weapon enchantments: independent actors, refresh, both expiry orders and save persistence passed");
#endif
    msg_flag=FALSE; set_magical_weapon(0,0,INVEN_RARM,FALSE);
    assert(inventory[INVEN_RARM].weight==active_base.weight && inventory[INVEN_RARM].to_d==active_base.to_d);
}
