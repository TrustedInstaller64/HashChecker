#pragma once

#include <string>
#include <utility>
#include <variant>

namespace infra {

/// A Rust-inspired Result type for explicit error handling without exceptions.
/// T = success value type, E = error type (defaults to std::string).
template <typename T, typename E = std::string>
class Result {
public:
    /// Create a success result.
    static Result ok(T value) {
        Result r;
        r.variant_ = std::move(value);
        return r;
    }

    /// Create an error result.
    static Result err(E error) {
        Result r;
        r.variant_ = std::move(error);
        return r;
    }

    Result(const Result&) = default;
    Result(Result&&) noexcept = default;
    Result& operator=(const Result&) = default;
    Result& operator=(Result&&) noexcept = default;

    /// Returns true if the result holds a success value.
    bool is_ok() const { return std::holds_alternative<T>(variant_); }

    /// Returns true if the result holds an error.
    bool is_err() const { return std::holds_alternative<E>(variant_); }

    /// Convenience: truthy when ok.
    explicit operator bool() const { return is_ok(); }

    /// Return the success value. Behaviour is undefined if this result is an error.
    T& unwrap() { return std::get<T>(variant_); }
    const T& unwrap() const { return std::get<T>(variant_); }

    /// Move the success value out. Behaviour is undefined if this result is an error.
    T unwrap_move() { return std::get<T>(std::move(variant_)); }

    /// Return the error value. Behaviour is undefined if this result is ok.
    E& error() { return std::get<E>(variant_); }
    const E& error() const { return std::get<E>(variant_); }

    /// Return the success value, or `fallback` if this result is an error.
    T value_or(T fallback) const {
        if (is_ok()) {
            return std::get<T>(variant_);
        }
        return fallback;
    }

private:
    Result() = default;

    std::variant<T, E> variant_;
};

} // namespace infra
