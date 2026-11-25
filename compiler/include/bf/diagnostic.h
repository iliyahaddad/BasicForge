#pragma once

#include "bf/bfc.h"
#include <string>
#include <vector>

namespace bfc {

enum class DiagnosticSeverity : uint8_t {
    Error,
    Warning,
    Note,
};

struct Diagnostic {
    DiagnosticSeverity severity;
    std::string message;
    SourceLocation location;
    std::vector<std::string> notes;
};

class DiagnosticSink {
public:
    virtual ~DiagnosticSink() = default;
    virtual void emit(const Diagnostic& d) = 0;
    bool has_errors() const { return had_error_; }

protected:
    bool had_error_ = false;
};

class ConsoleDiagnosticSink : public DiagnosticSink {
public:
    void emit(const Diagnostic& d) override;
};

} // namespace bfc
