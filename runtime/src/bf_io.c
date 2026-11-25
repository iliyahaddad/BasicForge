#include "bf_io.h"
#include <stdio.h>
#include <string.h>

static FILE* bfc_file_table[256];
static int bfc_next_file_handle = 1;

int bf_fopen(const char* filename, const char* mode) {
    FILE* f = fopen(filename, mode);
    if (!f) return -1;
    int handle = bfc_next_file_handle++;
    if (handle >= 256) { fclose(f); return -1; }
    bfc_file_table[handle] = f;
    return handle;
}

void bf_fclose(int handle) {
    if (handle >= 1 && handle < 256 && bfc_file_table[handle]) {
        fclose(bfc_file_table[handle]);
        bfc_file_table[handle] = NULL;
    }
}

void bf_fprint(int handle, const char* s) {
    if (handle >= 1 && handle < 256 && bfc_file_table[handle]) {
        fprintf(bfc_file_table[handle], "%s", s);
        fflush(bfc_file_table[handle]);
    }
}

void bf_fprint_int(int handle, int64_t v) {
    if (handle >= 1 && handle < 256 && bfc_file_table[handle]) {
        fprintf(bfc_file_table[handle], "%lld", v);
        fflush(bfc_file_table[handle]);
    }
}

void bf_fprint_double(int handle, double v) {
    if (handle >= 1 && handle < 256 && bfc_file_table[handle]) {
        fprintf(bfc_file_table[handle], "%g", v);
        fflush(bfc_file_table[handle]);
    }
}

char* bf_fgetline(int handle) {
    static char buf[8192];
    if (handle >= 1 && handle < 256 && bfc_file_table[handle]) {
        if (fgets(buf, 8191, bfc_file_table[handle])) {
            size_t len = strlen(buf);
            if (len > 0 && buf[len-1] == '\n') buf[len-1] = 0;
            return buf;
        }
    }
    return NULL;
}

int bf_feof(int handle) {
    if (handle >= 1 && handle < 256 && bfc_file_table[handle]) {
        return feof(bfc_file_table[handle]) ? 1 : 0;
    }
    return 1;
}