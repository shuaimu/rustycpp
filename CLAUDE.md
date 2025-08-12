# C++ Borrow Checker - Project Context for Claude

## Project Overview

This is a Rust-based static analyzer that applies Rust's ownership and borrowing rules to C++ code. The goal is to catch memory safety issues at compile-time without runtime overhead.

## Current State (Updated: Session completed reference checking implementation)

### What's Fully Implemented ✅
- ✅ **Complete reference borrow checking** for C++ const and mutable references
  - Multiple immutable borrows allowed
  - Single mutable borrow enforced
  - No mixing of mutable and immutable borrows
  - Clear error messages with variable names
- ✅ Basic project structure with modular architecture
- ✅ LibClang integration for parsing C++ AST (including reference bindings)
- ✅ IR (Intermediate Representation) with ownership tracking
- ✅ Ownership state tracking (Owned, Borrowed, Moved)
- ✅ Z3 solver integration for constraints (ready for future use)
- ✅ Colored diagnostic output
- ✅ CLI interface with clap
- ✅ **Comprehensive test suite**: 42 tests (29 unit, 13 integration)

### What's Partially Implemented ⚠️
- ⚠️ Move semantics detection (works for simple moves, not std::move)
- ⚠️ Lifetime analysis (framework exists, scope tracking not complete)
- ⚠️ AST to IR conversion (works for basic constructs, references, assignments)

### What's Not Implemented Yet ❌
- ❌ std::unique_ptr and std::shared_ptr analysis
- ❌ Full C++ construct support (templates, lambdas, etc.)
- ❌ Multi-file analysis
- ❌ Scope-based lifetime tracking
- ❌ Dangling reference detection

## Key Technical Decisions

1. **Language Choice**: Rust for memory safety and performance
2. **Parser**: LibClang for accurate C++ parsing
3. **Solver**: Z3 for lifetime constraint solving
4. **IR Design**: Ownership-aware representation separate from AST

## Project Structure

```
src/
├── main.rs           # Entry point, CLI handling
├── parser/
│   ├── mod.rs       # Parse orchestration
│   ├── ast_visitor.rs # AST traversal and extraction
│   └── annotations.rs # Custom annotation parsing
├── ir/
│   └── mod.rs       # Intermediate representation
├── analysis/
│   ├── mod.rs       # Main analysis entry
│   ├── ownership.rs # Ownership state tracking
│   ├── borrows.rs   # Borrow checking logic
│   └── lifetimes.rs # Lifetime inference
├── solver/
│   └── mod.rs       # Z3 constraint solving
└── diagnostics/
    └── mod.rs       # Error formatting
```

## Environment Setup Required

```bash
# macOS (updated paths)
export Z3_SYS_Z3_HEADER=/opt/homebrew/include/z3.h
export DYLD_LIBRARY_PATH=/opt/homebrew/Cellar/llvm/19.1.7/lib:$DYLD_LIBRARY_PATH

# Linux
export Z3_SYS_Z3_HEADER=/usr/include/z3.h
export LD_LIBRARY_PATH=/usr/lib/llvm-14/lib:$LD_LIBRARY_PATH
```

## Known Issues

1. **Include Paths**: Standard library headers (like `<iostream>`) aren't found by default
2. **Scope Tracking**: Borrow checking doesn't track scopes, all borrows are considered function-wide
3. **Limited C++ Support**: Templates, lambdas, and advanced C++ features not supported yet

## Testing Commands

```bash
# Set environment variables first (macOS)
export Z3_SYS_Z3_HEADER=/opt/homebrew/include/z3.h
export DYLD_LIBRARY_PATH=/opt/homebrew/Cellar/llvm/19.1.7/lib:$DYLD_LIBRARY_PATH

# Build the project
cargo build

# Run on example files
cargo run -- examples/reference_demo.cpp
cargo run -- examples/test_simple_refs.cpp

# Run all tests (42 tests total)
cargo test

# Run specific test with output
cargo test test_multiple_mutable_borrows -- --nocapture

# Check for warnings
cargo clippy
```

## Next Priority Tasks

### High Priority
1. **Implement scope-based lifetime tracking**:
   - Track variable scopes (blocks, functions)
   - Allow borrows to end when references go out of scope
   - Enable more precise borrow checking

2. **Add std::unique_ptr support**:
   - Parse and understand std::unique_ptr declarations
   - Track std::move() calls
   - Detect use-after-move for smart pointers

3. **Implement dangling reference detection**:
   - Track when references outlive their targets
   - Detect returning references to local variables
   - Add lifetime inference

### Medium Priority
1. **Smart pointer support**:
   - Recognize unique_ptr patterns
   - Track ownership transfers
   - Handle reset/release methods

2. **Improve diagnostics**:
   - Add code snippets to errors
   - Suggest fixes
   - Better formatting

3. **Add more test cases**:
   - Positive cases (should pass)
   - Negative cases (should fail)
   - Edge cases

### Low Priority
1. **Template support**
2. **Multi-file analysis**
3. **IDE integration**
4. **Performance optimization**

## Code Patterns to Follow

### Adding New Analysis
```rust
// In analysis/mod.rs
pub fn check_new_thing(ir: &IrProgram) -> Result<Vec<String>, String> {
    let mut errors = Vec::new();
    // Analysis logic here
    Ok(errors)
}
```

### Adding New IR Constructs
```rust
// In ir/mod.rs
#[derive(Debug, Clone)]
pub enum NewConstruct {
    // Define variants
}
```

### Error Reporting
```rust
// Use diagnostics module
use crate::diagnostics::format_error;
errors.push(format_error("message", location));
```

## Useful Resources

- [Clang AST Documentation](https://clang.llvm.org/docs/IntroductionToTheClangAST.html)
- [Rust Borrow Checker (Polonius)](https://github.com/rust-lang/polonius)
- [Z3 Rust Bindings](https://docs.rs/z3/latest/z3/)
- [LibClang Rust Bindings](https://docs.rs/clang/latest/clang/)

## Development Tips

1. **Use `cargo run -- -vv`** for verbose output during development
2. **Start with simple C++ files** without includes or templates
3. **Add debug prints** in the parser to understand AST structure
4. **Test incrementally** - get one feature working before moving to the next
5. **Keep the IR simple** initially, add complexity as needed

## Questions to Consider

When implementing new features, consider:
1. How does Rust handle this pattern?
2. What are the ownership implications?
3. What constraints need to be tracked?
4. How should errors be reported to users?
5. What's the simplest working implementation?

## Contact with Original Requirements

The user wants a compile-time checker for C++ that enforces Rust-like borrow checking rules. Key requirements:
- ✅ Standalone static analyzer (not a compiler plugin) - **DONE**
- ⚠️ Detect use-after-move - **Partial** (simple moves work, std::move not yet)
- ✅ Detect multiple mutable borrows - **DONE**
- ⚠️ Track lifetimes - **Partial** (framework exists, needs scope tracking)
- ✅ Provide clear error messages - **DONE**
- ✅ Support gradual adoption in existing codebases - **DONE** (can analyze individual files)

## Example Output

```
C++ Borrow Checker
Analyzing: examples/reference_demo.cpp
✗ Found 3 violation(s):
Cannot create mutable reference to 'value': already mutably borrowed
Cannot create mutable reference to 'value': already immutably borrowed
Cannot create immutable reference to 'value': already mutably borrowed
```

## Session Summary

This session successfully implemented:
1. Complete reference borrow checking (const and mutable)
2. Fixed AST parsing to properly extract reference bindings
3. Added comprehensive test suite (42 tests)
4. Created example files demonstrating the checker
5. Fixed all compilation errors and warnings
6. Documented the implementation thoroughly