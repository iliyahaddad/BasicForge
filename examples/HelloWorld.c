// BasicForge-generated C code
// CODEGEN_V2_MARKER_FINAL
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <io.h>
#include <setjmp.h>
#include <stdarg.h>

void bf_print_string(const char* s);
void bf_print_int(int64_t v);
void bf_print_double(double v);
void bf_print_bool(int v);
char* bf_str_concat(char*, char*);
int64_t bf_cint(double);
int64_t bf_clng(double);
double bf_cdbl(int64_t);
char* bf_cstr(int64_t);
int bf_cbool(int64_t);
int64_t bf_len(const char*);
char* bf_trim(char*);
char* bf_left(char*, int64_t);
char* bf_replace(char*, char*, char*);
static char* bf_str_copy(char*, const char*);
static int bf_str_len(const char*);

static char* bfc_exc_value = NULL;
static jmp_buf bfc_exc_buf;
static int bfc_exc_code = 0;

void bf_throw(const char* msg);
int bf_try_begin(void);
void bf_try_end(void);
void bf_catch_begin(void);
void bf_catch_end(void);
void bf_finally_begin(void);
void bf_finally_end(void);

char** bf_split(const char* s, const char* delim, int* out_count);

int bf_fopen(const char* filename, const char* mode);
void bf_fclose(int handle);
void bf_fprint(int handle, const char* s);
void bf_fprint_int(int handle, int64_t v);
void bf_fprint_double(int handle, double v);
char* bf_fgetline(int handle);
int bf_feof(int handle);


int bf_fopen(const char* filename, const char* mode) {
    FILE* f = fopen(filename, mode);
    if (!f) return -1;
    static int handle = 1;
    if (handle >= 256) { fclose(f); return -1; }
    static FILE* ftable[256];
    ftable[handle] = f;
    return handle++;
}

void bf_fclose(int handle) {
    static FILE* ftable[256];
    if (handle >= 1 && handle < 256 && ftable[handle]) {
        fclose(ftable[handle]);
        ftable[handle] = NULL;
    }
}

void bf_fprint(int handle, const char* s) {
    static FILE* ftable[256];
    if (handle >= 1 && handle < 256 && ftable[handle]) {
        fprintf(ftable[handle], "%s", s);
        fflush(ftable[handle]);
    }
}

void bf_fprint_int(int handle, int64_t v) {
    static FILE* ftable[256];
    if (handle >= 1 && handle < 256 && ftable[handle]) {
        fprintf(ftable[handle], "%lld", v);
        fflush(ftable[handle]);
    }
}

void bf_fprint_double(int handle, double v) {
    static FILE* ftable[256];
    if (handle >= 1 && handle < 256 && ftable[handle]) {
        fprintf(ftable[handle], "%g", v);
        fflush(ftable[handle]);
    }
}

char* bf_fgetline(int handle) {
    static char buf[8192];
    static FILE* ftable[256];
    if (handle >= 1 && handle < 256 && ftable[handle]) {
        if (fgets(buf, 8191, ftable[handle])) {
            int len = bf_str_len(buf);
            if (len > 0 && buf[len-1] == '\n') buf[len-1] = 0;
            return buf;
        }
    }
    return NULL;
}

int bf_feof(int handle) {
    static FILE* ftable[256];
    if (handle >= 1 && handle < 256 && ftable[handle]) {
        return feof(ftable[handle]) ? 1 : 0;
    }
    return 1;
}

void Main();

void Main() {
  bf_print_string("Hello World");
  return;
}

