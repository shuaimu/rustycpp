# C++ Borrow Checker - Standalone Release

This is a standalone release of the C++ Borrow Checker.

## Installation

### Quick Install
Run: `./install.sh`

### Manual Install
Copy `cpp-borrow-checker-standalone` to your PATH:
```bash
cp cpp-borrow-checker-standalone /usr/local/bin/cpp-borrow-checker
```

## Requirements

- LLVM/Clang libraries (usually already installed)
- For macOS: `brew install llvm`
- For Linux: `apt-get install llvm` or `yum install llvm`

## Usage

```bash
cpp-borrow-checker <file.cpp>
```

## Troubleshooting

If you get library not found errors:
1. Install LLVM: `brew install llvm` (macOS) or `apt-get install llvm` (Linux)
2. Use the wrapper script: `cpp-borrow-checker-standalone` instead of the raw binary

