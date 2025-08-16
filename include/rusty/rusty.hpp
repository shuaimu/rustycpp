#ifndef RUSTY_HPP
#define RUSTY_HPP

// Rusty - Rust-inspired safe types for C++
//
// This library provides Rust-like types with proper lifetime annotations
// that work with the Rusty C++ Checker to ensure memory safety.
//
// All types follow Rust's ownership and borrowing principles:
// - Single ownership (Box, Vec)
// - Shared immutable access (Rc, Arc)
// - Explicit nullability (Option)
// - Explicit error handling (Result)

#include "rusty/std_minimal.hpp"  // Minimal std support
#include "rusty/box.hpp"
#include "rusty/arc.hpp"
#include "rusty/rc.hpp"
#include "rusty/vec.hpp"
#include "rusty/option.hpp"
#include "rusty/result.hpp"

// Convenience aliases in rusty namespace
// @safe
namespace rusty {
    // Common Result types
    template<typename T>
    using ResultVoid = Result<T, void>;
    
    template<typename T>
    using ResultString = Result<T, const char*>;
    
    template<typename T>
    using ResultInt = Result<T, int>;
    
    // Smart pointer conversions (Rust-idiomatic names)
    template<typename T>
    // @lifetime: owned
    Box<T> from_raw(T* ptr) {
        return Box<T>(ptr);
    }
    
    // C++ style alias
    template<typename T>
    // @lifetime: owned
    Box<T> box_from_raw(T* ptr) {
        return from_raw(ptr);
    }
    
    template<typename T>
    // @lifetime: owned
    Arc<T> arc_from_box(Box<T>&& box) {
        T* ptr = box.into_raw();
        Arc<T> result = Arc<T>::new_(std::move(*ptr));
        delete ptr;
        return result;
    }
    
    template<typename T>
    // @lifetime: owned
    Rc<T> rc_from_box(Box<T>&& box) {
        T* ptr = box.into_raw();
        Rc<T> result = Rc<T>::new_(std::move(*ptr));
        delete ptr;
        return result;
    }
    
    // Rust-style type aliases for convenience
    template<typename T>
    using Boxed = Box<T>;
    
    template<typename T>
    using Shared = Arc<T>;
    
    template<typename T>
    using RefCounted = Rc<T>;
}

#endif // RUSTY_HPP