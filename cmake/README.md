# CMake Integration for Rusty C++ Checker

This directory contains CMake modules for integrating the Rusty C++ Checker into your CMake-based projects.

## Quick Start

### Installation

```bash
# From the project root
./install.sh
```

This will:
- Build the borrow checker binary
- Install it to `/usr/local/bin` (or `$INSTALL_PREFIX/bin`)
- Install the CMake module to `/usr/local/share/cmake/Modules`

### Basic Usage

In your `CMakeLists.txt`:

```cmake
# Include the module
include(CppBorrowChecker)

# Enable globally for all targets
enable_borrow_checking()

# Add your targets
add_executable(my_app main.cpp)

# The borrow checker will run automatically during build
```

## Integration Options

### 1. Global Checking (Check Everything)

```cmake
include(CppBorrowChecker)
enable_borrow_checking()

# All C++ files in all targets will be checked
add_executable(app main.cpp utils.cpp)
add_library(lib src1.cpp src2.cpp)
```

### 2. Per-Target Checking (Recommended)

```cmake
include(CppBorrowChecker)
set(ENABLE_BORROW_CHECKING ON)

add_executable(my_app main.cpp utils.cpp)

# Only check this specific target
add_borrow_check_target(my_app)
```

### 3. Per-File Checking (Most Granular)

```cmake
include(CppBorrowChecker)
set(ENABLE_BORROW_CHECKING ON)

# Check specific files only
add_borrow_check(src/critical.cpp)
add_borrow_check(src/memory_sensitive.cpp)
# src/legacy.cpp is not checked
```

### 4. With compile_commands.json

```cmake
# Enable compile_commands.json generation
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
setup_borrow_checker_compile_commands()

# This provides better include path handling
```

## Configuration Options

### CMake Variables

```cmake
# Enable/disable checking (default: OFF)
set(ENABLE_BORROW_CHECKING ON)

# Make check failures stop the build (default: OFF)
set(BORROW_CHECK_FATAL ON)

# Additional include paths for the checker
set(BORROW_CHECKER_INCLUDE_PATHS 
    "/usr/local/include/boost"
    "/opt/custom/include"
)
```

### Environment Variables

The checker respects standard C++ include path environment variables:
- `CPLUS_INCLUDE_PATH` - C++ include directories
- `C_INCLUDE_PATH` - C include directories
- `CPATH` - Both C and C++ includes

## Advanced Usage

### Custom Check Target

```cmake
# Create a target that checks all files
add_custom_target(check_all
    COMMAND cpp-borrow-checker 
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
            -I${CMAKE_CURRENT_SOURCE_DIR}/include
    COMMENT "Running borrow checker on all files"
)
```

### Integration with CTest

```cmake
enable_testing()

# Add borrow checking as a test
add_test(NAME borrow_check
    COMMAND rusty-cpp-checker 
            ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)
```

### Conditional Checking

```cmake
# Only enable in Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    enable_borrow_checking()
endif()

# Or as an option
option(ENABLE_SAFETY_CHECKS "Enable borrow checking" OFF)
if(ENABLE_SAFETY_CHECKS)
    enable_borrow_checking()
endif()
```

## Gradual Adoption Strategy

The borrow checker is designed for gradual adoption in existing codebases:

### Step 1: Start with Critical Files

```cmake
# Check only the most critical files first
add_borrow_check(src/memory_pool.cpp)
add_borrow_check(src/resource_manager.cpp)
```

### Step 2: Add Safety Annotations

In your C++ code:
```cpp
// @safe
namespace critical {
    // This namespace will be checked
    void process_data();
}

// Legacy code remains unchecked by default
void legacy_function() {
    // Not checked unless explicitly marked @safe
}
```

### Step 3: Expand Coverage

```cmake
# Gradually add more files
add_borrow_check_target(core_library)
# Then later...
add_borrow_check_target(network_module)
```

### Step 4: Enable Globally

```cmake
# When ready, enable for everything
enable_borrow_checking()
set(BORROW_CHECK_FATAL ON)  # Make it enforced
```

## Troubleshooting

### Checker Not Found

If CMake can't find the borrow checker:

```cmake
# Explicitly set the path
set(CPP_BORROW_CHECKER "/path/to/rusty-cpp-checker")
include(CppBorrowChecker)
```

### Include Path Issues

If the checker can't find headers:

```cmake
# Add include paths explicitly
set(BORROW_CHECKER_INCLUDE_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    /usr/include/c++/11  # System includes
)
```

### Performance

For large projects, consider:

```cmake
# Run checks in parallel
set(CMAKE_JOB_POOLS check_pool=4)
set_property(GLOBAL PROPERTY JOB_POOLS check_pool=4)

# Only check changed files (with ccache)
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()
```

## Example Project Structure

```
my_project/
├── CMakeLists.txt          # Main CMake file
├── cmake/
│   └── CppBorrowChecker.cmake  # Copy or symlink
├── include/
│   └── my_lib.h           # Headers with lifetime annotations
├── src/
│   ├── main.cpp           # @safe functions
│   ├── safe_module.cpp    # Fully checked
│   └── legacy.cpp         # Unchecked legacy code
└── build/
    └── compile_commands.json  # Generated by CMake
```

## CI/CD Integration

### GitHub Actions

```yaml
- name: Install Borrow Checker
  run: |
    git clone https://github.com/yourusername/rusty-cpp-checker
    cd rusty-cpp-checker
    ./install.sh

- name: Run Borrow Checks
  run: |
    mkdir build && cd build
    cmake .. -DENABLE_BORROW_CHECKING=ON
    make borrow_check_all
```

### Jenkins

```groovy
stage('Borrow Check') {
    steps {
        sh 'cmake -DENABLE_BORROW_CHECKING=ON ..'
        sh 'make -j4'  // Will run checks during build
    }
}
```

## License

Same as the Rusty C++ Checker project.