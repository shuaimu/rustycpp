// Test each control flow issue separately

namespace std {
    template<typename T>
    T&& move(T& t) { return static_cast<T&&>(t); }
}

// Test 1: Double mutable borrow (SHOULD ERROR)
void test1_double_borrow() {
    int value = 42;
    int& ref1 = value;
    int& ref2 = value;  // ERROR expected
}

// Test 2: Loop with move (SHOULD ERROR on 2nd iteration)
void test2_loop_move() {
    int x = 42;
    for (int i = 0; i < 2; i++) {
        int y = std::move(x);  // ERROR on second iteration
    }
}

// Test 3: Move in always-true condition (SHOULD ERROR on use)
void test3_conditional_move() {
    int x = 42;
    if (true) {
        int y = std::move(x);
    }
    int z = x;  // ERROR: use after move
}

// Test 4: Nested scopes (SHOULD BE OK)
void test4_nested_scopes() {
    int value = 42;
    {
        int& ref1 = value;
    }
    {
        int& ref2 = value;  // Should be OK
    }
}

// Test 5: Move in maybe condition (AMBIGUOUS)
void test5_maybe_move() {
    int x = 42;
    bool unknown = true;  // Checker doesn't know the value
    if (unknown) {
        int y = std::move(x);
    }
    int z = x;  // Might or might not be moved
}