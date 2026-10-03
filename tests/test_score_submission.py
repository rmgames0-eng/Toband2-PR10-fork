"""Build a real character dump, encode it for the web, without saving a submission file.

No public submission by default. --live performs two HTTPS submissions of a
clearly named test record; the operator must remove that record afterward.
"""
from pathlib import Path
import argparse
import re
import shutil
import subprocess
import tempfile
import urllib.parse

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build/party'
GCC = ROOT / 'tools/mingw32/bin/gcc.exe'
parser = argparse.ArgumentParser()
parser.add_argument('--live', action='store_true')
args = parser.parse_args()
h = (ROOT / 'tests/party_integration.c').read_bytes().decode('euc_jp')
h = '#include "report.c"\n' + h
start = h.index('    /* Q and')
end = h.index('\n}', start) + 2
test = r'''
    {
        BUF *payload, *small;
        FILE *f;
        char expected[]="name=%E8%AA%9E%26%2B%0A";
        {
            player_type saved = *p_ptr;
            bool options[6] = {preserve_mode, smart_learn, smart_cheat,
                powerup_home, ironman_rooms, easy_band};
            byte count = party_count, active = party_active;
            int n;
            preserve_mode=TRUE; smart_learn=TRUE; smart_cheat=FALSE;
            powerup_home=TRUE; ironman_rooms=FALSE; easy_band=FALSE;
            p_ptr->max_max_exp=1003; p_ptr->expfact=100;
            p_ptr->max_max_dlv=10; p_ptr->max_dlv_mult=50;
            p_ptr->winner_mult=2; p_ptr->ogre_mult=3;
            party_count=0; assert(total_points()==6503);
            for(n=1;n<=MAX_PARTY_MEMBERS;++n) {
                byte dead = party_members[n-1].dead;
                party_count=n; party_active=n-1;
                party_members[n-1].dead=FALSE;
                assert(total_points()==6503/(1L<<(n-1)));
                party_members[n-1].dead=TRUE;
                assert(total_points()==6503/(1L<<(n-1)));
                party_members[n-1].dead=dead;
            }
            /* Difficulty and both boss bonuses are reduced exactly once. */
            party_count=3; smart_cheat=TRUE;
            assert(total_points()==7553/4);
            easy_band=TRUE;
            party_count=1; n=total_points();
            party_count=2; assert(total_points()==n/2);
            party_count=3; assert(total_points()==n/4);
            *p_ptr=saved; party_count=count; party_active=active;
            preserve_mode=options[0]; smart_learn=options[1]; smart_cheat=options[2];
            powerup_home=options[3]; ironman_rooms=options[4]; easy_band=options[5];
            puts("Party score: solo, 2-16 members, dead members, rounding, options and boss bonuses passed");
        }
        strcpy(player_name,"送信確認用");
        strcpy(p_ptr->died_from,"動作確認（削除予定）");
        p_ptr->is_dead=1;
        party_count=3; party_active=1;
        party_members[0].player=*p_ptr;
        party_members[2].player=*p_ptr;
        strcpy(party_members[0].name,"生存仲間");
        strcpy(party_members[2].name,"<死亡仲間>");
        party_members[0].dead=FALSE; party_members[2].dead=TRUE;
        party_members[0].player.lev=23;
        party_members[0].player.cexp_info[p_ptr->pclass].clev=21;
        party_members[2].player.lev=12;
        party_members[2].player.cexp_info[p_ptr->pclass].clev=11;
        small=buf_new();assert(small);
        assert(score_form_field(small,"name","\xe8\xaa\x9e&+\n"));
        assert(small->size==strlen(expected));
        assert(!memcmp(small->data,expected,small->size));buf_delete(small);
        payload=score_make_payload("<script>test & +</script>");
        assert(payload && payload->size>1000);
        f=fopen("score-payload.tbs","wb");assert(f);
        assert(fwrite(payload->data,1,payload->size,f)==payload->size);fclose(f);
        if(argc==3) {
            assert(score_https_post(payload));
            assert(score_https_post(payload));
        }
        buf_delete(payload);
        puts("Score client: Japanese dump, form escaping, no automatic submission files passed");
    }
    return 0;
}
'''
h = h[:start] + test + h[end:]
with tempfile.TemporaryDirectory(prefix='toband-score-test-') as directory:
    tmp=Path(directory)
    shutil.copytree(BUILD/'lib', tmp/'lib', ignore=shutil.ignore_patterns('*.raw','save'))
    for p in (ROOT/'lib/edit').glob('*.txt'):
        (tmp/'lib/edit'/p.name).write_bytes(p.read_bytes().decode('euc_jp').encode('cp932'))
    (tmp/'lib/save').mkdir(exist_ok=True)
    (tmp/'test.c').write_bytes(h.encode('euc_jp'))
    objs=[str(p) for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband','report')]
    subprocess.run([str(GCC),'-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src'),str(tmp/'test.c'),*objs,'-o',str(tmp/'test.exe'),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    command=[str(tmp/'test.exe'),(tmp/'lib').as_posix()+'/']+(['live'] if args.live else [])
    result=subprocess.run(command,cwd=tmp,capture_output=True,timeout=90)
    print(result.stdout.decode('cp932',errors='replace'))
    if result.returncode:
        print(result.stderr.decode('cp932',errors='replace')); raise SystemExit(result.returncode)
    assert not list((tmp/'lib/user').glob('score-*.tbs'))
    raw=(tmp/'score-payload.tbs').read_bytes()
    data=urllib.parse.parse_qs(raw.decode('ascii'),encoding='cp932')
    assert 'player' not in data
    assert data['character']==['送信確認用']
    assert data['comment']==['<script>test & +</script>']
    dump=data['dump'][0]
    assert 'TOband-R3' in dump
    assert '[共有情報]' not in dump
    assert dump.index('[キャラクタの持ち物]') < dump.index('[人物 1:') < dump.index('[人物 3:') < dump.index('[チェックサム:')
    assert dump.count('[人物 1:')==1
    assert data['protocol']==['1']
    assert data['party_count']==['3']
    assert data['party_0_name']==['生存仲間']
    assert data['party_0_level']==['23'] and data['party_0_class_level']==['21']
    assert data['party_1_name']==['送信確認用'] and data['party_1_active']==['1']
    assert data['party_2_dead']==['1'] and data['party_2_class_level']==['11']
    (ROOT/'build/score-test-payload.tbs').write_bytes(raw)
    print('Payload round-trip passed; saved build/score-test-payload.tbs')
