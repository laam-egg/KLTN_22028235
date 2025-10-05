#include "efe/common/meth.h"

bool isPowerOf2(size_t x) {
    return x != 0 && (x & (x - 1)) == 0;
}
