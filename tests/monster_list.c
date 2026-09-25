#include "angband.h"
#include <assert.h>
static term test_term;
static int clear_requests;
static errr test_xtra(int n,int v) { if(n==TERM_XTRA_CLEAR)clear_requests++; if(n==TERM_XTRA_EVENT && v)Term_keypress(' '); return 0; }
static errr test_curs(int x,int y) { return 0; }
static errr test_wipe(int x,int y,int n) { return 0; }
static errr test_text(int x,int y,int n,byte a,cptr s) { return 0; }
static void fail_quit(cptr s) { if(s)fprintf(stderr,"%s\n",s);exit(3); }
static void test_visible_monster_window(void)
{
    term sub;
    monster_type saved_monsters[7];
    monster_race saved_races[4];
    char *saved_names = r_name;
    char names[] = "\0Soldier\0Unique\0Secret";
    char row[81];
    int saved_max = m_max, x, i;
    u32b saved_window = p_ptr->window, saved_rng = Rand_value;
    memcpy(saved_monsters, m_list, sizeof(saved_monsters));
    memcpy(saved_races, r_info, sizeof(saved_races));
    memset(m_list, 0, sizeof(saved_monsters));
    memset(r_info, 0, sizeof(saved_races));
    r_name = names;
    r_info[1].name=1; r_info[1].d_char='p'; r_info[1].level=12; r_info[1].r_tkills=1;
    r_info[2].name=9; r_info[2].d_char='U'; r_info[2].flags1=RF1_UNIQUE;
    r_info[3].name=16; r_info[3].d_char='S';
    for(i=1;i<7;i++) { m_list[i].r_idx=m_list[i].ap_r_idx=1; m_list[i].ml=TRUE; }
    MON_CSLEEP(&m_list[2])=10;
    m_list[3].r_idx=m_list[3].ap_r_idx=2;
    m_list[4].r_idx=m_list[4].ap_r_idx=3; m_list[4].ml=FALSE;
    m_list[5].smart1=SM1_PET;
    m_list[6].r_idx=0;
    /* A disguised monster must not leak its real race. */
    m_list[1].r_idx=3;
    m_max=7;
    term_init(&sub,80,5,256);
    sub.xtra_hook=test_xtra; sub.curs_hook=test_curs;
    sub.wipe_hook=test_wipe; sub.text_hook=test_text;
    angband_term[1]=&sub; window_flag[1]=PW_MONLIST;
    p_ptr->window=0; window_stuff();
    assert(Term==&test_term);
    for(x=0;x<80;x++) row[x]=sub.scr->c[0][x]; row[80]=0;
    assert(strstr(row,"Unique") && strstr(row,"??"));
    for(x=0;x<80;x++) row[x]=sub.scr->c[1][x]; row[80]=0;
    assert(strstr(row,"Soldier") && strstr(row,"12"));
    assert(row[2]=='2' && row[7]=='1');
    i=clear_requests; window_stuff(); assert(clear_requests==i);
    MON_CSLEEP(&m_list[2])=0;
    window_stuff(); assert(sub.scr->c[1][7]=='2');
    m_list[1].ml=m_list[2].ml=FALSE;
    window_stuff(); assert(sub.scr->c[1][2]==' ');
    p_ptr->image=1; window_stuff();
    for(x=0;x<80;x++) row[x]=sub.scr->c[0][x]; row[80]=0;
    assert(!strstr(row,"Unique"));
    p_ptr->image=0;
    m_list[1].ml=m_list[2].ml=TRUE;
    Term_activate(&sub); Term_resize(8,1); Term_activate(&test_term);
    window_stuff(); assert(Term==&test_term);
    assert(Rand_value==saved_rng);
    angband_term[1]=NULL; window_flag[1]=0; term_nuke(&sub);
    memcpy(m_list,saved_monsters,sizeof(saved_monsters));
    memcpy(r_info,saved_races,sizeof(saved_races));
    r_name=saved_names; m_max=saved_max; p_ptr->window=saved_window;
}
int main(int argc,char **argv) {
 char path[1024];assert(argc==2);quit_aux=fail_quit;
 term_init(&test_term,80,24,256);test_term.xtra_hook=test_xtra;test_term.curs_hook=test_curs;
 test_term.wipe_hook=test_wipe;test_term.text_hook=test_text;
 Term_activate(&test_term);angband_term[0]=&test_term;
 strcpy(path,argv[1]);init_file_paths(path);init_angband();Rand_quick=TRUE;Rand_value=12345;
 test_visible_monster_window();puts("Visible monster window: grouping, disguise, pets, sleep, hallucination, resize and RNG passed");return 0;
}
