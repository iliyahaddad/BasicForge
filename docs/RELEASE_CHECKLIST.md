# Release Checklist

1. Update `VERSION.toml` and the version in `compiler/CMakeLists.txt`.
2. Run the full test suite:
   - `ctest --test-dir build -C Release --output-on-failure`
   - `compiler/tests/run_tests.ps1`
3. Run all examples:
   - `examples/*.bf`
4. Build the IDE:
   - `cargo build --release`
5. Update `CHANGELOG.md`.
6. Build the installer:
   - `call "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\BasicForge.iss`
7. Create a git tag `vX.Y.Z` and push it; the Release workflow builds and
   publishes the Windows x64 package.
8. Verify the release package contains:
   - `bfc.exe`
   - `examples/`
   - `libraries/`
   - `sdks/`
   - `README.md`, `CHANGELOG.md`, `LICENSE`
9. Update the docs site if applicable.

## Versioning

Follow semantic versioning:

- **Major**: breaking language/CLI changes
- **Minor**: new features (controls, modules, commands)
- **Patch**: bug fixes