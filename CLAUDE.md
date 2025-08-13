# C++ Borrow Checker - Project Context for Claude

## Project Overview

This is a Rust-based static analyzer that applies Rust's ownership and borrowing rules to C++ code. The goal is to catch memory safety issues at compile-time without runtime overhead.

## Current State (Updated: Advanced lifetime checking completed)

### What's Fully Implemented ✅
- ✅ **Complete reference borrow checking** for C++ const and mutable references
  - Multiple immutable borrows allowed
  - Single mutable borrow enforced
  - No mixing of mutable and immutable borrows
  - Clear error messages with variable names
- ✅ **Cross-file analysis with lifetime annotations**
  - Rust-like lifetime syntax in headers (`&'a`, `&'a mut`, `owned`)
  - Header parsing and caching system
  - Include path resolution (-I flags, compile_commands.json, environment variables)
- ✅ **Advanced lifetime checking**
  - Scope-based lifetime tracking
  - Dangling reference detection
  - Transitive outlives checking ('a: 'b: 'c)
  - Automatic lifetime inference for local variables
- ✅ **Include path support**
  - CLI flags (-I)
  - Environment variables (CPLUS_INCLUDE_PATH, CPATH, etc.)
  - compile_commands.json parsing
  - Distinguishes quoted vs angle bracket includes
- ✅ Basic project structure with modular architecture
- ✅ LibClang integration for parsing C++ AST
- ✅ IR with CallExpr and Return statements
- ✅ Z3 solver integration for constraints
- ✅ Colored diagnostic output
- ✅ **Comprehensive test suite**: 56 tests (40 unit, 16 integration)

### What's Partially Implemented ⚠️
- ⚠️ Move semantics detection (simple moves work, not std::move)
- ⚠️ Control flow (basic blocks work, loops/conditionals limited)
- ⚠️ Method calls (basic support, no virtual functions)

### What's Not Implemented Yet ❌
- ❌ **std::move() detection** and use-after-move
- ❌ **Smart pointers** (std::unique_ptr, std::shared_ptr)
- ❌ **Templates** (no parsing or instantiation tracking)
- ❌ **Advanced control flow** (loops, switch, exceptions)
- ❌ **Constructors/Destructors** (no RAII tracking)
- ❌ **Lambdas and closures**
- ❌ **IDE integration** (no LSP server)
- ❌ **Fix suggestions** in error messages

## Key Technical Decisions

1. **Language Choice**: Rust for memory safety and performance
2. **Parser**: LibClang for accurate C++ parsing
3. **Solver**: Z3 for lifetime constraint solving
4. **IR Design**: Ownership-aware representation with CFG
5. **Analysis Strategy**: Per-translation-unit with header annotations (no .cpp-to-.cpp needed)

## Project Structure

```
src/
├── main.rs              # Entry point, CLI handling, include path resolution
├── parser/
│   ├── mod.rs          # Parse orchestration
│   ├── ast_visitor.rs  # AST traversal, function call extraction
│   ├── annotations.rs  # Lifetime annotation parsing
│   └── header_cache.rs # Header signature caching
├── ir/
│   └── mod.rs          # IR with CallExpr, Return, CFG
├── analysis/
│   ├── mod.rs              # Main analysis coordinator
│   ├── ownership.rs        # Ownership state tracking
│   ├── borrows.rs          # Basic borrow checking
│   ├── lifetimes.rs        # Original lifetime framework
│   ├── lifetime_checker.rs # Annotation-based checking
│   ├── scope_lifetime.rs   # Scope-based tracking
│   └── lifetime_inference.rs # Automatic inference
├── solver/
│   └── mod.rs          # Z3 constraint solving
└── diagnostics/
    └── mod.rs          # Error formatting
```

## Environment Setup

```bash
# macOS
export Z3_SYS_Z3_HEADER=/opt/homebrew/include/z3.h
export DYLD_LIBRARY_PATH=/opt/homebrew/Cellar/llvm/19.1.7/lib:$DYLD_LIBRARY_PATH

# Linux
export Z3_SYS_Z3_HEADER=/usr/include/z3.h
export LD_LIBRARY_PATH=/usr/lib/llvm-14/lib:$LD_LIBRARY_PATH

# Optional: Include paths via environment
export CPLUS_INCLUDE_PATH=/usr/include/c++:/usr/local/include
export CPATH=/usr/include
```

## Usage Examples

```bash
# Basic usage
cargo run -- file.cpp

# With include paths
cargo run -- file.cpp -I include -I /usr/local/include

# With compile_commands.json
cargo run -- file.cpp --compile-commands build/compile_commands.json

# Using environment variables
export CPLUS_INCLUDE_PATH=/project/include:/third_party/include
cargo run -- src/main.cpp
```

## Lifetime Annotation Syntax

```cpp
// In header files (.h/.hpp)

// @lifetime: &'a
const int& getRef();

// @lifetime: (&'a) -> &'a
const T& identity(const T& x);

// @lifetime: (&'a, &'b) -> &'a where 'a: 'b
const T& selectFirst(const T& a, const T& b);

// @lifetime: owned
std::unique_ptr<T> create();

// @lifetime: &'a mut
T& getMutable();
```

## Testing

```bash
# Run all tests (56 total)
cargo test

# Run specific test categories
cargo test lifetime   # Lifetime tests
cargo test borrow     # Borrow checking tests
cargo test cross_file # Cross-file tests

# Run with output
cargo test -- --nocapture

# Run examples
cargo run -- examples/lifetime_demo.cpp
cargo run -- examples/test_dangling.cpp
```

## Priority TODO List (What's Still Missing)

### 🔴 Critical for Modern C++ (Top Priority)
1. **std::move detection** - Essential for C++11+ code
2. **std::unique_ptr tracking** - Most common smart pointer
3. **std::shared_ptr support** - Widely used in production

### 🟡 Core Language Features
4. **Template support** - Required for real C++ projects
5. **Control flow analysis** - Loops/conditionals are fundamental
6. **Constructor/destructor tracking** - RAII is core to C++

### 🟢 Advanced Features
7. **Lambda captures** - Common in modern C++
8. **Better error messages** - Code snippets and fix suggestions
9. **IDE integration** - LSP server for real-time feedback
10. **Configuration files** - .borrow-checker.yml for customization

## Key Design Insights

### Why No .cpp-to-.cpp Analysis Needed

The tool correctly follows C++'s compilation model:
- Each `.cpp` file is analyzed independently
- Function signatures come from headers (with lifetime annotations)
- No need to see other `.cpp` implementations
- Matches how C++ compilers and Rust's borrow checker work

### Analysis Approach

1. **Parse headers** → Extract lifetime-annotated signatures
2. **Analyze .cpp** → Check implementation against contracts
3. **Validate calls** → Ensure lifetime constraints are met
4. **Report errors** → With clear messages and locations

## Code Patterns to Follow

### Adding New Analysis
```rust
// In analysis/mod.rs or new module
pub fn check_feature(program: &IrProgram, cache: &HeaderCache) -> Result<Vec<String>, String> {
    let mut errors = Vec::new();
    // Analysis logic
    Ok(errors)
}
```

### Adding Lifetime Annotations
```cpp
// In header file
// @lifetime: (&'a, &'b) -> &'a where 'a: 'b
const T& function(const T& longer, const T& shorter);
```

## Development Tips

1. **Test incrementally** - Use small C++ examples first
2. **Check parser output** - `cargo run -- file.cpp -vv` for debug
3. **Verify lifetimes** - Use `examples/test_*.cpp` for validation
4. **Run clippy** - `cargo clippy` for Rust best practices
5. **Update tests** - Add test cases for new features

## Example Output

```
C++ Borrow Checker
Analyzing: examples/test.cpp
Found 2 include path(s) from environment variables
✗ Found 3 violation(s):
Cannot create mutable reference to 'value': already immutably borrowed
Potential dangling reference: returning 'ref' which depends on local variable 'temp'
Cannot borrow 'x': variable is not alive in current scope
```

## Recent Achievements

This project has successfully implemented:
1. ✅ Cross-file analysis with lifetime annotations
2. ✅ Include path resolution from multiple sources
3. ✅ Scope-based lifetime tracking
4. ✅ Dangling reference detection
5. ✅ Transitive outlives checking
6. ✅ Automatic lifetime inference
7. ✅ Enhanced IR with function calls and returns
8. ✅ 56 comprehensive tests

## Contact with Original Requirements

The tool achieves the core goals:
- ✅ **Standalone static analyzer** - Works independently
- ⚠️ **Detect use-after-move** - Partial (needs std::move support)
- ✅ **Detect multiple mutable borrows** - Fully working
- ✅ **Track lifetimes** - Complete with inference and validation
- ✅ **Provide clear error messages** - With locations and context
- ✅ **Support gradual adoption** - Analyze individual files with annotations