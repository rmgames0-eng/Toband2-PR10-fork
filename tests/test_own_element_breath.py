"""Exercise production breath cases with projection replaced by a recorder."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
def function(text,name):
 a=text.index(name);a=text.rfind('\n',0,a)+1;op=text.index('{',a);depth=1;i=op+1
 while depth:
  depth+=(text[i]=='{')-(text[i]=='}');i+=1
 return text[a:i]
core=(ROOT/'src/xtra2.c').read_text(encoding='euc_jp')
s='#include "angband.h"\n#include <assert.h>\n'
s+=r"""
int py=1,px=1; bool mon_fight;
static int seen_type,seen_damage,calls;
void disturb(int a,int b) {} void sound(int n) {} void msg_format(cptr fmt,...) {}
s32b randnor(int mean,int stand) { return mean; }
static void record(int typ,int damage) { seen_type=typ;seen_damage=damage;calls++; }
static void breath(int y,int x,int m,int typ,int dam,int rad,bool br,bool nr) { assert(br && !nr && rad==0);record(typ,dam); }
static void monst_breath_monst(int m,int y,int x,int typ,int dam,int rad,bool br,bool nr) { assert(br && !nr && rad==0);record(typ,dam); }
"""
s+=function(core,'s16b get_opposite_elem(')+'\n'+function(core,'s16b get_cur_melem(')+'\n'
for n in (1,2):
 text=(ROOT/f'src/mspells{n}.c').read_text(encoding='euc_jp');a=text.index('/* RFA_BR_PURE_ELEM */');b=text.index('/* RFA_PURE_ELEM_BEAM */',a)
 s+=f'bool cast{n}(monster_type *m_ptr) {{ int x=1,y=1,fx=1,fy=1,m_idx=1,dam; bool known=TRUE,see_either=TRUE,blind=FALSE; cptr m_name="caster",t_name="target"; switch(313) {{\n'+text[a:b]+'\n} return TRUE;}\n'
s+=r"""
int main(void) {
 monster_type m;int e,n,types[]={GF_PURE_FIRE,GF_PURE_AQUA,GF_PURE_EARTH,GF_PURE_WIND};bool (*cast)(monster_type*);
 assert(RFA_BREATH_MASK & RFA_BR_PURE_ELEM);assert(RFA_ATTACK_MASK & RFA_BR_PURE_ELEM);assert(RFA_NOMAGIC_MASK & RFA_BR_PURE_ELEM);
 for(n=1;n<=2;n++) {
  cast=n==1?cast1:cast2;
  for(e=ELEM_FIRE;e<=ELEM_WIND;e++) {
   memset(&m,0,sizeof(m));m.elem=e;m.hp=9000;calls=0;
   assert(cast(&m) && calls==1 && seen_type==types[e] && seen_damage==700 && m.energy_need==200);
   m.hp=300;m.mtimed[MTIMED_OPPOSITE_ELEM]=1;calls=0;
   assert(cast(&m) && calls==1 && seen_type==types[get_opposite_elem(e)] && seen_damage==100);
  }
  m.elem=NO_ELEM;calls=0;assert(!cast(&m) && calls==0);
 }
 puts("Own-element breath: four elements, reversal, HP scaling, cap, recovery and no-element passed for both targets");return 0;
}
"""
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);c=p/'test.c';exe=p/'test.exe';c.write_bytes(s.encode('euc_jp'))
 subprocess.run([str(ROOT/'tools/mingw32/bin/gcc.exe'),'-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src'),str(c),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
