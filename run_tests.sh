#!/bin/bash
# Script to run tests with required environment variables

# Set Z3 header path for macOS (Homebrew installation)
export Z3_SYS_Z3_HEADER=/opt/homebrew/include/z3.h

# Set LLVM/Clang library path for macOS
export DYLD_LIBRARY_PATH=/opt/homebrew/Cellar/llvm/19.1.7/lib:$DYLD_LIBRARY_PATH

# Run cargo test with all required environment variables
cargo test "$@"