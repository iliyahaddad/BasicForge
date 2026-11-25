#pragma once

#include "bf/ir.h"
#include "bf/diagnostic.h"
#include <string>

namespace bfc {

// Runtime C code generator. Produces C source for BasicForge runtime functions.
class RuntimeCodeGen {
public:
    explicit RuntimeCodeGen(DiagnosticSink& sink);

    // Generate runtime C source file.
    // Returns true on success, false on error.
    bool generate_runtime_c(const std::string& output_path,
                            const std::string& entry_point_name,
                            bool with_ui = false,
                            const std::string& target = "console");

private:
    DiagnosticSink& sink_;
};

} // namespace bfc