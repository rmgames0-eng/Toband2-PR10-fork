"""Umbra damage, resistance bypass and Hand-of-Doom saving throw."""
from pathlib import Path
import subprocess,tempfile,shutil
ROOT=Path(__file__).resolve().parents[1];BUILD=ROOT/'build/party';GCC=ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h=(ROOT/'tests/party_integration.c').read_bytes().decode('euc_jp')
extra=r"""
static void umbra_tests(void)
{
    int caster,target,pass;
    monster_type *m;
    static char spaces[20000];memset(spaces,32,sizeof(spaces)-1);keys=spaces;key_index=0;
    assert(r_info[MON_ASMODE].flags6 & RF6_SPECIAL);
    wipe_m_list();cur_hgt=cur_wid=9;py=px=1;
    for(pass=0;pass<9;pass++){int x;for(x=0;x<9;x++)cave[pass][x].feat=(pass==0||x==0||pass==8||x==8)?FEAT_PERM_SOLID:FEAT_FLOOR;}
    caster=m_pop();m=&m_list[caster];m->r_idx=m->ap_r_idx=MON_ASMODE;
    m->fy=m->fx=3;m->hp=m->maxhp=m->max_maxhp=42000;cave[3][3].m_idx=caster;
    p_ptr->race_hp[0]=20000;
    easy_band=FALSE;Rand_quick=TRUE;Rand_value=127399;
    p_ptr->pclass=CLASS_SOLDIER;p_ptr->prace=RACE_HUMAN;party_rebind();
    for(pass=0;pass<5;pass++) {
        p_ptr->chp=p_ptr->mhp=10000;p_ptr->is_dead=0;
        p_ptr->invuln=p_ptr->wraith_form=p_ptr->multishadow=0;
        p_ptr->infected=0;p_ptr->evil_equip=p_ptr->ogre_equip=FALSE;
        p_ptr->update=p_ptr->redraw=p_ptr->window=0;
        p_ptr->blind=0;p_ptr->stoning=pass==4?42:0;
        p_ptr->resist_blind=p_ptr->resist_stone=TRUE;
        p_ptr->resist_dark=pass==2;
        p_ptr->skill_sav=pass==1?200:0;
        p_ptr->pclass=pass==3?CLASS_DARK_ELEMENT:CLASS_SOLDIER;party_rebind();
        project(caster,0,py,px,500,GF_UMBRA,PROJECT_KILL|PROJECT_PLAYER|PROJECT_JUMP,MODIFY_ELEM_MODE_NONE);
        assert(p_ptr->blind>=3 && p_ptr->blind<=7);
        assert(p_ptr->stoning==(pass==1?0:pass==4?42:1));
        assert(!p_ptr->is_dead);
        if(pass==2)assert(10000-p_ptr->chp>=181 && 10000-p_ptr->chp<=250);
        else assert(p_ptr->chp==(pass==3?10000:9500));
    }
    p_ptr->blind=p_ptr->stoning=0;
    target=m_pop();m=&m_list[target];m->r_idx=m->ap_r_idx=685;
    m->fy=2;m->fx=3;m->hp=m->maxhp=m->max_maxhp=40000;cave[2][3].m_idx=target;
    assert(r_info[685].flags3 & RF3_NO_CONF);
    assert(r_info[685].flagsr & RFR_RES_STON);
    project(caster,0,2,3,500,GF_UMBRA,PROJECT_KILL|PROJECT_JUMP,MODIFY_ELEM_MODE_NONE);
    assert(m->hp<40000 && MON_CONFUSED(m)>0 && MON_STONING(m)==1);
    puts("Umbra: dark damage, blind/stone resistance bypass, saving success/failure, immunity and monster effects passed");
}
"""
h=h.replace('int main(int argc,char **argv)',extra+'\nint main(int argc,char **argv)',1)
h=h.replace('    display_regressions();','    umbra_tests();\n    return 0;\n    display_regressions();',1)
for name in ('mspells1','mspells2'):
    s=(ROOT/'src'/f'{name}.c').read_bytes().decode('euc_jp');a=s.index('case MON_ASMODE:');part=s[a:s.index('break;',a)]
    assert '400 + randint1(200)' in part and 'GF_UMBRA, dam, 0, FALSE, FALSE' in part
with tempfile.TemporaryDirectory(prefix='toband-umbra-') as d:
    tmp=Path(d);shutil.copytree(BUILD/'lib',tmp/'lib',ignore=shutil.ignore_patterns('*.raw','save'))
    (tmp/'lib/edit/r_info.txt').write_bytes((ROOT/'lib/edit/r_info.txt').read_bytes().decode('euc_jp').encode('cp932'))
    (tmp/'lib/save').mkdir(exist_ok=True);(tmp/'test.c').write_bytes(h.encode('euc_jp'))
    objs=[str(p) for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    subprocess.run([str(GCC),*FLAGS,str(tmp/'test.c'),*objs,'-o',str(tmp/'test.exe'),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(tmp/'test.exe'),(tmp/'lib').as_posix()+'/'],cwd=tmp,capture_output=True,timeout=45)
    print(result.stdout.decode('cp932',errors='replace'))
    if result.returncode:
        print(result.returncode);print(result.stderr.decode('cp932',errors='replace'));raise SystemExit(result.returncode)
