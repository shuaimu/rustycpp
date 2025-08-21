cmake_minimum_required(VERSION 3.10)
project(MyProjectWithBorrowChecker CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Assuming rustycpp is added as a submodule at external/rustycpp
# You would run: git submodule add https://github.com/shuaimu/rustycpp external/rustycpp

# Include the submodule-friendly CMake integration
include(${CMAKE_CURRENT_SOURCE_DIR}/external/rustycpp/cmake/RustyCppSubmodule.cmake)

# Configure the borrow checker
set(RUSTYCPP_BUILD_TYPE "release")  # or "debug" for faster builds during development
set(ENABLE_BORROW_CHECKING ON)

# Optional: Make borrow check failures stop the build
# set(BORROW_CHECK_FATAL ON)

# Enable borrow checking for this project
enable_borrow_checking()

# Set up compile_commands.json generation for better include path handling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
setup_borrow_checker_compile_commands()

# Your project's include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Example library with borrow checking
add_library(my_safe_lib
    src/safe_container.cpp
    src/memory_pool.cpp
)

# Enable borrow checking for the entire library
add_borrow_check_target(my_safe_lib)

# Example executable
add_executable(my_app
    src/main.cpp
    src/application.cpp
)

target_link_libraries(my_app my_safe_lib)

# Option 1: Enable borrow checking for the entire executable
add_borrow_check_target(my_app)

# Option 2: Enable borrow checking for specific files only
# add_borrow_check(src/main.cpp)
# add_borrow_check(src/application.cpp)

# Custom target to manually run checks
add_custom_target(check_safety
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target borrow_check_all
    COMMENT "Running all borrow checks"
)

# Tests (if you have them)
enable_testing()
add_executable(my_tests
    tests/test_main.cpp
    tests/test_container.cpp
)
target_link_libraries(my_tests my_safe_lib)

# Enable borrow checking for tests
add_borrow_check_target(my_tests)

add_test(NAME safety_tests COMMAND my_tests)

# Installation
install(TARGETS my_app
    RUNTIME DESTINATION bin
)

install(TARGETS my_safe_lib
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Print configuration status
message(STATUS "=== Borrow Checker Configuration ===")
message(STATUS "Borrow checking enabled: ${ENABLE_BORROW_CHECKING}")
message(STATUS "Checker build type: ${RUSTYCPP_BUILD_TYPE}")
message(STATUS "Fatal on check failure: ${BORROW_CHECK_FATAL}")
message(STATUS "=====================================")