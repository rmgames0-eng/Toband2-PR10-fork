"""Umbra damage, resistance bypass and Hand-of-Doom saving throw."""
from pathlib import Path
import subprocess,tempfile,shutil
ROOT=Path(__file__).resolve().parents[1];BUILD=ROOT/'build/party';GCC=ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h=(ROOT/'tests/party_integration.c').read_bytes().decode('euc_jp')
extra=r"""
static void death_zero_tests(void)
{
    int hp;
    static char spaces[20000];memset(spaces,32,sizeof(spaces)-1);keys=spaces;key_index=0;auto_more=TRUE;
    p_ptr->pclass=CLASS_SOLDIER;p_ptr->prace=RACE_HUMAN;party_rebind();
    p_ptr->is_dead=0;p_ptr->mhp=100;p_ptr->infected=0;
    p_ptr->evil_equip=p_ptr->ogre_equip=FALSE;
    p_ptr->mermaid_in_water=FALSE;easy_band=TRUE;
    p_ptr->invuln=p_ptr->wraith_form=p_ptr->multishadow=100;
    for(hp=1;hp<=50;hp++) {
        p_ptr->chp=hp;p_ptr->chp_frac=123;
        assert(diablo_death("Death"));
        assert(p_ptr->chp==0 && p_ptr->chp_frac==0 && !p_ptr->is_dead);
        assert(!diablo_death("Death") && p_ptr->chp==0 && !p_ptr->is_dead);
    }
    p_ptr->chp=51;assert(!diablo_death("Death") && p_ptr->chp==51);
    p_ptr->mhp=99;p_ptr->chp=50;assert(!diablo_death("Death"));
    p_ptr->chp=49;assert(diablo_death("Death") && p_ptr->chp==0 && !p_ptr->is_dead);
    p_ptr->chp=-1;assert(!diablo_death("Death"));
    puts("Death: zero HP without death, no effect at zero, half-health boundary and protection bypass passed");
}
"""
h=h.replace('int main(int argc,char **argv)',extra+'\nint main(int argc,char **argv)',1)
h=h.replace('    display_regressions();','    death_zero_tests();\n    return 0;\n    display_regressions();',1)
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
