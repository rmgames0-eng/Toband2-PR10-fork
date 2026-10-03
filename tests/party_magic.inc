
static void cast_probe(int item)
{
    assert(party_active==1 && p_ptr->pclass==CLASS_CLERIC && party_casting);
    assert(!save_player());
    assert(do_spell(REALM_HOLY,3,SPELL_CAST));
    p_ptr->csp-=3;
    p_ptr->exp+=17;
    p_ptr->align_self[ALI_GNE]+=9;
    p_ptr->oppose_cold=77;
    inventory[INVEN_RARM].to_d+=2;
    energy_use=95;
}
static void cast_active_time(int item) { energy_use=95; }
static void cast_cancel(int item) { energy_use=0; }
static void cast_die(int item)
{
    take_hit(DAMAGE_NOESCAPE,100000,"casting regression");
    energy_use=100;
}

static void reserve_magic_regressions(void)
{

    {
        object_type book;
        int i, j, oldhp, oldmp, oldexp, reservehp, reservemp, active_damage, reserve_damage;
        p_ptr->playing=TRUE; p_ptr->is_dead=0; p_ptr->leaving=FALSE;
        p_ptr->inside_arena=FALSE; astral_mode=FALSE;
        p_ptr->pclass=CLASS_SOLDIER; p_ptr->prace=RACE_HUMAN;
        p_ptr->paralyzed=p_ptr->stun=p_ptr->blind=p_ptr->confused=p_ptr->shero=0;
        p_ptr->anti_magic=0; p_ptr->chp=20; p_ptr->csp=7;
        p_ptr->poisoned=0; p_ptr->riding=0;
        p_ptr->lev=p_ptr->max_plv=30; p_ptr->town_num=0; dun_level=1;
        for(i=0;i<MAX_CLASS;i++) p_ptr->cexpfact[i]=class_info[i].c_exp;
        p_ptr->cexp_info[CLASS_SOLDIER].clev=30;
        for(i=0;i<INVEN_TOTAL;i++) object_wipe(&inventory[i]);
        py=px=5; cur_hgt=cur_wid=20;
        for(i=0;i<20;i++) for(j=0;j<20;j++) { cave[i][j].feat=FEAT_FLOOR; cave[i][j].info=CAVE_GLOW|CAVE_MARK|CAVE_VIEW; }
        p_ptr->update=PU_BONUS|PU_HP|PU_MANA; update_stuff();
        party_rebind(); party_reset(); party_capture(); party_count=3;
        for(i=1;i<3;i++) {
            party_members[i]=party_members[0];
            sprintf(party_members[i].name,"Caster%d",i);
            party_members[i].player.pclass=CLASS_CLERIC;
            party_members[i].player.cexp_info[CLASS_CLERIC].clev=30;
            party_members[i].player.magic_exp[REALM_HOLY]=500;
            party_members[i].player.chp=1; party_members[i].player.csp=15;
        }
        object_prep(&book,lookup_kind(TV_HOLY_BOOK,0));
        assert(party_book_caster(&book)==1);
        party_members[1].dead=TRUE; assert(party_book_caster(&book)==2);
        party_members[1].dead=FALSE; party_members[1].player.chp=-1;
        assert(party_book_caster(&book)==2); party_members[1].player.chp=1;
        party_active=2; assert(party_book_caster(&book)==1); party_active=0;
        i=party_members[1].player.csp; party_members[1].player.csp=0;
        assert(party_book_caster(&book)==1); party_members[1].player.csp=i;
        p_ptr->inside_arena=TRUE; assert(party_book_caster(&book)==-1); p_ptr->inside_arena=FALSE;
        astral_mode=TRUE; assert(party_book_caster(&book)==-1); astral_mode=FALSE;
        p_ptr->pclass=CLASS_CLERIC; party_rebind(); assert(party_book_caster(&book)==0);
        p_ptr->pclass=CLASS_SOLDIER; party_rebind();
        oldhp=p_ptr->chp; oldmp=p_ptr->csp; oldexp=p_ptr->exp;
        active_damage=inventory[INVEN_RARM].to_d;
        reserve_damage=party_members[1].equipment[0].to_d;
        msg_flag=FALSE; party_cast_book(1,0,cast_probe);
        assert(party_active==0 && p_ptr->pclass==CLASS_SOLDIER && !party_casting);
        assert(p_ptr->chp>oldhp && p_ptr->csp==oldmp && p_ptr->exp==oldexp);
        assert(party_members[1].player.chp==1 && party_members[1].player.csp==12);
        assert(party_members[1].player.exp==oldexp+17 && party_members[1].player.oppose_cold==77 && p_ptr->oppose_cold!=77);
        assert(inventory[INVEN_RARM].to_d==active_damage && party_members[1].equipment[0].to_d==reserve_damage+2);
        assert(energy_use==190);
        oldhp=p_ptr->chp;
        reservehp=party_members[1].player.chp; reservemp=party_members[1].player.csp;
        party_cast_book(1,0,cast_cancel); assert(!energy_use && party_active==0);
        party_cast_book(0,0,cast_active_time); assert(energy_use==95 && party_active==0);
        assert(party_members[1].player.chp==reservehp && party_members[1].player.csp==reservemp);
        /* Run the actual m command, selecting the book in slot a and cancelling. */
        inventory[0]=book; always_show_list=FALSE; use_menu=FALSE;
        keys="a\033"; key_index=0; msg_flag=FALSE; repeat_clear();
        p_ptr->update=p_ptr->redraw=p_ptr->window=0;
        do_cmd_cast(); assert(party_active==0 && !energy_use && !party_casting);
        assert(party_members[1].player.csp==reservemp);

        /* Books on the floor use the same caster priority and cancellation. */
        object_wipe(&inventory[0]);
        i=o_pop(); o_list[i]=book; o_list[i].iy=py; o_list[i].ix=px; cave[py][px].o_idx=i;
        keys="-\033"; key_index=0; msg_flag=FALSE; repeat_clear();
        do_cmd_cast(); assert(party_active==0 && !energy_use && !party_casting);
        assert(party_members[1].player.csp==reservemp);
        delete_object_idx(i); inventory[0]=book;
        /* A cancelled direction after selecting a reserve's attack is free. */
        party_members[2].player.pclass=CLASS_WIZARD;
        party_members[2].player.cexp_info[CLASS_WIZARD].clev=30;
        party_members[2].player.magic_exp[REALM_MAGERY]=500;
        object_prep(&inventory[0],lookup_kind(TV_MAGERY_BOOK,0));
        i=party_members[2].player.csp;
        keys="aa\033"; key_index=0; msg_flag=FALSE; repeat_clear();
        Rand_quick=TRUE; Rand_value=12345;
        do_cmd_cast(); assert(party_active==0 && !energy_use && !party_casting);
        assert(party_members[2].player.csp==i && p_ptr->chp==oldhp);
        party_members[2].player.pclass=CLASS_CLERIC; inventory[0]=book;
        /* Actual healing uses reserve mana and heals the controlled actor. */
        party_members[1].player.chp=1;
        keys="ad                         "; key_index=0; msg_flag=FALSE; repeat_clear();
        Rand_quick=TRUE; Rand_value=12345;
        do_cmd_cast();
        assert(party_active==0 && p_ptr->chp>oldhp && p_ptr->csp==oldmp);
        assert(party_members[1].player.chp==1 && party_members[1].player.csp<reservemp && energy_use>0);
        reservehp=party_members[1].player.chp; reservemp=party_members[1].player.csp;
        can_save=TRUE; assert(save_player()); party_reset(); assert(load_player());
        assert(party_active==0 && party_members[1].player.chp==reservehp && party_members[1].player.csp==reservemp);
        weapon_regressions();
        /* A revived caster drops equipment on death without a new tarot choice. */
        party_members[1].revived=TRUE;
        object_prep(&party_members[1].equipment[0],lookup_kind(TV_SWORD,SV_DAGGER));
        msg_flag=FALSE; keys="                         "; key_index=0;
        party_cast_book(1,0,cast_die);
        assert(party_active==0 && !p_ptr->is_dead && party_members[1].dead && energy_use==200);
        assert(!party_members[1].equipment[0].k_idx);
        assert(party_book_caster(&book)==2);
        puts("Reserve magic: priority, exclusion, caster HP/MP/growth/equipment, cancellation, real book casting, save/load and fatal backlash passed");
    }

}
