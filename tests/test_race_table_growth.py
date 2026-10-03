"""Load an old-size monster table with runeweapon lore and live references."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build/party'
GCC = ROOT / 'tools/mingw32/bin/gcc.exe'
FLAGS = ['-std=gnu89', '-O2', '-DWIN32', '-DJP', '-DSJIS',
         '-finput-charset=EUC-JP', '-fexec-charset=CP932', '-I', str(ROOT/'src')]
with tempfile.TemporaryDirectory(prefix='toband-race-growth-') as td:
    td = Path(td)
    writer = (ROOT/'src/save.c').read_text(encoding='euc_jp')
    writer = writer.replace('tmp16u = max_r_idx;', 'tmp16u = max_r_idx - 1;')
    writer = writer.replace('wr_lore(i);', 'wr_lore(i < tmp16u ? i : i + 1);')
    writer = writer.replace('wr_s16b(m_ptr->r_idx);', 'wr_s16b(m_ptr->r_idx >= max_r_idx ? m_ptr->r_idx - 1 : m_ptr->r_idx);')
    writer = writer.replace('wr_s16b(m_ptr->ap_r_idx);', 'wr_s16b(m_ptr->ap_r_idx >= max_r_idx ? m_ptr->ap_r_idx - 1 : m_ptr->ap_r_idx);')
    (td/'old_save.c').write_bytes(writer.encode('euc_jp'))
    subprocess.run([str(GCC), *FLAGS, '-c', str(td/'old_save.c'), '-o', str(td/'old_save.o')], check=True)
    harness = (ROOT/'tests/party_integration.c').read_text(encoding='euc_jp')
    marker = '    party_rewards=3;\n    assert(save_player());\n    party_reset();\n    assert(load_player());'
    assert marker in harness
    harness = harness.replace(marker, """
    party_rewards=3;
    r_info[max_r_idx] = r_info[858];
    r_info[max_r_idx].r_pkills = 17;
    stock_mon[0].r_idx = max_r_idx;
    stock_mon[0].ap_r_idx = max_r_idx;
    stock_mon[0].hp = stock_mon[0].maxhp = stock_mon[0].max_maxhp = 100;
    assert(save_player());
    stock_mon[0].r_idx = stock_mon[0].ap_r_idx = 0;
    r_info[max_r_idx].r_pkills = 0;
    party_reset();
    assert(load_player());
    assert(stock_mon[0].r_idx == max_r_idx);
    assert(stock_mon[0].ap_r_idx == max_r_idx);
    assert(r_info[max_r_idx].r_pkills == 17);
    assert(r_info[max_r_idx - 1].max_num == 1);
    puts("Old monster table: runeweapon identity, appearance and lore preserved; new unique available");
    return 0;
""", 1)
    (td/'growth.c').write_bytes(harness.encode('euc_jp'))
    objects = [p for p in BUILD.glob('*.o') if p.stem not in ('main-win', 'angband', 'save')]
    exe = td/'growth.exe'
    subprocess.run([str(GCC), *FLAGS, str(td/'growth.c'), str(td/'old_save.o'), *map(str,objects), '-o', str(exe), '-lwinmm', '-lcomdlg32', '-lgdi32'], check=True)
    result = subprocess.run([str(exe), (BUILD/'lib').as_posix()+'/'], cwd=td, capture_output=True, timeout=30)
    print(result.stdout.decode('cp932', errors='replace'), end='')
    if result.returncode:
        print(result.stderr.decode('cp932', errors='replace'))
        raise SystemExit(result.returncode)
