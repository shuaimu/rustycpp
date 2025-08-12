// Test file for reference borrow checking

void test_const_reference() {
    int value = 42;
    const int& const_ref = value;  // Immutable borrow
    const int& another_const = value;  // Multiple immutable borrows OK
    
    // This would be an error if we could detect it:
    // const_ref = 10;  // ERROR: Cannot modify through const reference
}

void test_mutable_reference() {
    int value = 42;
    int& mut_ref = value;  // Mutable borrow
    
    // This would be an error:
    // const int& const_ref = value;  // ERROR: Cannot have immutable borrow while mutable exists
    // int& another_mut = value;  // ERROR: Cannot have multiple mutable borrows
    
    mut_ref = 100;  // OK: Can modify through mutable reference
}

void test_reference_aliasing() {
    int x = 10;
    int y = 20;
    
    const int& ref1 = x;
    const int& ref2 = x;  // Multiple const refs OK
    const int& ref3 = y;  // Ref to different value OK
}

// These would be errors but nested functions aren't allowed in C++
// void test_dangling_reference() {
//     int* get_local() {
//         int local = 42;
//         return &local;  // ERROR: Returning address of local
//     }
//     
//     int& get_ref() {
//         int local = 42;
//         return local;  // ERROR: Returning reference to local
//     }
// }

int* get_local() {
    int local = 42;
    return &local;  // ERROR: Returning address of local
}

int& get_ref() {
    int local = 42;
    return local;  // ERROR: Returning reference to local
}

void test_move_semantics() {
    // Simplified example without std::unique_ptr
    int* ptr1 = new int(42);
    int* ptr2 = ptr1;  // This is a copy in C++, not a move
    
    // Both pointers are valid in C++, but we might want to track this
    delete ptr1;
    // *ptr2 = 10;  // ERROR: Use after free (but hard to detect without ownership)
}

int main() {
    test_const_reference();
    test_mutable_reference();
    test_reference_aliasing();
    test_move_semantics();
    
    return 0;
}