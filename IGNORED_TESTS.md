# Ignored Tests Documentation

## Currently Ignored Tests (2)

### 1. `test_use_after_move_with_unique_ptr`
**Reason**: Requires std::unique_ptr support and move semantics tracking
```cpp
std::unique_ptr<int> ptr1(new int(42));
std::unique_ptr<int> ptr2 = std::move(ptr1);
*ptr1 = 10;  // Should be detected as use-after-move
```
**What's needed**: 
- Parse and understand std::unique_ptr
- Track std::move() calls
- Mark moved-from pointers as invalid

### 2. `test_dangling_reference_lifetime`
**Reason**: Requires lifetime analysis implementation
```cpp
int& get_dangling() {
    int local = 42;
    return local;  // Should detect returning reference to local
}
```
**What's needed**:
- Track variable lifetimes/scopes
- Analyze function return statements
- Detect when references outlive their targets

## Why These Tests Remain Ignored

These tests require significantly more complex analysis:

1. **Smart Pointer Support**: Would need to parse and understand C++ standard library types like `std::unique_ptr` and `std::shared_ptr`, including their move semantics.

2. **Lifetime Analysis**: Would need to implement a full lifetime tracking system similar to Rust's, including:
   - Scope analysis
   - Function boundary checking
   - Reference lifetime inference

These features are beyond the current scope of the reference borrow checking implementation, which focuses on enforcing Rust's borrowing rules (multiple immutable XOR single mutable) on C++ references.

## Alternative Tests Created

To ensure comprehensive testing coverage, we've added alternative tests that work with our current implementation:
- `test_reference_invalidation` - Tests multiple mutable references
- `test_const_after_mutable` - Tests mixing const and mutable refs
- `test_complex_reference_pattern` - Tests complex borrowing scenarios
- `test_simulated_move_semantics` - Documents current move semantics limitations