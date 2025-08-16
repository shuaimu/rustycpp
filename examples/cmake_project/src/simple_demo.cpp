// Simple demo without standard library headers
// This demonstrates the borrow checker with CMake

extern "C" int printf(const char*, ...);

// @safe
namespace demo {
    
    // @safe
    void test_borrow_checking() {
        int value = 42;
        const int& ref1 = value;  // First immutable borrow
        const int& ref2 = value;  // Second immutable borrow - OK
        
        // This would error if uncommented:
        // int& mut_ref = value;  // Can't have mutable with immutable
        
        printf("Values: %d %d\n", ref1, ref2);
    }
    
    // @safe
    void test_move_semantics() {
        int* ptr1 = new int(42);
        int* ptr2 = ptr1;  // Copy pointer
        ptr1 = nullptr;    // "Move" by nulling
        
        // Raw pointer operations should error in safe code:
        int val = *ptr2;  // Error: dereference requires unsafe
        
        delete ptr2;
    }
    
    // Unmarked function (unsafe by default)
    void unsafe_operation(int* ptr) {
        *ptr = 100;  // OK in unsafe code
    }
    
    // @safe
    void test_unsafe_propagation() {
        int x = 10;
        // This should error - calling unsafe function from safe
        unsafe_operation(&x);  // Error: requires unsafe context
    }
}

int main() {
    demo::test_borrow_checking();
    demo::test_move_semantics();
    demo::test_unsafe_propagation();
    return 0;
}