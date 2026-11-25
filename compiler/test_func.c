// BasicForge-generated C code
// CODEGEN_V2_MARKER_FINAL
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <io.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>

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
char* bf_right(char*, int64_t);
char* bf_mid(char*, int64_t, int64_t);
int64_t bf_instr(const char*, const char*);
char* bf_replace(char*, char*, char*);
char* bf_format(const char*, ...);
double bf_sqr(double);
double bf_abs(double);
int64_t bf_sgn(double);
double bf_fix(double);
double bf_int(double);
double bf_rnd(void);
int64_t bf_year(double);
int64_t bf_month(double);
int64_t bf_day(double);
double bf_now(void);
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

int64_t Add(int64_t a, int64_t b);
int64_t Mul(int64_t a, int64_t b);
void Main();

#line 1 ".bf"
int64_t Add(int64_t a, int64_t b) {
  int64_t _r0 = (int64_t)(a + b);
  return (int64_t)(_r0);
}

#line 4 ".bf"
int64_t Mul(int64_t a, int64_t b) {
  int64_t _r1 = (int64_t)(a * b);
  return (int64_t)(_r1);
}

#line 7 ".bf"
void Main() {
  int64_t r;
  int64_t _r2 = Add(10, 20);
  r = (int64_t)(_r2);
  bf_print_int(r);
  int64_t _r3 = Mul(3, 4);
  bf_print_int(_r3);
  return;
}

