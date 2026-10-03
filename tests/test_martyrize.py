from pathlib import Path
import tempfile,subprocess
r=Path(__file__).resolve().parents[1];b=r/'build/party';h=(r/'tests/party_integration.c').read_bytes().decode('euc_jp')
s=(r/'tests/party_cooperation.inc').read_text(encoding='utf-8');s=s[s.index('static void cooperation_tests(void)'):];s=s[:s.index('    /* The supporter determines')]
s=s.replace('static void cooperation_tests(void)','static void martyrize_tests(void)')
s+=r"""
    {
        int old_max=r_info[1].max_num, old_flags=r_info[1].flags1;
        party_members[1].player.pclass=CLASS_GENERAL;
        party_members[2].player.pclass=CLASS_SOLDIER;
        p_ptr->pclass=CLASS_LORD;party_rebind();
        r_info[MON_DOLGARUA].r_pkills=r_info[MON_FILARHH].r_pkills=0;
        quest[QUEST_DOLGARUA].status=quest[QUEST_FILARHH].status=QUEST_STATUS_UNTAKEN;
        p_ptr->csp=30;energy_use=0;
        assert(!do_spell(REALM_DRAKONITE,0,SPELL_CAST));
        assert(p_ptr->chp==100 && !p_ptr->is_dead);
        for(i=1;i<3;i++) {
            party_members[i].dead=TRUE;party_members[i].player.chp=-1;
            party_members[i].player.is_dead=DEATH_DEAD;
            for(j=0;j<INVEN_TOTAL-INVEN_RARM;j++)object_wipe(&party_members[i].equipment[j]);
        }
        assert(!party_can_revive(1)); /* caster still occupies the advanced slot */
        party_casting=TRUE;
        assert(!spell_okay(0,REALM_DRAKONITE));
        assert(!do_spell(REALM_DRAKONITE,0,SPELL_CAST));
        assert(party_members[1].dead && !p_ptr->is_dead && p_ptr->csp==30);
        party_casting=FALSE;
        party_members[2].player.lev=36;
        assert(!do_spell(REALM_DRAKONITE,0,SPELL_CAST));
        assert(party_members[1].dead && party_members[2].dead && !p_ptr->is_dead);
        party_members[2].player.lev=1;
        keys="n";key_index=0;msg_flag=FALSE;
        assert(!do_spell(REALM_DRAKONITE,0,SPELL_CAST));
        assert(party_members[1].dead && party_members[2].dead && !p_ptr->is_dead);
        r_info[1].max_num=0;r_info[1].flags1|=RF1_UNIQUE;
        keys="y                     ";key_index=0;msg_flag=FALSE;
        assert(do_spell(REALM_DRAKONITE,0,SPELL_CAST));
        assert(p_ptr->is_dead==DEATH_DEAD && p_ptr->chp<0 && party_active==0);
        for(i=1;i<3;i++) {
            assert(!party_members[i].dead && party_members[i].revived);
            assert(party_members[i].player.chp==party_members[i].player.mhp);
            assert(!party_members[i].player.csp && !party_members[i].equipment[0].k_idx);
        }
        assert(!r_info[1].max_num); /* old monster resurrection is gone */
        r_info[1].max_num=old_max;r_info[1].flags1=old_flags;
        object_prep(&inventory[INVEN_RARM],lookup_kind(TV_SWORD,SV_DAGGER));
        inventory[INVEN_RARM].number=1;
        party_members[0].revived=TRUE;can_save=FALSE;
        keys="b                     ";key_index=0;msg_flag=FALSE;
        assert(party_handle_death());assert(party_active==1 && !p_ptr->is_dead);
        assert(party_members[0].dead && !party_members[0].equipment[0].k_idx);
        strcpy(savefile,"martyrize-test.sav");
        assert(save_player());init_saved_floors(TRUE);party_reset();assert(load_player());
        assert(party_active==1 && party_members[0].dead && !party_members[2].dead);
        puts("Martyrize: no reserve casting, atomic limits excluding caster, cancellation, all-party revival, caster death/drop/succession and save passed");
    }
}
"""
h=h.replace('int main(int argc,char **argv)',s+'\nint main(int argc,char **argv)',1).replace('    display_regressions();','    martyrize_tests();\n    return 0;\n    display_regressions();',1)
with tempfile.TemporaryDirectory(prefix='martyrize-') as d:
 t=Path(d);(t/'test.c').write_bytes(h.encode('euc_jp'));exe=t/'test.exe';objs=[p for p in b.glob('*.o') if p.stem not in ('main-win','angband')]
 subprocess.run([str(r/'tools/mingw32/bin/gcc.exe'),'-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(r/'src'),str(t/'test.c'),*map(str,objs),'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
 out=subprocess.run([str(exe),(b/'lib').as_posix()+'/'],cwd=t,capture_output=True,timeout=30)
 print(out.stdout.decode('cp932',errors='replace'));print(out.stderr.decode('cp932',errors='replace'));assert out.returncode==0
