from pathlib import Path
import re, subprocess, shutil
root = Path(__file__).resolve().parent
bin = root / 'tools/mingw32/bin'
build = root / 'build/mingw'
build.mkdir(parents=True, exist_ok=True)
makefile = (root / 'src/makefile.bcc').read_text()
names = re.findall(r'([\w-]+)\.obj', makefile.split('OBJ =',1)[1].split('all :',1)[0])
flags = ['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-Wno-implicit-function-declaration','-Wno-int-conversion','-Wno-incompatible-pointer-types']
def run(args):
    subprocess.run([str(x) for x in args], cwd=root / 'src', check=True)
objects = []
for name in names:
    print('Compiling ' + name, flush=True)
    obj = build / (name + '.o')
    run([bin/'gcc.exe', *flags, '-c', root/'src'/(name+'.c'), '-o', obj])
    objects.append(obj)
rc = build/'angband.rc'
rc.write_bytes((root/'src/angband.rc').read_bytes().decode('euc_jp').encode('utf-8'))
run([bin/'windres.exe','--codepage=65001','-I',root/'src','-i',rc,'-o',build/'angband.o'])
run([bin/'gcc.exe','-mwindows','-static',*objects,build/'angband.o','-o',build/'TOband.exe','-lwinmm','-lcomdlg32','-lgdi32'])
print('Built: ' + str(build/'TOband.exe'))
# Package the runtime data beside the executable using Windows text encoding.
for source in (root/'lib').rglob('*'):
    target = build/'lib'/source.relative_to(root/'lib')
    if source.is_dir():
        target.mkdir(parents=True, exist_ok=True)
    elif not target.exists():
        target.parent.mkdir(parents=True, exist_ok=True)
        data = source.read_bytes()
        if source.suffix.lower() not in ('.fon', '.fnt'):
            data = data.decode('euc_jp').encode('cp932')
        target.write_bytes(data)
