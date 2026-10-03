from pathlib import Path
import tempfile, subprocess, shutil
ROOT=Path(__file__).resolve().parents[1];BUILD=ROOT/'build/party'
h=(ROOT/'tests/party_integration.c').read_bytes().decode('euc_jp')
extra=r"""
static void lamp_tests(void)
{
    int i,j,n,plain=0,seen=0,k=lookup_kind(TV_LITE,SV_LITE_FEANOR);
    int res[]={TR_RES_ACID,TR_RES_ELEC,TR_RES_FIRE,TR_RES_COLD,TR_RES_POIS,TR_RES_LITE,TR_RES_DARK,TR_RES_SHARDS,TR_RES_BLIND,TR_RES_CONF,TR_RES_SOUND,TR_RES_NETHER,TR_RES_STONE,TR_RES_CHAOS,TR_RES_DISEN,TR_RES_FEAR};
    object_type o;
    assert(k==203);
    assert(strstr(k_name+k_info[k].name,"イシュタル"));
    Rand_quick=TRUE;Rand_value=54321;
    for(i=0;i<1000;i++) {
        object_prep(&o,k);apply_magic(&o,30,0);
        n=0;for(j=0;j<16;j++)if(have_flag(o.art_flags,res[j]))n++;
        assert(n>=1);
        if(!o.name2 && !o.art_name) {
            assert(o.to_stat[A_STR]>=1 && o.to_stat[A_STR]<=3);
            assert(n==1 && !o.pval && !o.xtra4);
            seen |= 1 << o.to_stat[A_STR];plain++;
        }
    }
    assert(plain>100 && seen==14);
    object_prep(&o,lookup_kind(TV_LITE,SV_LITE_LANTERN));
    apply_magic(&o,1,0);assert(!o.to_stat[A_STR]);
    puts("Ishtar lamp: 1000 rolls, strength +1..3, random resistance, fuel and other light preserved passed");
}
"""
h=h.replace('int main(int argc,char **argv)',extra+'\nint main(int argc,char **argv)',1).replace('    init_angband();','    init_angband();\n    lamp_tests();\n    return 0;',1)
with tempfile.TemporaryDirectory(prefix='ishtar-lamp-') as d:
 t=Path(d);shutil.copytree(BUILD/'lib',t/'lib',ignore=shutil.ignore_patterns('*.raw','save'))
 (t/'lib/edit/k_info.txt').write_bytes((ROOT/'lib/edit/k_info.txt').read_bytes().decode('euc_jp').encode('cp932'))
 (t/'lib/save').mkdir(exist_ok=True);(t/'test.c').write_bytes(h.encode('euc_jp'))
 objs=[p for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
 subprocess.run([str(ROOT/'tools/mingw32/bin/gcc.exe'),'-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src'),str(t/'test.c'),*map(str,objs),'-o',str(t/'test.exe'),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
 out=subprocess.run([str(t/'test.exe'),(t/'lib').as_posix()+'/'],cwd=t,capture_output=True,timeout=30)
 print(out.stdout.decode('cp932',errors='replace'));print(out.stderr.decode('cp932',errors='replace'));assert out.returncode==0
