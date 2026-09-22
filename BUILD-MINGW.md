# TOband2-PR10 ビルド環境

導入先: H:\temp\TOband2-PR10\tools\mingw32
コンパイラ: WinLibs GCC 14.2.0 / MinGW-w64 12.0.0、i686、MSVCRT
Python: このPCに導入済みの Python 3.10 を使用。

## 再ビルド
H:\temp\TOband2-PR10\build-mingw.cmd をダブルクリック。
または PowerShell で次を実行:

    python H:\temp\TOband2-PR10\build-mingw.py

## 実行ファイル
H:\temp\TOband2-PR10\build\mingw\TOband.exe
同じディレクトリに実行用の lib データも配置済み。

元のソースと lib は保持。ソースのEUC-JPを読み、文字列をCP932で生成。
メニューリソースはUTF-8経由でコンパイル。実行用テキストデータはCP932へ変換。
既存の実行用データは再ビルド時に上書きしない（セーブや設定を保持するため）。

全Cファイルのコンパイル、リソース生成、リンクに成功。
実行ファイルは32bit Windows GUI形式。依存DLLはWindows標準DLLのみ。
ゲームの起動・プレイ動作は未確認。既存ソース由来のコンパイラ警告は残る。
システム全体のPATH変更は不要。

配布元:
https://github.com/brechtsanders/winlibs_mingw/releases/tag/14.2.0posix-19.1.7-12.0.0-msvcrt-r3
アーカイブSHA256（公開チェックサムと照合済み）:
4635cf52b469e2ef22ba5d04fd8688bd6f82b41d279feb0ddaf2d6640acd2f2f

## 別のPCでの準備

1. Python 3 を導入し、python コマンドを使えるようにする。
2. 上記配布元から次のアーカイブを取得する。
   winlibs-i686-posix-dwarf-gcc-14.2.0-mingw-w64msvcrt-12.0.0-r3.7z
3. 公開SHA256と照合してから、リポジトリ直下の tools に展開する。
   tools/mingw32/bin/gcc.exe が存在する配置にする。
4. リポジトリ直下で python build-mingw.py を実行する。
5. build/mingw/TOband.exe を起動する。

本文の H: で始まるパスは元の作業PCの例。別の場所へcloneしてもビルドスクリプトはその配置場所を自動認識する。
tools と build はGit管理対象外。

## 経験値計算の回帰テスト

    python tests/test_monster_exp.py

実際の経験値計算関数を抽出して32bit版GCCでコンパイルし、最適化なし・ありの両方で検証する。
高経験値の敵、経験値倍率、地上・増殖の減額、ユニーク、端数の繰り上がりを対象とする。
経験値計算の中間値は64bit化し、従来の多倍長除算を置き換えた。
元ソースのEUC-JPは維持している。Borland向けの64bit型宣言も用意しているが、今回のビルド確認はMinGWのみ。
