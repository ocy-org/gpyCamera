#ifndef ZXING_COMPAT_H
#define ZXING_COMPAT_H

#include <type_traits>

// 为旧编译器提供C++17特性兼容
#if __cplusplus < 201703L
namespace std {
    template<typename T>
    constexpr bool is_integral_v = is_integral<T>::value;

    template<typename T>
    constexpr bool is_unsigned_v = is_unsigned<T>::value;
}
#endif

#endif // ZXING_COMPAT_H
