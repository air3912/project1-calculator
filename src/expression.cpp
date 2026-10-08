#include "calculator/expression.hpp"
#include "calculator/checked_integer.hpp"
#include "calculator/stack.hpp"
#include <cctype>

namespace calculator {
namespace {

struct Operator {
    char symbol = '#'; // P/N are internal unary plus/minus operators.
    std::size_t position = 0;
};

std::string operatorName(char op) {
    if (op == 'P') return "u+";
    if (op == 'N') return "u-";
    return std::string(1, op);
}

int tableIndex(char op) {
    switch (op) {
    case '+': return 0;
    case '-': return 1;
    case '*': return 2;
    case '/': return 3;
    case '(': return 4;
    case ')': return 5;
    case '#': return 6;
    default: return -1;
    }
}

int priority(char op) {
    switch (op) {
    case '+': case '-': return 1;
    case '*': case '/': return 2;
    case 'P': case 'N': return 3;
    case '^': return 4;
    default: return 0;
    }
}

char relation(char top, char incoming) {
    // The assignment's exact seven-operator precedence table.
    static constexpr char table[7][7] = {
        {'>', '>', '<', '<', '<', '>', '>'},
        {'>', '>', '<', '<', '<', '>', '>'},
        {'>', '>', '>', '>', '<', '>', '>'},
        {'>', '>', '>', '>', '<', '>', '>'},
        {'<', '<', '<', '<', '<', '=', 'x'},
        {'>', '>', '>', '>', 'x', '>', '>'},
        {'<', '<', '<', '<', '<', 'x', '='}
    };
    const int a = tableIndex(top), b = tableIndex(incoming);
    if (a >= 0 && b >= 0) return table[a][b];
    // Extend the table for exponentiation and prefix signs.
    if (top == '(' || top == '#') return '<';
    if (incoming == '(') return '<';
    if (incoming == ')' || incoming == '#') return '>';
    if (top == '^' && incoming == '^') return '<'; // right associative
    return priority(top) < priority(incoming) ? '<' : '>';
}

class Evaluator {
public:
    Evaluator(const std::string& input, TraceCallback callback, void* context)
        : input_(input), callback_(callback), context_(context) {}

    std::int64_t run() {
        operators_.push(Operator{'#', 0});
        emit(0, "", "初始化：终止符 # 入运算符栈");
        bool expectOperand = true;
        std::size_t openParentheses = 0;
        while (true) {
            skipWhitespace();
            const std::size_t start = position_;
            const char ch = position_ == input_.size() ? '#' : input_[position_];
            if (ch >= '0' && ch <= '9') {
                if (!expectOperand) invalid("两个运算数之间缺少运算符", start);
                const std::int64_t value = readNumber();
                operands_.push(value);
                emit(start, input_.substr(start, position_ - start), "识别整数并入运算数栈");
                expectOperand = false;
                continue;
            }
            if (ch == '#') {
                if (position_ < input_.size()) {
                    ++position_;
                    skipWhitespace();
                    if (position_ != input_.size()) invalid("# 只能作为表达式末尾的终止符", start);
                }
                if (openParentheses != 0) {
                    std::size_t openingPosition = start;
                    for (std::size_t i = operators_.size(); i > 0; --i) {
                        if (operators_.at(i - 1).symbol == '(') {
                            openingPosition = operators_.at(i - 1).position;
                            break;
                        }
                    }
                    throw Error(ErrorCode::MismatchedParentheses, "左括号没有对应的右括号", openingPosition);
                }
                if (expectOperand) invalid("表达式为空或末尾缺少运算数", start);
                consumeOperator(Operator{'#', start});
                if (operands_.size() != 1) invalid("表达式不能归约为一个结果", start);
                emit(start, "#", "计算完成");
                return operands_.top();
            }
            if (ch == '(') {
                if (!expectOperand) invalid("左括号前缺少运算符，不支持隐式乘法", start);
                operators_.push(Operator{'(', start});
                ++openParentheses;
                ++position_;
                emit(start, "(", "左括号入运算符栈");
                continue;
            }
            if (ch == ')') {
                if (openParentheses == 0)
                    throw Error(ErrorCode::MismatchedParentheses, "右括号没有对应的左括号", start);
                if (expectOperand) invalid("括号内为空或右括号前缺少运算数", start);
                consumeOperator(Operator{')', start});
                --openParentheses;
                ++position_;
                continue;
            }
            if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^') {
                if (expectOperand) {
                    if (ch != '+' && ch != '-') invalid("二元运算符前缺少运算数", start);
                    // Prefix signs must be pushed without reducing a pending
                    // binary operator, including in expressions such as 2^-1.
                    operators_.push(Operator{ch == '+' ? 'P' : 'N', start});
                    emit(start, std::string(1, ch), "一元符号入运算符栈");
                } else {
                    consumeOperator(Operator{ch, start});
                    expectOperand = true;
                }
                ++position_;
                continue;
            }
            invalid("表达式中含有不支持的字符", start);
        }
    }

private:
    [[noreturn]] static void invalid(const std::string& message, std::size_t position) {
        throw Error(ErrorCode::InvalidExpression, message, position);
    }

    void skipWhitespace() {
        while (position_ < input_.size()
               && std::isspace(static_cast<unsigned char>(input_[position_]))) ++position_;
    }

    std::int64_t readNumber() {
        const std::size_t start = position_;
        std::int64_t value = 0;
        while (position_ < input_.size() && input_[position_] >= '0' && input_[position_] <= '9') {
            const int digit = input_[position_] - '0';
            if (value > (detail::integerMax - digit) / 10) detail::overflow(start);
            value = value * 10 + digit;
            ++position_;
        }
        return value;
    }

    void consumeOperator(Operator incoming) {
        while (true) {
            const char order = relation(operators_.top().symbol, incoming.symbol);
            if (order == '<') {
                operators_.push(incoming);
                emit(incoming.position, operatorName(incoming.symbol), "运算符入栈");
                return;
            }
            if (order == '=') {
                operators_.pop();
                emit(incoming.position, operatorName(incoming.symbol),
                     incoming.symbol == ')' ? "匹配括号并弹出左括号" : "匹配终止符 #");
                return;
            }
            if (order == 'x')
                throw Error(ErrorCode::MismatchedParentheses, "括号不匹配", incoming.position);
            reduce(incoming);
        }
    }

    void reduce(Operator incoming) {
        const Operator op = operators_.top();
        const bool unary = op.symbol == 'N' || op.symbol == 'P';
        if (operands_.size() < (unary ? 1u : 2u)) invalid("运算符缺少运算数", op.position);
        const std::int64_t right = operands_.top();
        std::int64_t result = 0;
        std::string action;
        if (unary) {
            result = op.symbol == 'N' ? detail::negate(right, op.position) : right;
            action = operatorName(op.symbol) + " " + std::to_string(right);
        } else {
            const std::int64_t left = operands_.at(operands_.size() - 2);
            switch (op.symbol) {
            case '+': result = detail::add(left, right, op.position); break;
            case '-': result = detail::subtract(left, right, op.position); break;
            case '*': result = detail::multiply(left, right, op.position); break;
            case '/': result = detail::divide(left, right, op.position); break;
            case '^': result = detail::power(left, right, op.position); break;
            default: invalid("不能对该符号执行计算", op.position);
            }
            action = std::to_string(left) + " " + operatorName(op.symbol) + " " + std::to_string(right);
        }
        operators_.pop();
        operands_.pop();
        if (!unary) operands_.pop();
        operands_.push(result);
        emit(incoming.position, operatorName(incoming.symbol),
             "归约：" + action + " = " + std::to_string(result));
    }

    void emit(std::size_t position, const std::string& token, const std::string& action) const {
        if (!callback_) return; // Avoid snapshot allocations when trace is off.
        TraceStep step{position, token, "[", "[", action};
        for (std::size_t i = 0; i < operators_.size(); ++i) {
            if (i != 0) step.operatorStack += ", ";
            step.operatorStack += operatorName(operators_.at(i).symbol);
        }
        for (std::size_t i = 0; i < operands_.size(); ++i) {
            if (i != 0) step.operandStack += ", ";
            step.operandStack += std::to_string(operands_.at(i));
        }
        step.operatorStack += ']';
        step.operandStack += ']';
        callback_(step, context_);
    }

    const std::string& input_;
    TraceCallback callback_;
    void* context_;
    std::size_t position_ = 0;
    Stack<Operator> operators_;
    Stack<std::int64_t> operands_;
};

} // namespace

std::int64_t evaluateExpression(const std::string& input, TraceCallback callback, void* context) {
    return Evaluator(input, callback, context).run();
}

} // namespace calculator
