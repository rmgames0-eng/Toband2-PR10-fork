from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1];b=r/'build/monlist'
with tempfile.TemporaryDirectory() as temp:
 t=Path(temp);exe=t/'test.exe'
 objects=[p for p in b.glob('*.o') if p.stem not in ('main-win','angband')]
 subprocess.run([str(r/'tools/mingw32/bin/gcc.exe'),'-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(r/'src'),str(r/'tests/monster_list.c'),*map(str,objects),'-o',str(exe),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
 subprocess.run([str(exe),(b/'lib').as_posix()+'/'],cwd=t,check=True,timeout=30)
