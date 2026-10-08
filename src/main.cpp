#include "calculator/error.hpp"
#include "calculator/expression.hpp"
#include "calculator/polynomial.hpp"
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

namespace {

void usage() {
    std::cout
        << "Project1 计算核心验证入口\n"
        << "  calculator expr \"表达式\" [--trace]\n"
        << "  calculator poly show \"n c1 e1 ... cn en\"\n"
        << "  calculator poly add|sub|mul \"多项式 A 序列\" \"多项式 B 序列\"\n"
        << "  calculator poly diff \"多项式序列\"\n"
        << "  calculator poly eval \"多项式序列\" x\n"
        << "例：calculator expr \"((2+3)*4+1)/3\" --trace\n"
        << "例：calculator poly mul \"2 1 1 1 0\" \"2 1 1 -1 0\"\n";
}

void printTrace(const calculator::TraceStep& step, void* context) {
    std::ostream& output = *static_cast<std::ostream*>(context);
    output << "位置=" << step.position << " 输入="
           << (step.inputToken.empty() ? "<初始化>" : step.inputToken)
           << "  运算符栈=" << step.operatorStack
           << "  运算数栈=" << step.operandStack
           << "  " << step.action << '\n';
}

void printPolynomial(const calculator::Polynomial& polynomial) {
    std::cout << "结果：" << polynomial.toString()
              << "\n序列：" << polynomial.toSequence() << '\n';
}

long double parseReal(const char* input) {
    char* end = nullptr;
    errno = 0;
    const long double value = std::strtold(input, &end);
    if (end == input || *end != '\0' || errno == ERANGE || !std::isfinite(value))
        throw calculator::Error(calculator::ErrorCode::InvalidInput, "x 必须是可表示的有限实数");
    return value;
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--help")) {
        usage();
        return 0;
    }
    try {
        const std::string mode = argv[1];
        if (mode == "expr" && (argc == 3 || argc == 4)) {
            const bool trace = argc == 4;
            if (trace && std::string(argv[3]) != "--trace")
                throw calculator::Error(calculator::ErrorCode::InvalidInput, "表达式模式仅支持 --trace 选项");
            const auto result = calculator::evaluateExpression(
                argv[2], trace ? printTrace : nullptr, &std::cout);
            std::cout << "结果：" << result << '\n';
            return 0;
        }
        if (mode == "poly" && argc >= 4) {
            const std::string operation = argv[2];
            const auto a = calculator::Polynomial::fromSequence(argv[3]);
            if (operation == "show" && argc == 4) printPolynomial(a);
            else if (operation == "diff" && argc == 4) printPolynomial(a.derivative());
            else if (operation == "eval" && argc == 5) {
                std::cout << "结果：" << std::setprecision(std::numeric_limits<long double>::max_digits10)
                          << a.evaluate(parseReal(argv[4])) << '\n';
            } else if ((operation == "add" || operation == "sub" || operation == "mul") && argc == 5) {
                const auto b = calculator::Polynomial::fromSequence(argv[4]);
                if (operation == "add") printPolynomial(a + b);
                else if (operation == "sub") printPolynomial(a - b);
                else printPolynomial(a * b);
            } else {
                throw calculator::Error(calculator::ErrorCode::InvalidInput, "多项式命令或参数数量不正确");
            }
            return 0;
        }
        throw calculator::Error(calculator::ErrorCode::InvalidInput, "命令或参数数量不正确，请使用 --help");
    } catch (const calculator::Error& error) {
        std::cerr << "错误：" << error.what();
        if (error.position() != calculator::Error::noPosition)
            std::cerr << "（输入位置 " << error.position() << "，从 0 开始）";
        std::cerr << '\n';
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "运行失败：" << error.what() << '\n';
        return 3;
    }
}
