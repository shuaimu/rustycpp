#include "transitive.h"

// Test transitive lifetime relationships
// If 'a: 'b and 'b: 'c, then 'a: 'c

void test_transitive_lifetimes() {
    int long_lived = 42;
    int medium_lived = 100;
    int short_lived = 200;
    
    // Create references with different lifetimes
    const int& ref_long = long_lived;    // lifetime 'a
    const int& ref_medium = medium_lived; // lifetime 'b where 'a: 'b
    const int& ref_short = short_lived;   // lifetime 'c where 'b: 'c
    
    // Test function that requires 'a: 'b
    const int& result1 = requires_outlives(ref_long, ref_medium); // OK
    
    // Test function that requires 'a: 'b  
    // const int& result2 = requires_outlives(ref_medium, ref_long); // ERROR: 'b doesn't outlive 'a
    
    // Test transitive: needs 'a: 'c (satisfied via 'a: 'b: 'c)
    const int& result3 = requires_transitive(ref_long, ref_medium, ref_short); // OK if transitive
}

// Test lifetime inference
void test_inference() {
    int x = 10;
    
    // Lifetime of ref1 starts here
    int& ref1 = x;
    
    // Some operations
    ref1 += 5;
    
    {
        // Inner scope - ref2 has shorter lifetime
        int& ref2 = ref1;
        ref2 += 10;
    } // ref2 lifetime ends here
    
    // ref1 is still valid
    ref1 += 20;
    
} // ref1 lifetime ends here

int main() {
    test_transitive_lifetimes();
    test_inference();
    return 0;
}