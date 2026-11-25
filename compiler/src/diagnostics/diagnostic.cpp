#include "bf/diagnostic.h"
#include <iostream>

namespace bfc {

void ConsoleDiagnosticSink::emit(const Diagnostic& d) {
    const char* sev = "?";
    switch (d.severity) {
        case DiagnosticSeverity::Error:   sev = "error";   break;
        case DiagnosticSeverity::Warning: sev = "warning"; break;
        case DiagnosticSeverity::Note:    sev = "note";    break;
    }
    std::cerr << d.location.file << "(" << d.location.line << ","
              << d.location.column << "): " << sev << ": " << d.message
              << "\n";
    for (const auto& note : d.notes) {
        std::cerr << "    note: " << note << "\n";
    }
    if (d.severity == DiagnosticSeverity::Error) {
        had_error_ = true;
    }
}

} // namespace bfc
