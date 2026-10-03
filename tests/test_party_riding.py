from pathlib import Path
import tempfile,subprocess
r=Path(__file__).resolve().parents[1];b=r/'build/party';h=(r/'tests/party_integration.c').read_bytes().decode('euc_jp')
s=(r/'tests/party_cooperation.inc').read_text(encoding='utf-8');s=s[s.index('static void cooperation_tests(void)'):];s=s[:s.index('    /* The supporter determines')]
s=s.replace('static void cooperation_tests(void)','static void riding_audit(void)')
s+=r"""
    {
        int mount=m_pop(),k,original_x=px,original_y=py;
        m_list[mount].r_idx=m_list[mount].ap_r_idx=2;
        m_list[mount].fy=py;m_list[mount].fx=px;m_list[mount].mspeed=120;
        r_info[2].flags7 |= RF7_RIDING;
        cave[py][px].m_idx=mount;p_ptr->riding=mount;
        energy_use=100;assert(!party_switch(1));assert(!energy_use && party_active==0);
        energy_use=100;do_cmd_party_next();assert(!energy_use && party_active==0);
        energy_use=100;do_cmd_party();assert(!energy_use && party_active==0);
        assert(p_ptr->riding==mount && px==original_x && py==original_y);
        /* Surround the pet: succession must still find a safe unoccupied grid. */
        for(k=0;k<8;k++)cave[py+ddy_ddd[k]][px+ddx_ddd[k]].feat=FEAT_PERM_SOLID;
        party_count=2;party_members[0].revived=TRUE;
        p_ptr->pet_extra_flags |= PF_RYOUTE;
        p_ptr->riding_ryoute=p_ptr->old_riding_ryoute=TRUE;
        p_ptr->chp=-1;p_ptr->is_dead=DEATH_DEAD;
        can_save=FALSE;
        assert(party_handle_death());
        assert(party_active==1 && !p_ptr->riding && !p_ptr->is_dead);
        assert(!(p_ptr->pet_extra_flags & PF_RYOUTE));
        assert(!p_ptr->riding_ryoute && !p_ptr->old_riding_ryoute);
        assert(cave[original_y][original_x].m_idx==mount);
        assert(m_list[mount].fy==original_y && m_list[mount].fx==original_x);
        assert(!cave[py][px].m_idx && cave_floor_bold(py,px));
        assert(party_members[0].dead && p_ptr->chp>0 && energy_use==0);
        /* Once dismounted, voluntary switching is available again. */
        party_members[0].dead=FALSE;party_members[0].player.chp=10;
        assert(party_switch(0) && energy_use==100 && !p_ptr->riding);
        puts("Riding: U/Y/direct switch blocked without a turn; death dismounts, pet stays, blocked surroundings handled, switching after dismount passed");
    }
}
"""
h=h.replace('int main(int argc,char **argv)',s+'\nint main(int argc,char **argv)',1).replace('    init_angband();','    init_angband();\n    riding_audit();\n    return 0;',1)
with tempfile.TemporaryDirectory(prefix='party-riding-') as d:
 t=Path(d);(t/'test.c').write_bytes(h.encode('euc_jp'));exe=t/'test.exe';objs=[p for p in b.glob('*.o') if p.stem not in ('main-win','angband')]
 subprocess.run([str(r/'tools/mingw32/bin/gcc.exe'),'-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(r/'src'),str(t/'test.c'),*map(str,objs),'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
 out=subprocess.run([str(exe),(b/'lib').as_posix()+'/'],cwd=t,capture_output=True,timeout=30)
 print(out.stdout.decode('cp932',errors='replace'));print(out.stderr.decode('cp932',errors='replace'));assert out.returncode==0
