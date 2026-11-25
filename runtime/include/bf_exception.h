#ifndef BF_EXCEPTION_H
#define BF_EXCEPTION_H

#include <stdint.h>

// Exception handling
void bf_throw(const char* msg);
int bf_try_begin(void);
void bf_try_end(void);
void bf_catch_begin(void);
void bf_catch_end(void);
void bf_finally_begin(void);
void bf_finally_end(void);

#endif // BF_EXCEPTION_H