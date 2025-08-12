// Demonstration of Rust-like borrow checking for C++ references

// This function shows multiple const references are allowed
void valid_const_refs() {
    int value = 42;
    const int& ref1 = value;  // First immutable borrow - OK
    const int& ref2 = value;  // Second immutable borrow - OK
    const int& ref3 = value;  // Third immutable borrow - OK
    
    // All three references can be used simultaneously
    int sum = ref1 + ref2 + ref3;  // OK - reading through const refs
}

// This function would violate Rust's borrow rules
void invalid_mutable_refs() {
    int value = 42;
    int& mut_ref1 = value;     // First mutable borrow - OK
    int& mut_ref2 = value;     // Second mutable borrow - ERROR!
    
    // This would lead to aliasing issues
    mut_ref1 = 10;
    mut_ref2 = 20;  // Which value does 'value' have?
}

// This function shows the conflict between mutable and immutable refs
void invalid_mixed_refs() {
    int value = 42;
    const int& const_ref = value;  // Immutable borrow - OK
    int& mut_ref = value;          // Mutable borrow - ERROR! Already immutably borrowed
    
    // This would violate the guarantee that const_ref won't see changes
    mut_ref = 100;
}

// Another violation: mutable first, then const
void invalid_mixed_refs2() {
    int value = 42;
    int& mut_ref = value;          // Mutable borrow - OK
    const int& const_ref = value;  // Immutable borrow - ERROR! Already mutably borrowed
}

// Valid pattern: scoped borrows (not yet tracked by our analyzer)
void valid_scoped_borrows() {
    int value = 42;
    
    {
        const int& ref1 = value;  // Immutable borrow in inner scope
        int x = ref1;  // Use the reference
    }  // ref1 goes out of scope
    
    int& mut_ref = value;  // Mutable borrow - Would be OK if we tracked scopes
    mut_ref = 100;
}

int main() {
    valid_const_refs();
    invalid_mutable_refs();
    invalid_mixed_refs();
    invalid_mixed_refs2();
    valid_scoped_borrows();
    
    return 0;
}