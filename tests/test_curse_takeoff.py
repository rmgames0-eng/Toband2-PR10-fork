"""Check the production curse-removal gate for every class and curse strength."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "src/cmd3.c").read_bytes().decode("euc_jp")
line = next(line.strip() for line in source.splitlines()
            if "TRC_PERMA_CURSE" in line and "CLASS_TERRORKNIGHT" in line)
condition = line[len("if ("):-1]
program = r'''#include "angband.h"
#include <assert.h>
static player_type player;
player_type *p_ptr=&player;
static int blocked(object_type *o_ptr) { return CONDITION; }
int main(void) {
    object_type item;
    int job, strength;
    memset(&item,0,sizeof(item));
    for(job=0;job<MAX_CLASS;job++) for(strength=0;strength<3;strength++) {
        int expected;
        p_ptr->pclass=job;
        item.curse_flags=TRC_CURSED;
        if(strength==1) item.curse_flags|=TRC_HEAVY_CURSE;
        if(strength==2) item.curse_flags|=TRC_PERMA_CURSE;
        expected=strength==2 || (job!=CLASS_TERRORKNIGHT && job!=CLASS_RELICSKNIGHT);
        assert(!!blocked(&item)==expected);
    }
    puts("All classes: ordinary, heavy and permanent curse gates passed");
    return 0;
}
'''.replace("CONDITION",condition)
with tempfile.TemporaryDirectory() as tmp:
    src=Path(tmp)/"test.c"
    exe=Path(tmp)/"test.exe"
    src.write_text(program)
    subprocess.run([str(ROOT/"tools/mingw32/bin/gcc.exe"),"-std=gnu89","-O2",
                    "-DWIN32","-DJP","-DSJIS","-I",str(ROOT/"src"),str(src),"-o",str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
