#include "bf_exception.h"
#include <setjmp.h>

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

void bf_catch_begin(void) {}
void bf_catch_end(void) {}
void bf_finally_begin(void) {}
void bf_finally_end(void) {}