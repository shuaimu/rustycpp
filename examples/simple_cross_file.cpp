#include "simple_header.h"

void test_with_header() {
    int value = 42;
    
    // Multiple immutable borrows - OK
    const int& ref1 = value;
    const int& ref2 = value;
    
    // This would be an error - can't have mutable while immutable exists
    // int& mut_ref = value;
}

void test_mutable_conflicts() {
    int data = 100;
    
    // Mutable borrow
    int& mut_ref = data;
    
    // This is an error - can't have immutable while mutable exists
    const int& const_ref = data;
}

int main() {
    test_with_header();
    test_mutable_conflicts();
    return 0;
}