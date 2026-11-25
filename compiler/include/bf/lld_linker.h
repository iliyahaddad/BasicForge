#pragma once

#include "bf/diagnostic.h"
#include <string>
#include <vector>

namespace bfc {

// LLD linker integration.
// Currently uses subprocess call to lld-link.exe.
// Future: direct library integration when LLD C++ API stabilizes.
class LldLinker {
public:
    explicit LldLinker(DiagnosticSink& sink);

    // Link object files into an executable.
    bool link_executable(const std::vector<std::string>& object_files,
                         const std::string& output_path,
                         const std::string& entry_point,
                         const std::string& subsystem = "console",
                         bool dll = false);

private:
    DiagnosticSink& sink_;
};

} // namespace bfc