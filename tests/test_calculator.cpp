#include "calculator/checked_integer.hpp"
#include "calculator/error.hpp"
#include "calculator/expression.hpp"
#include "calculator/polynomial.hpp"
#include "calculator/stack.hpp"
#include <climits>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <utility>

namespace {

using calculator::Error;
using calculator::ErrorCode;
using calculator::Polynomial;
using calculator::evaluateExpression;
using Integer = std::int64_t;

int checks = 0;
int failures = 0;

void check(bool condition, const std::string& label) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << label << '\n';
    }
}

template <typename F>
void expectError(F operation, ErrorCode code, const std::string& label,
                 std::size_t position = Error::noPosition) {
    try {
        operation();
        check(false, label + " (no error)");
    } catch (const Error& error) {
        check(error.code() == code, label + " (error code)");
        if (position != Error::noPosition)
            check(error.position() == position, label + " (error position)");
    }
}

void testStack() {
    calculator::Stack<Integer> stack;
    check(stack.empty(), "new stack empty");
    expectError([&] { stack.pop(); }, ErrorCode::EmptyStack, "empty pop");
    expectError([&] { stack.top(); }, ErrorCode::EmptyStack, "empty top");
    for (int i = 0; i < 1000; ++i) stack.push(i);
    check(stack.size() == 1000 && stack.at(0) == 0 && stack.top() == 999, "stack growth");
    for (int i = 999; i >= 0; --i) check(stack.pop() == i, "LIFO");
    expectError([&] { stack.at(0); }, ErrorCode::InvalidInput, "stack bounds");
    for (int i = 0; i < 8; ++i) stack.push(i);
    stack.push(stack.top()); // Growth with a reference into the old allocation.
    check(stack.top() == 7, "alias-safe stack push");
    stack.clear();
    check(stack.empty(), "stack clear");
}

struct ExpressionCase { const char* input; Integer expected; };

void testExpressionExamples() {
    const ExpressionCase cases[] = {
        {"10-7", 3}, {"4*6", 24}, {"10-2*3", 4}, {"2*3+4*5", 26},
        {"(2+3)*4", 20}, {"((2+3)*4+1)/3", 7}, {"20/4/5", 1},
        {"20/(4/2)", 10}, {"10-3-2", 5}, {"1+2*3-8/2", 3},
        {" 0010 + 2 # \t", 12}, {"0", 0}, {"(0)", 0},
        {"-3+2", -1}, {"3*(-2)", -6}, {"+3", 3}, {"--3", 3},
        {"1--2", 3}, {"1+-2", -1}, {"-(2+3)*4", -20},
        {"-7/3", -2}, {"7/-3", -2}, {"-7/-3", 2}, {"2*-3+4", -2},
        {"3^2", 9}, {"2^3^2", 512}, {"(2^3)^2", 64},
        {"-2^2", -4}, {"(-2)^2", 4}, {"-2^3", -8}, {"--2^2", 4},
        {"2^-0", 1}, {"2^(-1+3)", 4}, {"2^--3", 8},
        {"-2^-0", -1},
        {"2^3*4", 32}, {"2*3^2", 18}, {"0^0", 1},
        {"1^9223372036854775807", 1},
        {"9223372036854775807", std::numeric_limits<Integer>::max()},
        {"-9223372036854775807-1", std::numeric_limits<Integer>::min()},
        {"(-9223372036854775807-1)*1", std::numeric_limits<Integer>::min()},
        {"(-9223372036854775807-1)/2", -4611686018427387904LL},
        {"(-9223372036854775807-1)*0", 0},
        {"(-2)^63", std::numeric_limits<Integer>::min()}
    };
    for (const auto& item : cases) {
        check(evaluateExpression(item.input) == item.expected, std::string("expression ") + item.input);
    }
    std::string deep(2000, '(');
    deep += "123";
    deep += std::string(2000, ')');
    check(evaluateExpression(deep) == 123, "deep parentheses without recursive parser");
    check(evaluateExpression(std::string(2001, '-') + "1") == -1, "deep unary signs");
    std::string longSum = "1";
    for (int i = 0; i < 3000; ++i) longSum += "+1";
    check(evaluateExpression(longSum) == 3001, "long expression");
}

void testExpressionErrors() {
    const char* invalid[] = {
        "", "  ", "#", "1+", "*2", "2**3", "()", "(1+)",
        "1 2", "2(3)", "(2)3", "(2)(3)", "1.5", "abc", "2&3", "2^^3",
        "2^", "2/*3", "1#2", "1##", "-", "(+)"
    };
    for (const char* input : invalid)
        expectError([&] { evaluateExpression(input); }, ErrorCode::InvalidExpression,
                    std::string("invalid expression ") + input);
    const char* mismatched[] = {"(1", "1)", "((1)", "(1))", ")1(", "("};
    for (const char* input : mismatched)
        expectError([&] { evaluateExpression(input); }, ErrorCode::MismatchedParentheses,
                    std::string("unmatched parentheses ") + input);
    expectError([] { evaluateExpression("10/(3-3)"); }, ErrorCode::DivisionByZero, "division by zero", 2);
    const char* negativePowers[] = {"2^-1", "2^(-2)", "2^-3^0"};
    for (const char* input : negativePowers)
        expectError([&] { evaluateExpression(input); }, ErrorCode::NegativeExponent, "negative exponent", 1);
    const char* overflowing[] = {
        "9223372036854775808", "9999999999999999999999999999999", "9223372036854775807+1",
        "-9223372036854775807-2", "3037000500*3037000500", "2^63",
        "(-9223372036854775807-1)/-1", "-(-9223372036854775807-1)",
        "(-9223372036854775807-1)*-1", "(-9223372036854775807-1)-1"
    };
    for (const char* input : overflowing)
        expectError([&] { evaluateExpression(input); }, ErrorCode::Overflow, std::string("overflow ") + input);
    check(evaluateExpression("1+2") == 3, "usable after earlier errors");
}

struct TraceState {
    int steps = 0;
    int reductions = 0;
    calculator::TraceStep last;
};

void recordTrace(const calculator::TraceStep& step, void* context) {
    auto& state = *static_cast<TraceState*>(context);
    if (state.steps == 0)
        check(step.operatorStack == "[#]" && step.operandStack == "[]", "trace initialization");
    if (step.action.find("归约：") == 0) ++state.reductions;
    ++state.steps;
    state.last = step;
}

void testTrace() {
    TraceState state;
    check(evaluateExpression("10-2*3", recordTrace, &state) == 4, "trace result");
    check(state.steps > 6 && state.reductions == 2, "trace reductions");
    check(state.last.operatorStack == "[]" && state.last.operandStack == "[4]"
          && state.last.position == 6 && state.last.inputToken == "#", "trace final snapshots");
    TraceState second;
    evaluateExpression("1#", recordTrace, &second);
    check(second.last.position == 1, "explicit sentinel position");
}

void testPolynomialExamples() {
    const auto a = Polynomial::fromSequence("2 2 3 5 1");
    const auto b = Polynomial::fromSequence("2 3 3 4 0");
    check((a + b).toString() == "5x^3 + 5x + 4", "assignment polynomial addition");
    check((Polynomial::fromSequence("2 3 4 2 2")
           + Polynomial::fromSequence("2 -3 4 -2 2")).toString() == "0", "complete cancellation");
    check((Polynomial::fromSequence("1 1 2")
           - Polynomial::fromSequence("2 1 2 1 1")).toString() == "-x", "assignment subtraction");
    check((Polynomial::fromSequence("2 1 1 1 0")
           * Polynomial::fromSequence("2 1 1 -1 0")).toString() == "x^2 - 1", "assignment multiplication");
    check(Polynomial::fromSequence("3 3 3 2 2 7 0").derivative().toString() == "9x^2 + 4x", "assignment derivative");
    check(Polynomial::fromSequence("1 2 2").evaluate(2) == 8, "assignment evaluation");
    check(a.toString() == "2x^3 + 5x" && b.toString() == "3x^3 + 4", "operations preserve inputs");
    check(a.toSequence() == "2 2 3 5 1", "serialize sequence");
    check(Polynomial::fromSequence(a.toSequence()).toString() == a.toString(), "round trip");
    check(Polynomial::fromSequence(" 3 0 4 -1 1 1 0 ").toString() == "-x + 1", "drop zero terms");
    check(Polynomial::fromSequence("2 0 2 0 0").isZero(), "all coefficients zero");
    const auto zero = Polynomial::fromSequence("0");
    check(zero.termCount() == 0 && zero.toSequence() == "0" && zero.evaluate(0) == 0, "zero representation");
    check((zero + a).toString() == a.toString() && (a - zero).toString() == a.toString(), "zero identities");
    check((zero * a).isZero() && (a * zero).isZero() && zero.derivative().isZero(), "zero operations");
    check(Polynomial::monomial(7, 0).derivative().isZero(), "constant derivative");
    check(Polynomial::fromSequence("2 2 2 1 0").evaluate(0.5L) == 1.5L, "fractional x");
    check(a.evaluate(-2) == -26 && a.evaluate(0) == 0, "negative and zero x");
    check(Polynomial::fromSequence("1 1 2147483647").evaluate(1) == 1, "very sparse exponent");
    check(Polynomial::fromSequence("1 -9223372036854775808 0").toString() == "-9223372036854775808", "INT64_MIN format");
    const auto minimum = Polynomial::monomial(std::numeric_limits<Integer>::min(), 0);
    check((minimum - minimum).isZero(), "MIN minus MIN polynomial");
    Polynomial copied(a);
    const Polynomial& sameObject = copied;
    copied = sameObject;
    check(copied.toString() == a.toString(), "copy and self assignment");
    copied = b;
    check(a.toString() == "2x^3 + 5x" && copied.toString() == b.toString(), "deep copy assignment");
    Polynomial moved(std::move(copied));
    check(moved.toString() == b.toString() && copied.isZero(), "move construction");
    copied = std::move(moved);
    check(copied.toString() == b.toString() && moved.isZero(), "move assignment");
    check(a.coefficientAt(3) == 2 && a.coefficientAt(2) == 0 && a.coefficientAt(0) == 0, "coefficient lookup");
}

void testPolynomialErrors() {
    const char* invalid[] = {
        "", "abc", "-1", "1", "1 2", "0 1 0", "1 2 0 4", "1 2 -1",
        "2 1 0 2 1", "2 1 1 2 1", "1 1 2147483648", "2147483648",
        "1 2.0 0", "1+2 0", "1 2 0z", "1 --2 0"
    };
    for (const char* input : invalid)
        expectError([&] { Polynomial::fromSequence(input); }, ErrorCode::InvalidInput,
                    std::string("invalid polynomial ") + input);
    expectError([] { Polynomial::fromSequence("1 9223372036854775808 0"); }, ErrorCode::Overflow, "positive coefficient overflow");
    expectError([] { Polynomial::fromSequence("1 -9223372036854775809 0"); }, ErrorCode::Overflow, "negative coefficient overflow");
    expectError([] { Polynomial::monomial(1, -1); }, ErrorCode::InvalidInput, "negative monomial exponent");
    expectError([] { Polynomial().coefficientAt(-1); }, ErrorCode::InvalidInput, "negative lookup exponent");
    const auto maximum = Polynomial::monomial(std::numeric_limits<Integer>::max(), 2);
    expectError([&] { maximum + maximum; }, ErrorCode::Overflow, "polynomial addition overflow");
    expectError([&] { maximum * Polynomial::monomial(2, 0); }, ErrorCode::Overflow, "polynomial multiplication overflow");
    expectError([&] { maximum.derivative(); }, ErrorCode::Overflow, "derivative overflow");
    expectError([] { Polynomial() - Polynomial::monomial(std::numeric_limits<Integer>::min(), 0); }, ErrorCode::Overflow, "negation overflow");
    expectError([] { Polynomial::monomial(1, INT_MAX) * Polynomial::monomial(1, 1); }, ErrorCode::Overflow, "exponent overflow");
    expectError([] { Polynomial().evaluate(std::numeric_limits<long double>::infinity()); }, ErrorCode::InvalidInput, "infinite x");
    expectError([] { Polynomial().evaluate(std::numeric_limits<long double>::quiet_NaN()); }, ErrorCode::InvalidInput, "NaN x");
    expectError([] { Polynomial::monomial(1, 2).evaluate(std::numeric_limits<long double>::max()); }, ErrorCode::Overflow, "floating overflow");
    check(maximum.toSequence() == "1 9223372036854775807 2", "failed operations preserve operand");
}

// Deterministic differential tests use dense fixed arrays as an independent
// reference. They exercise list insertion, cancellation, and sparse gaps.
std::uint32_t randomState = 0xC0DE1234u;
unsigned randomNumber() {
    randomState = randomState * 1664525u + 1013904223u;
    return randomState;
}

Polynomial densePolynomial(const Integer* coefficients, int degree) {
    int count = 0;
    std::string terms;
    for (int i = degree; i >= 0; --i) {
        if (coefficients[i] == 0) continue;
        ++count;
        terms += ' ' + std::to_string(coefficients[i]) + ' ' + std::to_string(i);
    }
    return Polynomial::fromSequence(std::to_string(count) + terms);
}

void checkCoefficients(const Polynomial& actual, const Integer* expected, int degree,
                       const std::string& label) {
    std::size_t count = 0;
    for (int i = 0; i <= degree; ++i) {
        check(actual.coefficientAt(i) == expected[i], label);
        if (expected[i] != 0) ++count;
    }
    check(actual.termCount() == count, label + " term count");
    check(actual.coefficientAt(degree + 1) == 0, label + " degree bound");
    check(Polynomial::fromSequence(actual.toSequence()).toString() == actual.toString(), label + " round trip");
}

void testRandomPolynomials() {
    for (int trial = 0; trial < 300; ++trial) {
        Integer a[8]{}, b[8]{}, sum[8]{}, difference[8]{}, product[15]{}, derivative[8]{};
        for (int i = 0; i < 8; ++i) {
            a[i] = static_cast<int>(randomNumber() % 11) - 5;
            b[i] = static_cast<int>(randomNumber() % 11) - 5;
            sum[i] = a[i] + b[i];
            difference[i] = a[i] - b[i];
            if (i != 0) derivative[i - 1] = a[i] * i;
        }
        for (int i = 0; i < 8; ++i)
            for (int j = 0; j < 8; ++j) product[i + j] += a[i] * b[j];
        const auto pa = densePolynomial(a, 7), pb = densePolynomial(b, 7);
        checkCoefficients(pa + pb, sum, 7, "random add");
        checkCoefficients(pa - pb, difference, 7, "random subtract");
        checkCoefficients(pa * pb, product, 14, "random multiply");
        checkCoefficients(pa.derivative(), derivative, 7, "random derivative");
        const int x = static_cast<int>(randomNumber() % 7) - 3;
        Integer expected = 0;
        for (int i = 7; i >= 0; --i) expected = expected * x + a[i];
        check(pa.evaluate(x) == static_cast<long double>(expected), "random sparse vs dense evaluation");
        check((pa - pa).isZero(), "random self cancellation");
    }
}

struct GeneratedExpression { std::string text; Integer value; };

// Build bounded expression trees with an oracle computed independently of the
// parser. A small depth ensures reference arithmetic cannot overflow.
GeneratedExpression generateExpression(int depth) {
    if (depth == 0) {
        const Integer value = randomNumber() % 10;
        return {std::to_string(value), value};
    }
    auto a = generateExpression(depth - 1);
    auto b = generateExpression(depth - 1);
    const unsigned operation = randomNumber() % 5;
    if (operation == 4) return {"-(" + a.text + ")", -a.value};
    char symbol = '+';
    Integer value = 0;
    if (operation == 0) value = a.value + b.value;
    else if (operation == 1) { symbol = '-'; value = a.value - b.value; }
    else if (operation == 2) { symbol = '*'; value = a.value * b.value; }
    else {
        symbol = '/';
        if (b.value == 0) b = {"1", 1};
        value = a.value / b.value;
    }
    return {"(" + a.text + " " + symbol + " " + b.text + ")", value};
}

void testRandomExpressions() {
    for (int i = 0; i < 500; ++i) {
        const auto expression = generateExpression(4);
        check(evaluateExpression(expression.text) == expression.value, "random expression tree");
    }
    // Independently check the base table's precedence and left associativity
    // without parentheses, using small positive operands and nonzero divisors.
    for (int i = 0; i < 300; ++i) {
        const Integer a = randomNumber() % 100;
        const Integer b = randomNumber() % 100;
        const Integer c = randomNumber() % 9 + 1;
        const Integer d = randomNumber() % 9 + 1;
        const std::string expression = std::to_string(a) + "-" + std::to_string(b)
            + "*" + std::to_string(c) + "/" + std::to_string(d);
        check(evaluateExpression(expression) == a - (b * c) / d, "random unparenthesized precedence");
    }
}

} // namespace

int main() {
    try {
        testStack();
        testExpressionExamples();
        testExpressionErrors();
        testTrace();
        testPolynomialExamples();
        testPolynomialErrors();
        testRandomPolynomials();
        testRandomExpressions();
    } catch (const std::exception& error) {
        ++failures;
        std::cerr << "Unexpected exception: " << error.what() << '\n';
    }
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
