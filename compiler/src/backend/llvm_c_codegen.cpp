// LLVM C API-based codegen for BasicForge
// Uses the LLVM-C.lib which is available in the standard LLVM installation

#include "bf/codegen.h"
#include "bf/diagnostic.h"
#include "bf/ir.h"

#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Object.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>

namespace bfc {

class LlvmCBackend {
public:
    LlvmCBackend(DiagnosticSink& sink) : sink_(sink) {}

    bool generate_object(const IrModule& module, const std::string& output_path) {
        // Initialize LLVM
        LLVMInitializeX86TargetInfo();
        LLVMInitializeX86Target();
        LLVMInitializeX86TargetMC();
        LLVMInitializeX86AsmPrinter();
        LLVMInitializeX86AsmParser();
        
        // Create context and module
        context_ = LLVMContextCreate();
        module_ = LLVMModuleCreateWithName("basicforge_module");
        
        // Set target triple
        LLVMSetTarget(module_, "x86_64-pc-windows-msvc");
        
        // Create target machine
        char* triple = LLVMGetDefaultTargetTriple();
        LLVMTargetRef target;
        char* error = nullptr;
        if (LLVMGetTargetFromTriple(triple, &target, &error) != 0) {
            LLVMDisposeMessage(error);
            LLVMDisposeMessage(triple);
            return false;
        }
        
        char* cpu = LLVMGetHostCPUName();
        char* features = LLVMGetHostCPUFeatures();
        
        target_machine_ = LLVMCreateTargetMachine(
            target, triple, cpu, features,
            LLVMCodeGenLevelAggressive,
            LLVMRelocDefault,
            LLVMCodeModelDefault,
            LLVMCallConvDefault
        );
        
        LLVMDisposeMessage(triple);
        LLVMDisposeMessage(cpu);
        LLVMDisposeMessage(features);
        
        // Generate IR for each function
        for (const auto& func : module.functions) {
            generate_function(func);
        }
        
        // Verify module
        char* verify_error = nullptr;
        if (LLVMVerifyModule(module_, LLVMAbortProcessAction, &verify_error) != 0) {
            LLVMDisposeMessage(verify_error);
            return false;
        }
        
        // Emit object file
        char* obj_error = nullptr;
        if (LLVMTargetMachineEmitToFile(
            target_machine_, module_, output_path.c_str(),
            LLVMObjectFile, &obj_error) != 0) {
            LLVMDisposeMessage(obj_error);
            return false;
        }
        
        // Cleanup
        LLVMDisposeTargetMachine(target_machine_);
        LLVMDisposeModule(module_);
        LLVMContextDispose(context_);
        
        return true;
    }

private:
    DiagnosticSink& sink_;
    LLVMContextRef context_ = nullptr;
    LLVMModuleRef module_ = nullptr;
    LLVMTargetMachineRef target_machine_ = nullptr;
    std::unordered_map<std::string, LLVMValueRef> local_vars_;
    std::unordered_map<std::string, LLVMBasicBlockRef> block_map_;
    LLVMBuilderRef builder_ = nullptr;
    LLVMBasicBlockRef current_block_ = nullptr;

    LLVMTypeRef get_llvm_type(PrimitiveType type) {
        switch (type) {
            case PrimitiveType::Integer:
            case PrimitiveType::Long:
                return LLVMInt64Type();
            case PrimitiveType::Single:
                return LLVMFloatType();
            case PrimitiveType::Double:
                return LLVMDoubleType();
            case PrimitiveType::Boolean:
                return LLVMInt1Type();
            case PrimitiveType::String:
                return LLVMPointerType(LLVMInt8Type(), 0);
            default:
                return LLVMVoidType();
        }
    }

    void generate_function(const IrFunction& func) {
        // Create function type
        std::vector<LLVMTypeRef> param_types;
        for (const auto& param : func.parameters) {
            param_types.push_back(get_llvm_type(param.type));
        }
        
        LLVMTypeRef ret_type = get_llvm_type(func.return_type);
        LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types.data(), param_types.size(), 0);
        
        // Create function
        LLVMValueRef llvm_func = LLVMAddFunction(module_, func.name.c_str(), func_type);
        
        // Create entry block
        LLVMBasicBlockRef entry = LLVMAppendBasicBlock(llvm_func, "entry");
        builder_ = LLVMCreateBuilder();
        LLVMPositionBuilderAtEnd(builder_, entry);
        
        // Add parameters to local vars
        local_vars_.clear();
        unsigned int i = 0;
        for (const auto& param : func.parameters) {
            LLVMValueRef param_val = LLVMGetParam(llvm_func, i);
            LLVMSetValueName(param_val, param.name.c_str());
            
            // Create alloca for parameter
            LLVMValueRef alloca = LLVMBuildAlloca(builder_, get_llvm_type(param.type), param.name.c_str());
            LLVMBuildStore(builder_, param_val, alloca);
            local_vars_[param.name] = alloca;
            i++;
        }
        
        current_block_ = entry;
        block_map_["entry"] = entry;
        
        // Create basic blocks
        for (const auto& block : func.blocks) {
            if (block.name != "entry") {
                LLVMBasicBlockRef bb = LLVMAppendBasicBlock(llvm_func, block.name.c_str());
                block_map_[block.name] = bb;
            }
        }
        
        // Generate instructions for each block
        for (const auto& block : func.blocks) {
            current_block_ = block_map_[block.name];
            LLVMPositionBuilderAtEnd(builder_, current_block_);
            
            for (const auto& inst : block.instructions) {
                generate_instruction(inst);
            }
        }
        
        // Ensure function has a return
        if (!LLVMGetBasicBlockTerminator(current_block_)) {
            if (func.return_type == PrimitiveType::Void) {
                LLVMBuildRetVoid(builder_);
            } else {
                LLVMValueRef zero = LLVMConstInt(get_llvm_type(func.return_type), 0, 0);
                LLVMBuildRet(builder_, zero);
            }
        }
        
        LLVMDisposeBuilder(builder_);
    }

    void generate_instruction(const IrInstruction& inst) {
        switch (inst.opcode) {
            case IrOpcode::Add: {
                LLVMValueRef lhs = get_operand(inst.operands[0], local_vars_);
                LLVMValueRef rhs = get_operand(inst.operands[1], local_vars_);
                LLVMValueRef result = LLVMBuildAdd(builder_, lhs, rhs, "add_tmp");
                set_result(inst.result, result);
                break;
            }
            case IrOpcode::Sub: {
                LLVMValueRef lhs = get_operand(inst.operands[0], local_vars_);
                LLVMValueRef rhs = get_operand(inst.operands[1], local_vars_);
                LLVMValueRef result = LLVMBuildSub(builder_, lhs, rhs, "sub_tmp");
                set_result(inst.result, result);
                break;
            }
            case IrOpcode::Mul: {
                LLVMValueRef lhs = get_operand(inst.operands[0], local_vars_);
                LLVMValueRef rhs = get_operand(inst.operands[1], local_vars_);
                LLVMValueRef result = LLVMBuildMul(builder_, lhs, rhs, "mul_tmp");
                set_result(inst.result, result);
                break;
            }
            case IrOpcode::Div: {
                LLVMValueRef lhs = get_operand(inst.operands[0], local_vars_);
                LLVMValueRef rhs = get_operand(inst.operands[1], local_vars_);
                LLVMValueRef result = LLVMBuildSDiv(builder_, lhs, rhs, "div_tmp");
                set_result(inst.result, result);
                break;
            }
            case IrOpcode::Ret: {
                if (inst.operands.empty()) {
                    LLVMBuildRetVoid(builder_);
                } else {
                    LLVMValueRef ret_val = get_operand(inst.operands[0], local_vars_);
                    LLVMBuildRet(builder_, ret_val);
                }
                break;
            }
            case IrOpcode::Br: {
                LLVMBasicBlockRef target = block_map_[inst.label];
                LLVMBuildBr(builder_, target);
                break;
            }
            case IrOpcode::CondBr: {
                LLVMValueRef cond = get_operand(inst.operands[0], local_vars_);
                LLVMBasicBlockRef true_bb = block_map_[inst.label];
                LLVMBasicBlockRef false_bb = block_map_[inst.label2];
                LLVMBuildCondBr(builder_, cond, true_bb, false_bb);
                break;
            }
            case IrOpcode::Label: {
                // Label handled by block_map
                break;
            }
            default:
                break;
        }
    }

    LLVMValueRef get_operand(const IrValue& operand, std::unordered_map<std::string, LLVMValueRef>& vars) {
        if (operand.is_constant) {
            switch (operand.type) {
                case PrimitiveType::Integer:
                case PrimitiveType::Long:
                    return LLVMConstInt(LLVMInt64Type(), operand.int_value, 0);
                case PrimitiveType::Double:
                    return LLVMConstReal(LLVMDoubleType(), operand.double_value);
                case PrimitiveType::Boolean:
                    return LLVMConstInt(LLVMInt1Type(), operand.int_value, 0);
                default:
                    return LLVMConstInt(LLVMInt64Type(), 0, 0);
            }
        } else {
            auto it = vars.find(operand.name);
            if (it != vars.end()) {
                LLVMValueRef alloca = it->second;
                return LLVMBuildLoad2(builder_, LLVMTypeOf(alloca), alloca, "load_tmp");
            }
            return LLVMConstInt(LLVMInt64Type(), 0, 0);
        }
    }

    void set_result(const IrValue& result, LLVMValueRef value) {
        if (result.is_local) {
            LLVMValueRef alloca = LLVMBuildAlloca(builder_, LLVMTypeOf(value), result.name.c_str());
            LLVMBuildStore(builder_, value, alloca);
            local_vars_[result.name] = alloca;
        }
    }
};

// Update the LlvmCodeGen class to use LlvmCBackend
LlvmCodeGen::LlvmCodeGen(DiagnosticSink& sink) : impl_(new LlvmCBackend(sink)) {}

LlvmCodeGen::~LlvmCodeGen() = default;

bool LlvmCodeGen::generate_object(const IrModule& module, const std::string& output_path) {
    return impl_->generate_object(module, output_path);
}

} // namespace bfc