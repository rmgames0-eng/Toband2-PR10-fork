"""Verify that quick start keeps the founding character name."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build/party'
GCC=ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h=(ROOT/'tests/party_integration.c').read_text(encoding='euc_jp')
h=h.replace("    death_regressions();","    death_regressions();"+'\n    {\n        int trial;\n        for(trial=0;trial<2;trial++) {\n            previous_char.quick_ok=TRUE;\n            party_count=2; party_active=trial ? 0 : 1;\n            strcpy(party_members[0].name,"Founder");\n            strcpy(party_members[1].name,"Companion");\n            strcpy(player_name,trial ? "RenamedFounder" : "Companion");\n            astral_mode=FALSE; runeweapon_list[1].status=0;\n            auto_recruit=auto_power=death_input=home_test=inspect_party_menu=FALSE; keys="y"; key_index=0; msg_flag=FALSE; character_generated=FALSE;\n            player_birth();\n            assert(!strcmp(player_name,trial ? "RenamedFounder" : "Founder"));\n            assert(p_ptr->prace==previous_char.prace && p_ptr->pclass==previous_char.pclass);\n            assert(!party_count);\n            p_ptr->playing=TRUE; p_ptr->is_dead=0;\n            assert(save_player()); party_reset(); assert(load_player());\n            assert(!strcmp(player_name,trial ? "RenamedFounder" : "Founder"));\n            assert(party_count==1 && !strcmp(party_members[0].name,player_name));\n        }\n        puts("Quick start: founder name after a companion, current renamed founder, and save/load passed");\n    }\n',1)
with tempfile.TemporaryDirectory(prefix='toband-quick-name-') as d:
    d=Path(d);c=d/'test.c';exe=d/'test.exe';c.write_bytes(h.encode('euc_jp'))
    objects=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    # Exercise the production name snapshot and quick-start restore, while
    # omitting unrelated world teardown/generation from this headless fixture.
    birth=(ROOT/'src/birth.c').read_bytes().decode('euc_jp')
    marker='            if (!prepare_astral) my_strcpy(player_name, quick_name, sizeof(player_name));'
    assert marker in birth
    birth=birth.replace(marker,marker+'\n            return TRUE;',1)
    start=birth.index('void player_birth(void)')
    brace=birth.index('{',start); depth=1; end=brace+1
    while depth:
        depth+=(birth[end]=='{')-(birth[end]=='}'); end+=1
    stop=birth.index('\tparty_reset();',start)+len('\tparty_reset();')
    birth=birth[:stop]+'\n    (void)player_birth_quick(FALSE, quick_name);\n}'+birth[end:]
    birth_c=d/'birth.c'; birth_o=d/'birth.o'; birth_c.write_bytes(birth.encode('euc_jp'))
    subprocess.run([str(GCC),*FLAGS,'-c',str(birth_c),'-o',str(birth_o)],check=True)
    objects=[birth_o if p.stem=='birth' else p for p in objects]
    subprocess.run([str(GCC),*FLAGS,str(c),*map(str,objects),'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(exe),(BUILD/'lib').as_posix()+'/'],cwd=d,capture_output=True,timeout=10)
    print(result.stdout.decode('cp932',errors='replace'),end='')
    if result.returncode:
        print(result.stderr.decode('cp932',errors='replace'))
        raise SystemExit(result.returncode)
