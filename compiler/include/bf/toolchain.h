#pragma once

#include <cstdlib>
#include <fstream>
#include <string>

namespace bfc {

inline bool toolchain_file_exists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

// Locate an LLVM tool. Search order:
//   1. BFC_LLVM_BIN environment variable
//   2. common 64-bit / 32-bit LLVM install directories
//   3. PATH (return the bare tool name)
inline std::string find_llvm_tool(const std::string& tool) {
    if (const char* env = std::getenv("BFC_LLVM_BIN")) {
        if (env[0] != '\0') {
            std::string candidate = std::string(env) + "\\" + tool;
            if (toolchain_file_exists(candidate)) return candidate;
        }
    }
    const char* dirs[] = {
        "C:\\Program Files\\LLVM\\bin\\",
        "C:\\Program Files (x86)\\LLVM\\bin\\",
    };
    for (const char* dir : dirs) {
        std::string candidate = std::string(dir) + tool;
        if (toolchain_file_exists(candidate)) return candidate;
    }
    return tool;
}

} // namespace bfc