#ifndef RUSTY_OPTION_HPP
#define RUSTY_OPTION_HPP

#include <utility>
#include <stdexcept>

// Option<T> - Represents an optional value
// Equivalent to Rust's Option<T>
//
// Guarantees:
// - Type-safe null handling
// - No null pointer dereferencing
// - Explicit handling of absence

// @safe
namespace rusty {

// Tag types for Option variants
struct None_t {};
inline constexpr None_t None{};

template<typename T>
class Option {
private:
    bool has_value;
    union {
        T value;
        char dummy;  // For when there's no value
    };
    
public:
    // Constructors
    Option() : has_value(false), dummy(0) {}
    
    Option(None_t) : has_value(false), dummy(0) {}
    
    Option(T val) : has_value(true), value(std::move(val)) {}
    
    // Copy constructor
    Option(const Option& other) : has_value(other.has_value) {
        if (has_value) {
            new (&value) T(other.value);
        }
    }
    
    // Move constructor
    Option(Option&& other) noexcept : has_value(other.has_value) {
        if (has_value) {
            new (&value) T(std::move(other.value));
            other.has_value = false;
        }
    }
    
    // Copy assignment
    Option& operator=(const Option& other) {
        if (this != &other) {
            if (has_value) {
                value.~T();
            }
            has_value = other.has_value;
            if (has_value) {
                new (&value) T(other.value);
            }
        }
        return *this;
    }
    
    // Move assignment
    Option& operator=(Option&& other) noexcept {
        if (this != &other) {
            if (has_value) {
                value.~T();
            }
            has_value = other.has_value;
            if (has_value) {
                new (&value) T(std::move(other.value));
                other.has_value = false;
            }
        }
        return *this;
    }
    
    // Destructor
    ~Option() {
        if (has_value) {
            value.~T();
        }
    }
    
    // Check if Option contains a value
    bool is_some() const { return has_value; }
    bool is_none() const { return !has_value; }
    
    // Explicit bool conversion
    explicit operator bool() const { return has_value; }
    
    // Unwrap the value (panics if None) - Rust style
    // @lifetime: owned
    T unwrap() {
        if (!has_value) {
            throw std::runtime_error("Called unwrap on None");
        }
        T result = std::move(value);
        value.~T();
        has_value = false;
        return result;
    }
    
    // Expect with custom message - Rust style
    // @lifetime: owned
    T expect(const char* msg) {
        if (!has_value) {
            throw std::runtime_error(msg);
        }
        return unwrap();
    }
    
    // Unwrap with default value
    // @lifetime: owned
    T unwrap_or(T default_value) {
        if (has_value) {
            return unwrap();
        }
        return default_value;
    }
    
    // Get reference to value (panics if None)
    // @lifetime: (&'a) -> &'a
    T& unwrap_ref() {
        if (!has_value) {
            throw std::runtime_error("Called unwrap_ref on None");
        }
        return value;
    }
    
    // @lifetime: (&'a) -> &'a
    const T& unwrap_ref() const {
        if (!has_value) {
            throw std::runtime_error("Called unwrap_ref on None");
        }
        return value;
    }
    
    // Map function over the value
    template<typename F>
    // @lifetime: owned
    auto map(F&& f) -> Option<decltype(f(std::declval<T>()))> {
        using U = decltype(f(std::declval<T>()));
        if (has_value) {
            return Option<U>(f(std::move(value)));
        }
        return Option<U>(None);
    }
    
    // Map function over reference
    template<typename F>
    // @lifetime: (&'a) -> owned
    auto map_ref(F&& f) const -> Option<decltype(f(std::declval<const T&>()))> {
        using U = decltype(f(std::declval<const T&>()));
        if (has_value) {
            return Option<U>(f(value));
        }
        return Option<U>(None);
    }
    
    // Take the value out, leaving None
    // @lifetime: owned
    Option<T> take() {
        Option<T> result = std::move(*this);
        *this = Option<T>(None);
        return result;
    }
    
    // Replace the value
    void replace(T new_value) {
        if (has_value) {
            value = std::move(new_value);
        } else {
            new (&value) T(std::move(new_value));
            has_value = true;
        }
    }
};

// Helper function to create Some variant
template<typename T>
// @lifetime: owned
Option<T> Some(T value) {
    return Option<T>(std::move(value));
}

// Equality operators
template<typename T>
bool operator==(const Option<T>& lhs, const Option<T>& rhs) {
    if (lhs.is_none() && rhs.is_none()) return true;
    if (lhs.is_some() && rhs.is_some()) return lhs.unwrap_ref() == rhs.unwrap_ref();
    return false;
}

template<typename T>
bool operator!=(const Option<T>& lhs, const Option<T>& rhs) {
    return !(lhs == rhs);
}

} // namespace rusty

#endif // RUSTY_OPTION_HPP