"""Verify reserve regeneration and the production rest completion checks."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build/party'
GCC=ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h=(ROOT/'tests/party_integration.c').read_text(encoding='euc_jp')
h="extern void test_process_player_rest(void);\n"+h
h=h.replace("    death_regressions();","    death_regressions();"+r"""
    {
        int tick;
        p_ptr->is_dead=0; astral_mode=FALSE;
        p_ptr->mhp=p_ptr->chp=100; p_ptr->msp=p_ptr->csp=20;
        p_ptr->blind=p_ptr->confused=p_ptr->poisoned=p_ptr->afraid=0;
        p_ptr->stun=p_ptr->cut=p_ptr->slow=p_ptr->paralyzed=0;
        p_ptr->image=p_ptr->word_recall=p_ptr->alter_reality=p_ptr->inhibit_flood=p_ptr->tim_dec_blow=0;
        p_ptr->energy_need=0; p_ptr->pspeed=110; command_rep=1;
        party_active=0; party_count=3;
        party_members[1].dead=FALSE;
        party_members[1].player.mhp=100; party_members[1].player.chp=0;
        party_members[1].player.msp=20; party_members[1].player.csp=0;
        party_members[1].player.chp_frac=party_members[1].player.csp_frac=0;
        party_members[2].dead=TRUE; party_members[2].player.chp=-1;
        assert(!party_reserves_recovered());
        resting=-2; p_ptr->action=ACTION_REST;
        test_process_player_rest(); assert(resting==-2);
        for(tick=0;tick<100;tick++) party_regenerate();
        assert(party_members[1].player.chp>0 && party_members[1].player.csp>0);
        assert(party_members[2].player.chp==-1);
        test_process_player_rest(); assert(resting==-2);
        for(tick=0;tick<2000 && !party_reserves_recovered();tick++) party_regenerate();
        assert(party_reserves_recovered());
        test_process_player_rest(); assert(resting==0 && p_ptr->action==ACTION_NONE);
        /* MP-only depletion also keeps R& waiting. */
        party_members[1].player.csp=19;
        resting=-2; p_ptr->action=ACTION_REST;
        test_process_player_rest(); assert(resting==-2);
        /* R* still only waits for the controlled character. */
        resting=-1; test_process_player_rest(); assert(resting==0);
        party_members[1].player.csp=party_members[1].player.msp=0;
        assert(party_reserves_recovered());
        p_ptr->poisoned=1; resting=-2; p_ptr->action=ACTION_REST;
        test_process_player_rest(); assert(resting==-2);
        puts("Rest: reserves recover with time; R& waits for living reserve HP and MP; dead/zero-MP reserves and existing status checks passed");
        {
            int mode, hp_rate, mp_rate;
            player_type active_before;
            object_type gear_before[INVEN_TOTAL];
            player_type *saved_player=p_ptr;
            player_race *saved_race=rp_ptr;
            player_class *saved_class=cp_ptr;
            object_type *saved_inventory=inventory;
            /* The active actor's bonus must never leak into a reserve. */
            p_ptr->regenerate=p_ptr->regenerate_mana=TRUE;
            active_before=*p_ptr;
            memcpy(gear_before,inventory,sizeof(gear_before));
            for(mode=0;mode<9;mode++)
            {
                party_member *m=&party_members[1];
                memset(m,0,sizeof(*m));
                m->player.prace=RACE_HUMAN; m->player.pclass=CLASS_SOLDIER;
                m->player.lev=m->player.max_plv=1;
                m->player.cexp_info[CLASS_SOLDIER].clev=1;
                m->player.cexp_info[CLASS_SOLDIER].max_clev=1;
                m->player.mhp=m->player.msp=1000;
                hp_rate=mp_rate=PY_REGEN_NORMAL;
                if(mode==1 || mode==3 || mode==8)
                {
                    object_prep(&m->equipment[0],lookup_kind(TV_SWORD,SV_DAGGER));
                    add_flag(m->equipment[0].art_flags,TR_REGEN);
                    hp_rate*=2; mp_rate*=2;
                }
                if(mode==2 || mode==3)
                {
                    object_prep(&m->equipment[1],lookup_kind(TV_SWORD,SV_DAGGER));
                    add_flag(m->equipment[1].art_flags,TR_REGEN_MANA);
                    mp_rate*=2;
                }
                if(mode==4 || mode==5 || mode==8)
                {
                    m->player.grace=GRACE_REGEN;
                    if(mode==4) { hp_rate*=2; mp_rate*=2; }
                    if(mode==5 || mode==8) m->player.mutation=MUT_FLESH_ROT;
                }
                if(mode==6)
                {
                    m->player.cexp_info[CLASS_TERRORKNIGHT].clev=35;
                    hp_rate*=2; mp_rate*=2;
                }
                if(mode==7)
                {
                    /* Private aura remains active in reserve; derived flags are not trusted. */
                    m->player.pclass=CLASS_LORD;
                    m->player.tim_sh_aura=20; m->player.chargespell=20;
                    hp_rate*=2; mp_rate*=2;
                    m->player.regenerate=m->player.regenerate_mana=TRUE;
                }
                party_regenerate();
                assert((u64b)m->player.chp*65536+m->player.chp_frac == (u64b)1000*hp_rate+PY_REGEN_HPBASE);
                assert((u64b)m->player.csp*65536+m->player.csp_frac == (u64b)1000*mp_rate+PY_REGEN_MNBASE);
                assert(p_ptr==saved_player && rp_ptr==saved_race && cp_ptr==saved_class && inventory==saved_inventory);
                assert(!memcmp(p_ptr,&active_before,sizeof(active_before)));
                assert(!memcmp(inventory,gear_before,sizeof(gear_before)));
            }
            puts("Reserve regeneration: equipment, mana-only, stacking, innate grace/class, flesh rot, stale fields and active isolation passed");
        }

    }
""",1)
with tempfile.TemporaryDirectory(prefix='toband-party-rest-') as d:
    d=Path(d);c=d/'test.c';exe=d/'test.exe';c.write_bytes(h.encode('euc_jp'))
    objects=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    # Stop after the real rest checks, before the command/input loop.
    dungeon=(ROOT/'src/dungeon.c').read_bytes().decode('euc_jp')
    dungeon=dungeon.replace('static void process_player(void)','void test_process_player_rest(void)',1)
    dungeon=dungeon.replace('process_player();','test_process_player_rest();')
    dungeon=dungeon.replace('\t/* Handle "abort" */','\treturn;\n\t/* Handle "abort" */',1)
    dc=d/'dungeon.c'; do=d/'dungeon.o'; dc.write_bytes(dungeon.encode('euc_jp'))
    subprocess.run([str(GCC),*FLAGS,'-c',str(dc),'-o',str(do)],check=True)
    objects=[do if p.stem=='dungeon' else p for p in objects]

    subprocess.run([str(GCC),*FLAGS,str(c),*map(str,objects),'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(exe),(BUILD/'lib').as_posix()+'/'],cwd=d,capture_output=True,timeout=30)
    print(result.stdout.decode('cp932',errors='replace'),end='')
    if result.returncode:
        print(result.stderr.decode('cp932',errors='replace'))
        raise SystemExit(result.returncode)
