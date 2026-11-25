// BasicForge runtime C source
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <io.h>
#include <setjmp.h>
#include <stdarg.h>

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

static jmp_buf bfc_exc_buf;
static char* bfc_exc_value = NULL;
static int bfc_exc_code = 0;

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

void bf_try_end(void) {
    bfc_exc_value = NULL;
    bfc_exc_code = 0;
}

void bf_catch_begin(void) {
    /* exception value is in bfc_exc_value */
}

void bf_catch_end(void) {
    /* end of catch block */
}

void bf_finally_begin(void) {
}

void bf_finally_end(void) {
}


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

int main(void) { Main(); return 0; }
int _fltused = 0;
