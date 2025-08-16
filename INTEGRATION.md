# Build System Integration Guide

## CMake Integration

The Rusty C++ Checker provides seamless CMake integration for easy adoption in existing projects.

### Quick Setup

1. **Install the borrow checker:**
   ```bash
   ./install.sh
   ```

2. **Add to your CMakeLists.txt:**
   ```cmake
   include(CppBorrowChecker)
   enable_borrow_checking()
   ```

3. **Build your project:**
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

The borrow checker will run automatically during compilation and fail the build if violations are found.

### Integration Levels

#### Level 1: Check Critical Files Only
```cmake
include(CppBorrowChecker)
set(ENABLE_BORROW_CHECKING ON)

# Check only specific files
add_borrow_check(src/memory_critical.cpp)
add_borrow_check(src/resource_manager.cpp)
```

#### Level 2: Check Entire Targets
```cmake
add_executable(my_app main.cpp utils.cpp)
add_borrow_check_target(my_app)  # Check all files in target
```

#### Level 3: Global Checking
```cmake
enable_borrow_checking()  # Check everything
set(BORROW_CHECK_FATAL ON)  # Make violations fail the build
```

### Example Output

When violations are detected:
```
[100%] Borrow checking src/demo.cpp
C++ Borrow Checker
Analyzing: /path/to/src/demo.cpp
✗ Found 3 violation(s):
In function 'test': Unsafe pointer dereference at line 28
In function 'test': Calling unsafe function 'process' at line 42
make[2]: *** [borrow_check_demo.stamp] Error 1
```

## Make Integration

For traditional Makefiles:

```makefile
CXX = g++
BORROW_CHECKER = rusty-cpp-checker
CXXFLAGS = -std=c++17

# Add borrow checking rule
%.check: %.cpp
	$(BORROW_CHECKER) $< -I include

# Make checking a prerequisite
main.o: main.cpp main.check
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Or add a separate check target
check: $(SOURCES:.cpp=.check)

all: check main
```

## Bazel Integration

For Bazel projects, create a macro:

```python
# In borrow_checker.bzl
def cc_library_checked(name, srcs, **kwargs):
    # Run borrow checker
    native.genrule(
        name = name + "_check",
        srcs = srcs,
        outs = [name + ".checked"],
        cmd = "rusty-cpp-checker $(SRCS) && touch $@",
    )
    
    # Build library with check dependency
    native.cc_library(
        name = name,
        srcs = srcs,
        deps = [":" + name + "_check"],
        **kwargs
    )
```

Usage:
```python
load(":borrow_checker.bzl", "cc_library_checked")

cc_library_checked(
    name = "mylib",
    srcs = ["lib.cpp"],
    hdrs = ["lib.h"],
)
```

## Cargo Integration (for Rust/C++ hybrid projects)

In `build.rs`:
```rust
use std::process::Command;

fn main() {
    // Run borrow checker on C++ files
    let cpp_files = ["src/native.cpp", "src/bridge.cpp"];
    
    for file in &cpp_files {
        let output = Command::new("rusty-cpp-checker")
            .arg(file)
            .output()
            .expect("Failed to run borrow checker");
            
        if !output.status.success() {
            panic!("Borrow check failed for {}", file);
        }
    }
    
    // Continue with normal C++ compilation
    cc::Build::new()
        .files(&cpp_files)
        .compile("native");
}
```

## CI/CD Integration

### GitHub Actions
```yaml
name: Borrow Check
on: [push, pull_request]

jobs:
  check:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    
    - name: Install Rust
      uses: actions-rs/toolchain@v1
      with:
        toolchain: stable
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y libclang-dev libz3-dev
    
    - name: Build and install borrow checker
      run: |
        cargo build --release
        sudo cp target/release/rusty-cpp-checker /usr/local/bin/
    
    - name: Run borrow checks
      run: |
        rusty-cpp-checker src/*.cpp
```

### GitLab CI
```yaml
borrow-check:
  stage: test
  image: rust:latest
  before_script:
    - apt-get update && apt-get install -y libclang-dev libz3-dev
    - cargo build --release
  script:
    - ./target/release/rusty-cpp-checker src/*.cpp
```

### Jenkins
```groovy
pipeline {
    agent any
    stages {
        stage('Borrow Check') {
            steps {
                sh '''
                    cargo build --release
                    find src -name "*.cpp" -exec ./target/release/rusty-cpp-checker {} \\;
                '''
            }
        }
    }
}
```

## VS Code Integration

Add to `.vscode/tasks.json`:
```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Borrow Check Current File",
            "type": "shell",
            "command": "rusty-cpp-checker",
            "args": ["${file}"],
            "group": {
                "kind": "test",
                "isDefault": true
            },
            "presentation": {
                "reveal": "always",
                "panel": "new"
            },
            "problemMatcher": {
                "owner": "rusty-cpp-checker",
                "fileLocation": ["relative", "${workspaceFolder}"],
                "pattern": {
                    "regexp": "^(.*):(\\d+):(\\d+):\\s+(.*)$",
                    "file": 1,
                    "line": 2,
                    "column": 3,
                    "message": 4
                }
            }
        }
    ]
}
```

Then use `Ctrl+Shift+B` to check the current file.

## Gradual Adoption Strategy

1. **Start Small**: Begin with critical modules that handle memory/resources
2. **Add Annotations**: Mark safe functions with `// @safe`
3. **Fix Violations**: Address issues found by the checker
4. **Expand Coverage**: Gradually add more files to checking
5. **Enforce in CI**: Once stable, make checks mandatory in CI/CD

## Configuration

Create `.borrow-checker.json` in your project root:
```json
{
    "include_paths": [
        "include",
        "/usr/local/include"
    ],
    "exclude_patterns": [
        "test/*",
        "third_party/*"
    ],
    "treat_warnings_as_errors": true,
    "whitelisted_functions": [
        "custom_safe_func",
        "legacy_api_call"
    ]
}
```

## Troubleshooting

### Issue: Standard headers not found
**Solution**: Set include paths explicitly:
```bash
export CPLUS_INCLUDE_PATH=/usr/include/c++/11:/usr/include
rusty-cpp-checker file.cpp
```

### Issue: Build fails with borrow check errors
**Solution**: Either fix the violations or temporarily disable checking:
```cmake
set(ENABLE_BORROW_CHECKING OFF)  # Temporary disable
```

### Issue: Too many false positives
**Solution**: Use gradual adoption - mark only truly safe code as `@safe`:
```cpp
// @safe
void verified_safe_function() {
    // This will be checked
}

void legacy_function() {
    // This won't be checked (unsafe by default)
}
```

## Next Steps

- See `examples/cmake_project/` for a complete CMake example
- Read `CLAUDE.md` for technical details
- Check `README.md` for usage instructions
- File issues at: https://github.com/yourusername/rusty-cpp-checker/issues