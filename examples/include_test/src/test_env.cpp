// Local header - found relative to source
#include "local_header.h"

// These should be found via environment variables
#include <math_utils.h>
#include <utils.h>

void test_env_includes() {
    int x = 100;
    
    // From math_utils.h (first env path)
    int& ref = increment(x);
    
    // From utils.h (second env path)
    const char* msg = "hello";
    const char* echoed = echo(msg);
    
    // Test borrow checking
    const int& const_ref = x;
    int& mut_ref = x;  // ERROR: x already has immutable borrow
}

int main() {
    test_env_includes();
    return 0;
}