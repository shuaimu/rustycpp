// Examples showing what works vs what doesn't work with control flow

namespace std {
    template<typename T>
    T&& move(T& t) { return static_cast<T&&>(t); }
}

// ============================================================================
// WHAT WORKS: Simple, linear flow
// ============================================================================

namespace Working {
    
    // ✅ WORKS: Simple borrow checking
    void simple_borrow() {
        int value = 42;
        int& ref = value;
        // int& ref2 = value;  // This WOULD be caught as error
        ref = 100;
    }
    
    // ✅ WORKS: Move detection
    void simple_move() {
        int x = 42;
        int y = std::move(x);
        // int z = x;  // This WOULD be caught as use-after-move
    }
    
    // ✅ WORKS: Sequential operations
    void sequential() {
        int value = 42;
        int& ref1 = value;
        ref1 = 100;
        // Can't create ref2 while ref1 exists - this would be caught
    }
    
    // ✅ WORKS: Function calls with moves
    void take_ownership(int x) {}
    
    void function_call_move() {
        int x = 42;
        take_ownership(std::move(x));
        // int y = x;  // This WOULD be caught as use-after-move
    }
}

// ============================================================================
// WHAT DOESN'T WORK: Complex control flow
// ============================================================================

namespace Broken {
    
    // Forward declarations
    int get_mode();
    bool should_exit();
    
    // ❌ BROKEN: Loop iterations
    void loop_iterations() {
        int x = 42;
        
        // BUG: This moves x twice but checker doesn't catch it
        for (int i = 0; i < 2; i++) {
            int y = std::move(x);  // Second iteration is use-after-move!
            // Checker treats loop body as executing once
        }
    }
    
    // ❌ BROKEN: Conditional moves
    void conditional_move() {
        int x = 42;
        bool runtime_value = true;  // Checker doesn't know this value
        
        if (runtime_value) {
            int y = std::move(x);
        }
        
        // BUG or FALSE POSITIVE: Depends on runtime_value
        int z = x;  // Checker either always errors or never errors
        // Can't handle "maybe moved" state
    }
    
    // ❌ BROKEN: Scope isolation
    void scope_isolation() {
        int value = 42;
        
        {
            int& ref1 = value;
            ref1 = 100;
        }  // ref1 should be destroyed here
        
        {
            int& ref2 = value;  // FALSE POSITIVE: Checker might think ref1 still exists
            ref2 = 200;
        }
    }
    
    // ❌ BROKEN: Mutually exclusive branches
    void exclusive_branches() {
        int value = 42;
        int mode = get_mode();  // Runtime value
        
        if (mode == 1) {
            int& ref1 = value;
            ref1 = 100;
        }
        
        if (mode == 2) {
            int& ref2 = value;  // FALSE POSITIVE: Can't both be true
            ref2 = 200;
        }
        
        // Checker doesn't understand these are exclusive
    }
    
    // ❌ BROKEN: Early return
    void early_return() {
        int value = 42;
        int& ref1 = value;
        
        if (should_exit()) {
            return;  // ref1's lifetime ends here
        }
        
        // FALSE POSITIVE: This is only reached if ref1 is gone
        int& ref2 = value;
        ref2 = 200;
    }
    
    // ❌ BROKEN: Loop with break
    void loop_with_break() {
        int value = 42;
        
        for (int i = 0; i < 10; i++) {
            int& ref = value;
            if (i == 0) {
                break;  // Exit early - ref lifetime ends
            }
            ref = i;  // Never executed but checker doesn't know
        }
        
        // Should be OK but checker might see the borrow
        int& ref2 = value;
    }
    
    // ❌ NOT PARSED: Switch statements
    void switch_statement() {
        int value = 42;
        int choice = 1;
        
        switch (choice) {  // Entire switch might not be parsed
            case 1: {
                int& ref1 = value;
                ref1 = 100;
                break;
            }
            case 2: {
                int& ref2 = value;
                ref2 = 200;
                break;
            }
        }
    }
    
    // ❌ NOT PARSED: Try-catch
    void exception_handling() {
        int value = 42;
        
        try {
            int& ref1 = value;
            throw 1;  // ref1 destroyed by stack unwinding
        } catch (...) {
            int& ref2 = value;  // Should be OK
        }
    }
    
    // Helper functions
    int get_mode() { return 1; }
    bool should_exit() { return false; }
}

// ============================================================================
// COMPARISON: Same logic, different results
// ============================================================================

namespace Comparison {
    
    // This pattern WORKS (no control flow)
    void works_no_control_flow() {
        int x = 42;
        int y = std::move(x);
        // int z = x;  // ERROR: Correctly caught
    }
    
    // Same pattern DOESN'T WORK (with control flow)
    void broken_with_control_flow() {
        int x = 42;
        
        if (true) {  // Always true, but checker doesn't evaluate
            int y = std::move(x);
        }
        
        int z = x;  // ERROR: Not caught because it's after control flow
    }
    
    // Manual scope DOESN'T WORK
    void broken_manual_scope() {
        int value = 42;
        
        {  // Explicit scope
            int& ref1 = value;
        }  // ref1 destroyed
        
        int& ref2 = value;  // FALSE POSITIVE: Might be reported as error
    }
}

int main() {
    // The Working examples would catch errors correctly
    // The Broken examples have false positives or false negatives
    return 0;
}