#include "bf_date.h"
#include <time.h>

int64_t bf_year(double d) {
    time_t t = (time_t)d;
    struct tm* tm = localtime(&t);
    return tm ? tm->tm_year + 1900 : 0;
}

int64_t bf_month(double d) {
    time_t t = (time_t)d;
    struct tm* tm = localtime(&t);
    return tm ? tm->tm_mon + 1 : 0;
}

int64_t bf_day(double d) {
    time_t t = (time_t)d;
    struct tm* tm = localtime(&t);
    return tm ? tm->tm_mday : 0;
}

double bf_now(void) {
    return (double)time(NULL);
}