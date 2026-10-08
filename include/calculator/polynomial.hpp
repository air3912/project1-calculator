#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace calculator {

// A sparse polynomial stored as a singly linked list in descending exponent
// order. Every stored term has a nonzero coefficient and a unique exponent.
class Polynomial {
public:
    using Coefficient = std::int64_t;

    Polynomial() noexcept = default;
    ~Polynomial();
    Polynomial(const Polynomial& other);
    Polynomial(Polynomial&& other) noexcept;
    Polynomial& operator=(const Polynomial& other);
    Polynomial& operator=(Polynomial&& other) noexcept;

    // Format: n c1 e1 ... cn en; whitespace-separated integers.
    // Exponents must be strictly descending and in [0, INT_MAX].
    static Polynomial fromSequence(const std::string& input);
    static Polynomial monomial(Coefficient coefficient, int exponent);

    bool isZero() const noexcept { return head_ == nullptr; }
    std::size_t termCount() const noexcept { return size_; }
    Coefficient coefficientAt(int exponent) const;

    std::string toString() const;   // e.g. "2x^3 - x + 4"
    std::string toSequence() const; // zero polynomial: "0"
    Polynomial operator+(const Polynomial& other) const;
    Polynomial operator-(const Polynomial& other) const;
    Polynomial operator*(const Polynomial& other) const;
    Polynomial derivative() const;
    long double evaluate(long double x) const;

    void swap(Polynomial& other) noexcept;

private:
    struct Node {
        Coefficient coefficient;
        int exponent;
        Node* next;
    };

    void clear() noexcept;
    void append(Coefficient coefficient, int exponent);
    void accumulate(Coefficient coefficient, int exponent);
    Polynomial combine(const Polynomial& other, bool subtract) const;

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;
};

} // namespace calculator
