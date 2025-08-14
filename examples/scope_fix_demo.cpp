// Demonstration of scope tracking fix
// This file should have NO errors with scope tracking

void before_fix_false_positive() {
    int value = 42;
    
    // These are in separate scopes and should NOT conflict
    {
        int& ref1 = value;
        ref1 = 100;
    }  // ref1 destroyed here
    
    {
        int& ref2 = value;  // Should be OK!
        ref2 = 200;
    }  // ref2 destroyed here
}

void multiple_const_refs_in_scopes() {
    int value = 42;
    
    {
        const int& cref1 = value;
        {
            const int& cref2 = value;  // Nested, both const - OK
            {
                const int& cref3 = value;  // Triple nested - OK
                int sum = cref1 + cref2 + cref3;
            }
        }
    }
    
    // All const refs gone, can take mutable
    int& mref = value;
    mref = 100;
}

void sequential_scopes() {
    int data = 42;
    
    // Ten sequential scopes - should all be OK
    { int& r = data; r = 1; }
    { int& r = data; r = 2; }
    { int& r = data; r = 3; }
    { int& r = data; r = 4; }
    { int& r = data; r = 5; }
    { int& r = data; r = 6; }
    { int& r = data; r = 7; }
    { int& r = data; r = 8; }
    { int& r = data; r = 9; }
    { int& r = data; r = 10; }
}

// This function SHOULD have an error (for comparison)
void actual_error() {
    int value = 42;
    int& ref1 = value;
    int& ref2 = value;  // ERROR: Double mutable borrow
    ref1 = 100;
    ref2 = 200;
}

int main() {
    before_fix_false_positive();
    multiple_const_refs_in_scopes();
    sequential_scopes();
    actual_error();
    return 0;
}