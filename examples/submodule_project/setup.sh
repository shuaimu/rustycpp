#!/bin/bash

# Setup script for using rustycpp as a submodule

echo "Setting up rustycpp as a submodule..."

# Create the external directory
mkdir -p external

# Add rustycpp as a submodule (if not already added)
if [ ! -d "external/rustycpp" ]; then
    echo "Adding rustycpp as a git submodule..."
    git submodule add https://github.com/shuaimu/rustycpp external/rustycpp
    git submodule update --init --recursive
else
    echo "rustycpp submodule already exists, updating..."
    git submodule update --init --recursive
fi

# Create build directory
mkdir -p build

echo "Configuring CMake project..."
cd build
cmake .. -DENABLE_BORROW_CHECKING=ON -DRUSTYCPP_BUILD_TYPE=release

echo "Building the project (this will also build rusty-cpp-checker)..."
cmake --build .

echo ""
echo "Setup complete! The rusty-cpp-checker will be built automatically as part of your project."
echo ""
echo "To run borrow checks:"
echo "  cd build"
echo "  cmake --build . --target borrow_check_all"
echo ""
echo "To build and check in one command:"
echo "  cmake --build . --target all"