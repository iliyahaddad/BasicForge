#pragma once

#include "bf/ir.h"
#include "bf/diagnostic.h"
#include <string>
#include <vector>

namespace bfc {

enum class OptLevel : uint8_t {
    None = 0,
    O1 = 1,
    O2 = 2,
    O3 = 3,
};

// BasicForge IR optimizer. Performs language-independent optimizations
// on BasicForge IR before code generation.
class Optimizer {
public:
    explicit Optimizer(DiagnosticSink& sink, OptLevel level = OptLevel::O2)
        : sink_(sink), level_(level) {}

    // Optimize an IR module in-place. Returns true if any changes were made.
    bool optimize(IrModule& module);

    OptLevel level() const { return level_; }

private:
    DiagnosticSink& sink_;
    OptLevel level_;

    bool optimize_function(IrFunction& fn);
    bool constant_fold(IrFunction& fn);
    bool try_fold(const IrInstruction& inst, IrInstruction& result);
    bool is_constant_int(const IrValue& v, int64_t& out);
    bool dead_code_elim(IrFunction& fn);
    bool common_subexpr_elim(IrFunction& fn);
};

} // namespace bfc
