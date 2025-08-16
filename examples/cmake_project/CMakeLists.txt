cmake_minimum_required(VERSION 3.10)
project(BorrowCheckedProject CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the borrow checker module
# In a real project, you would install this module or use find_package
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/CppBorrowChecker.cmake)

# Option 1: Enable borrow checking globally for all targets
# enable_borrow_checking()

# Option 2: Enable selectively (recommended for gradual adoption)
set(ENABLE_BORROW_CHECKING ON)

# Optional: Make borrow check failures stop the build
# set(BORROW_CHECK_FATAL ON)

# Set up compile_commands.json generation for better include path handling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
setup_borrow_checker_compile_commands()

# Add include directories that the borrow checker should know about
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Example library with borrow checking (commented out due to std header issues)
# add_library(safe_lib
#     src/safe_string.cpp
#     src/memory_manager.cpp
# )
# 
# # Enable borrow checking for the library
# add_borrow_check_target(safe_lib)

# Simple demo without std headers
add_executable(simple_demo
    src/simple_demo.cpp
)

# Enable borrow checking for the demo
add_borrow_check(src/simple_demo.cpp)

# Example executable (commented out due to std header issues)
# add_executable(main_app
#     src/main.cpp
#     src/utils.cpp
# )
# 
# target_link_libraries(main_app safe_lib)
# 
# # Enable borrow checking for specific files only
# add_borrow_check(src/main.cpp)
# add_borrow_check(src/utils.cpp)  # Uncomment to check this file too

# Custom target to check all files at once
add_custom_target(check_all
    COMMAND ${CPP_BORROW_CHECKER} 
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
            -I${CMAKE_CURRENT_SOURCE_DIR}/include
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running borrow checker on all source files"
    VERBATIM
)

# Install target (optional)
install(TARGETS simple_demo
    RUNTIME DESTINATION bin
)

# Install the borrow checker binary (optional)
# install_borrow_checker()

# Print status
message(STATUS "Borrow checking is ${ENABLE_BORROW_CHECKING}")
if(CPP_BORROW_CHECKER)
    message(STATUS "Borrow checker found at: ${CPP_BORROW_CHECKER}")
else()
    message(WARNING "Borrow checker not found. Build it with: cargo build --release")
endif()