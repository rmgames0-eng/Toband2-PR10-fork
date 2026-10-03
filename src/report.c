/* File: report.c */

#define _GNU_SOURCE
#include "angband.h"
#include "party.h"

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#if defined(WINDOWS)
#include <winsock.h>
#elif defined(MACINTOSH)
#include <OpenTransport.h>
#include <OpenTptInternet.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>

#include <setjmp.h>
#include <signal.h>
#endif

/*
 * simple buffer library
 */

typedef struct {
	size_t max_size;
	size_t size;
	char *data;
} BUF;

#define	BUFSIZE	(65536)

#ifndef HAVE_VASPRINTF
#define vasprintf	Vasprintf

static int Vasprintf(char **buf, const char *fmt, va_list ap)
{
	int ret;

	*buf = malloc(1024);

#if defined(HAVE_VSNPRINTF)
	ret = vsnprintf(*buf, 1024, fmt, ap);
#else
	ret = vsprintf(*buf, fmt, ap);
#endif
	return ret;
}

#endif /* ifndef HAVE_VASPRINTF */ 

static BUF* buf_new(void)
{
	BUF *p;

	if ((p = malloc(sizeof(BUF))) == NULL)
		return NULL;

	p->size = 0;
	p->max_size = BUFSIZE;
	if ((p->data = malloc(BUFSIZE)) == NULL)
	{
		free(p);
		return NULL;
	}
	return p;
}

static void buf_delete(BUF *b)
{
	free(b->data);
	free(b);
}

static int buf_append(BUF *buf, const char *data, size_t size)
{
	while (buf->size + size > buf->max_size)
	{
		char *tmp;
		if ((tmp = malloc(buf->max_size * 2)) == NULL) return -1;

		memcpy(tmp, buf->data, buf->max_size);
		free(buf->data);

		buf->data = tmp;

		buf->max_size *= 2;
	}
	memcpy(buf->data + buf->size, data, size);
	buf->size += size;

	return buf->size;
}

static int buf_sprintf(BUF *buf, const char *fmt, ...)
{
	int		ret;
	char	*tmpbuf;
	va_list	ap;

	va_start(ap, fmt);
	vasprintf(&tmpbuf, fmt, ap);
	va_end(ap);

	if(!tmpbuf) return -1;

#ifdef MAC_MPW
	{
		/* '\n' is 0x0D and '\r' is 0x0A in MPW. Swap back these. */
		char *ptr;
		for (ptr = tmpbuf; *ptr; ptr++)
			if ('\n' == *ptr) *ptr = '\r';
	}
#endif

	ret = buf_append(buf, tmpbuf, strlen(tmpbuf));

	free(tmpbuf);

	return ret;
}


/*
 * Make screen dump to buffer
 */
cptr make_screen_dump(void)
{
	BUF *screen_buf;
	int y, x, i;
	cptr ret;

	byte a = 0, old_a = 0;
	char c = ' ';

	static cptr html_head[] = {
		"<html>\n<body text=\"#ffffff\" bgcolor=\"#000000\">\n",
		"<pre>",
		0,
	};
	static cptr html_foot[] = {
		"</pre>\n",
		"</body>\n</html>\n",
		0,
	};

	int wid, hgt;

	Term_get_size(&wid, &hgt);

	/* Alloc buffer */
	screen_buf = buf_new();
	if (screen_buf == NULL) return (NULL);

	for (i = 0; html_head[i]; i++)
		buf_sprintf(screen_buf, html_head[i]);

	/* Dump the screen */
	for (y = 0; y < hgt; y++)
	{
		/* Start the row */
		if (y != 0)
			buf_sprintf(screen_buf, "\n");

		/* Dump each row */
		for (x = 0; x < wid - 1; x++)
		{
			int rv, gv, bv;
			cptr cc = NULL;
			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));

			switch (c)
			{
			case '&': cc = "&amp;"; break;
			case '<': cc = "&lt;"; break;
			case '>': cc = "&gt;"; break;
#ifdef WINDOWS
			case 0x1f: c = '.'; break;
			case 0x7f: c = (a == 0x09) ? '%' : '#'; break;
#endif
			}

			a = a & 0x0F;
			if ((y == 0 && x == 0) || a != old_a) {
				rv = angband_color_table[a][1];
				gv = angband_color_table[a][2];
				bv = angband_color_table[a][3];
				buf_sprintf(screen_buf, "%s<font color=\"#%02x%02x%02x\">", 
					    ((y == 0 && x == 0) ? "" : "</font>"), rv, gv, bv);
				old_a = a;
			}
			if (cc)
				buf_sprintf(screen_buf, "%s", cc);
			else
				buf_sprintf(screen_buf, "%c", c);
		}
	}
	buf_sprintf(screen_buf, "</font>");

	for (i = 0; html_foot[i]; i++)
		buf_sprintf(screen_buf, html_foot[i]);

	/* Screen dump size is too big ? */
	if (screen_buf->size + 1> SCREEN_BUF_SIZE)
	{
		ret = NULL;
	}
	else
	{
		/* Terminate string */
		buf_append(screen_buf, "", 1);

		ret = string_make(screen_buf->data);
	}

	/* Free buffer */
	buf_delete(screen_buf);

	return ret;
}

/* Public score submission. Never upload a save or write a local submission file. */
static bool score_form_field(BUF *b, cptr key, cptr value)
{
    static const char hex[] = "0123456789ABCDEF";
    const unsigned char *p = (const unsigned char *)value;
    if (b->size && buf_append(b, "&", 1) < 0) return FALSE;
    if (buf_append(b, key, strlen(key)) < 0 || buf_append(b, "=", 1) < 0) return FALSE;
    for (; *p; p++)
    {
        char escaped[3];
        if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
            (*p >= '0' && *p <= '9') || *p == '-' || *p == '_' || *p == '.' || *p == '~')
        {
            if (buf_append(b, (const char *)p, 1) < 0) return FALSE;
        }
        else
        {
            escaped[0] = '%'; escaped[1] = hex[*p >> 4]; escaped[2] = hex[*p & 15];
            if (buf_append(b, escaped, 3) < 0) return FALSE;
        }
    }
    return TRUE;
}

static BUF *score_make_payload(cptr comment)
{
    FILE *file;
    long length;
    char *dump, number[64], key[64];
    int m, count = party_count ? party_count : 1;
    BUF *b;
    file = tmpfile();
    if (!file) return NULL;
    if (make_character_dump(file) || fflush(file) || (length = ftell(file)) <= 0 || length > 500000L)
    { fclose(file); return NULL; }
    rewind(file);
    dump = malloc(length + 1);
    if (!dump) { fclose(file); return NULL; }
    if (fread(dump, 1, length, file) != (size_t)length) { free(dump); fclose(file); return NULL; }
    fclose(file); dump[length] = 0;
    b = buf_new();
    if (!b) { free(dump); return NULL; }
#define SCORE_FIELD(k,v) do { if (!score_form_field(b,(k),(v))) goto fail; } while (0)
#define SCORE_NUMBER(k,v) do { sprintf(number,"%ld",(long)(v)); SCORE_FIELD(k,number); } while (0)
    SCORE_FIELD("protocol", "1");
#if defined(SJIS)
    SCORE_FIELD("encoding", "cp932");
#elif defined(JP)
    SCORE_FIELD("encoding", "euc-jp");
#else
    SCORE_FIELD("encoding", "utf-8");
#endif
    sprintf(number, "%d.%d.%d.%d", T_VER_MAJOR,T_VER_MINOR,T_VER_PATCH,T_VER_EXTRA);
    SCORE_FIELD("version", number);
    SCORE_FIELD("character", player_name);
    SCORE_FIELD("comment", comment);
    SCORE_FIELD("race", p_name + rp_ptr->name);
    SCORE_FIELD("class", c_name + cp_ptr->name);
    SCORE_FIELD("cause", p_ptr->died_from);
    SCORE_FIELD("outcome", p_ptr->total_winner ? "winner" : "dead");
    SCORE_NUMBER("level", p_ptr->lev);
    SCORE_NUMBER("class_level", p_ptr->cexp_info[p_ptr->pclass].clev);
    SCORE_NUMBER("score", total_points());
    SCORE_NUMBER("turns", turn_real(turn));
    SCORE_NUMBER("depth", dun_level);
    SCORE_NUMBER("party_count", count);
    for (m = 0; m < count; ++m)
    {
        bool active = !party_count || m == party_active;
        const player_type *p = active ? p_ptr : &party_members[m].player;
#define MEMBER_FIELD(k,v) do { sprintf(key,"party_%d_%s",m,(k)); SCORE_FIELD(key,(v)); } while (0)
#define MEMBER_NUMBER(k,v) do { sprintf(number,"%ld",(long)(v)); MEMBER_FIELD((k),number); } while (0)
        MEMBER_FIELD("name", active ? player_name : party_members[m].name);
        MEMBER_FIELD("race", p_name + race_info[p->prace].name);
        MEMBER_FIELD("class", c_name + class_info[p->pclass].name);
        MEMBER_NUMBER("level", p->lev);
        MEMBER_NUMBER("class_level", p->cexp_info[p->pclass].clev);
        MEMBER_NUMBER("active", active);
        MEMBER_NUMBER("dead", active ? p->is_dead != 0 : party_members[m].dead);
#undef MEMBER_FIELD
#undef MEMBER_NUMBER
    }
    SCORE_FIELD("dump", dump);
#undef SCORE_FIELD
#undef SCORE_NUMBER
    free(dump);
    if (b->size > 1572864) { buf_delete(b); return NULL; }
    return b;
fail:
    free(dump); buf_delete(b); return NULL;
}

#ifdef WINDOWS
#include <winhttp.h>

/* Resolve WinHTTP dynamically so non-network tools and old build files still link. */
static bool score_https_post(BUF *b)
{
    HMODULE module;
    HINTERNET session = NULL, connection = NULL, request = NULL;
    HINTERNET (WINAPI *http_open)(LPCWSTR,DWORD,LPCWSTR,LPCWSTR,DWORD);
    HINTERNET (WINAPI *http_connect)(HINTERNET,LPCWSTR,INTERNET_PORT,DWORD);
    HINTERNET (WINAPI *http_request)(HINTERNET,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR *,DWORD);
    BOOL (WINAPI *http_timeouts)(HINTERNET,int,int,int,int);
    BOOL (WINAPI *http_option)(HINTERNET,DWORD,LPVOID,DWORD);
    BOOL (WINAPI *http_send)(HINTERNET,LPCWSTR,DWORD,LPVOID,DWORD,DWORD,DWORD_PTR);
    BOOL (WINAPI *http_receive)(HINTERNET,LPVOID);
    BOOL (WINAPI *http_headers)(HINTERNET,DWORD,LPCWSTR,LPVOID,LPDWORD,LPDWORD);
    BOOL (WINAPI *http_read)(HINTERNET,LPVOID,DWORD,LPDWORD);
    BOOL (WINAPI *http_close)(HINTERNET);
    DWORD status = 0, size = sizeof(status), got = 0, used = 0;
    DWORD redirect_policy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    char response[1024], dll_path[MAX_PATH];
    bool success = FALSE;
    UINT n = GetSystemDirectoryA(dll_path, sizeof(dll_path));
    if (!n || n + 13 >= sizeof(dll_path)) return FALSE;
    strcat(dll_path, "\\winhttp.dll");
    module = LoadLibraryA(dll_path);
    if (!module) return FALSE;
#define HTTP_LOAD(var, name) do { *(FARPROC *)&var = GetProcAddress(module, name); if (!var) goto done; } while (0)
    HTTP_LOAD(http_open, "WinHttpOpen");
    HTTP_LOAD(http_connect, "WinHttpConnect");
    HTTP_LOAD(http_request, "WinHttpOpenRequest");
    HTTP_LOAD(http_timeouts, "WinHttpSetTimeouts");
    HTTP_LOAD(http_option, "WinHttpSetOption");
    HTTP_LOAD(http_send, "WinHttpSendRequest");
    HTTP_LOAD(http_receive, "WinHttpReceiveResponse");
    HTTP_LOAD(http_headers, "WinHttpQueryHeaders");
    HTTP_LOAD(http_read, "WinHttpReadData");
    HTTP_LOAD(http_close, "WinHttpCloseHandle");
#undef HTTP_LOAD
    session = http_open(L"TOband-R3/score-1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!session || !http_timeouts(session,5000,5000,10000,15000)) goto done;
    connection = http_connect(session,L"toband-wiki.duckdns.org",INTERNET_DEFAULT_HTTPS_PORT,0);
    if (!connection) goto done;
    request = http_request(connection,L"POST",L"/score/submit.php",NULL,NULL,NULL,WINHTTP_FLAG_SECURE);
    if (!request || !http_option(request,WINHTTP_OPTION_REDIRECT_POLICY,&redirect_policy,sizeof(redirect_policy))) goto done;
    if (!http_send(request,L"Content-Type: application/x-www-form-urlencoded\r\n",(DWORD)-1,b->data,b->size,b->size,0)) goto done;
    if (!http_receive(request,NULL)) goto done;
    if (!http_headers(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,NULL,&status,&size,NULL)) goto done;
    if (status != 200 && status != 201) goto done;
    do {
        if (!http_read(request,response+used,sizeof(response)-1-used,&got)) goto done;
        used += got;
    } while (got && used < sizeof(response)-1);
    response[used] = 0;
    success = strstr(response,"\"ok\":true") != NULL;
done:
    if (request) http_close(request);
    if (connection) http_close(connection);
    if (session) http_close(session);
    FreeLibrary(module);
    return success;
}
#endif

#ifdef JP
#define SCORE_TEXT(jp,en) (jp)
#else
#define SCORE_TEXT(jp,en) (en)
#endif

void report_score(void)
{
    char comment[160] = "";
    BUF *payload;
    if (!get_check(SCORE_TEXT("スコアサーバに送信しますか？ ", "Send your score to the score server? "))) return;
    msg_print(SCORE_TEXT("名前・装備・持ち物・仲間を含むダンプが公開されます。", "The dump includes your name, equipment, inventory and companions."));
    msg_print(NULL);
    if (!get_string(SCORE_TEXT("遺言（空欄可）: ", "Last words (optional): "), comment, sizeof(comment)-1)) return;
    payload = score_make_payload(comment);
    if (!payload) { msg_print(SCORE_TEXT("スコアの送信データを作成できませんでした。", "Could not create the score submission.")); return; }
    msg_print(SCORE_TEXT("スコアを送信しています…", "Sending score..."));
    Term_fresh();
#ifdef WINDOWS
    if (score_https_post(payload))
        msg_print(SCORE_TEXT("スコアを送信しました。https://toband-wiki.duckdns.org/score/", "Score sent: https://toband-wiki.duckdns.org/score/"));
    else
#endif
    {
        msg_print(SCORE_TEXT("スコアを送信できませんでした。", "Could not send the score."));

    }
    buf_delete(payload);
    msg_print(NULL);
}
