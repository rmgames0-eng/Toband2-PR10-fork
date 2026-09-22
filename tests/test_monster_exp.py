"""Compile the production EXP function and check the Win32 overflow regression.

Run: python tests/test_monster_exp.py
Optional argument: an older xtra2.c to demonstrate the regression.
"""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GCC = ROOT / 'tools/mingw32/bin/gcc.exe'
source = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / 'src/xtra2.c'
text = source.read_bytes().decode('euc_jp')
function = text[text.index('static s32b get_exp_from_mon_aux('):text.index('\nvoid get_exp_from_mon(')]
tables = (ROOT / 'src/tables.c').read_bytes().decode('euc_jp')
energy = tables[tables.index('byte extract_energy[200]'):]
energy = energy[:energy.index('};') + 2]
program = '''
#include "angband.h"
monster_race races[1], *r_info = races;
monster_special specials[1], *ms_info = specials;
s16b dun_level = 1;
bool ambush_flag = 0;
''' + energy + '\n' + function + r'''
int main(void)
{
    monster_type m = {0};
    int levels[] = {1, 25, 30, 48};
    long expected[] = {180000, 20000, 16875, 10800};
    long control[] = {144000, 16000, 13500, 8640};
    int i, multiplier, failures = 0, cases = 0;
    /* Lesser kraken: r_info.txt #740, default speed and special modifier. */
    races[0].level = 54;
    races[0].speed = m.mspeed = 120;
    races[0].hdice = 150;
    races[0].hside = 22;
    races[0].flags1 = RF1_FORCE_MAXHP;
    m.max_maxhp = 3300;
    if (sizeof(s32b) != 4 || sizeof(u64b) != 8) return 2;
    for (multiplier = 1; multiplier <= 2; multiplier++)
    {
        ms_info[0].exp_perc = 100 * multiplier;
        for (i = 0; i < 4; i++)
        {
            u32b fraction = 0;
            long actual, want = expected[i] * multiplier;
            races[0].mexp = 20000;
            actual = get_exp_from_mon_aux(3300, &m, levels[i], &fraction);
            cases++;
            if (actual != want || fraction != 0)
            {
                printf("FAIL level=%d modifier=%d: got %ld + %lu/65536, expected %ld\n",
                       levels[i], multiplier * 100, actual, (unsigned long)fraction, want);
                failures++;
            }
        }
    }
    /* A numerator below the signed 32-bit boundary must stay unchanged. */
    races[0].mexp = 16000;
    ms_info[0].exp_perc = 100;
    for (i = 0; i < 4; i++)
    {
        u32b fraction = 0;
        long actual = get_exp_from_mon_aux(3300, &m, levels[i], &fraction);
        cases++;
        if (actual != control[i] || fraction != 0) failures++;
    }
    /* Surface penalty plus a fractional carry from an earlier attack. */
    {
        u32b fraction = 32768;
        long actual;
        races[0].mexp = 20000;
        dun_level = 0;
        actual = get_exp_from_mon_aux(3300, &m, 30, &fraction);
        cases++;
        if (actual != 4219 || fraction != 16384) failures++;
        dun_level = 1;
        /* Reproducer penalty: one group of 400 kills divides EXP by four. */
        races[0].flags2 = RF2_MULTIPLY;
        races[0].r_pkills = 400;
        fraction = 0;
        actual = get_exp_from_mon_aux(3300, &m, 30, &fraction);
        cases++;
        if (actual != 4218 || fraction != 49152) failures++;
        races[0].flags2 = 0;
        /* Unique monsters ignore the special percentage multiplier. */
        races[0].flags1 |= RF1_UNIQUE;
        ms_info[0].exp_perc = 200;
        fraction = 0;
        actual = get_exp_from_mon_aux(3300, &m, 30, &fraction);
        cases++;
        if (actual != 16875 || fraction != 0) failures++;
    }
    printf("%d cases, %d failures\n", cases, failures);
    return failures ? 1 : 0;
}
'''
with tempfile.TemporaryDirectory(prefix='toband-exp-') as tmp:
    tmp = Path(tmp)
    cfile, exe = tmp / 'test.c', tmp / 'test.exe'
    cfile.write_text(program, encoding='ascii')
    for optimization in ('-O0', '-O2'):
        print('Testing ' + optimization, flush=True)
        subprocess.run([str(GCC), '-std=gnu89', optimization, '-DWIN32', '-I',
                        str(ROOT / 'src'), str(cfile), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)

