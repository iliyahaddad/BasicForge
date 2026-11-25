#ifndef BF_MATH_H
#define BF_MATH_H

#include <stdint.h>

// Math functions
double bf_sqr(double v);
double bf_abs(double v);
int64_t bf_sgn(double v);
double bf_fix(double v);
double bf_int(double v);
double bf_rnd(void);

#endif // BF_MATH_H