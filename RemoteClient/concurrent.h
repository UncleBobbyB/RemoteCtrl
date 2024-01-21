#pragma once

template <typename T>
class concurrent {
private:
    T object;
    mutable std::mutex m;

public:
    template <typename... Args>
    explicit concurrent(Args&&... args) : object(std::forward<Args>(args)...) {}

    // Copy constructor
    concurrent(const concurrent& other) {
        std::lock_guard<decltype(m)> lock(other.m);
        object = other.object;
    }

    template <typename Key>
    decltype(auto) operator[](const Key& key) {
        std::lock_guard<decltype(m)> lock(m);
        return object[key];
    }

    // non-const lambda version
    template <typename Func>
    auto invoke(Func&& func) {
        std::lock_guard<decltype(m)> lock(m);
        return func(object);
    }

    // Const version of invoke
    template <typename Func>
    auto invoke(Func&& func) const {
        std::lock_guard<decltype(m)> lock(m);
        return func(object);
    }

    friend std::ostream& operator<<(std::ostream& os, const concurrent<T>& other) {
        std::lock_guard<decltype(m)> lock(other.m);
        os << other.object;
        return os;
    }
};
