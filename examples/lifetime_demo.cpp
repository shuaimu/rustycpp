#include "lifetime_demo.h"

void test_lifetime_rules() {
    int local = 42;
    
    // Test 1: Function returning owned value - OK
    int owned = getValue();
    
    // Test 2: Function returning reference - OK
    const int& ref = getStaticRef();
    
    // Test 3: Identity function preserves lifetime - OK
    const int& ref2 = identity(ref);
    
    // Test 4: Function that transfers ownership - OK
    int* ptr = createInt();
    delete ptr;
    
    // Test 5: Multiple borrows - should error
    int value = 100;
    const int& r1 = value;  // Immutable borrow
    const int& r2 = value;  // Another immutable - OK
    int& mut_ref = value;   // ERROR: Can't have mutable while immutable exists
}

int main() {
    test_lifetime_rules();
    return 0;
}