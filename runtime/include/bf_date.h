#ifndef BF_DATE_H
#define BF_DATE_H

#include <stdint.h>

// Date/time functions
int64_t bf_year(double d);
int64_t bf_month(double d);
int64_t bf_day(double d);
double bf_now(void);

#endif // BF_DATE_H