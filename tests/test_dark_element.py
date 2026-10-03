"""Dark Element data, fixed-MP powers, personal elements and v5 save migration."""
from pathlib import Path
import subprocess, tempfile, shutil, sys
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build/party'
GCC=ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h=(ROOT/'tests/party_integration.c').read_bytes().decode('euc_jp')
extra=r"""
extern void dark_element_power(int command);
static int observed_type, observed_damage, genocide_calls;
bool test_dark_genocide(int power, int player_cast)
{
    assert(power==300 && player_cast==TRUE);
    genocide_calls++;
    return mass_genocide(power,player_cast);
}
bool test_dark_project(int type, int damage)
{
    observed_type=type; observed_damage=damage;
    return project_hack(type, damage);
}
static void dark_element_tests(void)
{
    int e, i;
    player_class *c=&class_info[CLASS_DARK_ELEMENT];
    assert(c->c_exp==200 && c->c_mhp==15 && c->c_msp==4);
    assert(c->max_attacks==8);
    assert(c->c_flags & PCF_NO_CHANGE);
    assert(!memcmp(&s_info[CLASS_DARK_ELEMENT],&s_info[CLASS_LORD],sizeof(s_info[0])));
    for(e=0;e<MAX_REALM;e++) if ((e>=1 && e<=4)||e==6)
        assert(!memcmp(m_info[CLASS_DARK_ELEMENT].info[e],m_info[CLASS_LICH].info[e],sizeof(m_info[0].info[e])));
    p_ptr->pclass=CLASS_DARK_ELEMENT;
    party_rebind();
    for(i=0;i<4;i++) assert(!can_choose_class(CLASS_DARK_ELEMENT,i));
    for(e=ELEM_FIRE;e<=ELEM_WIND;e++) {
        p_ptr->pelem=p_ptr->celem=e;
        p_ptr->no_elem=p_ptr->opposite_pelem=0;
        calc_bonuses();
        assert(p_ptr->telepathy);
        assert(p_ptr->immune_fire==(e==ELEM_FIRE || e==ELEM_AQUA));
        assert(p_ptr->immune_cold==(e==ELEM_FIRE || e==ELEM_AQUA));
        assert(p_ptr->immune_acid==(e==ELEM_EARTH || e==ELEM_WIND));
        assert(p_ptr->immune_elec==(e==ELEM_EARTH || e==ELEM_WIND));
        p_ptr->opposite_pelem=5;calc_bonuses();
        assert(p_ptr->immune_fire==(e==ELEM_FIRE || e==ELEM_AQUA));
        assert(p_ptr->immune_acid==(e==ELEM_EARTH || e==ELEM_WIND));
    }
    p_ptr->no_elem=5;calc_bonuses();
    assert(!p_ptr->immune_fire && !p_ptr->immune_cold && !p_ptr->immune_acid && !p_ptr->immune_elec);
    p_ptr->no_elem=p_ptr->opposite_pelem=0;
    calc_bonuses();
    assert(p_ptr->resist_fear && p_ptr->reflect && p_ptr->telepathy);
    assert(p_ptr->sh_fire && p_ptr->sh_elec && p_ptr->sh_cold);
    {
        player_type saved=*p_ptr;
        bool old_easy=easy_band;
        s32b old_exp=p_ptr->exp;
        easy_band=FALSE;
        p_ptr->chp=p_ptr->mhp=3000;
        p_ptr->invuln=p_ptr->wraith_form=p_ptr->multishadow=0;
        p_ptr->infected=p_ptr->is_dead=0;
        p_ptr->ogre_equip=p_ptr->evil_equip=FALSE;
        p_ptr->immune_holy=FALSE;
        p_ptr->update=p_ptr->redraw=p_ptr->window=0;
        project(-1,0,py,px,500,GF_DARK,PROJECT_KILL|PROJECT_PLAYER|PROJECT_JUMP,MODIFY_ELEM_MODE_NONE);
        assert(p_ptr->chp==3000);
        project(-1,0,py,px,500,GF_NETHER,PROJECT_KILL|PROJECT_PLAYER|PROJECT_JUMP,MODIFY_ELEM_MODE_NONE);
        assert(p_ptr->chp==3000 && p_ptr->exp==old_exp);
        project(-1,0,py,px,500,GF_HOLY_FIRE,PROJECT_KILL|PROJECT_PLAYER|PROJECT_JUMP,MODIFY_ELEM_MODE_NONE);
        assert(p_ptr->chp==2750);
        *p_ptr=saved; easy_band=old_easy;
    }

    for(e=ELEM_FIRE;e<=ELEM_WIND;e++) {
        p_ptr->pelem=p_ptr->celem=e;
        p_ptr->no_elem=p_ptr->opposite_pelem=0;
        init_realm_table();
        assert(can_use_realm(REALM_DEATH));
        assert(can_use_realm(REALM_FIRE+e));
        for(i=ELEM_FIRE;i<=ELEM_WIND;i++) if(i!=e) assert(!can_use_realm(REALM_FIRE+i));
        assert(!can_use_realm(REALM_MAGERY) && !can_use_realm(REALM_HOLY));
        p_ptr->csp=19; p_ptr->chp=100;
        dark_element_power(1003);
        assert(p_ptr->csp==19 && p_ptr->chp==100 && !energy_use);
        p_ptr->csp=20;
        dark_element_power(1003);
        assert(p_ptr->csp==0 && energy_use==100);
        assert(observed_damage>=501 && observed_damage<=1000);
        assert(observed_type==(e==ELEM_FIRE ? GF_FIRE : e==ELEM_AQUA ? GF_COLD : e==ELEM_EARTH ? GF_ACID : GF_ELEC));
    }
    p_ptr->celem=p_ptr->pelem=ELEM_FIRE; p_ptr->csp=30;
    repeat_push(1002);
    keys="     "; key_index=0; msg_print(NULL);
    keys="d"; key_index=0;
    dark_element_power(1002);
    assert(p_ptr->pelem==ELEM_WIND && p_ptr->celem==ELEM_WIND);
    assert(p_ptr->csp==30 && energy_use==100 && can_use_realm(REALM_WIND));
    keys="     "; key_index=0; msg_print(NULL);
    keys="\033"; key_index=0;
    dark_element_power(1001);
    assert(p_ptr->csp==30 && !energy_use);
    p_ptr->no_elem=10; init_realm_table();
    dark_element_power(1003);
    assert(p_ptr->csp==30 && !energy_use);
    auto_more=TRUE;keys="                    ";key_index=0;msg_flag=FALSE;
    p_ptr->csp=19;dark_element_power(1005);
    assert(p_ptr->csp==19 && !energy_use && !genocide_calls);
    p_ptr->csp=20;p_ptr->confused=1;dark_element_power(1005);
    assert(p_ptr->csp==20 && !energy_use && !genocide_calls);
    p_ptr->confused=0;dark_element_power(1005);
    assert(p_ptr->csp==0 && energy_use==100 && genocide_calls==1);
    p_ptr->no_elem=0; init_realm_table();
    party_capture();
#ifndef TEST_PARTY_V5
    p_ptr->class_sp[CLASS_DARK_ELEMENT][49]=1234;
    p_ptr->cexp_info[CLASS_DARK_ELEMENT].clev=50;
    p_ptr->cexp_info[CLASS_DARK_ELEMENT].max_clev=50;
    assert(save_player()); party_reset(); assert(load_player());
    assert(p_ptr->pclass==CLASS_DARK_ELEMENT && p_ptr->pelem==ELEM_WIND);
    assert(p_ptr->class_sp[CLASS_DARK_ELEMENT][49]==1234);
#endif
    puts("Dark Element: class/magic data, elemental realm switching, MP costs, cancellation and save/load passed");
}
"""
h=h.replace('int main(int argc,char **argv)',extra+'\nint main(int argc,char **argv)',1)
h=h.replace('    display_regressions();','    dark_element_tests();\n    return 0;\n    display_regressions();',1)
with tempfile.TemporaryDirectory(prefix='toband-dark-element-') as directory:
    tmp=Path(directory)
    shutil.copytree(BUILD/'lib',tmp/'lib',ignore=shutil.ignore_patterns('*.raw','save'))
    for name in ('misc.txt','p_class.txt','s_info.txt','m_info.txt'):
        (tmp/'lib/edit'/name).write_bytes((ROOT/'lib/edit'/name).read_bytes().decode('euc_jp').encode('cp932'))
    (tmp/'lib/save').mkdir(exist_ok=True)
    (tmp/'test.c').write_bytes(h.encode('euc_jp'))
    racial=(ROOT/'src/racial.c').read_bytes().decode('euc_jp').replace('static void dark_element_power(', 'void dark_element_power(',1)
    racial=racial.replace('mass_genocide(300, TRUE);','test_dark_genocide(300, TRUE);')
    racial=racial.replace('project_hack(typ, 500 + randint1(500));','test_dark_project(typ, 500 + randint1(500));')
    racial=racial.replace('#include "party.h"','#include "party.h"\nextern bool test_dark_project(int type, int damage);\nextern bool test_dark_genocide(int power, int player_cast);',1)
    (tmp/'racial.c').write_bytes(racial.encode('euc_jp'))
    objects=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband','racial')]
    old=(ROOT/'src/save.c').read_bytes().decode('euc_jp').replace('wr_byte(PARTY_SAVE_VERSION);','wr_byte(5);').replace('    wr_u16b(MAX_CLASS); /* Party v6: serialized class count. */','')
    old=old.replace('        /* Party v7: personal named technique. */\n        wr_string(member->player.temple_tech_name);\n        wr_u32b(member->player.temple_tech_effects);\n','')
    start=old.index('static void wr_party(void)');end=old.index('\n}\n',start)+3
    old=old[:start]+old[start:end].replace('i < (count)','i < ((count) == MAX_CLASS ? 39 : ((count) == MAX_CLASS * PY_MAX_LEVEL ? 39 * PY_MAX_LEVEL : (count)))').replace('j < MAX_CLASS','j < 39')+old[end:]
    (tmp/'save_v5.c').write_bytes(old.encode('euc_jp'))
    subprocess.run([str(GCC),*FLAGS,'-c',str(tmp/'save_v5.c'),'-o',str(tmp/'save_v5.o')],check=True)
    for legacy in ((False,) if "--current-only" in sys.argv else (False,True)):
        exe=tmp/'test.exe'
        linked=[tmp/'save_v5.o' if legacy and p.stem=='save' else p for p in objects]
        subprocess.run([str(GCC),*FLAGS,*(['-DTEST_PARTY_V5'] if legacy else []),str(tmp/'test.c'),str(tmp/'racial.c'),*map(str,linked),'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
        result=subprocess.run([str(exe),(tmp/'lib').as_posix()+'/'],cwd=tmp,capture_output=True,timeout=45)
        print(result.stdout.decode('cp932',errors='replace'),end='')
        if result.returncode:
            print(result.stderr.decode('cp932',errors='replace'))
            raise SystemExit(result.returncode)
