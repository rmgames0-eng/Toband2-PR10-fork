"""Execute production ore-ammo creation with inventory/UI test doubles."""
from pathlib import Path
import subprocess, tempfile, sys
ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'src/object2.c').read_bytes().decode('euc_jp')
function = source[source.index('static void make_ammo_from_ore'):source.index('static bool item_tester_hook_normal_ammo')]
if len(sys.argv) > 1:
    function = Path(sys.argv[1]).read_text(encoding='utf-8')
program = r'''
#include <assert.h>
#include <string.h>
#include <stddef.h>
#define TRUE 1
#define FALSE 0
#define MAX_NLEN 100
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX_STACK_SIZE 100
#define AMMO_MATERIAL_SV_MITHRIL 1
#define AMMO_MATERIAL_SV_ADAMANTITE 2
#define SV_GOLD_MITHRIL 1
#define SV_GOLD_ADAMANTITE 2
#define TV_GOLD 0
#define TV_BULLET 1
#define TV_ROUND 2
#define TV_SHELL 3
#define OD_NAME_ONLY 0
#define PU_GOLD 1
#define PR_GOLD 1
#define IDENT_MENTAL 1
#define msg_format(...) ((void)0)
typedef struct { int cost, name; } object_kind;
typedef struct { int number, weight, discount, ident; } object_type;
typedef struct { long au[3]; int update, redraw; } player_type;
static player_type player, *p_ptr=&player;
static object_kind k_info[4]={{0,0},{3,0},{7,0}};
static char k_name[]="ore";
static int energy_use, py, px, phase, material, kind, amount, capacity;
static int carried, dropped, inscriptions, carry_calls, drop_calls, abort_at;
static int get_com(const char *s,char *c,int b) {
    ++phase; if (phase==abort_at) return 0;
    *c=phase==1 ? (material==1?'M':'A') : " BRS"[kind]; return 1;
}
static int lookup_kind(int t,int s) { return t==TV_GOLD?s:t; }
static void object_prep(object_type *o,int k) { memset(o,0,sizeof(*o)); o->weight=k; }
static void object_desc(char *s,object_type *o,int f) { strcpy(s,"ammo"); }
static int get_quantity(void *p,int max) { assert(amount<=max); return amount; }
static void object_aware(object_type *o) {}
static void object_known(object_type *o) {}
static int inven_carry_okay(object_type *o) { return capacity; }
static int inven_carry(object_type *o) { ++carry_calls; carried+=o->number; return 4; }
static int drop_near(object_type *o,int c,int y,int x) { ++drop_calls; dropped+=o->number; return 7; }
static void autopick_alter_item(int i,int b) { assert(i==(capacity?4:-7)); ++inscriptions; }
''' + function + r'''
int main(void) {
 int repeat, initial, expected, mode;
 for(material=1;material<=2;++material)
 for(kind=1;kind<=3;++kind)
 for(capacity=1;capacity>=0;--capacity)
 for(amount=1;amount<=99;amount+=49) {
   player.au[material]=1000000; carried=13; dropped=0;
   carry_calls=drop_calls=inscriptions=0; abort_at=0;
   for(repeat=1;repeat<=4;++repeat) {
     initial=player.au[material]; phase=0; energy_use=0;
     make_ammo_from_ore(); expected=amount*k_info[material].cost*kind*2;
     assert(player.au[material]==initial-expected);
     assert(carried+dropped==13+amount*repeat);
     assert(carry_calls+drop_calls==repeat);
     assert(inscriptions==repeat && energy_use==100);
   }
 }
 for(mode=0;mode<4;++mode) {
   material=1;kind=1;capacity=1;amount=mode==0?0:1;
   abort_at=mode==1?1:mode==2?2:0;
   player.au[1]=mode==3?0:1000; initial=player.au[1];
   phase=carried=dropped=carry_calls=drop_calls=inscriptions=energy_use=0;
   make_ammo_from_ore();
   assert(player.au[1]==initial && !carried && !dropped && !energy_use);
 }
 return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    c = Path(tmp)/'test.c'
    c.write_text(program, encoding='utf-8')
    for opt in ('-O0','-O2'):
        exe = Path(tmp)/'test.exe'
        subprocess.run([str(ROOT/'tools/mingw32/bin/gcc.exe'),'-std=gnu99',opt,str(c),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
        print(opt + ': 144 repeated creation checks and 4 cancellation/resource checks passed')
