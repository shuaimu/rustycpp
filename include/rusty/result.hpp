#ifndef RUSTY_RESULT_HPP
#define RUSTY_RESULT_HPP

#include <utility>
#include <stdexcept>
#include <variant>

// Result<T, E> - Represents either success (Ok) or failure (Err)
// Equivalent to Rust's Result<T, E>
//
// Guarantees:
// - Explicit error handling
// - No exceptions unless explicitly unwrapped
// - Type-safe error propagation

// @safe
namespace rusty {

template<typename T, typename E>
class Result {
private:
    std::variant<T, E> data;
    bool is_ok_value;
    
public:
    // Constructors for Ok variant
    static Result Ok(T value) {
        Result r;
        r.data = std::move(value);
        r.is_ok_value = true;
        return r;
    }
    
    // Constructors for Err variant
    static Result Err(E error) {
        Result r;
        r.data = std::move(error);
        r.is_ok_value = false;
        return r;
    }
    
    // Default constructor (private, use Ok/Err factories)
    Result() : is_ok_value(false) {}
    
    // Copy constructor
    Result(const Result& other) 
        : data(other.data), is_ok_value(other.is_ok_value) {}
    
    // Move constructor
    Result(Result&& other) noexcept 
        : data(std::move(other.data)), is_ok_value(other.is_ok_value) {}
    
    // Copy assignment
    Result& operator=(const Result& other) {
        if (this != &other) {
            data = other.data;
            is_ok_value = other.is_ok_value;
        }
        return *this;
    }
    
    // Move assignment
    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            data = std::move(other.data);
            is_ok_value = other.is_ok_value;
        }
        return *this;
    }
    
    // Check if Result is Ok
    bool is_ok() const { return is_ok_value; }
    bool is_err() const { return !is_ok_value; }
    
    // Explicit bool conversion (true if Ok)
    explicit operator bool() const { return is_ok_value; }
    
    // Unwrap the Ok value (panics if Err)
    // @lifetime: owned
    T unwrap() {
        if (!is_ok_value) {
            throw std::runtime_error("Called unwrap on Err");
        }
        return std::move(std::get<T>(data));
    }
    
    // Unwrap the Err value (panics if Ok)
    // @lifetime: owned
    E unwrap_err() {
        if (is_ok_value) {
            throw std::runtime_error("Called unwrap_err on Ok");
        }
        return std::move(std::get<E>(data));
    }
    
    // Unwrap with default value
    // @lifetime: owned
    T unwrap_or(T default_value) {
        if (is_ok_value) {
            return unwrap();
        }
        return default_value;
    }
    
    // Get reference to Ok value (panics if Err)
    // @lifetime: (&'a) -> &'a
    T& unwrap_ref() {
        if (!is_ok_value) {
            throw std::runtime_error("Called unwrap_ref on Err");
        }
        return std::get<T>(data);
    }
    
    // @lifetime: (&'a) -> &'a
    const T& unwrap_ref() const {
        if (!is_ok_value) {
            throw std::runtime_error("Called unwrap_ref on Err");
        }
        return std::get<T>(data);
    }
    
    // Get reference to Err value (panics if Ok)
    // @lifetime: (&'a) -> &'a
    E& unwrap_err_ref() {
        if (is_ok_value) {
            throw std::runtime_error("Called unwrap_err_ref on Ok");
        }
        return std::get<E>(data);
    }
    
    // @lifetime: (&'a) -> &'a
    const E& unwrap_err_ref() const {
        if (is_ok_value) {
            throw std::runtime_error("Called unwrap_err_ref on Ok");
        }
        return std::get<E>(data);
    }
    
    // Map function over Ok value
    template<typename F>
    // @lifetime: owned
    auto map(F&& f) -> Result<decltype(f(std::declval<T>())), E> {
        using U = decltype(f(std::declval<T>()));
        if (is_ok_value) {
            return Result<U, E>::Ok(f(std::get<T>(std::move(data))));
        }
        return Result<U, E>::Err(std::get<E>(std::move(data)));
    }
    
    // Map function over Err value
    template<typename F>
    // @lifetime: owned
    auto map_err(F&& f) -> Result<T, decltype(f(std::declval<E>()))> {
        using U = decltype(f(std::declval<E>()));
        if (!is_ok_value) {
            return Result<T, U>::Err(f(std::get<E>(std::move(data))));
        }
        return Result<T, U>::Ok(std::get<T>(std::move(data)));
    }
    
    // Chain Results (monadic bind)
    template<typename F>
    // @lifetime: owned
    auto and_then(F&& f) -> decltype(f(std::declval<T>())) {
        using ResultType = decltype(f(std::declval<T>()));
        if (is_ok_value) {
            return f(std::get<T>(std::move(data)));
        }
        return ResultType::Err(std::get<E>(std::move(data)));
    }
    
    // Get Ok value or execute function
    template<typename F>
    // @lifetime: owned
    T unwrap_or_else(F&& f) {
        if (is_ok_value) {
            return std::get<T>(std::move(data));
        }
        return f(std::get<E>(std::move(data)));
    }
};

// Helper functions for creating Results
template<typename T, typename E>
// @lifetime: owned
Result<T, E> Ok(T value) {
    return Result<T, E>::Ok(std::move(value));
}

template<typename T, typename E>
// @lifetime: owned
Result<T, E> Err(E error) {
    return Result<T, E>::Err(std::move(error));
}

// Equality operators
template<typename T, typename E>
bool operator==(const Result<T, E>& lhs, const Result<T, E>& rhs) {
    if (lhs.is_ok() != rhs.is_ok()) return false;
    if (lhs.is_ok()) {
        return lhs.unwrap_ref() == rhs.unwrap_ref();
    }
    return lhs.unwrap_err_ref() == rhs.unwrap_err_ref();
}

template<typename T, typename E>
bool operator!=(const Result<T, E>& lhs, const Result<T, E>& rhs) {
    return !(lhs == rhs);
}

} // namespace rusty

#endif // RUSTY_RESULT_HPP