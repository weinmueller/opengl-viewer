#pragma once

#include <variant>
#include <string>
#include <stdexcept>

// Generic Result type for error handling
// Usage:
//   Result<MeshData> load() { return Result<MeshData>::ok(data); }
//   Result<void> init() { return Result<void>::error("Failed to init"); }
template<typename T, typename E = std::string>
class Result {
public:
    static Result ok(T value) {
        return Result(std::move(value));
    }

    static Result error(E error) {
        return Result(Error{std::move(error)});
    }

    bool isOk() const {
        return std::holds_alternative<T>(m_value);
    }

    bool isError() const {
        return !isOk();
    }

    T& value() {
        if (!isOk()) {
            throw std::runtime_error("Result::value() called on error");
        }
        return std::get<T>(m_value);
    }

    const T& value() const {
        if (!isOk()) {
            throw std::runtime_error("Result::value() called on error");
        }
        return std::get<T>(m_value);
    }

    E& error() {
        if (isOk()) {
            throw std::runtime_error("Result::error() called on ok");
        }
        return std::get<Error>(m_value).error;
    }

    const E& error() const {
        if (isOk()) {
            throw std::runtime_error("Result::error() called on ok");
        }
        return std::get<Error>(m_value).error;
    }

    // Access value or return default
    T valueOr(T defaultValue) const {
        return isOk() ? std::get<T>(m_value) : std::move(defaultValue);
    }

    // Explicit bool conversion
    explicit operator bool() const {
        return isOk();
    }

private:
    struct Error {
        E error;
    };

    explicit Result(T value) : m_value(std::move(value)) {}
    explicit Result(Error error) : m_value(std::move(error)) {}

    std::variant<T, Error> m_value;
};

// Specialization for void return type
template<typename E>
class Result<void, E> {
public:
    static Result ok() {
        return Result(true);
    }

    static Result error(E error) {
        return Result(std::move(error));
    }

    bool isOk() const {
        return m_ok;
    }

    bool isError() const {
        return !m_ok;
    }

    E& error() {
        if (m_ok) {
            throw std::runtime_error("Result::error() called on ok");
        }
        return m_error;
    }

    const E& error() const {
        if (m_ok) {
            throw std::runtime_error("Result::error() called on ok");
        }
        return m_error;
    }

    explicit operator bool() const {
        return m_ok;
    }

private:
    explicit Result(bool ok) : m_ok(ok) {}
    explicit Result(E error) : m_ok(false), m_error(std::move(error)) {}

    bool m_ok;
    E m_error;
};
