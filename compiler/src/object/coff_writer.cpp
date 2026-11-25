#include "bf/coff_writer.h"
#include "bf/diagnostic.h"
#include <fstream>

namespace bfc {

CoffWriter::CoffWriter(DiagnosticSink& sink) : sink_(sink) {}

bool CoffWriter::write_object(const std::string& object_path, const std::string& llvm_module_path) {
    // Currently delegated to LLVM's object emission via target_machine_->addPassesToEmitFile
    // This stub exists for future custom COFF writing if needed.
    (void)object_path;
    (void)llvm_module_path;
    return true;
}

} // namespace bfc