// Simple example demonstrating const reference borrowing rules
// This shows how Rust's borrow checker rules apply to C++ const references

#include <iostream>

// @safe
void demonstrate_const_ref_borrowing() {
    int value = 42;
    
    // Multiple const references are allowed (immutable borrows)
    const int& ref1 = value;  // First immutable borrow - OK
    const int& ref2 = value;  // Second immutable borrow - OK
    const int& ref3 = value;  // Third immutable borrow - OK
    
    // All can be used simultaneously to read the value
    std::cout << "ref1: " << ref1 << std::endl;  // OK
    std::cout << "ref2: " << ref2 << std::endl;  // OK
    std::cout << "ref3: " << ref3 << std::endl;  // OK
    
    int sum = ref1 + ref2 + ref3;  // OK - reading through const refs
    std::cout << "Sum: " << sum << std::endl;
}

// @safe  
void demonstrate_const_ref_violation() {
    int value = 42;
    
    const int& const_ref = value;  // Immutable borrow - OK
    int& mut_ref = value;          // ERROR: Cannot have mutable borrow when immutable exists
    
    // This would violate the guarantee that const_ref won't see unexpected changes
    mut_ref = 100;  // If allowed, const_ref would suddenly see value 100
}

int main() {
    demonstrate_const_ref_borrowing();
    demonstrate_const_ref_violation();
    return 0;
}
