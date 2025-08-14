#!/bin/bash
# Installer for cpp-borrow-checker

INSTALL_DIR="/usr/local/bin"

echo "Installing cpp-borrow-checker to $INSTALL_DIR..."

# Check for required dependencies
if ! command -v clang &> /dev/null; then
    echo "Warning: clang not found. Please install LLVM/Clang:"
    echo "  brew install llvm"
fi

# Copy the binary
sudo cp cpp-borrow-checker-standalone "$INSTALL_DIR/cpp-borrow-checker"
sudo chmod +x "$INSTALL_DIR/cpp-borrow-checker"

echo "Installation complete!"
echo "You can now run: cpp-borrow-checker <file.cpp>"
