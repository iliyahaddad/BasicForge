#include "bf_string.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

void bf_print_string(const char* s) {
    if (s) { printf("%s\n", s); fflush(stdout); }
}

void bf_print_int(int64_t v) {
    printf("%lld\n", v);
    fflush(stdout);
}

void bf_print_double(double v) {
    printf("%g\n", v);
    fflush(stdout);
}

void bf_print_bool(int v) {
    printf(v ? "True\n" : "False\n");
    fflush(stdout);
}

char* bf_str_concat(const char* a, const char* b) {
    static char buf[8192];
    if (!a) a = "";
    if (!b) b = "";
    snprintf(buf, 8192, "%s%s", a, b);
    return buf;
}

int64_t bf_len(const char* s) {
    return s ? (int64_t)strlen(s) : 0;
}

char* bf_trim(const char* s) {
    static char buf[8192];
    if (!s) { buf[0] = 0; return buf; }
    const char* start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    const char* end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1))) end--;
    size_t n = end - start;
    if (n >= 8192) n = 8191;
    memcpy(buf, start, n);
    buf[n] = 0;
    return buf;
}

char* bf_left(const char* s, int64_t n) {
    static char buf[8192];
    if (!s || n <= 0) { buf[0] = 0; return buf; }
    size_t len = strlen(s);
    if ((size_t)n > len) n = (int64_t)len;
    if (n >= 8192) n = 8191;
    memcpy(buf, s, n);
    buf[n] = 0;
    return buf;
}

char* bf_right(const char* s, int64_t n) {
    static char buf[8192];
    if (!s || n <= 0) { buf[0] = 0; return buf; }
    size_t len = strlen(s);
    if ((size_t)n > len) n = (int64_t)len;
    if (n >= 8192) n = 8191;
    memcpy(buf, s + len - n, n);
    buf[n] = 0;
    return buf;
}

char* bf_mid(const char* s, int64_t start, int64_t count) {
    static char buf[8192];
    if (!s || start < 1 || count <= 0) { buf[0] = 0; return buf; }
    size_t len = strlen(s);
    if ((size_t)(start - 1) >= len) { buf[0] = 0; return buf; }
    const char* p = s + (start - 1);
    size_t avail = len - (start - 1);
    if ((size_t)count > avail) count = (int64_t)avail;
    if (count >= 8192) count = 8191;
    memcpy(buf, p, count);
    buf[count] = 0;
    return buf;
}

int64_t bf_instr(const char* s, const char* sub) {
    if (!s || !sub) return 0;
    const char* p = strstr(s, sub);
    return p ? (int64_t)(p - s) + 1 : 0;
}

char* bf_replace(const char* s, const char* old, const char* new_) {
    static char buf[8192];
    if (!s || !old) { buf[0] = 0; return buf; }
    if (!new_) new_ = "";
    size_t old_len = strlen(old);
    if (old_len == 0) { snprintf(buf, 8192, "%s", s); return buf; }
    const char* p = s;
    char* out = buf;
    int remaining = 8191;
    while (*p && remaining > 0) {
        if (strncmp(p, old, old_len) == 0) {
            size_t new_len = strlen(new_);
            if (remaining <= (int)new_len) break;
            memcpy(out, new_, new_len);
            out += new_len;
            remaining -= new_len;
            p += old_len;
        } else {
            *out++ = *p++;
            remaining--;
        }
    }
    *out = 0;
    return buf;
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
    if (!delim) delim = "";
    for (int i = 0; i < count; i++) {
        if (i > 0) strncat(buf, delim, 8191 - strlen(buf));
        strncat(buf, arr[i], 8191 - strlen(buf));
    }
    return buf;
}

char** bf_split(const char* s, const char* delim, int* out_count) {
    static char* result[256];
    int count = 0;
    if (!s || !delim) { *out_count = 0; return result; }
    char buf[8192];
    snprintf(buf, 8192, "%s", s);
    char* token = strtok(buf, delim);
    while (token && count < 256) {
        result[count++] = token;
        token = strtok(NULL, delim);
    }
    *out_count = count;
    return result;
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