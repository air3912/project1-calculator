#pragma once

#include "calculator/error.hpp"
#include <cstddef>
#include <limits>
#include <memory>

namespace calculator {

// A hand-written sequential stack. No STL containers, iterators or algorithms.
// unique_ptr owns the raw array and keeps reallocation exception-safe.
template <typename T>
class Stack {
public:
    Stack() = default;
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    bool empty() const noexcept { return size_ == 0; }
    std::size_t size() const noexcept { return size_; }

    void push(const T& value) {
        // Preserve value even if it aliases an element in the old array.
        T copy = value;
        if (size_ == capacity_) grow();
        data_[size_] = copy;
        ++size_;
    }

    T pop() {
        if (empty()) throw Error(ErrorCode::EmptyStack, "不能从空栈弹出元素");
        T result = data_[size_ - 1];
        --size_;
        return result;
    }

    const T& top() const {
        if (empty()) throw Error(ErrorCode::EmptyStack, "空栈没有栈顶元素");
        return data_[size_ - 1];
    }

    // Read-only access from bottom to top, for trace display.
    const T& at(std::size_t index) const {
        if (index >= size_)
            throw Error(ErrorCode::InvalidInput, "栈下标越界");
        return data_[index];
    }

    void clear() noexcept { size_ = 0; }

private:
    void grow() {
        if (capacity_ > std::numeric_limits<std::size_t>::max() / 2 / sizeof(T))
            throw Error(ErrorCode::Overflow, "栈容量超出可分配范围");
        const std::size_t newCapacity = capacity_ == 0 ? 8 : capacity_ * 2;
        std::unique_ptr<T[]> next(new T[newCapacity]);
        for (std::size_t i = 0; i < size_; ++i) next[i] = data_[i];
        data_.swap(next);
        capacity_ = newCapacity;
    }

    std::unique_ptr<T[]> data_;
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;
};

} // namespace calculator
