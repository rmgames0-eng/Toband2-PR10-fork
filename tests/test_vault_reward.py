"""Check randomized Vault artifacts through real quest parsing and placement."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build/party'
GCC=ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h=(ROOT/'tests/party_integration.c').read_text(encoding='euc_jp')
marker='    death_regressions();'
assert marker in h
h=h.replace(marker,r"""
    death_regressions();
    {
        int ids[14]={73,88,90,70,158,99,108,107,152,58,76,100,179,96};
        int i,j,trial,chosen,seen[14]={0},found,room,decoys,kind;
        cptr edit_dir;
        for(i=0;i<14;i++) a_info[ids[i]].cur_num=0;
        for(trial=0;trial<500;trial++) {
            quest[QUEST_VAULT].status=QUEST_STATUS_UNTAKEN;
            Rand_quick=TRUE; Rand_value=12345+trial*7919;
            assert(choose_vault_artifact());
            chosen=quest[QUEST_VAULT].k_idx;
            for(i=0;i<14 && ids[i]!=chosen;i++);
            assert(i<14); seen[i]++;
        }
        for(i=0;i<14;i++) assert(seen[i]>0);
        for(i=0;i<14;i++) {
            for(j=0;j<14;j++) a_info[ids[j]].cur_num=(i!=j);
            quest[QUEST_VAULT].status=QUEST_STATUS_UNTAKEN;
            assert(choose_vault_artifact() && quest[QUEST_VAULT].k_idx==ids[i]);
        }
        for(i=0;i<14;i++) a_info[ids[i]].cur_num=1;
        quest[QUEST_VAULT].status=QUEST_STATUS_UNTAKEN;
        assert(!choose_vault_artifact());
        quest[QUEST_VAULT].status=QUEST_STATUS_TAKEN;
        quest[QUEST_VAULT].k_idx=158;
        p_ptr->inside_quest=QUEST_VAULT;
        quest_text_line=0; init_flags=INIT_SHOW_TEXT|INIT_ASSIGN;
        assert(!process_dungeon_file("q_info.txt",0,0,0,0));
        assert(quest[QUEST_VAULT].k_idx==158);
        assert(quest_text_line==5 && quest_text[2][0]);
        p_ptr->inside_quest=0; p_ptr->playing=TRUE; p_ptr->is_dead=0;
        assert(save_player()); party_reset(); assert(load_player());
        assert(quest[QUEST_VAULT].k_idx==158);
        /* Accepted old saves retain Oracion. */
        quest[QUEST_VAULT].k_idx=88; p_ptr->inside_quest=QUEST_VAULT;
        quest_text_line=0; init_flags=INIT_SHOW_TEXT|INIT_ASSIGN;
        assert(!process_dungeon_file("q_info.txt",0,0,0,0));
        assert(quest[QUEST_VAULT].k_idx==88);
        edit_dir=ANGBAND_DIR_EDIT; ANGBAND_DIR_EDIT=".";
        for(i=0;i<14;i++) for(room=0;room<4;room++) {
            Rand_quick=TRUE; Rand_value=12345+i*7919+room*101;
            wipe_o_list();
            seed_town=room;
            quest[QUEST_VAULT].k_idx=ids[i]; a_info[ids[i]].cur_num=0;
            init_flags=INIT_CREATE_DUNGEON;
            assert(!process_dungeon_file("vault-fixture.txt",2,2,3,6));
            found=decoys=0;
            kind=lookup_kind(a_info[ids[i]].tval,a_info[ids[i]].sval);
            for(j=1;j<o_max;j++) if(o_list[j].k_idx) {
                assert(o_list[j].k_idx==kind);
                if(o_list[j].name1==ids[i]) {
                    found++;
                } else {
                    assert(!o_list[j].name1);
                    decoys++;
                }
            }
            assert(found==1 && decoys==3 && a_info[ids[i]].cur_num==1);
        }
        ANGBAND_DIR_EDIT=edit_dir;
        puts("Vault: all 14 candidates, generated-item exclusion, exhausted pool, stable text/save and exact artifact placement passed");
    }
""",1)
with tempfile.TemporaryDirectory(prefix='toband-vault-') as d:
    d=Path(d);c=d/'test.c';exe=d/'test.exe';c.write_bytes(h.encode('euc_jp'))
    quest_data=(ROOT/'lib/edit/q0000004.txt').read_text(encoding='euc_jp')
    placement=quest_data.split('?:[EQU $RANDOM4 0]',1)[1].split('# Random monster',1)[0]
    (d/'vault-fixture.txt').write_text('?:[EQU $RANDOM4 0]'+placement+'D:abcd\n',encoding='ascii')
    objects=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    subprocess.run([str(GCC),*FLAGS,str(c),*map(str,objects),'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(exe),(BUILD/'lib').as_posix()+'/'],cwd=d,capture_output=True,timeout=30)
    print(result.stdout.decode('cp932',errors='replace'),end='')
    if result.returncode:
        print('Return code:',result.returncode); print(result.stderr.decode('cp932',errors='replace'))
        raise SystemExit(result.returncode)
