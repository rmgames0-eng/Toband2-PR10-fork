"""Check randomized Vault artifacts through real quest parsing and placement."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build/party'
GCC=ROOT/'tools/mingw32/bin/gcc.exe'
FLAGS=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
h=(ROOT/'tests/party_integration.c').read_text(encoding='euc_jp')
marker='    /* Arena commands cannot switch, and defeat stays with arena exit handling. */'
assert marker in h
h=h.replace(marker,'\n    p_ptr->tim_infra=47; party_members[1].player.tim_infra=99;\n    /* Direct cycling, wraparound, dead skips, and no-turn rejection. */\n    msg_flag=FALSE; do_cmd_party_next(); assert(party_active==1 && energy_use==100 && p_ptr->tim_infra==47);\n    msg_flag=FALSE; do_cmd_party_next(); assert(party_active==2 && energy_use==100 && p_ptr->tim_infra==47);\n    msg_flag=FALSE; do_cmd_party_next(); assert(party_active==0 && energy_use==100 && p_ptr->tim_infra==47);\n    party_members[1].dead=TRUE;\n    msg_flag=FALSE; do_cmd_party_next(); assert(party_active==2 && energy_use==100 && p_ptr->tim_infra==47);\n    msg_flag=FALSE; do_cmd_party_next(); assert(party_active==0);\n    party_members[2].player.chp=-1;\n    msg_flag=FALSE; do_cmd_party_next(); assert(party_active==0 && energy_use==0);\n    party_members[1].dead=FALSE; party_members[2].player.chp=12;\n    p_ptr->inside_arena=TRUE;\n    msg_flag=FALSE; do_cmd_party_next(); assert(party_active==0 && energy_use==0);\n    p_ptr->inside_arena=FALSE; astral_mode=TRUE;\n    msg_flag=FALSE; do_cmd_party_next(); assert(party_active==0 && energy_use==0);\n    astral_mode=FALSE; party_count=1;\n    msg_flag=FALSE; do_cmd_party_next(); assert(party_active==0 && energy_use==0);\n    party_count=3; msg_flag=FALSE;\n    puts("Next companion: cycle, wraparound, shared infravision, dead skips, no allies, arena and special-mode guards passed");\n'+'    return;\n'+marker,1)
with tempfile.TemporaryDirectory(prefix='toband-party-next-') as d:
    d=Path(d);c=d/'test.c';exe=d/'test.exe';c.write_bytes(h.encode('euc_jp'))
    objects=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    subprocess.run([str(GCC),*FLAGS,str(c),*map(str,objects),'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(exe),(BUILD/'lib').as_posix()+'/'],cwd=d,capture_output=True,timeout=30)
    print(result.stdout.decode('cp932',errors='replace'),end='')
    if result.returncode:
        print(result.stderr.decode('cp932',errors='replace'))
        raise SystemExit(result.returncode)
