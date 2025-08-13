// Test quoted include (searches relative to source first)
#include "local_header.h"

// Test angle bracket include (searches -I paths)
#include <math_utils.h>

void test_includes() {
    int a = 10;
    int b = 20;
    
    // Test function from math_utils.h (found via -I)
    const int& max_val = max(a, b);
    
    // Test function from local_header.h (found relative to source)
    const int& passed = passThrough(a);
    
    // Test borrow checking
    const int& ref1 = a;
    const int& ref2 = a;  // OK - multiple immutable borrows
    
    // This would be an error
    int& mut_ref = a;  // ERROR: can't have mutable while immutable exists
}

int main() {
    test_includes();
    return 0;
}