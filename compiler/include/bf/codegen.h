#pragma once

#include "bf/ir.h"
#include "bf/diagnostic.h"
#include <string>
#include <vector>
#include <memory>

namespace bfc {

// Code generator. Translates BasicForge IR to native object files
// using C code generation + clang.exe + lld-link.exe.
class LlvmCodeGen {
public:
    explicit LlvmCodeGen(DiagnosticSink& sink);
    ~LlvmCodeGen();

    // Generate object file from BasicForge IR module.
    // Returns true on success, false on error.
    bool generate_object(const IrModule& module, const std::string& output_path);

    // Generate executable by linking object files with runtime.
    // Returns true on success, false on error.
    bool link_executable(const std::vector<std::string>& object_files,
                         const std::string& output_path,
                         const std::string& entry_point);

private:
    class CodeGenImpl;
    std::unique_ptr<CodeGenImpl> impl_;
};

} // namespace bfc