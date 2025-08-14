# Control Flow Limitations in the C++ Borrow Checker

This document explains what control flow constructs are not properly handled by the current implementation and provides examples of false positives and false negatives.

## Summary of Limitations

The borrow checker currently treats all statements in a function as if they execute in a single linear flow, without understanding:
- Loop iterations
- Conditional branches
- Scope boundaries (blocks)
- Early returns
- Exception handling

## 1. Loop Iterations Not Tracked

### Problem
The checker doesn't understand that loops execute multiple times. It treats the loop body as if it executes once.

### Example - False Negative
```cpp
void loop_move_bug() {
    int x = 42;
    for (int i = 0; i < 2; i++) {
        int y = std::move(x);  // ERROR: Second iteration uses moved value!
    }
}
```
**Expected:** Error on second iteration (use-after-move)
**Actual:** No error detected

### Example - False Positive
```cpp
void loop_borrow_reset() {
    int value = 42;
    int* ptr = nullptr;
    
    for (int i = 0; i < 3; i++) {
        if (i == 0) {
            ptr = &value;  // Borrow on first iteration
        } else {
            ptr = nullptr;  // Clear on other iterations
        }
    }
    // ptr is nullptr here
    int& ref = value;  // Should be OK
}
```
**Expected:** No error (ptr is nullptr after loop)
**Actual:** Might report double borrow

## 2. Conditional Branches (Path Sensitivity)

### Problem
The checker doesn't understand that if/else branches are mutually exclusive.

### Example - False Positive
```cpp
void exclusive_branches() {
    int value = 42;
    bool condition = get_runtime_value();
    
    if (condition) {
        int& ref1 = value;
        use(ref1);
    } else {
        int& ref2 = value;  // Different branch - should be OK
        use(ref2);
    }
}
```
**Expected:** No error (branches are mutually exclusive)
**Actual:** Might report double borrow

### Example - False Negative
```cpp
void maybe_moved() {
    int x = 42;
    if (runtime_condition()) {
        int y = std::move(x);  // Might move
    }
    int z = x;  // Might be use-after-move
}
```
**Expected:** Warning about potential use-after-move
**Actual:** Either always errors or never errors

## 3. Scope Boundaries Not Tracked

### Problem
The checker doesn't properly track when references go out of scope in blocks.

### Example - False Positive
```cpp
void nested_scopes() {
    int value = 42;
    
    {
        int& ref1 = value;
        ref1 = 100;
    }  // ref1 out of scope
    
    {
        int& ref2 = value;  // Should be OK
        ref2 = 200;
    }
}
```
**Expected:** No error (refs in different scopes)
**Actual:** Might report double borrow

## 4. Control Flow Statements Not Parsed

### Not Supported At All
- `switch` statements
- `goto` and labels
- `do-while` loops (might be partially supported)
- Exception handling (`try`/`catch`/`throw`)

### Example
```cpp
void switch_example() {
    int value = 42;
    int choice = get_choice();
    
    switch (choice) {
        case 1:
            int& ref1 = value;
            break;
        case 2:
            int& ref2 = value;  // Different case
            break;
    }
}
```
**Result:** Switch statement likely not parsed at all

## 5. Early Returns and Break/Continue

### Problem
The checker doesn't understand that early returns end a function's execution flow.

### Example - False Positive
```cpp
void early_return() {
    int value = 42;
    int& ref1 = value;
    
    if (some_condition()) {
        return;  // ref1's lifetime ends here
    }
    
    int& ref2 = value;  // Should be OK if condition was true
}
```
**Expected:** Understands that paths don't overlap
**Actual:** Sees both borrows in same function

## 6. What Actually Works

Despite these limitations, the checker DOES correctly handle:
- Basic sequential flow within a function
- Simple variable declarations and assignments
- Direct borrows and moves (without complex control flow)
- Basic function calls
- std::move detection in simple cases

## 7. Real Example Test Results

Testing `test_each_control_flow.cpp`:
- ✅ **Correctly detects:** Double mutable borrow in straight-line code
- ❌ **Misses:** Move in loop (second iteration use-after-move)
- ❌ **Misses:** Use-after-move when move is in an if(true) block
- ❌ **False positive:** Reports error for references in separate scopes

## Implications for Users

When using this borrow checker, be aware that:

1. **Loops:** The checker won't catch bugs that occur in second or later iterations
2. **Conditionals:** You may get false positives for mutually exclusive branches
3. **Scopes:** Block scopes don't properly isolate borrows
4. **Complex flow:** switch, goto, exceptions are not analyzed at all

## Workarounds

To get the most accurate results:
1. Test loop bodies as separate functions
2. Avoid complex conditional borrowing patterns
3. Use explicit scope blocks sparingly
4. Keep functions simple and linear when possible

## Future Improvements Needed

To properly handle control flow, the checker needs:
1. **Control Flow Graph (CFG) construction** that understands all C++ control structures
2. **Path-sensitive analysis** to track different execution paths
3. **Loop analysis** to understand iteration and loop invariants
4. **Scope tracking** to know when references go out of scope
5. **Exception flow analysis** for try/catch blocks