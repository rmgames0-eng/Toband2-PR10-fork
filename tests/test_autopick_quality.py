"""Exercise quality rules and real floor sensing/pickup without user saves."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build/party'
GCC = ROOT / 'tools/mingw32/bin/gcc.exe'
FLAGS = ['-std=gnu89', '-O2', '-DWIN32', '-DJP', '-DSJIS',
         '-finput-charset=EUC-JP', '-fexec-charset=CP932', '-I', str(ROOT / 'src')]
TEST = r'''
#include <assert.h>
void test_autopick_quality(void)
{
    autopick_type e;
    object_type o;
    int rule, feel, mode, k, slot, sense;
    cptr line;
    const char *rules[] = {"上質以上のアイテム", "高級品以上のアイテム"};
    k = lookup_kind(TV_SWORD, SV_DAGGER);
    assert(k);
    for (rule = 0; rule < 2; rule++)
    {
        assert(autopick_new_entry(&e, rules[rule], TRUE));
        assert(!e.name[0]);
        line = autopick_line_from_entry(&e);
        assert(!strcmp(line, rules[rule]));
        string_free(line);
        for (feel = FEEL_NONE; feel <= FEEL_SPECIAL; feel++)
        {
            object_prep(&o, k);
            o.ident = IDENT_SENSE; o.feeling = feel;
            /* Hidden true quality must not influence the reported feeling. */
            o.name2 = 1; o.to_h = 9;
            assert(!!is_autopick_aux(&o, &e, "") ==
                (feel == FEEL_SPECIAL || feel == FEEL_EXCELLENT ||
                 (!rule && feel == FEEL_GOOD)));
            o.ident = 0;
            assert(!is_autopick_aux(&o, &e, ""));
        }
        for (mode = 0; mode < 4; mode++)
        {
            object_prep(&o, k); o.ident = IDENT_KNOWN;
            if (mode == 1) o.to_h = 1;
            if (mode == 2) o.name2 = 1;
            if (mode == 3) o.name1 = 1;
            o.inscription = quark_add("keep");
            assert(!!is_autopick_aux(&o, &e, "") == (mode >= rule + 1));
            o.ident |= IDENT_BROKEN;
            assert(!is_autopick_aux(&o, &e, ""));
        }
        autopick_free_entry(&e);
    }
    assert(autopick_new_entry(&e, "未鑑定のアイテム", TRUE));
    for (feel = FEEL_NONE; feel <= FEEL_TRIED; feel++)
    {
        object_prep(&o, k); o.ident = IDENT_SENSE; o.feeling = feel;
        assert(is_autopick_aux(&o, &e, ""));
        o.ident |= IDENT_KNOWN;
        assert(!is_autopick_aux(&o, &e, ""));
    }
    autopick_free_entry(&e);
    for (rule = 0; rule < 2; rule++)
    {
        assert(autopick_new_entry(&e, rule ? "未鑑定の高級品以上の武器" : "未鑑定の上質以上の武器", TRUE));
        for (feel = FEEL_GOOD; feel <= FEEL_SPECIAL; feel++)
        {
            object_prep(&o, k); o.ident = IDENT_SENSE; o.feeling = feel;
            assert(!!is_autopick_aux(&o, &e, "") == (!rule || feel >= FEEL_EXCELLENT));
            o.ident |= IDENT_KNOWN;
            assert(!is_autopick_aux(&o, &e, ""));
        }
        autopick_free_entry(&e);
    }

    assert(autopick_new_entry(&e, "未鑑定の不明な品質の武器", TRUE));
    assert(!e.name[0]);
    line = autopick_line_from_entry(&e);
    assert(!strcmp(line, "未鑑定の不明な品質の武器")); string_free(line);
    for (feel = FEEL_NONE; feel <= FEEL_TRIED; feel++) {
        object_prep(&o, k); o.ident = IDENT_SENSE; o.feeling = feel;
        o.name2 = 1; o.to_h = 99;
        assert(!!is_autopick_aux(&o, &e, "") == (feel == FEEL_NONE));
        o.ident |= IDENT_KNOWN;
        assert(!is_autopick_aux(&o, &e, ""));
    }
    object_prep(&o, k); o.name2 = 1; o.to_h = 99;
    assert(is_autopick_aux(&o, &e, ""));
    object_prep(&o, lookup_kind(TV_FOOD, SV_FOOD_RATION));
    assert(!is_autopick_aux(&o, &e, ""));
    autopick_free_entry(&e);
    /* Real encounter with an unsensed floor item: sensing then pickup. */
    py = px = 10;
    p_ptr->confused = 0; p_ptr->grace = 0;
    p_ptr->skill_exp[SKILL_SENSE_ARMS] = SKILL_EXP_AVERAGE * 10;
    Rand_quick = TRUE; Rand_value = 12345;

    for (mode = 0; mode < 3; mode++) {
        init_autopick();
        assert(autopick_new_entry(&e, "未鑑定の不明な品質の武器", TRUE));
        autopick_list[max_autopick++] = e;
        memset(inventory, 0, sizeof(object_type) * INVEN_TOTAL);
        inven_cnt = 0; p_ptr->total_weight = 0;
        p_ptr->confused = mode == 0 ? 10 : 0;
        slot = o_pop(); assert(slot);
        object_prep(&o_list[slot], k);
        o_list[slot].iy = py; o_list[slot].ix = px;
        if (mode == 2) { o_list[slot].ident = IDENT_SENSE; o_list[slot].feeling = FEEL_GOOD; }
        cave[py][px].o_idx = slot;
        msg_flag = FALSE; autopick_pickup_items(&cave[py][px]);
        assert((inven_cnt == 1) == (mode == 0));
        if (mode) { assert(o_list[slot].ident & IDENT_SENSE); delete_object_idx(slot); }
    }
    p_ptr->confused = 0;
    puts("Unknown-quality rule: parsing, roundtrip, hidden data, identified/sensed exclusion and real floor pickup passed");
    for (sense = 0; sense < 2; sense++)
    for (rule = 0; rule < 2; rule++)
    {
        /* Novice with zero WIS selects LIGHT; average selects HEAVY. */
        p_ptr->stat_use[A_WIS] = 0;
        p_ptr->skill_exp[SKILL_SENSE_ARMS] = (sense ? SKILL_EXP_AVERAGE : SKILL_EXP_NOVICE) * 10;
        init_autopick();
        assert(autopick_new_entry(&e, rule ? "未鑑定の高級品以上の武器" : "未鑑定の上質以上の武器", TRUE));
        autopick_list[max_autopick++] = e;
        for (mode = 0; mode < 3; mode++)
        {
            memset(inventory, 0, sizeof(object_type) * INVEN_TOTAL);
            inven_cnt = 0; p_ptr->total_weight = 0;
            slot = o_pop(); assert(slot);
            object_prep(&o_list[slot], k);
            o_list[slot].iy = py; o_list[slot].ix = px;
            if (mode == 1) o_list[slot].to_h = 1;
            if (mode == 2) o_list[slot].name2 = 1;
            cave[py][px].o_idx = slot;
            assert(is_autopick(&o_list[slot]) == -1);
            msg_flag = FALSE;
            autopick_pickup_items(&cave[py][px]);
            assert((inven_cnt == 1) == (mode >= rule + 1));
            if (inven_cnt)
            {
                assert(inventory[0].ident & IDENT_SENSE);
                assert(!cave[py][px].o_idx);
            }
            else
            {
                assert(o_list[slot].ident & IDENT_SENSE);
                delete_object_idx(slot);
            }
        }
    }
    puts("Quality parsing/roundtrip, feelings, hidden properties, known quality, inscriptions and immediate floor pickup passed");
}
'''
with tempfile.TemporaryDirectory(prefix='toband-autopick-') as directory:
    tmp = Path(directory)
    src = tmp / 'autopick.c'
    src.write_bytes((ROOT / 'src/autopick.c').read_bytes() + TEST.encode('euc_jp'))
    obj = tmp / 'autopick.o'
    subprocess.run([str(GCC), *FLAGS, '-c', str(src), '-o', str(obj)], check=True)
    harness = (ROOT / 'tests/party_integration.c').read_text(encoding='euc_jp')
    harness = 'extern void test_autopick_quality(void);\n' + harness
    harness = harness.replace('    init_angband();',
                              '    init_angband();\n    test_autopick_quality();\n    return 0;', 1)
    source = tmp / 'test.c'
    source.write_bytes(harness.encode('euc_jp'))
    objects = [p for p in BUILD.glob('*.o') if p.stem not in ('main-win', 'angband', 'autopick')]
    exe = tmp / 'test.exe'
    subprocess.run([str(GCC), *FLAGS, str(source), str(obj), *map(str, objects),
                    '-o', str(exe), '-lwinmm', '-lcomdlg32', '-lgdi32'], check=True)
    result = subprocess.run([str(exe), (BUILD / 'lib').as_posix() + '/'],
                            cwd=tmp, capture_output=True, timeout=30)
    print(result.stdout.decode('cp932', errors='replace'), end='')
    if result.returncode:
        print(result.stderr.decode('cp932', errors='replace'))
        raise SystemExit(result.returncode)

