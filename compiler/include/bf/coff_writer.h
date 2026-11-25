#pragma once

#include "bf/diagnostic.h"
#include <string>
#include <vector>

namespace bfc {

// COFF object file writer using LLVM's Object library.
// Currently a stub - LLVM's object emission handles this.
class CoffWriter {
public:
    explicit CoffWriter(DiagnosticSink& sink);

    // Write object file from LLVM module.
    // Currently delegates to LLVM's object emission.
    bool write_object(const std::string& object_path, const std::string& llvm_module_path);

private:
    DiagnosticSink& sink_;
};

} // namespace bfc