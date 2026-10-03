"""Exercise real monster deaths and exclusive Ogre artifact drops."""
from pathlib import Path
import subprocess,tempfile,shutil
ROOT=Path(__file__).resolve().parents[1];BUILD=ROOT/'build/party'
h=(ROOT/'tests/party_integration.c').read_bytes().decode('euc_jp')
extra=r"""
static void ogre_drop_tests(void)
{
    int i,j,x,y,id=0,pass,midx,count;
    int arts[4]={ART_OGRE_HELM,ART_OGRE_ARMOR,ART_OGRE_SHIELD,ART_OGRE_BLADE};
    int gods[4]={MON_FELLANA,MON_HOLP,MON_ISHTALLE,MON_FILARHH};
    static char spaces[20000];
    memset(spaces,32,sizeof(spaces)-1);keys=spaces;key_index=0;
    Rand_quick=TRUE;Rand_value=19377;
    p_ptr->inside_quest=0;dungeon_type=DUNGEON_RUINS;dun_level=1000;
    character_dungeon=TRUE;p_ptr->floor_id=7;
    for(i=1;i<max_r_idx;i++)if(r_info[i].d_char=='O' && r_info[i].level==60){id=i;break;}
    assert(id);
    for(i=0;i<4;i++){
        assert(a_info[arts[i]].gen_flags & TRG_QUESTITEM);
        assert(a_info[arts[i]].rarity==150+i*10);
        a_info[arts[i]].rarity=1; /* Deterministic success; never alter game data. */
    }
    /* Ordinary Ogre, clone, pet, no-drop death, and four old god sources. */
    for(pass=0;pass<8;pass++) {
        wipe_o_list();wipe_m_list();cur_hgt=cur_wid=15;py=px=1;
        for(y=0;y<15;y++)for(x=0;x<15;x++) {cave[y][x].feat=FEAT_FLOOR;cave[y][x].mimic=0;}
        for(i=0;i<4;i++)a_info[arts[i]].cur_num=0;
        j=pass<4?id:gods[pass-4];r_info[j].max_num=100;r_info[j].cur_num=0;
        assert(place_monster_aux(0,7,7,j,PM_IGNORE_TERRAIN|PM_IGNORE_AMGRID));
        midx=cave[7][7].m_idx;
        if(pass==1)m_list[midx].smart1|=SM1_CLONED;
        if(pass==2)set_pet(&m_list[midx]);
        p_ptr->wizard=TRUE; /* Old guaranteed god drops must remain absent in wizard mode too. */
        monster_death(midx,pass!=3,FALSE);
        count=0;
        for(i=1;i<o_max;i++)for(j=0;j<4;j++)if(o_list[i].k_idx && o_list[i].name1==arts[j])count++;
        assert(count==(pass==0?4:0));
        for(i=0;i<4;i++)assert(a_info[arts[i]].cur_num==(pass==0?1:0));
        if(pass==0) {
            for(i=0;i<4;i++)assert(a_info[arts[i]].floor_id==7);
            monster_death(midx,TRUE,FALSE);
            count=0;for(i=1;i<o_max;i++)for(j=0;j<4;j++)if(o_list[i].k_idx && o_list[i].name1==arts[j])count++;
            assert(count==4);
        }
    }
    puts("Ogre equipment: four rarity rolls, exclusive drops, no god/clone/pet drops, no duplicates passed");
}
"""
h=h.replace('int main(int argc,char **argv)',extra+'\nint main(int argc,char **argv)',1)
h=h.replace('    display_regressions();','    ogre_drop_tests();\n    return 0;\n    display_regressions();',1)
with tempfile.TemporaryDirectory(prefix='toband-ogre-drops-') as directory:
    tmp=Path(directory);shutil.copytree(BUILD/'lib',tmp/'lib',ignore=shutil.ignore_patterns('*.raw','save'))
    for name in ('a_info.txt','r_info.txt','d_info.txt'):
        (tmp/'lib/edit'/name).write_bytes((ROOT/'lib/edit'/name).read_bytes().decode('euc_jp').encode('cp932'))
    (tmp/'lib/save').mkdir(exist_ok=True)
    (tmp/'test.c').write_bytes(h.encode('euc_jp'))
    flags=['-std=gnu89','-O2','-DWIN32','-DJP','-DSJIS','-finput-charset=EUC-JP','-fexec-charset=CP932','-I',str(ROOT/'src')]
    objs=[str(p) for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband')]
    subprocess.run([str(ROOT/'tools/mingw32/bin/gcc.exe'),*flags,str(tmp/'test.c'),*objs,'-o',str(tmp/'test.exe'),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    result=subprocess.run([str(tmp/'test.exe'),(tmp/'lib').as_posix()+'/'],cwd=tmp,capture_output=True,timeout=45)
    print(result.stdout.decode('cp932',errors='replace'))
    if result.returncode:
        print(result.stderr.decode('cp932',errors='replace'));raise SystemExit(result.returncode)

