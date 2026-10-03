"""Check data flags, equipment-only class gates and I-command descriptions."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build/party'
GCC = ROOT / 'tools/mingw32/bin/gcc.exe'
FLAGS = ['-std=gnu89', '-O2', '-DWIN32', '-DJP', '-DSJIS',
         '-finput-charset=EUC-JP', '-fexec-charset=CP932', '-I', str(ROOT/'src')]
h = (ROOT/'tests/party_integration.c').read_text(encoding='euc_jp')
test = r'''
static void class_item_tests(void)
{
    int i, c, aid, flag, slot;
    object_type item;
    u32b flags[TR_FLAG_SIZE];
    FILE *out;
    char buf[8192];
    size_t n;
    astral_mode = FALSE;
    p_ptr->chp = 100; p_ptr->is_dead = FALSE;
    p_ptr->prace = 0; p_ptr->lev = p_ptr->max_plv = 30;
    party_reset();
    for (i = 0; i < A_MAX; i++) p_ptr->stat_max[i] = 238;
    for (c = 1; c < 2; c++) {
        int target = c ? CLASS_HIGHWITCH : CLASS_GENERAL;
        aid = c ? ART_PUMPKIN : ART_SONIC_BLADE;
        flag = c ? TR_CLASS_HIGHWITCH : TR_CLASS_GENERAL;
        slot = c ? INVEN_LITE : INVEN_RARM;
        p_ptr->pclass = c ? CLASS_WITCH : CLASS_KNIGHT;
        p_ptr->psex = c ? SEX_FEMALE : SEX_MALE;
        p_ptr->align[ALI_LNC] = c ? 0 : 100;
        memset(p_ptr->cexp_info, 0, sizeof(p_ptr->cexp_info));
        p_ptr->cexp_info[p_ptr->pclass].clev = c ? 40 : 30;
        p_ptr->cexp_info[p_ptr->pclass].max_clev = c ? 40 : 30;
        party_rebind(); party_capture();
        memset(inventory, 0, sizeof(object_type) * INVEN_TOTAL);
        object_prep(&item, lookup_kind(a_info[aid].tval, a_info[aid].sval));
        item.name1 = aid;
        object_flags(&item, flags);
        assert(have_flag(flags, flag));
        assert(!can_choose_class(target, CLASS_CHOOSE_MODE_NORMAL));
        inventory[0] = item;
        assert(!can_choose_class(target, CLASS_CHOOSE_MODE_NORMAL));
        object_wipe(&inventory[0]);
        inventory[slot] = item;
        if (c) {
            assert(!can_choose_class(target, CLASS_CHOOSE_MODE_NORMAL));
            object_prep(&inventory[INVEN_HEAD], lookup_kind(a_info[ART_DENEB].tval, a_info[ART_DENEB].sval));
            inventory[INVEN_HEAD].name1 = ART_DENEB;
            object_wipe(&inventory[slot]);
            assert(!can_choose_class(target, CLASS_CHOOSE_MODE_NORMAL));
            inventory[slot] = item;
        }
        /* Identification is not required. */
        assert(can_choose_class(target, CLASS_CHOOSE_MODE_NORMAL));
        p_ptr->cexp_info[p_ptr->pclass].clev--;
        assert(!can_choose_class(target, CLASS_CHOOSE_MODE_NORMAL));
        p_ptr->cexp_info[p_ptr->pclass].clev++;
        object_wipe(&inventory[slot]);
        /* A different item bearing the flag works; identity is not hard-coded. */
        object_prep(&inventory[INVEN_RIGHT], lookup_kind(TV_RING, SV_RING_PROTECTION));
        add_flag(inventory[INVEN_RIGHT].art_flags, flag);
        assert(can_choose_class(target, CLASS_CHOOSE_MODE_NORMAL));
        object_wipe(&inventory[INVEN_RIGHT]);
        assert(!can_choose_class(target, CLASS_CHOOSE_MODE_NORMAL));
        /* A reserve's equipment does not unlock the active actor's class. */
        party_capture(); party_count = 2;
        party_members[1] = party_members[0];
        party_members[1].equipment[slot-INVEN_RARM] = item;
        assert(!can_choose_class(target, CLASS_CHOOSE_MODE_NORMAL));
        party_count = 1;
        item.ident = IDENT_KNOWN | IDENT_MENTAL;
        out = tmpfile(); assert(out);
        assert(screen_object(&item, out, TRUE));
        rewind(out); n = fread(buf, 1, sizeof(buf)-1, out); buf[n] = 0; fclose(out);
        assert(strstr(buf, c ? "それはうぃっちへのクラスチェンジを可能とする。"
                             : "それはジェネラルへのクラスチェンジを可能とする。"));
    }

    memset(inventory, 0, sizeof(object_type) * INVEN_TOTAL);
    p_ptr->pclass = CLASS_SOLDIER; p_ptr->psex = SEX_MALE;
    p_ptr->align[ALI_LNC] = 100;
    memset(p_ptr->cexp_info, 0, sizeof(p_ptr->cexp_info));
    party_rebind(); party_capture();
    misc_event_flags = 0;
    chaos_frame[ETHNICITY_ZENOBIAN] = chaos_frame[ETHNICITY_LODIS] = 100;
    for (i = 1; i < max_r_idx; i++) r_info[i].max_num = 1;
    assert(can_choose_class(CLASS_WHITEKNIGHT, CLASS_CHOOSE_MODE_BLDGS));
    assert(can_choose_class(CLASS_TEMPLEKNIGHT, CLASS_CHOOSE_MODE_BLDGS));
    chaos_frame[ETHNICITY_ZENOBIAN] = 99;
    assert(!can_choose_class(CLASS_WHITEKNIGHT, CLASS_CHOOSE_MODE_BLDGS));
    chaos_frame[ETHNICITY_ZENOBIAN] = 100;
    chaos_frame[ETHNICITY_LODIS] = 99;
    assert(!can_choose_class(CLASS_TEMPLEKNIGHT, CLASS_CHOOSE_MODE_BLDGS));
    chaos_frame[ETHNICITY_LODIS] = 100;
    p_ptr->pclass=CLASS_KNIGHT; party_rebind();
    p_ptr->cexp_info[CLASS_KNIGHT].clev=p_ptr->cexp_info[CLASS_KNIGHT].max_clev=30;
    assert(can_choose_class(CLASS_GENERAL,CLASS_CHOOSE_MODE_NORMAL));
    p_ptr->cexp_info[CLASS_KNIGHT].clev=29;
    assert(!can_choose_class(CLASS_GENERAL,CLASS_CHOOSE_MODE_NORMAL));
    p_ptr->pclass=CLASS_SOLDIER; party_rebind();
    memset(p_ptr->cexp_info,0,sizeof(p_ptr->cexp_info));

    memset(inventory, 0, sizeof(object_type) * INVEN_TOTAL);
    p_ptr->align[ALI_LNC] = p_ptr->align[ALI_GNE] = -100;
    assert(!can_choose_class(CLASS_RELICSKNIGHT, CLASS_CHOOSE_MODE_NORMAL));
    for (i = 0; i < 4; i++) {
        int artifacts[] = {ART_OGRE_HELM,ART_OGRE_ARMOR,ART_OGRE_SHIELD,ART_OGRE_BLADE};
        aid = artifacts[i];
        object_prep(&item, lookup_kind(a_info[aid].tval, a_info[aid].sval));
        item.name1 = aid;
        object_flags(&item, flags);
        assert(have_flag(flags, TR_CLASS_RELICSKNIGHT));
        slot = wield_slot(&item);
        assert(slot >= INVEN_RARM && slot < INVEN_TOTAL);
        inventory[0] = item;
        assert(!can_choose_class(CLASS_RELICSKNIGHT, CLASS_CHOOSE_MODE_NORMAL));
        object_wipe(&inventory[0]); inventory[slot] = item;
        party_count = 1;
        assert(!can_choose_class(CLASS_RELICSKNIGHT, CLASS_CHOOSE_MODE_NORMAL));
        party_capture(); party_count = 3;
        party_members[1] = party_members[0]; party_members[2] = party_members[0];
        party_members[1].dead = party_members[2].dead = TRUE;

        assert(can_choose_class(CLASS_RELICSKNIGHT, CLASS_CHOOSE_MODE_NORMAL));
        p_ptr->align[ALI_GNE] = 100;
        assert(!can_choose_class(CLASS_RELICSKNIGHT, CLASS_CHOOSE_MODE_NORMAL));
        p_ptr->align[ALI_GNE] = -100;
        party_capture(); party_count = 2; party_members[1] = party_members[0];
        assert(!can_choose_class(CLASS_RELICSKNIGHT, CLASS_CHOOSE_MODE_NORMAL));
        party_members[1].dead = TRUE;
        assert(!can_choose_class(CLASS_RELICSKNIGHT, CLASS_CHOOSE_MODE_NORMAL));
        party_count = 3; party_members[2].dead = FALSE;
        assert(!can_choose_class(CLASS_RELICSKNIGHT, CLASS_CHOOSE_MODE_NORMAL));
        party_members[2].dead = TRUE;
        assert(can_choose_class(CLASS_RELICSKNIGHT, CLASS_CHOOSE_MODE_NORMAL));
        party_count = 1;
        out = tmpfile(); assert(out); item.ident = IDENT_KNOWN | IDENT_MENTAL;
        assert(screen_object(&item, out, TRUE));
        rewind(out); n = fread(buf, 1, sizeof(buf)-1, out); buf[n] = 0; fclose(out);
        assert(strstr(buf, "それはレリクスナイトへのクラスチェンジを可能とする。"));
        object_wipe(&inventory[slot]);
        assert(!can_choose_class(CLASS_RELICSKNIGHT, CLASS_CHOOSE_MODE_NORMAL));
    }
    puts("Relics-Knight: four Ogre artifacts, equipment-only, alignment / companion gates and descriptions passed");


    /* Check every newly assigned item, including death-transformation gates. */
    {
        int classes[] = {CLASS_GUNNER,CLASS_NINJAMASTER,CLASS_ARCHMAGE,CLASS_FREYA,
            CLASS_CRESCENT,CLASS_VAMPIRE,CLASS_MEDIUM,CLASS_SUCCUBUS,
            CLASS_GRAPPLER,CLASS_ENIGMAHUNTER,CLASS_LORD,CLASS_LORD,CLASS_ANGELKNIGHT};
        int artifacts[] = {40,43,120,97,126,114,150,42,59,18,92,133,145};
        const char *names[] = {"ガンナー","ニンジャマスター","ゴエティック","フレイヤ",
            "クレセント","ヴァンパイア","巫女","サキュバス","グラップラー",
            "エニグマハンター","ロード","ロード","エンジェルナイト"};
        for (i = 0; i < 13; i++) {
            int mode = (i == 5 || i == 12) ? CLASS_CHOOSE_MODE_DEATH : i == 7 ? CLASS_CHOOSE_MODE_BLDGS : CLASS_CHOOSE_MODE_NORMAL;
            char expected[160];
            astral_mode = mode == CLASS_CHOOSE_MODE_NORMAL;
            p_ptr->pclass = i == 7 ? CLASS_WITCH : CLASS_SOLDIER; p_ptr->psex = (class_info[classes[i]].c_flags & PCF_SEX_FEMALE) ? SEX_FEMALE : SEX_MALE;
            p_ptr->align[ALI_LNC] = (class_info[classes[i]].c_flags & PCF_ALIGN_LAWFUL) ? 100 : -100; p_ptr->align[ALI_GNE] = i == 5 ? -100 : 100;
            p_ptr->infected = 1; p_ptr->stat_use[A_CON] = p_ptr->stat_use[A_CHR] = 400;
            party_rebind(); party_capture();
            memset(inventory, 0, sizeof(object_type) * INVEN_TOTAL);
            aid = artifacts[i];
            object_prep(&item, lookup_kind(a_info[aid].tval, a_info[aid].sval));
            item.name1 = aid; slot = wield_slot(&item);
            assert(slot >= INVEN_RARM && slot < INVEN_TOTAL);
            if (i == 7) {
                assert(can_choose_class(CLASS_SUCCUBUS, CLASS_CHOOSE_MODE_BLDGS));
                assert(!can_choose_class(CLASS_SUCCUBUS, CLASS_CHOOSE_MODE_NORMAL));
                p_ptr->pclass = CLASS_SOLDIER; party_rebind();
                assert(!can_choose_class(CLASS_SUCCUBUS, CLASS_CHOOSE_MODE_BLDGS));
                continue;
            }
            if(!can_choose_class(classes[i],mode)){fprintf(stderr,"No-equipment class failure i=%d class=%d mode=%d\n",i,classes[i],mode);abort();}
            inventory[0] = item;
            if(!can_choose_class(classes[i],mode)){fprintf(stderr,"No-equipment class failure i=%d class=%d mode=%d\n",i,classes[i],mode);abort();}
            object_wipe(&inventory[0]); inventory[slot] = item;
            if (!can_choose_class(classes[i], mode)) { fprintf(stderr, "Class item failure %d class %d artifact %d\n", i, classes[i], aid); abort(); }
            item.ident = IDENT_KNOWN | IDENT_MENTAL;
            out = tmpfile(); assert(out); assert(screen_object(&item, out, TRUE));
            rewind(out); n = fread(buf, 1, sizeof(buf)-1, out); buf[n] = 0; fclose(out);
            sprintf(expected, "それは%sへのクラスチェンジを可能とする。", names[i]);
            assert(!strstr(buf, expected));
            if (i == 12) {
                p_ptr->align[ALI_GNE] = -100;
                assert(!can_choose_class(CLASS_ANGELKNIGHT, mode));
                p_ptr->align[ALI_GNE] = 100;
            }
            if (i == 5) {
                p_ptr->infected = 0;
                assert(!can_choose_class(CLASS_VAMPIRE, mode));
                p_ptr->infected = 1;
            }
            object_wipe(&inventory[slot]);
            if(!can_choose_class(classes[i],mode)){fprintf(stderr,"No-equipment class failure i=%d class=%d mode=%d\n",i,classes[i],mode);abort();}
        }
        astral_mode = FALSE;
    }
    puts("Other advanced classes: no equipment required; Angel/Vampire death gates preserved");
    puts("General / White Knight / Temple Knight: equipment removed, level and faction conditions preserved");

    for(aid=1;aid<max_a_idx;aid++) {
        if(!a_info[aid].name || !a_info[aid].tval)continue;
        object_prep(&item,lookup_kind(a_info[aid].tval,a_info[aid].sval));
        if(!item.k_idx)continue;
        item.name1=aid;item.ident=IDENT_KNOWN|IDENT_MENTAL;
        add_flag(item.art_flags,TR_CLASS_GENERAL);
        add_flag(item.art_flags,TR_CLASS_TEMPLEKNIGHT);
        object_flags(&item,flags);
        assert(!have_flag(flags,TR_CLASS_GENERAL) && !have_flag(flags,TR_CLASS_TEMPLEKNIGHT));
        out=tmpfile();assert(out);assert(screen_object(&item,out,TRUE));
        rewind(out);n=fread(buf,1,sizeof(buf)-1,out);buf[n]=0;fclose(out);
        if(strstr(buf,"へのクラスチェンジを可能とする。"))
            assert(strstr(buf,"うぃっちへの") || strstr(buf,"レリクスナイトへの"));
        if(aid==ART_LICH)assert(strstr(buf,"リッチとして転生"));
    }
    puts("Class item flags: parsed, equipped-only, active actor only, existing conditions and I descriptions passed");
}
'''
h = h.replace('int main(int argc,char **argv)', test+'\nint main(int argc,char **argv)', 1)
h = h.replace('    init_angband();', '    init_angband();\n    class_item_tests();\n    return 0;', 1)
with tempfile.TemporaryDirectory(prefix='toband-class-items-') as directory:
    tmp = Path(directory)
    # Use isolated runtime data and regenerate raw caches from current templates.
    import shutil
    shutil.copytree(BUILD/'lib', tmp/'lib', ignore=shutil.ignore_patterns('*.raw', 'save'))
    for name in ('a_info.txt', 'e_info.txt'):
        (tmp/'lib/edit'/name).write_bytes((ROOT/'lib/edit'/name).read_bytes().decode('euc_jp').encode('cp932'))
    (tmp/'lib/save').mkdir(exist_ok=True)
    source = tmp/'test.c'; source.write_bytes(h.encode('euc_jp'))
    objects = [p for p in BUILD.glob('*.o') if p.stem not in ('main-win', 'angband')]
    exe = tmp/'test.exe'
    subprocess.run([str(GCC), *FLAGS, str(source), *map(str, objects), '-o', str(exe),
                    '-lwinmm', '-lcomdlg32', '-lgdi32'], check=True)
    result = subprocess.run([str(exe), (tmp/'lib').as_posix()+'/'], cwd=tmp,
                            capture_output=True, timeout=45)
    print(result.stdout.decode('cp932', errors='replace'), end='')
    if result.returncode:
        print(result.stderr.decode('cp932', errors='replace'))
        raise SystemExit(result.returncode)
