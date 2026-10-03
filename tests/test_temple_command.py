"""Real data, naming UI, combat and save compatibility for Temple Command."""
from pathlib import Path
import subprocess, tempfile, shutil

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build/party'
GCC = ROOT / 'tools/mingw32/bin/gcc.exe'
FLAGS = ['-std=gnu89', '-O2', '-DWIN32', '-DJP', '-DSJIS', '-finput-charset=EUC-JP', '-fexec-charset=CP932', '-I', str(ROOT/'src')]
h = (ROOT/'tests/party_integration.c').read_bytes().decode('euc_jp')
h = '#include "cmd1.c"\nextern void change_player_class(void);\n' + h
h = h.replace('static int clear_requests;', 'static int clear_requests;\nstatic bool check_naming;')
h = h.replace('        if (death_input) {', r'''
        if (check_naming) {
            int row,col,at=0; char display[24*81];
            for(row=0;row<24;row++) { for(col=0;col<80;col++) display[at++]=Term->scr->c[row][col]; display[at++]='\n'; }
            display[at-1]=0;
            assert(strstr(display,"追加効果") && strstr(display,"MP20"));
            assert(p_ptr->temple_tech_effects);
            check_naming=FALSE;
        }
        if (death_input) {
''')
start = h.index('    /* Q and')
end = h.index('\n}', start) + 2
body = r'''
    {
        int j,n,base;
        u32b effects, flags[TR_FLAG_SIZE];
        object_type weapon;
        monster_type *m=&m_list[1];
        auto_more=TRUE; character_generated=character_dungeon=FALSE;
        keys="                                                                 ";key_index=0;
        py=px=5;cur_hgt=cur_wid=20;dun_level=1;
        for(i=0;i<20;i++)for(j=0;j<20;j++) {
            cave[i][j].feat=(i==0||j==0||i==19||j==19)?FEAT_PERM_SOLID:FEAT_FLOOR;
            cave[i][j].info=CAVE_GLOW|CAVE_MARK|CAVE_VIEW;
        }
        p_ptr->pclass=CLASS_TEMPLEKNIGHT;p_ptr->prace=RACE_HUMAN;p_ptr->psex=SEX_MALE;
        p_ptr->chp=100;p_ptr->is_dead=0;party_rebind();party_reset();party_capture();
        party_count=3;party_members[1]=party_members[0];party_members[2]=party_members[0];
        quest[QUEST_FILARHH].status=QUEST_STATUS_FINISHED;
        chaos_frame[ETHNICITY_LODIS]=149;assert(!can_choose_class(CLASS_TEMPLECOMMAND,CLASS_CHOOSE_MODE_NORMAL));
        chaos_frame[ETHNICITY_LODIS]=150;assert(can_choose_class(CLASS_TEMPLECOMMAND,CLASS_CHOOSE_MODE_NORMAL));
        assert(!can_choose_class(CLASS_TEMPLECOMMAND,CLASS_CHOOSE_MODE_BIRTH));
        assert(!can_choose_class(CLASS_TEMPLECOMMAND,CLASS_CHOOSE_MODE_BLDGS));
        party_members[1].player.pclass=CLASS_WIZARD;
        assert(!can_choose_class(CLASS_TEMPLECOMMAND,CLASS_CHOOSE_MODE_NORMAL));
        party_members[1].dead=TRUE;assert(!can_choose_class(CLASS_TEMPLECOMMAND,CLASS_CHOOSE_MODE_NORMAL));
        party_members[1].dead=FALSE;party_members[1].player.pclass=CLASS_TEMPLECOMMAND;
        assert(!can_choose_class(CLASS_TEMPLECOMMAND,CLASS_CHOOSE_MODE_NORMAL));
        party_members[1].player.pclass=CLASS_TEMPLEKNIGHT;
        for(i=1;i<=2;i++) {
            party_count=i;assert(!can_choose_class(CLASS_TEMPLECOMMAND,CLASS_CHOOSE_MODE_NORMAL));
        }
        party_count=3;
        for(i=1;i<3;i++) {
            party_members[i].dead=TRUE;assert(!temple_command_eligible());party_members[i].dead=FALSE;
            party_members[i].player.is_dead=DEATH_DEAD;assert(!temple_command_eligible());party_members[i].player.is_dead=0;
            party_members[i].player.chp=-1;assert(!temple_command_eligible());party_members[i].player.chp=100;
        }
        assert(can_choose_class(CLASS_TEMPLECOMMAND,CLASS_CHOOSE_MODE_NORMAL));

        quest[QUEST_FILARHH].status=QUEST_STATUS_UNTAKEN;r_info[MON_FILARHH].r_pkills=0;
        assert(can_choose_class(CLASS_TEMPLECOMMAND,CLASS_CHOOSE_MODE_NORMAL));
        assert(party_exp_level_limit()==0);
        party_members[1].player.pclass=CLASS_WHITEKNIGHT;
        assert(party_exp_level_limit()==0);
        party_members[1].dead=TRUE;
        assert(party_can_revive(1));
        party_members[1].dead=FALSE;
        party_members[1].player.lev=36;
        assert(party_can_enter_class(CLASS_WHITEKNIGHT));
        assert(party_can_enter_class(CLASS_TEMPLEKNIGHT));
        assert(!party_can_enter_class(CLASS_TEMPLECOMMAND));
        party_members[1].player.lev=1;
        party_members[1].player.pclass=CLASS_GENERAL;
        assert(party_exp_level_limit()==35);
        assert(!party_can_enter_class(CLASS_TEMPLECOMMAND));
        party_members[1].player.pclass=CLASS_TEMPLEKNIGHT;
        assert(can_choose_class(CLASS_TEMPLECOMMAND,CLASS_CHOOSE_MODE_NORMAL));
        quest[QUEST_FILARHH].status=QUEST_STATUS_FINISHED;
        assert(class_info[CLASS_TEMPLECOMMAND].c_mhp==10 && class_info[CLASS_TEMPLECOMMAND].c_msp==4);
        assert(class_info[CLASS_TEMPLECOMMAND].c_flags & PCF_NO_CHANGE);
        assert(!memcmp(&s_info[CLASS_TEMPLECOMMAND],&s_info[CLASS_TEMPLEKNIGHT],sizeof(s_info[0])));
        if(argc==3 && !strcmp(argv[2],"legacy-write")) {
            p_ptr->is_dead=DEATH_DEAD; assert(save_player());return 0;
        }
        if(argc==3 && !strcmp(argv[2],"legacy-read")) {
            p_ptr->temple_tech_effects=1;strcpy(p_ptr->temple_tech_name,"Stale");
            assert(load_player());assert(party_count==3 && p_ptr->pclass==CLASS_TEMPLEKNIGHT);
            for(i=0;i<3;i++)assert(!party_members[i].player.temple_tech_effects && !party_members[i].player.temple_tech_name[0]);
            assert(!p_ptr->temple_tech_effects && !p_ptr->temple_tech_name[0]);
            puts("Version 6 / 40-class full save migration passed");return 0;
        }
        {
            char input[100]; int choice=0;
            for(i=0;i<CLASS_TEMPLECOMMAND;i++) {
                player_class *c=&class_info[i];
                if(i==p_ptr->pclass || !(c->c_flags & PCF_SEX_MALE) ||
                    (c->c_flags & (PCF_REINCARNATE|PCF_ALIEN)))continue;
                if((c->c_flags & PCF_SECRET) && !can_choose_class(i,CLASS_CHOOSE_MODE_NORMAL))continue;
                choice++;
            }
            sprintf(input,"%cy 昇格技\r                    ",choice<26?'a'+choice:'A'+choice-26);
            keys=input;key_index=0;change_player_class();
            assert(p_ptr->pclass==CLASS_TEMPLECOMMAND && p_ptr->temple_tech_effects);
            assert(!strcmp(p_ptr->temple_tech_name,"昇格技"));msg_print(NULL);
            p_ptr->temple_tech_effects=0;p_ptr->temple_tech_name[0]=0;
        }
        assert(temple_command_name_valid("一二三四五六七八九十"));
        assert(!temple_command_name_valid("一二三四五六七八九十一"));
        assert(!temple_command_name_valid("12345678901"));
        assert(!temple_command_name_valid(" ") && !temple_command_name_valid(""));
        check_naming=TRUE;keys="一二三四五六七八九十\r                    ";key_index=0;
        temple_command_learn();assert(!check_naming);
        assert(!strcmp(p_ptr->temple_tech_name,"一二三四五六七八九十"));
        effects=p_ptr->temple_tech_effects;n=0;for(;effects;effects&=effects-1)n++;
        assert(n>=1 && n<=3);
        effects=p_ptr->temple_tech_effects;msg_print(NULL);
        temple_command_learn();assert(p_ptr->temple_tech_effects==effects);
        for(i=0;i<60;i++) {
            p_ptr->temple_tech_effects=0;keys="Test\r                    ";key_index=0;
            temple_command_learn();msg_print(NULL);effects=p_ptr->temple_tech_effects;n=0;
            assert(!(effects & ~((1UL<<TEMPLE_EFFECT_COUNT)-1)));
            for(;effects;effects&=effects-1)n++;
            assert(n>=1 && n<=3);
        }
        p_ptr->temple_tech_effects=(1UL<<1)|TEMPLE_KNOCKBACK;strcpy(p_ptr->temple_tech_name,"酸撃");
        party_capture();
        party_members[1].player.temple_tech_effects=(1UL<<3);
        strcpy(party_members[1].player.temple_tech_name,"炎撃");
        strcpy(party_members[1].name,"Reserve");
        assert(party_switch(1));assert(!strcmp(p_ptr->temple_tech_name,"炎撃"));
        assert(party_switch(0));assert(!strcmp(p_ptr->temple_tech_name,"酸撃"));
        assert(save_player());party_reset();p_ptr->temple_tech_effects=0;p_ptr->temple_tech_name[0]=0;
        assert(load_player());assert(party_count==3);
        assert(!strcmp(p_ptr->temple_tech_name,"酸撃") && p_ptr->temple_tech_effects==((1UL<<1)|TEMPLE_KNOCKBACK));
        assert(!strcmp(party_members[1].player.temple_tech_name,"炎撃"));
        party_count=1;party_active=0;p_ptr->is_dead=0;p_ptr->chp=p_ptr->mhp=10000;
        p_ptr->pclass=CLASS_TEMPLECOMMAND;party_rebind();
        p_ptr->migite=TRUE;p_ptr->hidarite=FALSE;p_ptr->num_blow[0]=1;
        p_ptr->skill_thn=10000;p_ptr->afraid=p_ptr->confused=p_ptr->stun=0;
        p_ptr->to_d[0]=p_ptr->to_h[0]=p_ptr->to_dd[0]=p_ptr->to_ds[0]=0;
        memset(inventory,0,sizeof(object_type)*INVEN_TOTAL);
        object_prep(&weapon,lookup_kind(TV_SWORD,SV_LONG_SWORD));weapon.dd=2;weapon.ds=6;weapon.to_h=100;
        inventory[INVEN_RARM]=weapon;
        memset(m,0,sizeof(*m));m->r_idx=m->ap_r_idx=1;m->fy=5;m->fx=7;m->ml=TRUE;
        m->maxhp=m->hp=30000;m_max=2;m_cnt=1;cave[5][7].m_idx=1;
        r_info[1].flags1=r_info[1].flags2=r_info[1].flags3=r_info[1].flagsr=0;r_info[1].ac=0;
        set_hostile(m);
        temple_attack_effects=0;base=tot_dam_aux(&weapon,10,m,TRUE);
        temple_attack_effects=1UL<<1;assert(tot_dam_aux(&weapon,10,m,TRUE)>base);
        assert(tot_dam_aux(&weapon,10,m,FALSE)==base);temple_attack_effects=0;
        assert(tot_dam_aux(&weapon,10,m,TRUE)==base);
        memset(flags,0,sizeof(flags));temple_command_flags((1UL<<0)|(1UL<<1),flags);
        assert(have_flag(flags,TR_VORPAL) && have_flag(flags,TR_BRAND_ACID));
        p_ptr->temple_tech_effects=TEMPLE_KNOCKBACK;
        keys="                                     ";key_index=0;
        for(i=0;i<100 && cave[5][7].m_idx;i++)temple_command_attack(5,7);
        assert(cave[5][8].m_idx==1 && !cave[5][7].m_idx);
        assert(!memcmp(&inventory[INVEN_RARM],&weapon,sizeof(weapon)));
        assert(!temple_attack_effects);
        cave[5][8].m_idx=0;cave[5][7].m_idx=1;m->fx=7;cave[5][8].feat=FEAT_PERM_SOLID;
        for(i=0;i<5;i++)temple_command_attack(5,7);
        assert(cave[5][7].m_idx==1);cave[5][8].feat=FEAT_FLOOR;
        cave[5][6].feat=FEAT_WALL_EXTRA;assert(!temple_command_attack(5,7));cave[5][6].feat=FEAT_FLOOR;
        cave[5][7].m_idx=0;cave[5][8].m_idx=1;m->fx=8;
        assert(!temple_command_attack(5,8));cave[5][8].m_idx=0;cave[5][7].m_idx=1;m->fx=7;
        p_ptr->csp=19;temple_command_power();assert(p_ptr->csp==19 && !energy_use);msg_print(NULL);
        p_ptr->csp=50;repeat_clear();keys="\033";key_index=0;temple_command_power();assert(p_ptr->csp==50 && !energy_use);
        repeat_clear();keys="6                    ";key_index=0;temple_command_power();assert(p_ptr->csp==30 && energy_use==100);
        assert(!temple_attack_effects);
        puts("Temple Command: entry, displayed effects, names, rolls, switching, save/load, brands, range, knockback, MP and cancellation passed");
    }
    return 0;
}
'''
h = h[:start] + body + h[end:]
with tempfile.TemporaryDirectory(prefix='toband-temple-command-') as directory:
    tmp = Path(directory)
    shutil.copytree(BUILD/'lib', tmp/'lib', ignore=shutil.ignore_patterns('*.raw','save'))
    for p in (ROOT/'lib/edit').glob('*.txt'):
        (tmp/'lib/edit'/p.name).write_bytes(p.read_bytes().decode('euc_jp').encode('cp932'))
    (tmp/'lib/save').mkdir(exist_ok=True)
    (tmp/'test.c').write_bytes(h.encode('euc_jp'))
    objs = [str(p) for p in BUILD.glob('*.o') if p.stem not in ('main-win','angband','cmd1','store')]
    store=(ROOT/'src/store.c').read_bytes().decode('euc_jp').replace('static void change_player_class(void)','void change_player_class(void)')
    (tmp/'store.c').write_bytes(store.encode('euc_jp'))
    subprocess.run([str(GCC),*FLAGS,'-c',str(tmp/'store.c'),'-o',str(tmp/'store.o')],check=True)
    objs.append(str(tmp/'store.o'))
    subprocess.run([str(GCC),*FLAGS,str(tmp/'test.c'),*objs,'-o',str(tmp/'test.exe'),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    def run(exe,mode=None):
        r=subprocess.run([str(exe),(tmp/'lib').as_posix()+'/']+([mode] if mode else []),cwd=tmp,capture_output=True,timeout=90)
        print(r.stdout.decode('cp932',errors='replace'))
        if r.returncode:
            print(r.stderr.decode('cp932',errors='replace'))
            raise SystemExit(r.returncode)
    run(tmp/'test.exe')
    old=(ROOT/'src/save.c').read_bytes().decode('euc_jp')
    old=old.replace('#include "party.h"','#include "party.h"\n#undef MAX_CLASS\n#define MAX_CLASS 40')
    old=old.replace('wr_byte(PARTY_SAVE_VERSION);','wr_byte(6);')
    old=old.replace('        /* Party v7: personal named technique. */\n        wr_string(member->player.temple_tech_name);\n        wr_u32b(member->player.temple_tech_effects);\n','')
    (tmp/'old-save.c').write_bytes(old.encode('euc_jp'))
    subprocess.run([str(GCC),*FLAGS,'-c',str(tmp/'old-save.c'),'-o',str(tmp/'old-save.o')],check=True)
    oldobjs=[x for x in objs if Path(x).stem!='save']
    subprocess.run([str(GCC),*FLAGS,str(tmp/'test.c'),*oldobjs,str(tmp/'old-save.o'),'-o',str(tmp/'old.exe'),'-lwinmm','-lcomdlg32','-lgdi32'],check=True)
    run(tmp/'old.exe','legacy-write')
    run(tmp/'test.exe','legacy-read')
