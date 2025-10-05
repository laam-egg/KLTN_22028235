/**
 * Well, it is, math, actually.
 */

#ifndef EFE_METH_INCLUDED
#define EFE_METH_INCLUDED

#include <cstddef>
#include <type_traits>


bool isPowerOf2(size_t x);


#define TEMPLATE_T_ARITHMETIC \
    template< \
        typename T, \
        typename std::enable_if_t<std::is_arithmetic_v<T>, bool> = true \
    > \

#define TEMPLATE_T_U_BOTH_ARITHMETIC \
    template< \
        typename T, \
        typename U, \
        typename std::enable_if_t< \
            std::conjunction_v< \
                std::is_arithmetic<T>, \
                std::is_arithmetic<U> \
            >, \
            bool \
        > = true \
    > \



TEMPLATE_T_U_BOTH_ARITHMETIC
U arraySum(U startingValue, T const* const arr, size_t arrSize) {
    U s = startingValue;
    for (size_t i = 0; i < arrSize; ++i) {
        s += static_cast<U>(arr[i]);
    }
    return s;
}

TEMPLATE_T_ARITHMETIC
void arrayMul(T multiplier, T* const arr, size_t arrSize) {
    for (size_t i = 0; i < arrSize; ++i) {
        arr[i] *= multiplier;
    }
}

TEMPLATE_T_U_BOTH_ARITHMETIC
void arrayCopy(U* const dest, T const* const src, size_t sz) {
    for (size_t i = 0; i < sz; ++i) {
        dest[i] = static_cast<U>(src[i]);
    }
}

#endif // EFE_METH_INCLUDED
