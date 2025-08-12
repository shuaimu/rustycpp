// Simple test for reference borrow checking

void test_multiple_const_refs() {
    int value = 42;
    const int& ref1 = value;  // First immutable borrow
    const int& ref2 = value;  // Second immutable borrow - should be OK
}

void test_mutable_and_const_conflict() {
    int value = 42;
    int& mut_ref = value;      // Mutable borrow
    const int& const_ref = value;  // Immutable borrow while mutable exists - ERROR
}

void test_multiple_mutable_refs() {
    int value = 42;
    int& mut_ref1 = value;     // First mutable borrow
    int& mut_ref2 = value;     // Second mutable borrow - ERROR
}

int main() {
    test_multiple_const_refs();
    test_mutable_and_const_conflict();
    test_multiple_mutable_refs();
    return 0;
}