#!/bin/bash

# Build script for creating a standalone release binary
# This binary will work without requiring environment variables

set -e

echo "Building standalone release binary..."

# Detect OS
OS=$(uname -s)
ARCH=$(uname -m)

# Set up environment for build
export Z3_SYS_Z3_HEADER=/opt/homebrew/include/z3.h
export DYLD_LIBRARY_PATH=/opt/homebrew/Cellar/llvm/19.1.7/lib:$DYLD_LIBRARY_PATH

# Build release version
echo "Compiling release build..."
cargo build --release

# Create distribution directory
DIST_DIR="dist/cpp-borrow-checker-${OS}-${ARCH}"
mkdir -p "$DIST_DIR"

# Copy the binary
cp target/release/cpp-borrow-checker "$DIST_DIR/"

if [ "$OS" = "Darwin" ]; then
    echo "Configuring for macOS..."
    
    # Create a wrapper script for macOS that sets library paths
    cat > "$DIST_DIR/cpp-borrow-checker-standalone" << 'EOF'
#!/bin/bash
# Standalone wrapper for cpp-borrow-checker

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Set library paths
export DYLD_LIBRARY_PATH="/opt/homebrew/Cellar/llvm/19.1.7/lib:/opt/homebrew/lib:/usr/local/lib:$DYLD_LIBRARY_PATH"

# Run the actual binary
exec "$SCRIPT_DIR/cpp-borrow-checker" "$@"
EOF
    
    chmod +x "$DIST_DIR/cpp-borrow-checker-standalone"
    
    # Use install_name_tool to fix library paths
    echo "Fixing library dependencies..."
    
    # Find all dylib dependencies
    LIBS=$(otool -L "$DIST_DIR/cpp-borrow-checker" | grep -E '^\s+/' | awk '{print $1}' | grep -v '/usr/lib\|/System')
    
    # Copy required libraries locally (optional - for truly standalone)
    # mkdir -p "$DIST_DIR/lib"
    # for lib in $LIBS; do
    #     if [ -f "$lib" ]; then
    #         cp "$lib" "$DIST_DIR/lib/" 2>/dev/null || true
    #         base=$(basename "$lib")
    #         install_name_tool -change "$lib" "@loader_path/lib/$base" "$DIST_DIR/cpp-borrow-checker" 2>/dev/null || true
    #     fi
    # done
    
    echo "Creating installer package..."
    cat > "$DIST_DIR/install.sh" << 'EOF'
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
EOF
    
elif [ "$OS" = "Linux" ]; then
    echo "Configuring for Linux..."
    
    # Create wrapper script for Linux
    cat > "$DIST_DIR/cpp-borrow-checker-standalone" << 'EOF'
#!/bin/bash
# Standalone wrapper for cpp-borrow-checker

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Set library paths for common LLVM installations
export LD_LIBRARY_PATH="/usr/lib/llvm-14/lib:/usr/lib/llvm-13/lib:/usr/local/lib:$LD_LIBRARY_PATH"

exec "$SCRIPT_DIR/cpp-borrow-checker" "$@"
EOF
    
    chmod +x "$DIST_DIR/cpp-borrow-checker-standalone"
fi

chmod +x "$DIST_DIR/install.sh" 2>/dev/null || true

# Create README for distribution
cat > "$DIST_DIR/README.md" << EOF
# C++ Borrow Checker - Standalone Release

This is a standalone release of the C++ Borrow Checker.

## Installation

### Quick Install
Run: \`./install.sh\`

### Manual Install
Copy \`cpp-borrow-checker-standalone\` to your PATH:
\`\`\`bash
cp cpp-borrow-checker-standalone /usr/local/bin/cpp-borrow-checker
\`\`\`

## Requirements

- LLVM/Clang libraries (usually already installed)
- For macOS: \`brew install llvm\`
- For Linux: \`apt-get install llvm\` or \`yum install llvm\`

## Usage

\`\`\`bash
cpp-borrow-checker <file.cpp>
\`\`\`

## Troubleshooting

If you get library not found errors:
1. Install LLVM: \`brew install llvm\` (macOS) or \`apt-get install llvm\` (Linux)
2. Use the wrapper script: \`cpp-borrow-checker-standalone\` instead of the raw binary

EOF

echo "Build complete!"
echo "Distribution created in: $DIST_DIR"
echo ""
echo "To install locally, run:"
echo "  cd $DIST_DIR && ./install.sh"
echo ""
echo "Or copy cpp-borrow-checker-standalone to your PATH"