# C++ Borrow Checker - Project Context for Claude

## Project Overview

This is a Rust-based static analyzer that applies Rust's ownership and borrowing rules to C++ code. The goal is to catch memory safety issues at compile-time without runtime overhead.

## Current State (Updated: Safe/unsafe annotation support for gradual adoption)

### What's Fully Implemented ✅
- ✅ **Complete reference borrow checking** for C++ const and mutable references
  - Multiple immutable borrows allowed
  - Single mutable borrow enforced
  - No mixing of mutable and immutable borrows
  - Clear error messages with variable names
- ✅ **std::move detection and use-after-move checking**
  - Detects std::move() calls in assignments and function arguments
  - Tracks moved-from state of variables
  - Reports use-after-move errors
  - Handles both direct moves and moves in function calls
- ✅ **Scope tracking for accurate borrow checking**
  - Tracks when `{}` blocks begin and end
  - Automatically cleans up borrows when they go out of scope
  - Eliminates false positives from sequential scopes
  - Properly handles nested scopes
- ✅ **Loop analysis with 2-iteration simulation**
  - Detects use-after-move in loops (for, while, do-while)
  - Simulates 2 iterations to catch errors on second pass
  - Properly clears loop-local borrows between iterations
  - Tracks moved state across loop iterations
- ✅ **If/else conditional analysis with path-sensitivity**
  - Parses if/else statements and conditions
  - Conservative path-sensitive analysis
  - Variable is moved only if moved in ALL paths
  - Borrows cleared when not present in all branches
  - Handles nested conditionals
- ✅ **Safe/unsafe annotation support for gradual adoption**
  - C++ files are unsafe by default (no checking) for compatibility
  - `// @safe` at file beginning enables checking for entire file
  - `@safe` function annotation enables checking for specific functions
  - `@unsafe` function annotation disables checking even in safe files
  - Allows gradual migration of existing codebases
  - Skip checks in performance-critical sections
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
- ✅ **Comprehensive test suite**: 69 tests (40 unit, 29 integration including loop, conditional, and safe/unsafe tests)

### What's Partially Implemented ⚠️
- ⚠️ Control flow (basic blocks work, loops/conditionals limited)
- ⚠️ Reassignment after move (not tracked yet)
- ⚠️ Method calls (basic support, no virtual functions)

### What's Not Implemented Yet ❌

#### Critical for Modern C++
- ❌ **Smart pointers** (std::unique_ptr, std::shared_ptr, std::weak_ptr)
  - No ownership transfer tracking for unique_ptr
  - No reference counting for shared_ptr
  - Would require parsing member function calls (reset, release, etc.)
  
- ❌ **Templates** 
  - Template declarations ignored
  - No instantiation tracking
  - Generic code goes unchecked

#### Important for Correctness
  
- ❌ **Constructor/Destructor (RAII)**
  - Object lifetime not tracked
  - Destructor calls not analyzed
  - RAII patterns not understood

#### Nice to Have
- ❌ **Reassignment after move**
  - Can't track when moved variable becomes valid again
  - `x = std::move(y); x = 42;` - x valid again but not tracked
  
- ❌ **Method calls**
  - Only free functions work
  - No `this` pointer tracking
  - Virtual functions not supported
  
- ❌ **Exception handling**
  - Try/catch blocks ignored
  - Stack unwinding not modeled
  
- ❌ **Lambdas and closures**
  - Capture semantics not analyzed
  - Closure lifetime not tracked
  
- ❌ **Better diagnostics**
  - No code snippets in errors
  - No fix suggestions
  - No explanation of borrowing rules
  
- ❌ **IDE integration**
  - No Language Server Protocol (LSP)
  - CLI only

## How Rust's Borrow Checker Handles Loops

Rust uses a sophisticated approach to detect use-after-move in loops:

1. **Control Flow Graph with Back Edges**: Loops have edges from end back to beginning
2. **Fixed-Point Iteration**: Analysis runs until no more state changes
3. **Three-State Tracking**: Variables are "definitely initialized", "definitely uninitialized", or "maybe initialized"
4. **Conservative Analysis**: "Maybe initialized" treated as error for moves

Example of what Rust catches:
```rust
for i in 0..2 {
    let y = x;  // ERROR: value moved here, in previous iteration of loop
}
```

To implement similar analysis in our checker:
- Detect loop back edges in CFG
- Analyze loop body twice (simulating two iterations)
- Track "maybe moved" state for variables
- Error if "maybe moved" variable is used

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

## Implementation Priority

Based on impact and complexity, recommended order:

### High Priority (Most Impact)
1. **std::unique_ptr support** - Ubiquitous in modern C++
2. **Loop iteration analysis** - Catches real bugs
3. **Basic templates** - Required for real codebases

### Medium Priority
4. **Path-sensitive analysis** - Reduces false positives
5. **Constructor/Destructor** - RAII understanding
6. **Method calls** - Object-oriented code

### Low Priority
7. **Reassignment tracking** - Edge cases
8. **Exception handling** - Complex
9. **Better diagnostics** - Current ones work
10. **IDE integration** - CLI is functional

## Contact with Original Requirements

The tool achieves the core goals:
- ✅ **Standalone static analyzer** - Works independently
- ⚠️ **Detect use-after-move** - Partial (needs std::move support)
- ✅ **Detect multiple mutable borrows** - Fully working
- ✅ **Track lifetimes** - Complete with inference and validation
- ✅ **Provide clear error messages** - With locations and context
- ✅ **Support gradual adoption** - Analyze individual files with annotations