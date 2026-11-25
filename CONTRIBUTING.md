# Contributing to BasicForge

Thank you for your interest in contributing to BasicForge!

## Getting Started

### Prerequisites
- C++20 compatible compiler (MSVC, Clang, or GCC)
- CMake 3.20+
- Rust 1.70+ (for IDE)
- LLVM 17+ (optional, for LLVM backend)
- Git

### Building from Source

```bash
# Clone the repository
git clone https://github.com/basicforge/basicforge.git
cd basicforge

# Build compiler
cd compiler
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Build IDE
cd ../ide
cargo build --release
```

### Running Tests

```bash
# Compiler tests
cd compiler/build
ctest -C Release

# IDE tests
cd ide
cargo test
```

## Development Guidelines

### Code Style
- **C++**: Follow LLVM coding style, use `.clang-format` and `.clang-tidy`
- **Rust**: Follow `rustfmt` and `clippy` guidelines
- **Commits**: Use conventional commit messages

### Pull Request Process
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/my-feature`)
4. Make your changes
5. Run tests and ensure they pass
6. Format your code (`clang-format -i` for C++, `cargo fmt` for Rust)
7. Submit a pull request

### Reporting Bugs
- Use GitHub Issues
- Include: OS, compiler version, steps to reproduce, expected vs actual behavior

### Feature Requests
- Use GitHub Discussions
- Describe the use case and expected behavior

## License

See LICENSE for details.
