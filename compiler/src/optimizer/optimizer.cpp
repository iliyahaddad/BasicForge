#include "bf/optimizer.h"
#include <unordered_map>
#include <map>
#include <set>
#include <cstddef>

namespace bfc {

bool Optimizer::optimize(IrModule& module) {
    if (level_ == OptLevel::None) {
        return true;
    }

    bool changed = false;
    for (auto& fn : module.functions) {
        if (optimize_function(fn)) {
            changed = true;
        }
    }
    return changed;
}

bool Optimizer::optimize_function(IrFunction& fn) {
    bool changed = false;

    if (level_ >= OptLevel::O1) {
        if (constant_fold(fn)) changed = true;
    }

    if (level_ >= OptLevel::O2) {
        if (dead_code_elim(fn)) changed = true;
        if (common_subexpr_elim(fn)) changed = true;
    }

    return changed;
}

bool Optimizer::constant_fold(IrFunction& fn) {
    bool changed = false;

    for (auto& block : fn.blocks) {
        auto& instructions = block.instructions;
        size_t i = 0;
        while (i < instructions.size()) {
            IrInstruction folded;
            if (try_fold(instructions[i], folded)) {
                instructions[i] = folded;
                changed = true;
            }
            i++;
        }
    }

    return changed;
}

bool Optimizer::try_fold(const IrInstruction& inst, IrInstruction& result) {
    switch (inst.opcode) {
        case IrOpcode::Add:
        case IrOpcode::Sub:
        case IrOpcode::Mul:
        case IrOpcode::Div:
        case IrOpcode::Mod: {
            int64_t l, r;
            if (is_constant_int(inst.operands[0], l) &&
                is_constant_int(inst.operands[1], r)) {
                int64_t res = 0;
                switch (inst.opcode) {
                    case IrOpcode::Add: res = l + r; break;
                    case IrOpcode::Sub: res = l - r; break;
                    case IrOpcode::Mul: res = l * r; break;
                    case IrOpcode::Div: if (r != 0) res = l / r; else return false; break;
                    case IrOpcode::Mod: if (r != 0) res = l % r; else return false; break;
                    default: return false;
                }
                result = inst;
                result.opcode = IrOpcode::ConstInt;
                result.operands.clear();
                result.result.kind = IrValue::Kind::Constant;
                result.result.type = PrimitiveType::Integer;
                result.result.int_value = res;
                return true;
            }
            return false;
        }
        case IrOpcode::Eq:
        case IrOpcode::Ne:
        case IrOpcode::Lt:
        case IrOpcode::Gt:
        case IrOpcode::Le:
        case IrOpcode::Ge: {
            int64_t l, r;
            if (is_constant_int(inst.operands[0], l) &&
                is_constant_int(inst.operands[1], r)) {
                int res = 0;
                switch (inst.opcode) {
                    case IrOpcode::Eq: res = (l == r) ? 1 : 0; break;
                    case IrOpcode::Ne: res = (l != r) ? 1 : 0; break;
                    case IrOpcode::Lt: res = (l < r) ? 1 : 0; break;
                    case IrOpcode::Gt: res = (l > r) ? 1 : 0; break;
                    case IrOpcode::Le: res = (l <= r) ? 1 : 0; break;
                    case IrOpcode::Ge: res = (l >= r) ? 1 : 0; break;
                    default: return false;
                }
                result = inst;
                result.opcode = IrOpcode::ConstInt;
                result.operands.clear();
                result.result.kind = IrValue::Kind::Constant;
                result.result.type = PrimitiveType::Integer;
                result.result.int_value = res;
                return true;
            }
            return false;
        }
        default:
            return false;
    }
}

bool Optimizer::is_constant_int(const IrValue& v, int64_t& out) {
    if (v.kind == IrValue::Kind::Constant &&
        (v.type == PrimitiveType::Integer || v.type == PrimitiveType::Long)) {
        out = v.int_value;
        return true;
    }
    return false;
}

bool Optimizer::dead_code_elim(IrFunction& fn) {
    bool changed = false;

    for (auto& block : fn.blocks) {
        auto& instructions = block.instructions;

        // Mark which local variables are used
        std::set<std::string> used_locals;
        for (const auto& inst : instructions) {
            for (const auto& op : inst.operands) {
                if (op.kind == IrValue::Kind::Local || op.kind == IrValue::Kind::Parameter) {
                    used_locals.insert(op.name);
                }
            }
        }

        // Remove stores to unused locals
        auto new_end = std::remove_if(instructions.begin(), instructions.end(),
            [&](const IrInstruction& inst) {
                if (inst.opcode == IrOpcode::Store) {
                    if (used_locals.find(inst.result.name) == used_locals.end()) {
                        changed = true;
                        return true;
                    }
                }
                return false;
            });
        instructions.erase(new_end, instructions.end());
    }

    return changed;
}

bool Optimizer::common_subexpr_elim(IrFunction& fn) {
    bool changed = false;

    for (auto& block : fn.blocks) {
        auto& instructions = block.instructions;
        std::map<std::string, size_t> seen;

        size_t i = 0;
        while (i < instructions.size()) {
            const auto& inst = instructions[i];
            std::string key = std::to_string(static_cast<uint8_t>(inst.opcode));
            for (const auto& op : inst.operands) {
                key += ":" + op.name + ":" + std::to_string(static_cast<uint8_t>(op.kind));
            }

            auto it = seen.find(key);
            if (it != seen.end()) {
                // Found duplicate - this is a candidate for CSE
                // Full CSE would replace all uses of inst.result with
                // instructions[it->second].result
                changed = true;
            } else {
                seen[key] = i;
            }
            i++;
        }
    }

    return changed;
}

} // namespace bfc
