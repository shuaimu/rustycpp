# RustyCppSubmodule.cmake - Submodule integration for Rusty C++ Checker
#
# This module enables using rusty-cpp-checker as a git submodule without global installation.
# It builds the checker as part of your project and uses it directly from the build directory.
#
# Usage in your project's CMakeLists.txt:
#   add_subdirectory(external/rustycpp)  # Or wherever you placed the submodule
#   include(${CMAKE_CURRENT_SOURCE_DIR}/external/rustycpp/cmake/RustyCppSubmodule.cmake)
#   
#   # Then use the same functions as before:
#   enable_borrow_checking()
#   add_borrow_check_target(my_target)
#   add_borrow_check(my_file.cpp)

# Detect the rusty-cpp directory (should be the parent of this cmake file)
get_filename_component(RUSTYCPP_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# Option to control the Rust build type
set(RUSTYCPP_BUILD_TYPE "release" CACHE STRING "Build type for rusty-cpp-checker (debug or release)")
set_property(CACHE RUSTYCPP_BUILD_TYPE PROPERTY STRINGS "debug" "release")

# Option to enable/disable borrow checking
option(ENABLE_BORROW_CHECKING "Enable C++ Borrow Checking" OFF)
option(BORROW_CHECK_FATAL "Make borrow check failures fatal" OFF)

# Determine the target directory and binary name based on build type
if(RUSTYCPP_BUILD_TYPE STREQUAL "release")
    set(RUSTYCPP_TARGET_DIR "${RUSTYCPP_DIR}/target/release")
    set(CARGO_BUILD_FLAGS "--release")
else()
    set(RUSTYCPP_TARGET_DIR "${RUSTYCPP_DIR}/target/debug")
    set(CARGO_BUILD_FLAGS "")
endif()

# Set the checker binary path
set(CPP_BORROW_CHECKER "${RUSTYCPP_TARGET_DIR}/rusty-cpp-checker")
if(WIN32)
    set(CPP_BORROW_CHECKER "${CPP_BORROW_CHECKER}.exe")
endif()

# Create a custom target to build the rusty-cpp-checker
add_custom_target(build_rusty_cpp_checker
    COMMAND cargo build ${CARGO_BUILD_FLAGS}
    WORKING_DIRECTORY ${RUSTYCPP_DIR}
    COMMENT "Building rusty-cpp-checker (${RUSTYCPP_BUILD_TYPE} mode)"
    VERBATIM
)

# Create the output directory if it doesn't exist
file(MAKE_DIRECTORY ${RUSTYCPP_TARGET_DIR})

# Mark the checker binary as a generated file
set_source_files_properties(${CPP_BORROW_CHECKER} PROPERTIES GENERATED TRUE)

# Function to ensure the checker is built before use
function(ensure_checker_built TARGET_NAME)
    add_dependencies(${TARGET_NAME} build_rusty_cpp_checker)
endfunction()

# Function to add borrow checking for a single file
function(add_borrow_check SOURCE_FILE)
    if(NOT ENABLE_BORROW_CHECKING)
        return()
    endif()
    
    get_filename_component(FILE_NAME ${SOURCE_FILE} NAME_WE)
    set(CHECK_NAME "borrow_check_${FILE_NAME}")
    
    # Get include directories from the current directory properties
    get_directory_property(INCLUDE_DIRS INCLUDE_DIRECTORIES)
    
    # Build include flags
    set(INCLUDE_FLAGS)
    foreach(DIR ${INCLUDE_DIRS})
        list(APPEND INCLUDE_FLAGS "-I${DIR}")
    endforeach()
    
    # Add user-specified includes if they exist
    if(DEFINED BORROW_CHECKER_INCLUDE_PATHS)
        foreach(DIR ${BORROW_CHECKER_INCLUDE_PATHS})
            list(APPEND INCLUDE_FLAGS "-I${DIR}")
        endforeach()
    endif()
    
    # Get absolute path for the source file
    get_filename_component(SOURCE_ABS ${SOURCE_FILE} ABSOLUTE)
    
    # Create custom command for borrow checking
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CHECK_NAME}.stamp
        COMMAND ${CPP_BORROW_CHECKER} ${SOURCE_ABS} ${INCLUDE_FLAGS}
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/${CHECK_NAME}.stamp
        DEPENDS ${SOURCE_ABS} ${CPP_BORROW_CHECKER}
        COMMENT "Borrow checking ${SOURCE_FILE}"
        VERBATIM
    )
    
    # Add to custom target
    add_custom_target(${CHECK_NAME} ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${CHECK_NAME}.stamp
    )
    
    # Ensure the checker is built first
    ensure_checker_built(${CHECK_NAME})
    
    # Make it fatal if requested
    if(BORROW_CHECK_FATAL)
        set_property(TARGET ${CHECK_NAME} PROPERTY JOB_POOL_COMPILE console)
    endif()
endfunction()

# Function to add borrow checking for a target
function(add_borrow_check_target TARGET_NAME)
    if(NOT ENABLE_BORROW_CHECKING)
        return()
    endif()
    
    # Get source files from target
    get_target_property(SOURCES ${TARGET_NAME} SOURCES)
    
    # Get include directories from target
    get_target_property(TARGET_INCLUDES ${TARGET_NAME} INCLUDE_DIRECTORIES)
    
    # Build include flags
    set(INCLUDE_FLAGS)
    if(TARGET_INCLUDES)
        foreach(DIR ${TARGET_INCLUDES})
            list(APPEND INCLUDE_FLAGS "-I${DIR}")
        endforeach()
    endif()
    
    # Create a custom target for all checks of this target
    set(ALL_CHECKS_TARGET "borrow_check_all_${TARGET_NAME}")
    add_custom_target(${ALL_CHECKS_TARGET})
    ensure_checker_built(${ALL_CHECKS_TARGET})
    
    # Add checks for each source file
    foreach(SOURCE ${SOURCES})
        # Only check .cpp files
        if(SOURCE MATCHES "\\.(cpp|cc|cxx)$")
            get_filename_component(FILE_NAME ${SOURCE} NAME_WE)
            set(CHECK_NAME "borrow_check_${TARGET_NAME}_${FILE_NAME}")
            
            # Get absolute path for the source file
            get_filename_component(SOURCE_ABS ${SOURCE} ABSOLUTE)
            
            add_custom_command(
                OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CHECK_NAME}.stamp
                COMMAND ${CPP_BORROW_CHECKER} ${SOURCE_ABS} ${INCLUDE_FLAGS}
                COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/${CHECK_NAME}.stamp
                DEPENDS ${SOURCE_ABS} ${CPP_BORROW_CHECKER}
                COMMENT "Borrow checking ${SOURCE} (from ${TARGET_NAME})"
                VERBATIM
            )
            
            # Add custom target for this check
            add_custom_target(${CHECK_NAME}
                DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${CHECK_NAME}.stamp
            )
            
            # Add to the all checks target
            add_dependencies(${ALL_CHECKS_TARGET} ${CHECK_NAME})
        endif()
    endforeach()
    
    # Make the main target depend on all checks
    add_dependencies(${TARGET_NAME} ${ALL_CHECKS_TARGET})
endfunction()

# Function to enable borrow checking globally
function(enable_borrow_checking)
    set(ENABLE_BORROW_CHECKING ON PARENT_SCOPE)
    message(STATUS "C++ Borrow Checking enabled")
    message(STATUS "Checker will be built at: ${CPP_BORROW_CHECKER}")
    message(STATUS "Build type: ${RUSTYCPP_BUILD_TYPE}")
endfunction()

# Create a custom target for checking all files
if(ENABLE_BORROW_CHECKING)
    add_custom_target(borrow_check_all
        COMMENT "Running borrow checker on all C++ files"
    )
    ensure_checker_built(borrow_check_all)
endif()

# Function to add compile_commands.json support
function(setup_borrow_checker_compile_commands)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON PARENT_SCOPE)
    
    # Create a custom target that runs borrow checker with compile_commands.json
    add_custom_target(borrow_check_with_compile_commands
        COMMAND ${CPP_BORROW_CHECKER} 
                --compile-commands ${CMAKE_BINARY_DIR}/compile_commands.json
                \${SOURCE_FILE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS ${CPP_BORROW_CHECKER}
        COMMENT "Run: cmake --build . --target borrow_check_with_compile_commands -- SOURCE_FILE=<path/to/file.cpp>"
        VERBATIM
    )
    ensure_checker_built(borrow_check_with_compile_commands)
endfunction()

# Macro to mark files as safe
macro(mark_safe)
    foreach(FILE ${ARGN})
        set_source_files_properties(${FILE} PROPERTIES 
            COMPILE_DEFINITIONS "BORROW_CHECKER_SAFE"
        )
    endforeach()
endmacro()

# Print initial status
message(STATUS "Rusty C++ Checker submodule found at: ${RUSTYCPP_DIR}")
message(STATUS "Checker will be built on demand at: ${CPP_BORROW_CHECKER}")