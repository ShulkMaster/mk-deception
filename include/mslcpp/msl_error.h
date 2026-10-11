#ifndef MSLCPP_MSL_ERROR_H
#define MSLCPP_MSL_ERROR_H

extern "C" {
#include "runtime/cfile.h"
}

extern "C" int fprintf(FILE* file, const char* format, ...);
extern "C" void abort(void);

#define __msl_error(str) \
    {                    \
        fprintf(&__files[2], str); \
        abort();         \
    }

#endif
