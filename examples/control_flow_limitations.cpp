// Examples demonstrating control flow limitations in the borrow checker
// These show cases where the checker either gives false positives or misses real issues

// ============================================================================
// 1. LOOP LIMITATIONS - Loops are not properly analyzed
// ============================================================================

void loop_false_positive() {
    int value = 42;
    int* ptr = nullptr;
    
    // The checker doesn't understand loop iterations
    for (int i = 0; i < 3; i++) {
        if (i == 0) {
            ptr = &value;  // First iteration: take reference
        } else {
            ptr = nullptr;  // Later iterations: clear reference
        }
    }
    // ptr is nullptr here, but checker might think it still borrows value
    
    int& ref = value;  // Should be OK, but checker might complain about double borrow
}

void loop_missed_error() {
    int values[3] = {1, 2, 3};
    int* ptr1 = nullptr;
    int* ptr2 = nullptr;
    
    // The checker doesn't track iterations
    for (int i = 0; i < 3; i++) {
        ptr1 = &values[i];
        ptr2 = &values[i];  // ERROR: Should detect double mutable borrow in same iteration
        // But checker treats loop body as single block
    }
}

void while_loop_move() {
    struct Resource {
        int data;
    };
    
    Resource r;
    bool condition = true;
    
    while (condition) {
        Resource r2 = std::move(r);  // ERROR: Moved in first iteration
        // Second iteration would use after move, but checker doesn't track this
        condition = false;  // Prevents actual runtime error
    }
}

// ============================================================================
// 2. CONDITIONAL FLOW LIMITATIONS - Path-sensitive analysis missing
// ============================================================================

void conditional_false_positive() {
    int value = 42;
    bool condition = true;
    
    int* ptr = nullptr;
    if (condition) {
        ptr = &value;  // Borrow happens only if condition is true
    }
    
    if (!condition) {
        int& ref = value;  // This is actually safe (mutually exclusive paths)
        // But checker sees both borrows in same function scope
    }
}

void conditional_missed_error() {
    int value = 42;
    int* ptr1 = nullptr;
    int* ptr2 = nullptr;
    
    bool cond1 = true;
    bool cond2 = true;
    
    if (cond1) {
        ptr1 = &value;
    }
    
    if (cond2) {
        ptr2 = &value;  // Could be double borrow if both conditions true
    }
    
    if (cond1 && cond2) {
        // Both pointers are active here - should detect the conflict
        *ptr1 = 100;
        *ptr2 = 200;  // ERROR: Both mutable borrows active
    }
}

void move_in_branch() {
    struct Resource {
        int data;
    };
    
    Resource r;
    bool condition = get_condition();
    
    if (condition) {
        Resource r2 = std::move(r);  // Moved only in this branch
    }
    
    // r might or might not be moved here
    r.data = 42;  // ERROR if condition was true, OK otherwise
    // Checker either always errors or never errors, can't handle uncertainty
}

// ============================================================================
// 3. SWITCH STATEMENT - Not parsed at all
// ============================================================================

void switch_not_supported() {
    int value = 42;
    int choice = 1;
    
    int* ptr = nullptr;
    
    switch (choice) {
        case 1:
            ptr = &value;
            break;
        case 2:
            // No borrow here
            break;
        default:
            ptr = &value;
            break;
    }
    
    // Checker likely doesn't parse switch statements at all
    int& ref = value;  // Might or might not conflict
}

// ============================================================================
// 4. EARLY RETURNS - Lifetime not properly shortened
// ============================================================================

void early_return_false_positive() {
    int value = 42;
    int* ptr = &value;
    
    if (some_condition()) {
        return;  // ptr's lifetime ends here
    }
    
    // ptr is no longer active here
    int& ref = value;  // Should be OK, but checker sees both in function scope
}

// ============================================================================
// 5. GOTO AND LABELS - Completely unsupported
// ============================================================================

void goto_example() {
    int value = 42;
    int* ptr = nullptr;
    
    goto skip;
    ptr = &value;  // This is skipped
    
skip:
    int& ref = value;  // Should be OK, but checker might not understand goto
}

// ============================================================================
// 6. EXCEPTION HANDLING - Try/catch not analyzed
// ============================================================================

void exception_handling() {
    int value = 42;
    
    try {
        int* ptr = &value;
        throw std::runtime_error("error");
        // ptr's lifetime ends here due to exception
    } catch (...) {
        int& ref = value;  // Should be OK, but checker doesn't understand exceptions
    }
}

// ============================================================================
// 7. LOOP BREAK/CONTINUE - Control flow not tracked
// ============================================================================

void loop_with_break() {
    int values[5] = {1, 2, 3, 4, 5};
    
    for (int i = 0; i < 5; i++) {
        int* ptr = &values[i];
        
        if (i == 2) {
            break;  // Early exit - ptr lifetime ends
        }
        
        if (i == 1) {
            continue;  // Skip rest - ptr lifetime ends differently
        }
        
        // Complex lifetime depending on control flow
        *ptr = i * 2;
    }
}

// ============================================================================
// 8. NESTED LOOPS - Scope confusion
// ============================================================================

void nested_loops() {
    int matrix[3][3];
    
    for (int i = 0; i < 3; i++) {
        int* row_ptr = &matrix[i][0];
        
        for (int j = 0; j < 3; j++) {
            int* elem_ptr = &matrix[i][j];
            // Inner scope borrowing - checker might not track nesting properly
        }
        
        // elem_ptr should be out of scope here
        *row_ptr = i;  // Should be OK
    }
}

// ============================================================================
// 9. DO-WHILE LOOPS - Often not parsed correctly
// ============================================================================

void do_while_example() {
    int value = 42;
    int* ptr = nullptr;
    int counter = 0;
    
    do {
        if (counter == 0) {
            ptr = &value;
        } else {
            ptr = nullptr;
        }
        counter++;
    } while (counter < 2);
    
    // ptr state depends on loop analysis
    int& ref = value;  // Might false positive
}

// ============================================================================
// 10. CONDITIONAL OPERATOR (ternary) - Treated as single expression
// ============================================================================

void ternary_operator() {
    int value1 = 42;
    int value2 = 100;
    bool condition = true;
    
    // Borrow happens in only one branch of ternary
    int* ptr = condition ? &value1 : &value2;
    
    // This should be OK (different values borrowed)
    int& ref1 = value1;  // Might false positive if condition is false
    int& ref2 = value2;  // Might false positive if condition is true
}

// ============================================================================
// Helper functions (not important for examples)
// ============================================================================

bool some_condition() { return true; }
bool get_condition() { return true; }

int main() {
    // These examples demonstrate limitations, not actual runnable code
    return 0;
}