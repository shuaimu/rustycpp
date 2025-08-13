// Test for dangling reference detection

int& create_dangling_ref() {
    int local = 42;
    return local;  // ERROR: returning reference to local variable
}

const int& another_dangling() {
    int temp = 100;
    const int& ref = temp;
    return ref;  // ERROR: ref points to local 'temp'
}

int* create_dangling_ptr() {
    int local = 42;
    return &local;  // ERROR: returning address of local variable
}

// This should be OK - returning reference to static
const int& get_static_ref() {
    static int static_val = 42;
    return static_val;
}

// Test scope-based lifetime issues
void test_scope_lifetimes() {
    int* ptr = nullptr;
    
    {  // Inner scope
        int scoped_val = 100;
        ptr = &scoped_val;  // Taking address of scoped variable
    }  // scoped_val dies here
    
    // ERROR: Using ptr here would access destroyed variable
    // *ptr = 200;
}

int main() {
    // These calls would all create dangling references
    // int& bad1 = create_dangling_ref();
    // const int& bad2 = another_dangling();
    // int* bad3 = create_dangling_ptr();
    
    // This is OK
    const int& good = get_static_ref();
    
    test_scope_lifetimes();
    
    return 0;
}