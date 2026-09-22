"""Build a clean Windows distribution from Git-tracked data only."""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('version', help='Release label, e.g. pr10-expfix1')
parser.add_argument('--output-dir', type=Path, default=ROOT / 'build/dist')
args = parser.parse_args()
if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._-]*', args.version):
    parser.error('Version must contain only letters, numbers, dot, underscore or hyphen')
output = args.output_dir.resolve()
output.mkdir(parents=True, exist_ok=True)
name = 'TOband2-' + args.version + '-windows-x86'
archive = output / (name + '.zip')
if archive.exists():
    parser.error('Archive already exists: ' + str(archive))
tracked = subprocess.check_output(['git', '-C', str(ROOT), 'ls-files', '-z']).decode('utf-8').split('\0')
revision = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
with tempfile.TemporaryDirectory(prefix='toband-release-') as tmp:
    build = Path(tmp)
    subprocess.run([sys.executable, str(ROOT / 'build-mingw.py'), '--build-dir', str(build)], check=True)
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        z.write(build / 'TOband.exe', name + '/TOband.exe')
        for relative in tracked:
            if relative.startswith('lib/') or relative in ('readme.txt', 'readme_angband', 'autopick.txt'):
                data = (ROOT / relative).read_bytes()
                if Path(relative).suffix.lower() not in ('.fon', '.fnt'):
                    data = data.decode('euc_jp').encode('cp932')
                z.writestr(name + '/' + relative, data)
        readme = f'''TOband2 {args.version} / Windows 32bit版

ZIPをフォルダーごと展開し、TOband.exeを起動してください。
libフォルダーはTOband.exeと同じ場所に置いてください。
Pythonやコンパイラのインストールは不要です。

変更: モンスター経験値の桁あふれ・除算処理を修正。
検証: 経験値回帰テストとビルドを実施。ゲーム内のプレイ確認は未実施。
既存のゲームに上書きせず、別フォルダーへ展開してください。

ソース: https://github.com/rmgames0-eng/Toband2-PR10-fork
コミット: {revision}
原版の説明・著作権表記: readme.txt / readme_angband
'''
        z.writestr(name + '/README-release.txt', readme.encode('utf-8-sig'))
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        names = z.namelist()
        assert name + '/lib/xtra/font/8X13.FON' in names
        assert not any(n.lower().endswith(('.ini', '.o', '.obj')) for n in names)
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
archive.with_suffix('.zip.sha256').write_bytes((digest + '  ' + archive.name + '\n').encode('ascii'))
print('Release ZIP: ' + str(archive))
print('SHA256: ' + digest)
