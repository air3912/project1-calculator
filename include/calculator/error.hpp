#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

namespace calculator {

enum class ErrorCode {
    InvalidInput,
    InvalidExpression,
    MismatchedParentheses,
    DivisionByZero,
    Overflow,
    NegativeExponent,
    EmptyStack
};

// position is a zero-based byte offset in the input; noPosition means unrelated
// to a specific input character. All calculation failures use this exception.
class Error : public std::runtime_error {
public:
    static constexpr std::size_t noPosition = static_cast<std::size_t>(-1);

    Error(ErrorCode code, const std::string& message,
          std::size_t position = noPosition)
        : std::runtime_error(message), code_(code), position_(position) {}

    ErrorCode code() const noexcept { return code_; }
    std::size_t position() const noexcept { return position_; }

private:
    ErrorCode code_;
    std::size_t position_;
};

} // namespace calculator
