# Building Release Binaries

This document explains how to build standalone release binaries that don't require users to set environment variables.

## Quick Build

```bash
# Build a standalone release package
./build_release.sh
```

This creates a distribution package in `dist/cpp-borrow-checker-<OS>-<ARCH>/` with:
- The compiled binary
- A wrapper script that handles library paths
- An installer script
- README with instructions

## Manual Build

```bash
# Build optimized release binary
cargo build --release

# The binary will be at:
# target/release/cpp-borrow-checker
```

## How It Works

The release binary includes:

1. **Embedded Library Paths (RPATH)**: The binary has runtime library search paths embedded via the linker:
   - `@loader_path/../lib` (relative to binary location)
   - `/opt/homebrew/lib` (Homebrew on macOS)
   - `/usr/lib/llvm-14/lib` (common on Linux)

2. **Build Configuration**: `.cargo/config.toml` sets:
   - Link-time optimization (LTO) for smaller binaries
   - Strip symbols for size reduction
   - RPATH settings for finding libraries at runtime

3. **Wrapper Script**: The standalone wrapper script:
   - Sets `DYLD_LIBRARY_PATH` (macOS) or `LD_LIBRARY_PATH` (Linux)
   - Ensures libraries are found even if not in standard locations
   - Falls back to system paths

## Distribution

### Creating a Release Package

```bash
# Run the build script
./build_release.sh

# Package will be created in:
# dist/cpp-borrow-checker-Darwin-arm64/  (on macOS ARM64)
# dist/cpp-borrow-checker-Linux-x86_64/  (on Linux x64)
```

### Installing from Package

Users can install the standalone binary:

```bash
cd dist/cpp-borrow-checker-*/
./install.sh
```

Or manually:

```bash
cp cpp-borrow-checker-standalone /usr/local/bin/cpp-borrow-checker
```

## Platform-Specific Notes

### macOS

- Uses `@loader_path` and `@rpath` for relative library paths
- Requires LLVM from Homebrew: `brew install llvm`
- Binary looks for libraries in:
  - `/opt/homebrew/lib` (ARM64)
  - `/usr/local/lib` (Intel)
  - Relative to binary location

### Linux

- Uses `$ORIGIN` for relative library paths
- Looks for LLVM in standard locations:
  - `/usr/lib/llvm-*/lib`
  - `/usr/local/lib`
- May need to install: `apt-get install llvm` or `yum install llvm`

### Windows

- Uses PATH for DLL resolution
- Set `LLVM_PATH` environment variable during build
- Distribute with required DLLs

## Troubleshooting

If the release binary can't find libraries:

1. **Use the wrapper script**: `cpp-borrow-checker-standalone`
2. **Install dependencies**: 
   - macOS: `brew install llvm z3`
   - Linux: `apt-get install llvm libz3-dev`
3. **Check library paths**:
   - macOS: `otool -L cpp-borrow-checker`
   - Linux: `ldd cpp-borrow-checker`

## Creating a Fully Portable Binary

For maximum portability, you can bundle libraries:

```bash
# macOS example
mkdir portable
cp target/release/cpp-borrow-checker portable/
mkdir portable/lib

# Copy required libraries
cp /opt/homebrew/opt/llvm/lib/libclang.dylib portable/lib/
cp /opt/homebrew/opt/z3/lib/libz3.*.dylib portable/lib/

# Update library paths
install_name_tool -change /opt/homebrew/opt/llvm/lib/libclang.dylib @loader_path/lib/libclang.dylib portable/cpp-borrow-checker
install_name_tool -change /opt/homebrew/opt/z3/lib/libz3.4.13.dylib @loader_path/lib/libz3.4.13.dylib portable/cpp-borrow-checker
```

## Release Checklist

- [ ] Run tests: `cargo test`
- [ ] Update version in `Cargo.toml`
- [ ] Build release: `./build_release.sh`
- [ ] Test standalone binary
- [ ] Create GitHub release with artifacts
- [ ] Update installation instructions