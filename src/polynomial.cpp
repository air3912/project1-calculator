#include "calculator/polynomial.hpp"
#include "calculator/checked_integer.hpp"
#include <cctype>
#include <climits>
#include <cmath>

namespace calculator {
namespace {

bool whitespace(char ch) {
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

// Parse directly into a bounded unsigned magnitude; never overflow while
// reading a literal, including the special magnitude of INT64_MIN.
std::int64_t readInteger(const std::string& input, std::size_t& position) {
    while (position < input.size() && whitespace(input[position])) ++position;
    const std::size_t start = position;
    bool negative = false;
    if (position < input.size() && (input[position] == '-' || input[position] == '+'))
        negative = input[position++] == '-';
    if (position == input.size() || input[position] < '0' || input[position] > '9')
        throw Error(ErrorCode::InvalidInput, "多项式输入需要完整的整数序列", start);
    const detail::Unsigned limit = negative ? detail::magnitude(detail::integerMin)
                                            : detail::integerMax;
    detail::Unsigned value = 0;
    while (position < input.size() && input[position] >= '0' && input[position] <= '9') {
        const unsigned digit = static_cast<unsigned>(input[position] - '0');
        if (value > (limit - digit) / 10) detail::overflow(start);
        value = value * 10 + digit;
        ++position;
    }
    if (position < input.size() && !whitespace(input[position]))
        throw Error(ErrorCode::InvalidInput, "多项式序列中的整数必须用空白分隔", position);
    if (!negative) return static_cast<std::int64_t>(value);
    if (value == detail::magnitude(detail::integerMin)) return detail::integerMin;
    return -static_cast<std::int64_t>(value);
}

long double finite(long double value) {
    if (!std::isfinite(value))
        throw Error(ErrorCode::Overflow, "多项式求值超出浮点数可表示范围");
    return value;
}

long double realPower(long double base, int exponent) {
    long double result = 1;
    while (exponent > 0) {
        if (exponent % 2 != 0) result = finite(result * base);
        exponent /= 2;
        if (exponent != 0) base = finite(base * base);
    }
    return result;
}

} // namespace

Polynomial::~Polynomial() { clear(); }

Polynomial::Polynomial(const Polynomial& other) {
    try {
        for (const Node* node = other.head_; node; node = node->next)
            append(node->coefficient, node->exponent);
    } catch (...) {
        clear();
        throw;
    }
}

Polynomial::Polynomial(Polynomial&& other) noexcept { swap(other); }

Polynomial& Polynomial::operator=(const Polynomial& other) {
    if (this != &other) {
        Polynomial copy(other);
        swap(copy);
    }
    return *this;
}

Polynomial& Polynomial::operator=(Polynomial&& other) noexcept {
    if (this != &other) {
        clear();
        swap(other);
    }
    return *this;
}

void Polynomial::swap(Polynomial& other) noexcept {
    Node* oldHead = head_;
    Node* oldTail = tail_;
    const std::size_t oldSize = size_;
    head_ = other.head_;
    tail_ = other.tail_;
    size_ = other.size_;
    other.head_ = oldHead;
    other.tail_ = oldTail;
    other.size_ = oldSize;
}

void Polynomial::clear() noexcept {
    while (head_) {
        Node* next = head_->next;
        delete head_;
        head_ = next;
    }
    tail_ = nullptr;
    size_ = 0;
}

void Polynomial::append(Coefficient coefficient, int exponent) {
    if (coefficient == 0) return;
    Node* node = new Node{coefficient, exponent, nullptr};
    if (tail_) tail_->next = node;
    else head_ = node;
    tail_ = node;
    ++size_;
}

Polynomial Polynomial::fromSequence(const std::string& input) {
    std::size_t position = 0;
    const Coefficient count = readInteger(input, position);
    if (count < 0 || count > INT_MAX)
        throw Error(ErrorCode::InvalidInput, "多项式项数必须在 0 到 INT_MAX 之间", 0);
    Polynomial result;
    int previousExponent = 0;
    for (Coefficient i = 0; i < count; ++i) {
        const Coefficient coefficient = readInteger(input, position);
        while (position < input.size() && whitespace(input[position])) ++position;
        const std::size_t exponentStart = position;
        const Coefficient exponent = readInteger(input, position);
        if (exponent < 0 || exponent > INT_MAX)
            throw Error(ErrorCode::InvalidInput, "多项式指数必须在 0 到 INT_MAX 之间", exponentStart);
        if (i != 0 && exponent >= previousExponent)
            throw Error(ErrorCode::InvalidInput, "多项式指数必须严格降序，不能重复", exponentStart);
        previousExponent = static_cast<int>(exponent);
        result.append(coefficient, previousExponent);
    }
    while (position < input.size() && whitespace(input[position])) ++position;
    if (position != input.size())
        throw Error(ErrorCode::InvalidInput, "项数与系数、指数对的数量不一致", position);
    return result;
}

Polynomial Polynomial::monomial(Coefficient coefficient, int exponent) {
    if (exponent < 0) throw Error(ErrorCode::InvalidInput, "多项式指数不能为负数");
    Polynomial result;
    result.append(coefficient, exponent);
    return result;
}

Polynomial::Coefficient Polynomial::coefficientAt(int exponent) const {
    if (exponent < 0) throw Error(ErrorCode::InvalidInput, "多项式指数不能为负数");
    const Node* node = head_;
    while (node && node->exponent > exponent) node = node->next;
    return node && node->exponent == exponent ? node->coefficient : 0;
}

std::string Polynomial::toString() const {
    if (isZero()) return "0";
    std::string output;
    for (const Node* node = head_; node; node = node->next) {
        const bool negative = node->coefficient < 0;
        if (node == head_) {
            if (negative) output += '-';
        } else {
            output += negative ? " - " : " + ";
        }
        const detail::Unsigned absolute = detail::magnitude(node->coefficient);
        if (node->exponent == 0 || absolute != 1) output += std::to_string(absolute);
        if (node->exponent != 0) {
            output += 'x';
            if (node->exponent != 1) output += '^' + std::to_string(node->exponent);
        }
    }
    return output;
}

std::string Polynomial::toSequence() const {
    std::string output = std::to_string(size_);
    for (const Node* node = head_; node; node = node->next)
        output += ' ' + std::to_string(node->coefficient) + ' ' + std::to_string(node->exponent);
    return output;
}

Polynomial Polynomial::combine(const Polynomial& other, bool subtract) const {
    Polynomial result;
    const Node* a = head_;
    const Node* b = other.head_;
    // Merge two ordered lists. Cancellation terms are omitted by append().
    while (a || b) {
        if (!b || (a && a->exponent > b->exponent)) {
            result.append(a->coefficient, a->exponent);
            a = a->next;
        } else if (!a || b->exponent > a->exponent) {
            result.append(subtract ? detail::negate(b->coefficient) : b->coefficient,
                          b->exponent);
            b = b->next;
        } else {
            result.append(subtract ? detail::subtract(a->coefficient, b->coefficient)
                                   : detail::add(a->coefficient, b->coefficient),
                          a->exponent);
            a = a->next;
            b = b->next;
        }
    }
    return result;
}

Polynomial Polynomial::operator+(const Polynomial& other) const { return combine(other, false); }
Polynomial Polynomial::operator-(const Polynomial& other) const { return combine(other, true); }

void Polynomial::accumulate(Coefficient coefficient, int exponent) {
    if (coefficient == 0) return;
    Node** link = &head_;
    Node* previous = nullptr;
    while (*link && (*link)->exponent > exponent) {
        previous = *link;
        link = &(*link)->next;
    }
    if (*link && (*link)->exponent == exponent) {
        Node* node = *link;
        const Coefficient sum = detail::add(node->coefficient, coefficient);
        if (sum != 0) {
            node->coefficient = sum;
        } else {
            *link = node->next;
            if (tail_ == node) tail_ = previous;
            delete node;
            --size_;
        }
    } else {
        Node* node = new Node{coefficient, exponent, *link};
        *link = node;
        if (!node->next) tail_ = node;
        ++size_;
    }
}

Polynomial Polynomial::operator*(const Polynomial& other) const {
    Polynomial result;
    // Multiply every pair of terms, inserting/merging into the ordered list.
    for (const Node* a = head_; a; a = a->next) {
        for (const Node* b = other.head_; b; b = b->next) {
            if (a->exponent > INT_MAX - b->exponent)
                throw Error(ErrorCode::Overflow, "乘积多项式的指数超出 INT_MAX");
            result.accumulate(detail::multiply(a->coefficient, b->coefficient),
                              a->exponent + b->exponent);
        }
    }
    return result;
}

Polynomial Polynomial::derivative() const {
    Polynomial result;
    for (const Node* node = head_; node; node = node->next) {
        if (node->exponent != 0)
            result.append(detail::multiply(node->coefficient, node->exponent), node->exponent - 1);
    }
    return result;
}

long double Polynomial::evaluate(long double x) const {
    if (!std::isfinite(x))
        throw Error(ErrorCode::InvalidInput, "x 必须是有限实数");
    if (isZero()) return 0;
    // Sparse Horner evaluation uses exponent gaps instead of filling missing
    // terms; powers are evaluated by repeated squaring.
    long double result = static_cast<long double>(head_->coefficient);
    int previousExponent = head_->exponent;
    for (const Node* node = head_->next; node; node = node->next) {
        result = finite(finite(result * realPower(x, previousExponent - node->exponent))
                        + static_cast<long double>(node->coefficient));
        previousExponent = node->exponent;
    }
    return finite(result * realPower(x, previousExponent));
}

} // namespace calculator
