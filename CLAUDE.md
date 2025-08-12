# C++ Borrow Checker - Project Context for Claude

## Project Overview

This is a Rust-based static analyzer that applies Rust's ownership and borrowing rules to C++ code. The goal is to catch memory safety issues at compile-time without runtime overhead.

## Current State

### What's Implemented
- ✅ Basic project structure with modular architecture
- ✅ LibClang integration for parsing C++ AST
- ✅ Initial IR (Intermediate Representation) design
- ✅ Ownership tracking framework
- ✅ Borrow checking skeleton
- ✅ Z3 solver integration for constraints
- ✅ Colored diagnostic output
- ✅ CLI interface with clap

### What's Partially Implemented
- ⚠️ AST to IR conversion (basic functions only)
- ⚠️ Move semantics detection (structure in place, logic incomplete)
- ⚠️ Lifetime analysis (framework exists, inference not complete)

### What's Not Implemented Yet
- ❌ Full C++ construct support (templates, lambdas, etc.)
- ❌ Smart pointer analysis (unique_ptr, shared_ptr)
- ❌ Multi-file analysis
- ❌ Actual error detection in the analysis phase
- ❌ Source location tracking in errors

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
# macOS
export Z3_SYS_Z3_HEADER=/opt/homebrew/opt/z3/include/z3.h
export DYLD_LIBRARY_PATH=/opt/homebrew/opt/llvm/lib:$DYLD_LIBRARY_PATH

# Linux
export Z3_SYS_Z3_HEADER=/usr/include/z3.h
export LD_LIBRARY_PATH=/usr/lib/llvm-14/lib:$LD_LIBRARY_PATH
```

## Known Issues

1. **Include Paths**: Standard library headers aren't found by default
2. **Incomplete Analysis**: The analysis phase doesn't actually detect errors yet
3. **Limited C++ Support**: Only basic functions are parsed correctly

## Testing Commands

```bash
# Build the project
cargo build

# Run on simple test (works)
cargo run -- examples/simple_test.cpp

# Run tests
cargo test

# Check for warnings
cargo clippy
```

## Next Priority Tasks

### High Priority
1. **Complete AST to IR conversion** for basic C++ constructs:
   - Variable declarations with initialization
   - Assignments and moves
   - Function calls
   - Basic control flow

2. **Implement use-after-move detection**:
   - Track std::move() calls
   - Mark variables as moved
   - Detect usage of moved variables

3. **Add source location tracking**:
   - Preserve line/column info from AST
   - Include in error messages

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
- Standalone static analyzer (not a compiler plugin)
- Detect use-after-move
- Detect multiple mutable borrows
- Track lifetimes
- Provide clear error messages
- Support gradual adoption in existing codebases