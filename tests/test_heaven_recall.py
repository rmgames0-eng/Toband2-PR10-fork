"""Exercise production recall gates with visited/unvisited Heaven and restrictions."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "src/spells3.c").read_bytes().decode("euc_jp")
chooser = source.split("static int choose_dungeon(cptr note)", 1)[1].split("if (d_info[i].final_guardian)", 1)[0]
filters = "\n".join(re.findall(r"if \([^\n]+\) continue;", chooser))
recall = source.split("bool recall_player(int turns)", 1)[1]
gate = recall.split("if (", 1)[1].split("\n\t{", 1)[0].strip()[:-1]
program = r'''
#include "angband.h"
#include <assert.h>
static player_type player;
player_type *p_ptr = &player;
static dungeon_info_type dungeons[32];
dungeon_info_type *d_info = dungeons;
static s16b depths[32];
s16b *max_dlv = depths;
static quest_type quests[2];
quest_type *quest = quests;
byte dungeon_type;
bool astral_mode;
u32b misc_event_flags;
static int selectable(int i) {
    int once;
    for (once=0; once<1; once++) {
        FILTERS
        return 1;
    }
    return 0;
}
static int blocked(void) { return GATE; }
int main(void) {
    int depth;
    dungeon_type=DUNGEON_HEAVEN;
    d_info[dungeon_type].flags1=DF1_CLOSED|DF1_NO_BACK|DF1_UPWARD;
    d_info[dungeon_type].mindepth=1000;
    d_info[dungeon_type].maxdepth=1100;
    for(depth=0;depth<=1100;depth++) {
        max_dlv[dungeon_type]=depth;
        assert(selectable(dungeon_type)==(depth>=1000));
    }
    assert(!blocked());
    p_ptr->inside_arena=1; assert(blocked()); p_ptr->inside_arena=0;
    astral_mode=1; assert(blocked()); astral_mode=0;
    p_ptr->inside_quest=1;
    quest[1].flags=QUEST_FLAG_NO_RECALL; assert(blocked());
    quest[1].flags=0; assert(!blocked()); p_ptr->inside_quest=0;
    dungeon_type=1;
    d_info[1].mindepth=1; d_info[1].maxdepth=100; max_dlv[1]=20;
    d_info[1].flags1=DF1_CLOSED;
    assert(blocked()); assert(!selectable(1));
    d_info[1].flags1=0;
    assert(!blocked()); assert(selectable(1));
    max_dlv[1]=0; assert(!selectable(1));
    puts("Heaven recall: first-entry, revisits, ordinary dungeons and restrictions passed");
    return 0;
}
'''.replace("FILTERS", filters).replace("GATE", gate)
with tempfile.TemporaryDirectory() as tmp:
    src = Path(tmp) / "test.c"
    exe = Path(tmp) / "test.exe"
    src.write_text(program)
    subprocess.run([str(ROOT / "tools/mingw32/bin/gcc.exe"), "-std=gnu89", "-O2",
                    "-DWIN32", "-DJP", "-DSJIS", "-I", str(ROOT / "src"),
                    str(src), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
