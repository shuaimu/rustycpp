# Rusty C++ Checker

A standalone static analyzer that enforces Rust-like ownership and borrowing rules for C++ code, bringing memory safety guarantees to existing C++ codebases without runtime overhead.

## 🎯 Vision

This project aims to catch memory safety issues at compile-time by applying Rust's proven ownership model to C++ code. It helps prevent common bugs like use-after-move, double-free, and dangling references before they reach production.

## ✨ Features

### Core Capabilities
- **🔒 Ownership Tracking**: Ensures single ownership of resources with move semantics
- **🔄 Borrow Checking**: Enforces Rust's borrowing rules (multiple readers XOR single writer)
- **⏳ Lifetime Analysis**: Validates that references don't outlive their data
- **🎯 Smart Pointer Support**: Special handling for `std::unique_ptr`, `std::shared_ptr`, and `std::weak_ptr`
- **🎨 Beautiful Diagnostics**: Clear, actionable error messages with source locations

### Detected Issues
- Use-after-move violations
- Multiple mutable borrows
- Dangling references
- Lifetime constraint violations
- RAII violations
- Data races (through borrow checking)

## 📦 Installation

### Prerequisites

- **Rust**: 1.70+ (for building the analyzer)
- **LLVM/Clang**: 14+ (for parsing C++)
- **Z3**: 4.8+ (for constraint solving)

### macOS

```bash
# Install dependencies
brew install llvm z3

# Clone the repository
git clone https://github.com/yourusername/rusty-cpp-checker
cd rusty-cpp-checker

# Build the project
cargo build --release

# Run tests
./run_tests.sh

# Add to PATH (optional)
export PATH="$PATH:$(pwd)/target/release"
```

**Note**: The project includes a `.cargo/config.toml` file that automatically sets the required environment variables for Z3. If you encounter build issues, you may need to adjust the paths in this file based on your system configuration.

### Linux (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install llvm-14-dev libclang-14-dev libz3-dev

# Clone and build
git clone https://github.com/yourusername/rusty-cpp-checker
cd rusty-cpp-checker
cargo build --release
```

### Windows

```bash
# Install LLVM from https://releases.llvm.org/
# Install Z3 from https://github.com/Z3Prover/z3/releases
# Set environment variables:
set LIBCLANG_PATH=C:\Program Files\LLVM\lib
set Z3_SYS_Z3_HEADER=C:\z3\include\z3.h

# Build
cargo build --release
```

## 🚀 Usage

### Basic Usage

```bash
# Analyze a single file
rusty-cpp-checker path/to/file.cpp

# Analyze with verbose output
rusty-cpp-checker -vv path/to/file.cpp

# Output in JSON format (for IDE integration)
rusty-cpp-checker --format json path/to/file.cpp
```

### Standalone Binary (No Environment Variables Required)

For release distributions, we provide a standalone binary that doesn't require setting environment variables:

```bash
# Build standalone release
./build_release.sh

# Install from distribution
cd dist/rusty-cpp-checker-*/
./install.sh

# Or use directly
./rusty-cpp-checker-standalone file.cpp
```

See [RELEASE.md](RELEASE.md) for details on building and distributing standalone binaries.

### Environment Setup (macOS)

For convenience, add these to your shell profile:

```bash
# ~/.zshrc or ~/.bashrc
export Z3_SYS_Z3_HEADER=/opt/homebrew/opt/z3/include/z3.h
export DYLD_LIBRARY_PATH=/opt/homebrew/opt/llvm/lib:$DYLD_LIBRARY_PATH
```

## 🛡️ Safety Annotations

The borrow checker uses a unified annotation system for gradual adoption in existing codebases:

### Unified Rule
`@safe` and `@unsafe` annotations attach to the **next** code element (namespace, function, or first statement).

```cpp
// Example 1: Namespace-level safety
// @safe
namespace myapp {
    void func() { /* checked */ }
}

// Example 2: Function-level safety
// @safe
void checked_function() { /* checked */ }

void unchecked_function() { /* not checked - default is unsafe */ }

// Example 3: First-element rule
// @safe
int global = 42;  // Makes entire file safe

// Example 4: Unsafe blocks within safe functions
// @safe
void mixed_safety() {
    int value = 42;
    int& ref1 = value;
    
    // @unsafe
    {
        int& ref2 = value;  // Not checked in unsafe block
    }
    // @endunsafe
}
```

### Default Behavior
- Files are **unsafe by default** (no checking) for backward compatibility
- Use `@safe` to opt into borrow checking
- Use `@unsafe` to explicitly disable checking

## 📝 Examples

### Example 1: Use After Move

```cpp
#include <memory>

void bad_code() {
    std::unique_ptr<int> ptr1 = std::make_unique<int>(42);
    std::unique_ptr<int> ptr2 = std::move(ptr1);
    
    *ptr1 = 10;  // ERROR: Use after move!
}
```

**Output:**
```
error: use of moved value: `ptr1`
  --> example.cpp:6:5
   |
6  |     *ptr1 = 10;
   |     ^^^^^ value used here after move
   |
note: value moved here
  --> example.cpp:5:34
   |
5  |     std::unique_ptr<int> ptr2 = std::move(ptr1);
   |                                  ^^^^^^^^^^^^^^
```

### Example 2: Multiple Mutable Borrows

```cpp
void bad_borrow() {
    int value = 42;
    int& ref1 = value;
    int& ref2 = value;  // ERROR: Cannot borrow as mutable twice
}
```

### Example 3: Lifetime Violation

```cpp
int& dangling_reference() {
    int local = 42;
    return local;  // ERROR: Returning reference to local variable
}
```

## 🏗️ Architecture

```
┌─────────────┐     ┌──────────┐     ┌────────┐
│   C++ Code  │────▶│  Parser  │────▶│   IR   │
└─────────────┘     └──────────┘     └────────┘
                          │                │
                    (libclang)              ▼
                                    ┌──────────────┐
┌─────────────┐     ┌──────────┐   │   Analysis   │
│ Diagnostics │◀────│  Solver  │◀──│   Engine     │
└─────────────┘     └──────────┘   └──────────────┘
                         │                │
                       (Z3)        (Ownership/Lifetime)
```

### Components

- **Parser** (`src/parser/`): Uses libclang to build C++ AST
- **IR** (`src/ir/`): Ownership-aware intermediate representation
- **Analysis** (`src/analysis/`): Core borrow checking algorithms
- **Solver** (`src/solver/`): Z3-based constraint solving for lifetimes
- **Diagnostics** (`src/diagnostics/`): User-friendly error reporting

## 🎯 Roadmap

### Phase 1: Foundation (Current)
- [x] Basic project structure
- [x] Clang integration
- [x] Initial IR design
- [x] Simple ownership tracking
- [ ] Use-after-move detection

### Phase 2: Core Features
- [ ] Complete borrow checking
- [ ] Lifetime inference
- [ ] Smart pointer analysis
- [ ] Template support
- [ ] Multi-file analysis

### Phase 3: Production Ready
- [ ] IDE integration (VSCode, CLion)
- [ ] CI/CD integration
- [ ] Performance optimization
- [ ] Incremental analysis
- [ ] Fix suggestions

### Phase 4: Advanced
- [ ] Async/await support
- [ ] Thread safety analysis
- [ ] Custom annotations
- [ ] Auto-fixing capabilities

## 🤝 Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Areas We Need Help
- Implementing more C++ AST patterns
- Improving error messages
- Writing test cases
- Documentation
- IDE plugins

## 📚 Documentation

- [Architecture Overview](docs/ARCHITECTURE.md)
- [Borrow Checking Algorithm](docs/ALGORITHM.md)
- [Contributing Guide](CONTRIBUTING.md)
- [API Reference](docs/API.md)

## 🔬 Research Papers

This project is inspired by:
- [Rust's Borrow Checker (Polonius)](https://github.com/rust-lang/polonius)
- [Linear Types for Safe Manual Memory Management](https://www.microsoft.com/en-us/research/publication/linear-types-for-safe-manual-memory-management/)
- [Region-Based Memory Management](https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-262.pdf)

## 📄 License

MIT License - see [LICENSE](LICENSE) for details

## 🙏 Acknowledgments

- Rust team for the ownership model inspiration
- LLVM/Clang team for the excellent C++ parsing infrastructure
- Z3 team for the powerful constraint solver
- All contributors and early adopters

## 📞 Contact

- **Issues**: [GitHub Issues](https://github.com/yourusername/cpp-borrow-checker/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/cpp-borrow-checker/discussions)
- **Email**: your.email@example.com

---

**⚠️ Note**: This is an experimental tool. While it can catch many issues, it should not be the only safety measure in production code. Always use in conjunction with other testing and verification methods.
