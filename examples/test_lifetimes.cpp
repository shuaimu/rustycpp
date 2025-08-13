#include "lifetime_test.h"

void test_lifetime_violations() {
    int local = 42;
    
    // This should be OK - returning owned value
    int owned = getValue();
    
    // This should be OK - lifetime 'a preserved
    const int& ref1 = getRef();
    const int& ref2 = identity(ref1);
    
    // This would violate lifetime constraint 'a: 'b
    // selectFirst requires first argument to outlive second
    const int& short_lived = local;
    const int& long_lived = getRef();
    // const int& bad = selectFirst(short_lived, long_lived); // ERROR: lifetime violation
    
    // Returning reference to local - should be caught
    const int& dangling = returnLocal(); // This should error
}

// Function that returns reference to local variable
const int& returnLocal() {
    int local = 100;
    return local; // ERROR: returning reference to local
}

int main() {
    test_lifetime_violations();
    return 0;
}