"""Exercise production EXP addition and weapon-bit boundaries on Win32."""
from pathlib import Path
import subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/effects.c').read_bytes().decode('euc_jp')
def function(sig):
    start=s.index(sig); pos=s.index('{',start); depth=1; end=pos+1
    while depth:
        depth+=(s[end]=='{')-(s[end]=='}'); end+=1
    return s[start:end]
code=r'''#include "angband.h"
#include <assert.h>
static player_type player;
player_type *p_ptr=&player;
void check_class_experience(void) {
    p_ptr->cexp_info[0].cexp=MAX(0,MIN(PY_MAX_EXP,p_ptr->cexp_info[0].cexp));
    p_ptr->cexp_info[0].max_cexp=MAX(0,MIN(PY_MAX_EXP,p_ptr->cexp_info[0].max_cexp));
}
void check_racial_experience(void) {
    p_ptr->exp=MAX(0,MIN(PY_MAX_EXP,p_ptr->exp));
    p_ptr->max_exp=MAX(0,MIN(PY_MAX_EXP,p_ptr->max_exp));
}
'''
if 'static s32b bounded_exp_sum(' in s: code+=function('static s32b bounded_exp_sum(')
code+=function('void gain_class_exp(')+function('void gain_racial_exp(')
birth=(ROOT/'src/birth.c').read_bytes().decode('euc_jp')
a=birth.index('if (((race_info[i].r_flags')
b=birth.index('continue;',a)
condition=birth[a:b].strip()[len('if ('):-1].strip()
code+='static player_race races[1]; player_race *race_info=races;\n'
code+='static int excluded_race(void) { int i=0; return '+condition+'; }\n'
code+=r'''
int main(void) {
    volatile int wt;
    int i;
    s32b values[]={0,1,PY_MAX_EXP-1,PY_MAX_EXP};
    s32b awards[]={0,1,200,PY_MAX_EXP,2147483647L};
    int v,a;
    for(v=0;v<4;v++) for(a=0;a<5;a++) {
        s32b expected=(s32b)MIN((u64b)PY_MAX_EXP,(u64b)values[v]+awards[a]);
        memset(&player,0,sizeof(player));
        player.exp=player.cexp_info[0].cexp=values[v];
        player.max_exp=player.cexp_info[0].max_cexp=PY_MAX_EXP;
        gain_class_exp(awards[a]); gain_racial_exp(awards[a]);
        assert(player.exp==expected && player.cexp_info[0].cexp==expected);
    }
    player.exp=player.cexp_info[0].cexp=200;
    gain_class_exp(-100); gain_racial_exp(-100);
    assert(player.exp==200 && player.cexp_info[0].cexp==200);
    for(i=-1;i<=MAX_WT+1;i++) {
        wt=i;
        assert(weapon_type_bit(wt)==((i>0 && i<MAX_WT) ? (1UL<<(i-1)):0UL));
    }
    for(i=0;i<256;i++) {
        races[0].r_flags=i;
        player.psex=SEX_MALE;
        assert(!!excluded_race()==!!(i&(PRF_SEX_FEMALE|PRF_EVOLUTION)));
        player.psex=SEX_FEMALE;
        assert(!!excluded_race()==!!(i&(PRF_SEX_MALE|PRF_EVOLUTION)));
    }
    puts("EXP, weapon-type and race-flag boundaries passed");
    return 0;
}
'''
with tempfile.TemporaryDirectory() as temp:
    path=Path(temp); (path/'test.c').write_text(code)
    for opt in ('-O0','-O2'):
        subprocess.run([str(ROOT/'tools/mingw32/bin/gcc.exe'),'-std=gnu89',opt,'-DWIN32','-DJP','-DSJIS','-I',str(ROOT/'src'),str(path/'test.c'),'-o',str(path/'test.exe')],check=True)
        subprocess.run([str(path/'test.exe')],check=True)
