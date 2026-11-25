#ifndef BF_IO_H
#define BF_IO_H

#include <stdint.h>

// Console I/O
void bf_print_string(const char* s);
void bf_print_int(int64_t v);
void bf_print_double(double v);
void bf_print_bool(int v);

// File I/O
int bf_fopen(const char* filename, const char* mode);
void bf_fclose(int handle);
void bf_fprint(int handle, const char* s);
void bf_fprint_int(int handle, int64_t v);
void bf_fprint_double(int handle, double v);
char* bf_fgetline(int handle);
int bf_feof(int handle);

#endif // BF_IO_H