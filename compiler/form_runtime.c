// BasicForge runtime C source
#ifndef WINAPI_FAMILY_PARTITION
#define WINAPI_FAMILY_PARTITION(x) 1
#endif
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <io.h>
#include <setjmp.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
// UCRT cdecl names are underscore-prefixed; map the standard names so
// shared-library links resolve against ucrt.lib.


// ---- Minimal Win32 declarations (avoid windows.h macro conflicts) ----
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HMODULE;
typedef void* HANDLE;
typedef unsigned int UINT;
typedef unsigned short WORD;
typedef unsigned long long WPARAM;
typedef long long LPARAM;
typedef long long LRESULT;
typedef unsigned int DWORD;
typedef DWORD* LPDWORD;
typedef int BOOL;
typedef int INT;
typedef void* LPVOID;
typedef void* HGLOBAL;
typedef void* HDROP;
typedef unsigned long long SIZE_T;
typedef const char* LPCSTR;
typedef char* LPSTR;
typedef long long LONG_PTR;
typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef struct { long x, y; } POINT;
typedef struct {
    HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; DWORD time; POINT pt;
} MSG;
typedef struct {
    UINT style; WNDPROC lpfnWndProc; int cbClsExtra; int cbWndExtra;
    HINSTANCE hInstance; void* hIcon; void* hCursor; void* hbrBackground;
    LPCSTR lpszMenuName; LPCSTR lpszClassName;
} WNDCLASS;
HMODULE GetModuleHandleA(LPCSTR);
WORD RegisterClassA(const WNDCLASS*);
HWND CreateWindowExA(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, void*, HINSTANCE, void*);
int ShowWindow(HWND, int);
BOOL UpdateWindow(HWND);
BOOL GetMessageA(MSG*, HWND, UINT, UINT);
BOOL TranslateMessage(const MSG*);
LRESULT DispatchMessageA(const MSG*);
LRESULT DefWindowProcA(HWND, UINT, WPARAM, LPARAM);
void PostQuitMessage(int);
int GetWindowTextA(HWND, LPSTR, int);
BOOL SetWindowTextA(HWND, LPCSTR);
DWORD GetLastError(void);
BOOL PostMessageA(HWND, UINT, WPARAM, LPARAM);
LONG_PTR SetWindowLongPtrA(HWND, int, LONG_PTR);
LRESULT CallWindowProcA(WNDPROC, HWND, UINT, WPARAM, LPARAM);
#define GetModuleHandle GetModuleHandleA
#define RegisterClass RegisterClassA
#define DefWindowProc DefWindowProcA
#define GetMessage GetMessageA
#define DispatchMessage DispatchMessageA
#define GetWindowText GetWindowTextA
#define SetWindowText SetWindowTextA
#define WS_CHILD 0x40000000u
#define WS_VISIBLE 0x10000000u
#define WS_BORDER 0x00800000u
#define WS_OVERLAPPEDWINDOW 0x00CF0000u
#define CW_USEDEFAULT ((int)0x80000000)
#define SW_SHOW 5
#define BS_PUSHBUTTON 0
#define BS_AUTOCHECKBOX 3
#define BS_AUTORADIOBUTTON 9
#define BS_GROUPBOX 7
#define ES_AUTOHSCROLL 0x80
#define CBS_DROPDOWN 2
#define BN_CLICKED 0
#define EN_CHANGE 0x300
#define WM_COMMAND 0x0111
#define WM_DESTROY 0x0002
#define WM_CLOSE 0x0010
#define WM_SETFOCUS 0x0007
#define WM_KILLFOCUS 0x0008
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_MOUSEMOVE 0x0200
#define GWLP_WNDPROC (-4)
#define ERROR_CLASS_ALREADY_EXISTS 1410
#define TRUE 1
#define FALSE 0
#define WINAPI

// ---- Extended standard library runtime ----
typedef struct { DWORD dwSize; DWORD dwICC; } INITCOMMONCONTROLSEX;
BOOL InitCommonControlsEx(const INITCOMMONCONTROLSEX*);
#define ICC_BAR_CLASSES 0x00000004
#define ICC_PROGRESS_CLASS 0x00000020
#define ICC_LISTVIEW_CLASSES 0x00000001
#define ICC_TREEVIEW_CLASSES 0x00000002
#define ICC_DATE_CLASSES 0x00000100
#define ICC_TAB_CLASSES 0x00000008
#define ICC_UPDOWN_CLASS 0x00000010
#define ICC_COOL_CLASSES 0x00000400
#define ICC_HOTKEY_CLASS 0x00000040
#define ICC_STANDARD_CLASSES 0x00004000
#define SBS_HORZ 0
#define TBS_HORZ 0
#define DTS_SHORTDATEFORMAT 0
#define LVS_REPORT 0
#define ES_MULTILINE 0x0004
#define ES_AUTOVSCROLL 0x0040
#define WS_VSCROLL 0x00200000
#define TCS_TABS 0
#define WM_HSCROLL 0x0114
#define WM_MOUSEWHEEL 0x020A
#define WM_PAINT 0x000F
#define WM_DROPFILES 0x0233
#define BN_DOUBLECLICKED 5
#define CBN_SELCHANGE 1
#define LBN_SELCHANGE 1
void Sleep(DWORD);
BOOL Beep(DWORD, DWORD);
void DragAcceptFiles(HWND, BOOL);
UINT DragQueryFileA(HDROP, UINT, LPSTR, UINT);
void DragFinish(HDROP);
BOOL SetConsoleTitleA(LPCSTR);
HINSTANCE ShellExecuteA(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT);
BOOL OpenClipboard(HWND);
BOOL EmptyClipboard(void);
HANDLE SetClipboardData(UINT, HANDLE);
HANDLE GetClipboardData(UINT);
BOOL CloseClipboard(void);
HGLOBAL GlobalAlloc(UINT, SIZE_T);
LPVOID GlobalLock(HGLOBAL);
BOOL GlobalUnlock(HGLOBAL);
typedef const wchar_t* LPCWSTR;
int WideCharToMultiByte(UINT, DWORD, LPCWSTR, int, LPSTR, int, LPCSTR, LPVOID);
int MessageBoxA(HWND, LPCSTR, LPCSTR, UINT);

static char* bf_strdup(const char* s) {
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}
char* bf_upper(const char* s) {
    char* r = bf_strdup(s ? s : "");
    for (char* p = r; *p; ++p) *p = (char)toupper((unsigned char)*p);
    return r;
}
char* bf_lower(const char* s) {
    char* r = bf_strdup(s ? s : "");
    for (char* p = r; *p; ++p) *p = (char)tolower((unsigned char)*p);
    return r;
}
char* bf_chr(int c) {
    char* r = (char*)malloc(2);
    if (!r) return bf_strdup("");
    r[0] = (char)c;
    r[1] = 0;
    return r;
}
int bf_asc(const char* s) {
    return (s && s[0]) ? (int)(unsigned char)s[0] : 0;
}
int bf_file_exists(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}
char* bf_read_all_text(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return bf_strdup("");
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0) n = 0;
    char* buf = (char*)malloc((size_t)n + 1);
    if (!buf) { fclose(f); return bf_strdup(""); }
    size_t rd = fread(buf, 1, (size_t)n, f);
    buf[rd] = 0;
    fclose(f);
    return buf;
}
int bf_write_all_text(const char* path, const char* text) {
    FILE* f = fopen(path, "wb");
    if (!f) return 0;
    fputs(text ? text : "", f);
    fclose(f);
    return 1;
}
int bf_append_all_text(const char* path, const char* text) {
    FILE* f = fopen(path, "ab");
    if (!f) return 0;
    fputs(text ? text : "", f);
    fclose(f);
    return 1;
}
char* bf_get_env(const char* name) {
    const char* v = getenv(name);
    return bf_strdup(v ? v : "");
}
int bf_set_env(const char* name, const char* value) {
    return _putenv_s(name, value) == 0 ? 1 : 0;
}
void bf_sleep(int ms) { Sleep((DWORD)ms); }
void bf_beep(int freq, int ms) { Beep((DWORD)freq, (DWORD)ms); }
void bf_clear_screen(void) { system("cls"); }
void bf_set_title(const char* title) { SetConsoleTitleA(title ? title : ""); }
char* bf_read_line(void) {
    char buf[4096];
    if (!fgets(buf, sizeof(buf), stdin)) return bf_strdup("");
    size_t n = strlen(buf);
    while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) buf[--n] = 0;
    return bf_strdup(buf);
}
void bf_message_box(const char* text, const char* caption) {
    MessageBoxA(NULL, text ? text : "", caption ? caption : "", 0);
}
void bf_shell_execute(const char* file) {
    ShellExecuteA(NULL, "open", file ? file : "", NULL, NULL, 1);
}
char* bf_clipboard_get(void) {
    char* result = bf_strdup("");
    if (!OpenClipboard(NULL)) return result;
    HANDLE h = GetClipboardData(1);
    if (h) {
        char* p = (char*)GlobalLock(h);
        if (p) { free(result); result = bf_strdup(p); GlobalUnlock(h); }
    }
    CloseClipboard();
    return result;
}
void bf_clipboard_set(const char* text) {
    if (!text || !OpenClipboard(NULL)) return;
    EmptyClipboard();
    size_t n = strlen(text) + 1;
    HGLOBAL h = GlobalAlloc(0x0042, n);
    if (h) {
        char* p = (char*)GlobalLock(h);
        if (p) { memcpy(p, text, n); GlobalUnlock(h); SetClipboardData(1, h); }
    }
    CloseClipboard();
}

// PHASE2_RUNTIME_UPDATE
static jmp_buf bfc_exc_buf;
static char* bfc_exc_value = NULL;
static int bfc_exc_code = 0;

static char* bf_str_copy(char* dst, const char* src);
static int bf_str_len(const char* s);

extern void Main(void);

void bf_print_string(const char* s) { if (s) { printf("%s\n", s); fflush(stdout); } }
void bf_print_int(int64_t v) { char buf[32]; sprintf(buf, "%lld\n", v); printf("%s", buf); }
void bf_print_double(double v) { char buf[64]; sprintf(buf, "%g\n", v); printf("%s", buf); }
void bf_print_bool(int v) { printf(v ? "True\n" : "False\n"); }

static char* bf_str_copy(char* dst, const char* src) {
    char* d = dst;
    while ((*d++ = *src++) != 0) ;
    return dst;
}

static int bf_str_len(const char* s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}

char* bf_str_concat(const char* a, const char* b) {
    static char buf[8192];
    if (!a) a = "";
    if (!b) b = "";
    int la = bf_str_len(a);
    int lb = bf_str_len(b);
    if (la + lb >= 8192) lb = 8191 - la;
    bf_str_copy(buf, a);
    bf_str_copy(buf + la, b);
    return buf;
}

int64_t bf_cint(double v) { return (int64_t)v; }
int64_t bf_clng(double v) { return (int64_t)v; }
double bf_cdbl(int64_t v) { return (double)v; }

char* bf_cstr(int64_t v) {
    static char buf[64];
    snprintf(buf, 64, "%lld", v);
    return buf;
}

char* bf_cstr_double(double v) {
    static char buf[64];
    snprintf(buf, 64, "%g", v);
    return buf;
}

int bf_cbool(int64_t v) { return v != 0 ? 1 : 0; }

int64_t bf_len(const char* s) { return bf_str_len(s); }

char* bf_trim(const char* s) {
    static char buf[8192];
    if (!s) { buf[0] = 0; return buf; }
    const char* start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    const char* end = start;
    while (*end) end++;
    while (end > start && isspace((unsigned char)*(end - 1))) end--;
    int n = (int)(end - start);
    if (n >= 8192) n = 8191;
    for (int i = 0; i < n; i++) buf[i] = start[i];
    buf[n] = 0;
    return buf;
}

char* bf_left(const char* s, int64_t n) {
    static char buf[8192];
    if (!s || n <= 0) { buf[0] = 0; return buf; }
    int len = bf_str_len(s);
    if ((int64_t)len < n) n = (int64_t)len;
    if (n >= 8192) n = 8191;
    for (int64_t i = 0; i < n; i++) buf[i] = s[i];
    buf[n] = 0;
    return buf;
}

char* bf_right(const char* s, int64_t n) {
    static char buf[8192];
    if (!s || n <= 0) { buf[0] = 0; return buf; }
    int len = bf_str_len(s);
    if ((int64_t)len < n) n = (int64_t)len;
    if (n >= 8192) n = 8191;
    int start = len - (int)n;
    for (int64_t i = 0; i < n; i++) buf[i] = s[start + i];
    buf[n] = 0;
    return buf;
}

char* bf_mid(const char* s, int64_t start, int64_t count) {
    static char buf[8192];
    if (!s || start < 1 || count <= 0) { buf[0] = 0; return buf; }
    int len = bf_str_len(s);
    if ((size_t)(start - 1) >= (size_t)len) { buf[0] = 0; return buf; }
    const char* p = s + (start - 1);
    int avail = len - (start - 1);
    if ((int64_t)avail < count) count = (int64_t)avail;
    if (count >= 8192) count = 8191;
    for (int64_t i = 0; i < count; i++) buf[i] = p[i];
    buf[count] = 0;
    return buf;
}

int64_t bf_instr(const char* s, const char* sub) {
    if (!s || !sub) return 0;
    int sl = bf_str_len(s);
    int tl = bf_str_len(sub);
    if (tl == 0) return 1;
    if (tl > sl) return 0;
    for (int i = 0; i <= sl - tl; i++) {
        int j;
        for (j = 0; j < tl; j++) {
            if (s[i + j] != sub[j]) break;
        }
        if (j == tl) return i + 1;
    }
    return 0;
}

char* bf_replace(const char* s, const char* old, const char* new_) {
    static char buf[8192];
    if (!s || !old) { buf[0] = 0; return buf; }
    int buf_pos = 0;
    const char* p = s;
    int old_len = bf_str_len(old);
    if (old_len == 0) {
        bf_str_copy(buf, s);
        return buf;
    }
    int new_len = bf_str_len(new_);
    while (*p && buf_pos < 8191) {
        int match = 1;
        for (int i = 0; i < old_len; i++) {
            if (p[i] != old[i]) { match = 0; break; }
        }
        if (match) {
            if (buf_pos + new_len >= 8191) break;
            for (int i = 0; i < new_len; i++) buf[buf_pos++] = new_[i];
            p += old_len;
        } else {
            buf[buf_pos++] = *p++;
        }
    }
    buf[buf_pos] = 0;
    return buf;
}


double bf_sqr(double v) { return sqrt(v); }
double bf_abs(double v) { return fabs(v); }
int64_t bf_sgn(double v) { if (v > 0) return 1; else if (v < 0) return -1; else return 0; }
double bf_fix(double v) { return v >= 0 ? floor(v) : ceil(v); }
double bf_int(double v) { return floor(v); }
double bf_rnd(void) { return (double)rand() / (double)RAND_MAX; }


int64_t bf_year(double d) { time_t t = (time_t)d; struct tm* tm = localtime(&t); return tm ? tm->tm_year + 1900 : 0; }
int64_t bf_month(double d) { time_t t = (time_t)d; struct tm* tm = localtime(&t); return tm ? tm->tm_mon + 1 : 0; }
int64_t bf_day(double d) { time_t t = (time_t)d; struct tm* tm = localtime(&t); return tm ? tm->tm_mday : 0; }
double bf_now(void) { return (double)time(NULL); }


char* bf_format(const char* fmt, ...) {
    static char buf[8192];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, 8191, fmt, args);
    va_end(args);
    return buf;
}

char* bf_join(const char* arr[], int count, const char* delim) {
    static char buf[8192];
    buf[0] = 0;
    int delim_len = bf_str_len(delim);
    for (int i = 0; i < count; i++) {
        if (i > 0) bf_str_copy(buf + bf_str_len(buf), delim);
        bf_str_copy(buf + bf_str_len(buf), arr[i]);
    }
    return buf;
}

char** bf_split(const char* s, const char* delim, int* out_count) {
    static char* result[1024];
    int count = 0;
    if (!s || !delim) { *out_count = 0; return result; }
    int delim_len = bf_str_len(delim);
    const char* p = s;
    while (*p) {
        int match = 1;
        for (int i = 0; i < delim_len; i++) {
            if (p[i] != delim[i]) { match = 0; break; }
        }
        if (match) {
            result[count++] = (char*)p;
            p += delim_len;
        } else {
            p++;
        }
    }
    *out_count = count;
    return result;
}


#include <io.h>

static FILE* bfc_file_table[256];
static int bfc_next_file_handle = 1;

int bf_fopen(const char* filename, const char* mode) {
    FILE* f = fopen(filename, mode);
    if (!f) return -1;
    int handle = bfc_next_file_handle++;
    if (handle >= 256) { fclose(f); return -1; }
    bfc_file_table[handle] = f;
    return handle;
}

void bf_fclose(int handle) {
    if (handle >= 1 && handle < 256 && bfc_file_table[handle]) {
        fclose(bfc_file_table[handle]);
        bfc_file_table[handle] = NULL;
    }
}

void bf_fprint(int handle, const char* s) {
    if (handle >= 1 && handle < 256 && bfc_file_table[handle]) {
        fprintf(bfc_file_table[handle], "%s", s);
        fflush(bfc_file_table[handle]);
    }
}

void bf_fprint_int(int handle, int64_t v) {
    if (handle >= 1 && handle < 256 && bfc_file_table[handle]) {
        fprintf(bfc_file_table[handle], "%lld", v);
        fflush(bfc_file_table[handle]);
    }
}

void bf_fprint_double(int handle, double v) {
    if (handle >= 1 && handle < 256 && bfc_file_table[handle]) {
        fprintf(bfc_file_table[handle], "%g", v);
        fflush(bfc_file_table[handle]);
    }
}

char* bf_fgetline(int handle) {
    static char buf[8192];
    if (handle >= 1 && handle < 256 && bfc_file_table[handle]) {
        if (fgets(buf, 8191, bfc_file_table[handle])) {
            // Remove trailing newline
            int len = bf_str_len(buf);
            if (len > 0 && buf[len-1] == '\n') buf[len-1] = 0;
            return buf;
        }
    }
    return NULL;
}

int bf_feof(int handle) {
    if (handle >= 1 && handle < 256 && bfc_file_table[handle]) {
        return feof(bfc_file_table[handle]) ? 1 : 0;
    }
    return 1;
}

void bf_throw(const char* msg) {
    bfc_exc_value = (char*)msg;
    bfc_exc_code = 1;
    longjmp(bfc_exc_buf, 1);
}

int bf_try_begin(void) {
    bfc_exc_value = NULL;
    bfc_exc_code = 0;
    return setjmp(bfc_exc_buf);
}


// ---- Form Designer UI runtime (Win32) ----
static HWND bf_ui_hwnd[128];
static int bf_ui_click[128];
static int bf_ui_change[128];
static int bf_ui_double[128];
static int bf_ui_sel[128];
static int bf_ui_scroll[128];
static int bf_ui_focus[128];
static int bf_ui_blur[128];
static int bf_ui_keydown[128];
static int bf_ui_keyup[128];
static int bf_ui_mousemove[128];
static int bf_ui_mousewheel[128];
static int bf_ui_paint[128];
static int bf_ui_drop[128];
static WNDPROC bf_ui_orig[128];
static char bf_ui_names[128][64];
static char bf_drop_file[1024];
static HWND bf_ui_form = NULL;
static int bf_ui_count = 0;
void bf_dispatch_event(int id);

static int bf_ui_find_hwnd(HWND h) {
    for (int i = 0; i < bf_ui_count; i++) if (bf_ui_hwnd[i] == h) return i;
    return -1;
}

static LRESULT bf_ui_ctrl_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    int idx = bf_ui_find_hwnd(h);
    if (idx >= 0) {
        if (msg == WM_SETFOCUS && bf_ui_focus[idx] >= 0) {
            bf_dispatch_event(bf_ui_focus[idx]);
        } else if (msg == WM_KILLFOCUS && bf_ui_blur[idx] >= 0) {
            bf_dispatch_event(bf_ui_blur[idx]);
        } else if (msg == WM_KEYDOWN && bf_ui_keydown[idx] >= 0) {
            bf_dispatch_event(bf_ui_keydown[idx]);
        } else if (msg == WM_KEYUP && bf_ui_keyup[idx] >= 0) {
            bf_dispatch_event(bf_ui_keyup[idx]);
        } else if (msg == WM_MOUSEMOVE && bf_ui_mousemove[idx] >= 0) {
            bf_dispatch_event(bf_ui_mousemove[idx]);
        } else if (msg == WM_MOUSEWHEEL && bf_ui_mousewheel[idx] >= 0) {
            bf_dispatch_event(bf_ui_mousewheel[idx]);
        } else if (msg == WM_PAINT && bf_ui_paint[idx] >= 0) {
            bf_dispatch_event(bf_ui_paint[idx]);
        } else if (msg == WM_DROPFILES && bf_ui_drop[idx] >= 0) {
            DragQueryFileA((HDROP)wp, 0, bf_drop_file, sizeof(bf_drop_file));
            bf_dispatch_event(bf_ui_drop[idx]);
            DragFinish((HDROP)wp);
        }
    }
    if (idx >= 0 && bf_ui_orig[idx]) return CallWindowProcA(bf_ui_orig[idx], h, msg, wp, lp);
    return DefWindowProcA(h, msg, wp, lp);
}

static LRESULT bf_ui_wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_COMMAND) {
        int ctrl = (int)(wp & 0xFFFFu);
        int code = (int)((wp >> 16) & 0xFFFFu);
        for (int i = 0; i < bf_ui_count; i++) {
            if ((int)(intptr_t)bf_ui_hwnd[i] == ctrl) {
                if (code == BN_CLICKED && bf_ui_click[i] >= 0) bf_dispatch_event(bf_ui_click[i]);
                else if (code == BN_DOUBLECLICKED && bf_ui_double[i] >= 0) bf_dispatch_event(bf_ui_double[i]);
                else if (code == EN_CHANGE && bf_ui_change[i] >= 0) bf_dispatch_event(bf_ui_change[i]);
                else if ((code == CBN_SELCHANGE || code == LBN_SELCHANGE) && bf_ui_sel[i] >= 0) bf_dispatch_event(bf_ui_sel[i]);
            }
        }
    }
    if (msg == WM_HSCROLL) {
        for (int i = 0; i < bf_ui_count; i++) {
            if ((int)(intptr_t)bf_ui_hwnd[i] == (int)(intptr_t)lp && bf_ui_scroll[i] >= 0) {
                bf_dispatch_event(bf_ui_scroll[i]);
            }
        }
    }
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(h, msg, wp, lp);
}

static int bf_ui_register(void) {
    static int done = 0;
    if (done) return 1;
    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = bf_ui_wndproc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "BasicForgeForm";
    if (!RegisterClass(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 0;
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES | ICC_PROGRESS_CLASS | ICC_LISTVIEW_CLASSES |
                ICC_TREEVIEW_CLASSES | ICC_DATE_CLASSES | ICC_TAB_CLASSES |
                ICC_UPDOWN_CLASS | ICC_COOL_CLASSES | ICC_HOTKEY_CLASS |
                ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);
    done = 1;
    return 1;
}

static int bf_ui_add(const char* cls, DWORD style, const char* text,
                     int x, int y, int w, int h, int64_t form) {
    if (bf_ui_count >= 128) return -1;
    HWND hwnd = CreateWindowExA(0, cls, text ? text : "", WS_CHILD | WS_VISIBLE | style,
                             x, y, w, h, (HWND)form, NULL,
                             GetModuleHandle(NULL), NULL);
    bf_ui_hwnd[bf_ui_count] = hwnd;
    bf_ui_click[bf_ui_count] = -1;
    bf_ui_change[bf_ui_count] = -1;
    bf_ui_double[bf_ui_count] = -1;
    bf_ui_sel[bf_ui_count] = -1;
    bf_ui_scroll[bf_ui_count] = -1;
    bf_ui_focus[bf_ui_count] = -1;
    bf_ui_blur[bf_ui_count] = -1;
    bf_ui_keydown[bf_ui_count] = -1;
    bf_ui_keyup[bf_ui_count] = -1;
    bf_ui_mousemove[bf_ui_count] = -1;
    bf_ui_mousewheel[bf_ui_count] = -1;
    bf_ui_paint[bf_ui_count] = -1;
    bf_ui_drop[bf_ui_count] = -1;
    bf_ui_orig[bf_ui_count] = (WNDPROC)(uintptr_t)SetWindowLongPtrA(
        hwnd, GWLP_WNDPROC, (LONG_PTR)(uintptr_t)bf_ui_ctrl_proc);
    bf_ui_count++;
    return bf_ui_count - 1;
}

int64_t bf_form_new(const char* title, int w, int h) {
    if (!bf_ui_register()) return 0;
    HWND hwnd = CreateWindowExA(0, "BasicForgeForm", title ? title : "", WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, w, h, NULL, NULL,
                        GetModuleHandle(NULL), NULL);
    bf_ui_form = hwnd;
    DragAcceptFiles(hwnd, TRUE);
    return (int64_t)hwnd;
}
int bf_button_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("BUTTON", BS_PUSHBUTTON, t, x, y, w, h, f);
}
int bf_label_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("STATIC", 0, t, x, y, w, h, f);
}
int bf_textbox_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("EDIT", WS_BORDER | ES_AUTOHSCROLL, t, x, y, w, h, f);
}
int bf_checkbox_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("BUTTON", BS_AUTOCHECKBOX, t, x, y, w, h, f);
}
int bf_radiobutton_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("BUTTON", BS_AUTORADIOBUTTON, t, x, y, w, h, f);
}
int bf_groupbox_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("BUTTON", BS_GROUPBOX, t, x, y, w, h, f);
}
int bf_panel_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("STATIC", 0, t, x, y, w, h, f);
}
int bf_picturebox_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("STATIC", 0, t, x, y, w, h, f);
}
int bf_listbox_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("LISTBOX", 0, t, x, y, w, h, f);
}
int bf_combobox_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("COMBOBOX", CBS_DROPDOWN, t, x, y, w, h, f);
}
int bf_trackbar_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("msctls_trackbar32", TBS_HORZ, t, x, y, w, h, f);
}
int bf_progressbar_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("msctls_progress32", 0, t, x, y, w, h, f);
}
int bf_datetimepicker_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("SysDateTimePick32", DTS_SHORTDATEFORMAT, t, x, y, w, h, f);
}
int bf_listview_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("SysListView32", LVS_REPORT, t, x, y, w, h, f);
}
int bf_treeview_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("SysTreeView32", 0, t, x, y, w, h, f);
}
int bf_richtextbox_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("EDIT", WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
                     t, x, y, w, h, f);
}
int bf_tabcontrol_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("SysTabControl32", TCS_TABS, t, x, y, w, h, f);
}
int bf_toolbar_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("ToolbarWindow32", 0, t, x, y, w, h, f);
}
int bf_statusbar_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("msctls_statusbar32", 0, t, x, y, w, h, f);
}
int bf_updown_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("msctls_updown32", 0, t, x, y, w, h, f);
}
int bf_monthcalendar_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("SysMonthCal32", 0, t, x, y, w, h, f);
}
int bf_scrollbar_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("SCROLLBAR", SBS_HORZ, t, x, y, w, h, f);
}
int bf_hotkey_new(int64_t f, const char* t, int x, int y, int w, int h) {
    return bf_ui_add("msctls_hotkey32", 0, t, x, y, w, h, f);
}
void bf_set_click(int c, int id) { if (c >= 0 && c < bf_ui_count) bf_ui_click[c] = id; }
void bf_set_textchanged(int c, int id) { if (c >= 0 && c < bf_ui_count) bf_ui_change[c] = id; }
void bf_set_doubleclick(int c, int id) { if (c >= 0 && c < bf_ui_count) bf_ui_double[c] = id; }
void bf_set_selection(int c, int id) { if (c >= 0 && c < bf_ui_count) bf_ui_sel[c] = id; }
void bf_set_scroll(int c, int id) { if (c >= 0 && c < bf_ui_count) bf_ui_scroll[c] = id; }
void bf_set_focus(int c, int id) { if (c >= 0 && c < bf_ui_count) bf_ui_focus[c] = id; }
void bf_set_blur(int c, int id) { if (c >= 0 && c < bf_ui_count) bf_ui_blur[c] = id; }
void bf_set_keydown(int c, int id) { if (c >= 0 && c < bf_ui_count) bf_ui_keydown[c] = id; }
void bf_set_keyup(int c, int id) { if (c >= 0 && c < bf_ui_count) bf_ui_keyup[c] = id; }
void bf_set_mousemove(int c, int id) { if (c >= 0 && c < bf_ui_count) bf_ui_mousemove[c] = id; }
void bf_set_mousewheel(int c, int id) { if (c >= 0 && c < bf_ui_count) bf_ui_mousewheel[c] = id; }
void bf_set_paint(int c, int id) { if (c >= 0 && c < bf_ui_count) bf_ui_paint[c] = id; }
void bf_set_drop(int c, int id) {
    if (c >= 0 && c < bf_ui_count) bf_ui_drop[c] = id;
}
char* bf_get_dropped_file(void) { return bf_drop_file; }
void bf_form_run(int64_t f) {
    ShowWindow((HWND)f, SW_SHOW);
    UpdateWindow((HWND)f);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
char* bf_get_text(int c) {
    static char buf[1024];
    buf[0] = 0;
    if (c >= 0 && c < bf_ui_count) GetWindowText(bf_ui_hwnd[c], buf, (int)sizeof(buf));
    return buf;
}
void bf_set_text(int c, const char* t) {
    if (c >= 0 && c < bf_ui_count) SetWindowText(bf_ui_hwnd[c], t ? t : "");
}
int bf_form_test_click(int c) {
    if (c >= 0 && c < bf_ui_count && bf_ui_click[c] >= 0) bf_dispatch_event(bf_ui_click[c]);
    return 0;
}
int bf_form_close_first(void) {
    if (bf_ui_form) PostMessageA(bf_ui_form, WM_CLOSE, 0, 0);
    return 0;
}
void bf_ui_set_name(int c, const char* name) {
    if (c >= 0 && c < bf_ui_count && name) {
        strncpy(bf_ui_names[c], name, 63);
        bf_ui_names[c][63] = 0;
    }
}
static int bf_ui_find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < bf_ui_count; i++) {
        if (strcmp(bf_ui_names[i], name) == 0) return i;
    }
    return -1;
}
char* bf_get_text_name(const char* name) {
    return bf_get_text(bf_ui_find(name));
}
void bf_set_text_name(const char* name, const char* text) {
    int c = bf_ui_find(name);
    if (c >= 0) bf_set_text(c, text);
}
void bf_try_end(void) {
    bfc_exc_value = NULL;
    bfc_exc_code = 0;
}

void bf_catch_begin(void) {}
void bf_catch_end(void) {}
void bf_finally_begin(void) {}
void bf_finally_end(void) {}

int main(void) { Main(); return 0; }
