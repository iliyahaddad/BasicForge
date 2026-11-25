#include "bf_math.h"
#include <math.h>
#include <stdlib.h>

double bf_sqr(double v) { return sqrt(v); }
double bf_abs(double v) { return fabs(v); }
int64_t bf_sgn(double v) { return v > 0 ? 1 : (v < 0 ? -1 : 0); }
double bf_fix(double v) { return v >= 0 ? floor(v) : ceil(v); }
double bf_int(double v) { return floor(v); }
double bf_rnd(void) { return (double)rand() / (double)RAND_MAX; }