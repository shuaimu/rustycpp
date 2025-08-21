#ifndef RUSTY_VEC_HPP
#define RUSTY_VEC_HPP

#include <memory>
#include <algorithm>
#include <initializer_list>
#include <cassert>
#include <utility>  // for std::move, std::forward
#include <cstddef>  // for size_t
#include <cstring>  // for memcpy

// Vec<T> - A growable array with owned elements
// Equivalent to Rust's Vec<T>
//
// Guarantees:
// - Single ownership of the container
// - Elements are owned by the Vec
// - Automatic memory management
// - Move semantics for the container

// @safe
namespace rusty {

template<typename T>
class Vec {
private:
    T* data;
    size_t length;
    size_t capacity;
    
    void grow() {
        size_t new_capacity = capacity == 0 ? 1 : capacity * 2;
        T* new_data = static_cast<T*>(::operator new(new_capacity * sizeof(T)));
        
        // Move existing elements
        for (size_t i = 0; i < length; ++i) {
            new (&new_data[i]) T(std::move(data[i]));
            data[i].~T();
        }
        
        ::operator delete(data);
        data = new_data;
        capacity = new_capacity;
    }
    
public:
    // Default constructor - empty vec
    Vec() : data(nullptr), length(0), capacity(0) {}
    
    // Rust-idiomatic factory method - Vec::new()
    // @lifetime: owned
    static Vec<T> new_() {
        return Vec<T>();
    }
    
    // Rust-idiomatic factory with capacity - Vec::with_capacity()
    // @lifetime: owned
    static Vec<T> with_capacity(size_t cap) {
        Vec<T> v;
        if (cap > 0) {
            v.data = static_cast<T*>(::operator new(cap * sizeof(T)));
            v.capacity = cap;
        }
        return v;
    }
    
    // Constructor with initial capacity (C++ style)
    explicit Vec(size_t initial_capacity) 
        : data(nullptr), length(0), capacity(0) {
        if (initial_capacity > 0) {
            data = static_cast<T*>(::operator new(initial_capacity * sizeof(T)));
            capacity = initial_capacity;
        }
    }
    
    // Initializer list constructor
    Vec(std::initializer_list<T> init) 
        : data(nullptr), length(0), capacity(0) {
        reserve(init.size());
        for (const T& item : init) {
            push(item);
        }
    }
    
    // No copy constructor - Vec cannot be copied
    Vec(const Vec&) = delete;
    Vec& operator=(const Vec&) = delete;
    
    // Move constructor
    Vec(Vec&& other) noexcept 
        : data(other.data), length(other.length), capacity(other.capacity) {
        other.data = nullptr;
        other.length = 0;
        other.capacity = 0;
    }
    
    // Move assignment
    Vec& operator=(Vec&& other) noexcept {
        if (this != &other) {
            // Clean up existing data
            clear();
            ::operator delete(data);
            
            // Take ownership
            data = other.data;
            length = other.length;
            capacity = other.capacity;
            
            other.data = nullptr;
            other.length = 0;
            other.capacity = 0;
        }
        return *this;
    }
    
    // Destructor
    ~Vec() {
        clear();
        ::operator delete(data);
    }
    
    // Push element to the back
    void push(T value) {
        if (length >= capacity) {
            grow();
        }
        new (&data[length]) T(std::move(value));
        ++length;
    }
    
    // Pop element from the back
    // Returns empty Option-like type if vec is empty
    T pop() {
        assert(length > 0);
        --length;
        T result = std::move(data[length]);
        data[length].~T();
        return result;
    }
    
    // Access element by index
    // @lifetime: (&'a) -> &'a
    T& operator[](size_t index) {
        assert(index < length);
        return data[index];
    }
    
    // @lifetime: (&'a) -> &'a
    const T& operator[](size_t index) const {
        assert(index < length);
        return data[index];
    }
    
    // Get first element
    // @lifetime: (&'a) -> &'a
    T& front() {
        assert(length > 0);
        return data[0];
    }
    
    // @lifetime: (&'a) -> &'a
    const T& front() const {
        assert(length > 0);
        return data[0];
    }
    
    // Get last element
    // @lifetime: (&'a) -> &'a
    T& back() {
        assert(length > 0);
        return data[length - 1];
    }
    
    // @lifetime: (&'a) -> &'a
    const T& back() const {
        assert(length > 0);
        return data[length - 1];
    }
    
    // Get size
    size_t len() const { return length; }
    size_t size() const { return length; }
    
    // Check if empty
    bool is_empty() const { return length == 0; }
    
    // Get capacity
    size_t cap() const { return capacity; }
    
    // Reserve capacity
    void reserve(size_t new_capacity) {
        if (new_capacity > capacity) {
            T* new_data = static_cast<T*>(::operator new(new_capacity * sizeof(T)));
            
            // Move existing elements
            for (size_t i = 0; i < length; ++i) {
                new (&new_data[i]) T(std::move(data[i]));
                data[i].~T();
            }
            
            ::operator delete(data);
            data = new_data;
            capacity = new_capacity;
        }
    }
    
    // Clear all elements
    void clear() {
        for (size_t i = 0; i < length; ++i) {
            data[i].~T();
        }
        length = 0;
    }
    
    // Iterator support
    // @lifetime: (&'a) -> &'a
    T* begin() { return data; }
    const T* begin() const { return data; }
    
    // @lifetime: (&'a) -> &'a
    T* end() { return data + length; }
    const T* end() const { return data + length; }
    
    // Clone the Vec (explicit deep copy)
    Vec clone() const {
        Vec result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            result.push(data[i]);  // Requires T to be copyable
        }
        return result;
    }
};

// Helper function to create a Vec
template<typename T>
// @lifetime: owned
Vec<T> vec_of(std::initializer_list<T> init) {
    return Vec<T>(init);
}

} // namespace rusty

#endif // RUSTY_VEC_HPP