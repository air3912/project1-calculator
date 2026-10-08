#pragma once

#include "calculator/error.hpp"
#include <cstdint>
#include <limits>

namespace calculator::detail {

using Integer = std::int64_t;
using Unsigned = std::uint64_t;
constexpr Integer integerMin = std::numeric_limits<Integer>::min();
constexpr Integer integerMax = std::numeric_limits<Integer>::max();

// Unlike abs(), this also handles INT64_MIN without signed overflow.
inline Unsigned magnitude(Integer value) noexcept {
    return value < 0 ? static_cast<Unsigned>(-(value + 1)) + 1
                     : static_cast<Unsigned>(value);
}

[[noreturn]] inline void overflow(std::size_t position = Error::noPosition) {
    throw Error(ErrorCode::Overflow, "整数计算超出 64 位有符号整数范围", position);
}

inline Integer add(Integer a, Integer b, std::size_t pos = Error::noPosition) {
    if ((b > 0 && a > integerMax - b) || (b < 0 && a < integerMin - b))
        overflow(pos);
    return a + b;
}

inline Integer subtract(Integer a, Integer b, std::size_t pos = Error::noPosition) {
    if ((b > 0 && a < integerMin + b) || (b < 0 && a > integerMax + b))
        overflow(pos);
    return a - b;
}

inline Integer negate(Integer a, std::size_t pos = Error::noPosition) {
    if (a == integerMin) overflow(pos);
    return -a;
}

inline Integer multiply(Integer a, Integer b, std::size_t pos = Error::noPosition) {
    const bool negative = (a < 0) != (b < 0);
    const Unsigned limit = negative ? magnitude(integerMin)
                                    : static_cast<Unsigned>(integerMax);
    const Unsigned ua = magnitude(a), ub = magnitude(b);
    if (ub != 0 && ua > limit / ub) overflow(pos);
    const Unsigned product = ua * ub;
    if (!negative) return static_cast<Integer>(product);
    if (product == magnitude(integerMin)) return integerMin;
    return -static_cast<Integer>(product);
}

inline Integer divide(Integer a, Integer b, std::size_t pos = Error::noPosition) {
    if (b == 0) throw Error(ErrorCode::DivisionByZero, "除数不能为 0", pos);
    if (a == integerMin && b == -1) overflow(pos);
    return a / b; // C++ integer division truncates toward zero.
}

inline Integer power(Integer base, Integer exponent,
                     std::size_t pos = Error::noPosition) {
    if (exponent < 0)
        throw Error(ErrorCode::NegativeExponent, "整数乘方不支持负指数", pos);
    Integer result = 1; // Define 0^0 as 1.
    while (exponent > 0) {
        if (exponent % 2 != 0) result = multiply(result, base, pos);
        exponent /= 2;
        if (exponent != 0) base = multiply(base, base, pos);
    }
    return result;
}

} // namespace calculator::detail
