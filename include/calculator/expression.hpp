#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace calculator {

// Snapshots describe state AFTER the named action, in bottom-to-top order.
// u+ and u- mean unary plus/minus. position is a zero-based input byte offset;
// an automatically appended # uses input.size().
struct TraceStep {
    std::size_t position;
    std::string inputToken;
    std::string operatorStack;
    std::string operandStack;
    std::string action;
};

using TraceCallback = void (*)(const TraceStep& step, void* context);

// Grammar: nonnegative integer literals, + - * / ^, parentheses, unary +/-.
// Whitespace is ignored; a single trailing # is optional.
// ^ is right-associative and binds tighter than unary +/-.
// Errors are reported by calculator::Error; no global calculation state.
std::int64_t evaluateExpression(const std::string& input,
                                TraceCallback callback = nullptr,
                                void* context = nullptr);

} // namespace calculator
