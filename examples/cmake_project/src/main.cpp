#include "safe_string.h"
#include <iostream>
#include <memory>

extern "C" int printf(const char*, ...);

// @safe
void demonstrate_borrow_checking() {
    int value = 42;
    const int& ref1 = value;  // Immutable borrow
    const int& ref2 = value;  // Multiple immutable borrows OK
    
    // int& mut_ref = value;  // Would error: can't have mutable with immutable
    
    printf("Values: %d %d\n", ref1, ref2);
}

// @safe  
void demonstrate_move_semantics() {
    auto ptr1 = std::make_unique<int>(42);
    auto ptr2 = std::move(ptr1);  // Move ownership
    
    // This would be caught as use-after-move:
    // int val = *ptr1;  // Error: use after move
    
    int val2 = *ptr2;  // OK: ptr2 owns the value
    printf("Value: %d\n", val2);
}

// @safe
void demonstrate_safe_string() {
    safe::SafeString str1("Hello");
    const std::string& ref = str1.get();  // Borrow
    
    printf("String: %s\n", ref.c_str());
    
    // Transfer ownership
    safe::SafeString str2(std::move(str1));
    
    // This would error:
    // const std::string& bad_ref = str1.get();  // Use after move
}

int main() {
    demonstrate_borrow_checking();
    demonstrate_move_semantics();
    demonstrate_safe_string();
    
    return 0;
}