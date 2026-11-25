#ifndef BF_STRING_H
#define BF_STRING_H

#include <stdint.h>

// String operations
void bf_print_string(const char* s);
void bf_print_int(int64_t v);
void bf_print_double(double v);
void bf_print_bool(int v);
char* bf_str_concat(const char* a, const char* b);
int64_t bf_len(const char* s);
char* bf_trim(const char* s);
char* bf_left(const char* s, int64_t n);
char* bf_right(const char* s, int64_t n);
char* bf_mid(const char* s, int64_t start, int64_t count);
int64_t bf_instr(const char* s, const char* sub);
char* bf_replace(const char* s, const char* old, const char* new_);
char* bf_format(const char* fmt, ...);
char* bf_join(const char* arr[], int count, const char* delim);
char** bf_split(const char* s, const char* delim, int* out_count);

// Type conversions
int64_t bf_cint(double v);
int64_t bf_clng(double v);
double bf_cdbl(int64_t v);
char* bf_cstr(int64_t v);
char* bf_cstr_double(double v);
int bf_cbool(int64_t v);

#endif // BF_STRING_H