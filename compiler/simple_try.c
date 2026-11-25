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

void Main();

#line 1 ".bf"
void Main() {
  int64_t x;
  x = (int64_t)(10);
  if (bf_try_begin()) goto L2;
L0:
;
  bf_print_string("In Try");
  int _r0 = (x > 5);
  if (!(_r0)) goto L7;
L6:
;
  bf_throw("Too big");
  goto L7;
L8:
;
L7:
;
  bf_try_end();
  goto L4;
  goto L4;
L2:
;
L4:
;
  bf_finally_begin();
  bf_finally_end();
L5:
;
  bf_print_string("Done");
  return;
}

