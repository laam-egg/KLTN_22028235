#ifndef LOGGING_INCLUDED
#define LOGGING_INCLUDED

#include "efe/common/nop.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define __FILENAME__ (&__FILE__[SOURCE_PATH_SIZE])

#define LOG_PREFIX(type) fprintf(stderr, "--- At file %s | line %d | " type ":\n", __FILENAME__, __LINE__)

#define LOG_ERROR(...) { LOG_PREFIX("ERROR"); fprintf(stderr, __VA_ARGS__); } nop()

#define LOG_FATAL_ERROR(...) { LOG_ERROR(__VA_ARGS__); exit(1); } nop()

#endif // LOGGING_INCLUDED
