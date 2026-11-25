#include "bf/codegen.h"
#include "bf/ir.h"
#include "bf/diagnostic.h"
#include "bf/toolchain.h"
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdlib>
#include <cwchar>
#include <cstdio>

static bool run_command_bat(const std::vector<std::string>& args) {
    // Build command with quoted arguments for the .bat file
    std::string cmd;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i != 0) cmd += " ";
        const std::string& a = args[i];
        if (a.find(' ') != std::string::npos || a.find('\t') != std::string::npos) {
            cmd += "\"" + a + "\"";
        } else {
            cmd += a;
        }
    }

    if (std::getenv("BFC_DEBUG_LINK")) {
        std::fprintf(stderr, "[bfc link] %s\n", cmd.c_str());
    }

    // Write command to a temporary .bat file to avoid cmd.exe quoting issues
    // with paths containing spaces (e.g. "C:\Program Files\LLVM\bin\clang.exe")
    std::string bat_path = "_bfc_cmd.bat";
    {
        std::ofstream bat_file(bat_path);
        if (!bat_file) {
            return false;
        }
        bat_file << "@echo off\r\n";
        bat_file << cmd << "\r\n";
    }

    // Execute the .bat file using _wsystem
    std::wstring wbat_path(bat_path.begin(), bat_path.end());
    int rc = _wsystem(wbat_path.c_str());

    // Clean up
    remove(bat_path.c_str());

    return rc == 0;
}

#ifdef BFC_LLVM_HEADERS_AVAILABLE
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/IntrinsicFunctions.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetParser.h>
#include <llvm/Support/Host.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/CodeGen/Passes.h>
#include <llvm/CodeGen/TargetLoweringObjectFileImpl.h>
#include <llvm/CodeGen/TargetLoweringObjectFile.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCStreamer.h>
#include <llvm/MC/MCAsmBackend.h>
#include <llvm/MC/MCCodeEmitter.h>
#include <llvm/MC/MCObjectFileInfo.h>
#include <llvm/MC/MCSymbol.h>
#include <llvm/MC/MCSection.h>
#include <llvm/MC/MCSectionCOFF.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Object/COFF.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Vectorize.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <llvm/Transforms/Utils/PromoteMemoryToRegister.h>
#include <llvm/Transforms/IPO/Inliner.h>
#include <llvm/Transforms/Scalar/Reassociate.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Scalar/DCE.h>
#include <llvm/Transforms/Scalar/InstCombine.h>
#include <lld/Common/Driver.h>
#include <llvm/Support/WithColor.h>
#endif

namespace bfc {

// Field labels have the form "name:offset". Shared by the LLVM and C backends.
static uint32_t parse_field_offset(const std::string& label) {
    size_t colon = label.find(':');
    if (colon == std::string::npos) return 0;
    try {
        return static_cast<uint32_t>(std::stoul(label.substr(colon + 1)));
    } catch (...) {
        return 0;
    }
}

class LlvmCodeGen::CodeGenImpl {
public:
    CodeGenImpl(DiagnosticSink& sink) : sink_(sink)
#ifdef BFC_LLVM_HEADERS_AVAILABLE
        , context_(std::make_unique<llvm::LLVMContext>()), builder_(std::make_unique<llvm::IRBuilder<>>(*context_))
#endif
    {
#ifdef BFC_LLVM_HEADERS_AVAILABLE
        // Initialize LLVM targets
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();
#endif
    }

    ~CodeGenImpl() = default;

    bool generate_object(const IrModule& module, const std::string& output_path) {
#ifdef BFC_LLVM_HEADERS_AVAILABLE
        return generate_object_llvm(module, output_path);
#else
        return generate_object_fallback(module, output_path);
#endif
    }

    bool link_executable(const std::vector<std::string>& object_files,
                         const std::string& output_path,
                         const std::string& entry_point) {
#ifdef BFC_LLVM_HEADERS_AVAILABLE
        return link_with_lld(object_files, output_path, entry_point);
#else
        return link_with_lld_fallback(object_files, output_path, entry_point);
#endif
    }

private:
    DiagnosticSink& sink_;

#ifdef BFC_LLVM_HEADERS_AVAILABLE
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;

    // Type mapping from BasicForge to LLVM
    llvm::Type* get_llvm_type(PrimitiveType t) {
        switch (t) {
            case PrimitiveType::Void:   return llvm::Type::getVoidTy(*context_);
            case PrimitiveType::Integer: return llvm::Type::getInt64Ty(*context_);
            case PrimitiveType::Long:   return llvm::Type::getInt64Ty(*context_);
            case PrimitiveType::Single: return llvm::Type::getFloatTy(*context_);
            case PrimitiveType::Double: return llvm::Type::getDoubleTy(*context_);
            case PrimitiveType::Boolean: return llvm::Type::getInt1Ty(*context_);
            case PrimitiveType::String: return llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0);
        }
        return llvm::Type::getInt64Ty(*context_);
    }

    llvm::Type* get_llvm_type(const TypeRef& type) {
        if (type.is_array) {
            llvm::Type* elem_type = get_llvm_type(type.kind);
            if (type.array_size && type.array_size->kind == IrValue::Kind::Constant) {
                uint64_t size = type.array_size->int_value;
                return llvm::ArrayType::get(elem_type, size);
            }
            return llvm::PointerType::get(elem_type, 0);
        }
        return get_llvm_type(type.kind);
    }

    bool generate_object_llvm(const IrModule& module, const std::string& output_path) {
        // Create LLVM module
        auto llvm_module = std::make_unique<llvm::Module>("basicforge_module", *context_);
        llvm_module->setTargetTriple("x86_64-pc-windows-msvc");
        llvm_module->setDataLayout("e-m:w-p:64:64-i64:64-f80:128-n8:16:32:64-S128");

        // Set up target machine
        std::string target_triple = "x86_64-pc-windows-msvc";
        std::string cpu = "x86-64";
        std::string features = "";

        std::string error;
        const llvm::Target* target = llvm::TargetRegistry::lookupTarget(target_triple, error);
        if (!target) {
            Diagnostic d;
            d.severity = DiagnosticSeverity::Error;
            d.message = "Failed to lookup target: " + error;
            d.location = SourceLocation{};
            sink_.emit(d);
            return false;
        }

        llvm::TargetOptions options;
        options.AllowFPOpFusion = llvm::FPOpFusion::Fast;
        options.UnsafeFPMath = false;
        options.NoInfsFPMath = false;
        options.NoNaNsFPMath = false;
        options.TrapFuncName = "__builtin_trap";
        options.LessPreciseFPMADOption = false;
        options.HonorSignDependentRoundingFPMathOption = false;
        options.UseInitArray = true;
        options.FunctionSections = true;
        options.DataSections = true;
        options.UniqueSectionNames = true;
        options.ExplicitEmits = true;
        options.NoImplicitFloat = false;

        std::unique_ptr<llvm::TargetMachine> target_machine(
            target->createTargetMachine(target_triple, cpu, features, options, llvm::Reloc::Static)
        );

        if (!target_machine) {
            Diagnostic d;
            d.severity = DiagnosticSeverity::Error;
            d.message = "Failed to create target machine";
            d.location = SourceLocation{};
            sink_.emit(d);
            return false;
        }

        llvm_module->setTargetTriple(target_triple);
        llvm_module->setDataLayout(target_machine->createDataLayout());

        // Build LLVM IR from BasicForge IR
        if (!build_llvm_ir(module, *llvm_module)) {
            return false;
        }

        // Verify module
        std::string verify_error;
        llvm::raw_string_ostream verify_stream(verify_error);
        if (llvm::verifyModule(*llvm_module, &verify_stream)) {
            Diagnostic d;
            d.severity = DiagnosticSeverity::Error;
            d.message = "Module verification failed: " + verify_error;
            d.location = SourceLocation{};
            sink_.emit(d);
            return false;
        }

        // Run optimization passes
        if (!run_optimization_passes(*llvm_module)) {
            return false;
        }

        // Emit object file
        std::error_code ec;
        llvm::raw_fd_ostream obj_stream(output_path, ec, llvm::sys::fs::OF_None);
        if (ec) {
            Diagnostic d;
            d.severity = DiagnosticSeverity::Error;
            d.message = "Failed to open output file: " + ec.message();
            d.location = SourceLocation{};
            sink_.emit(d);
            return false;
        }

        llvm::legacy::PassManager pass_manager;
        target_machine->addPassesToEmitFile(pass_manager, obj_stream, nullptr, llvm::CodeGenFileType::ObjectFile);
        pass_manager.run(*llvm_module);

        if (obj_stream.has_error()) {
            Diagnostic d;
            d.severity = DiagnosticSeverity::Error;
            d.message = "Error writing object file";
            d.location = SourceLocation{};
            sink_.emit(d);
            return false;
        }

        Diagnostic d;
        d.severity = DiagnosticSeverity::Note;
        d.message = "[bfc] Compiled to object: " + output_path;
        d.location = SourceLocation{};
        sink_.emit(d);
        return true;
    }

    bool build_llvm_ir(const IrModule& module, llvm::Module& llvm_module) {
        // Declare runtime functions
        declare_runtime_functions(llvm_module);

        // Map of BasicForge function names to LLVM functions
        std::unordered_map<std::string, llvm::Function*> fn_map;

        // First pass: declare all functions
        for (const auto& fn : module.functions) {
            std::vector<llvm::Type*> param_types;
            for (const auto& param : fn.parameters) {
                param_types.push_back(get_llvm_type(param.type));
            }
            llvm::FunctionType* fn_type = llvm::FunctionType::get(
                get_llvm_type(fn.return_type), param_types, false
            );

            llvm::Function* fn_llvm = llvm::Function::Create(
                fn_type, llvm::Function::ExternalLinkage,
                sanitize_name(fn.name), &llvm_module
            );

            // Set calling convention
            fn_llvm->setCallingConv(llvm::CallingConv::C);

            fn_map[fn.name] = fn_llvm;
        }

        // Second pass: define function bodies
        for (const auto& fn : module.functions) {
            llvm::Function* fn_llvm = fn_map[fn.name];
            if (!fn_llvm) continue;

            std::unordered_map<std::string, llvm::Value*> local_vars;
            std::unordered_map<std::string, llvm::BasicBlock*> bb_map;

            // Create all basic blocks first
            // First, collect all labels that are used as fallthrough targets (false target of Branch that is the next block)
            std::unordered_set<std::string> fallthrough_labels;
            for (const auto& bb : fn.blocks) {
                for (const auto& inst : bb.instructions) {
                    if (inst.opcode == IrOpcode::Branch) {
                        // Check if this branch's false target is the next block
                        // We'll determine this later when we know the block order
                    }
                }
            }
            
            // Create all basic blocks first
            // But skip fallthrough labels - they will be handled by fallthrough
            std::unordered_map<std::string, llvm::BasicBlock*> bb_map;
            std::unordered_set<std::string> created_blocks;
            
            // First pass: create blocks in order, but track which labels are fallthrough targets
            std::vector<std::pair<std::string, const IrBasicBlock*>> block_order;
            for (const auto& bb : fn.blocks) {
                block_order.push_back({bb.name, &bb});
            }
            
            // Identify fallthrough labels: false targets of Branch that are the next block
            std::unordered_set<std::string> fallthrough_target_labels;
            for (size_t i = 0; i + 1 < block_order.size(); ++i) {
                const auto& bb = *block_order[i].second;
                for (const auto& inst : bb.instructions) {
                    if (inst.opcode == IrOpcode::Branch) {
                        std::string false_target = inst.label2;
                        std::string next_block_name = block_order[i + 1].first;
                        if (false_target == next_block_name) {
                            fallthrough_target_labels.insert(false_target);
                        }
                    }
                }
            }
            
            // Create all basic blocks first
            for (const auto& [name, bb_ptr] : block_order) {
                llvm::BasicBlock* llvm_bb;
                if (name == "entry") {
                    llvm_bb = llvm::BasicBlock::Create(*context_, "entry", fn_llvm);
                } else {
                    llvm_bb = llvm::BasicBlock::Create(*context_, name, fn_llvm);
                }
                bb_map[name] = llvm_bb;
                created_blocks.insert(name);
            }
            
// Emit instructions for each basic block
            for (const auto& bb : fn.blocks) {
                llvm::BasicBlock* llvm_bb = bb_map[bb.name];
                builder_->SetInsertPoint(llvm_bb);

                local_vars.clear();

                // Allocate parameters in entry block
                if (bb.name == "entry") {
                    size_t arg_idx = 0;
                    for (auto& arg : fn_llvm->args()) {
                        if (arg_idx < fn.parameters.size()) {
                            const auto& param = fn.parameters[arg_idx];
                            llvm::AllocaInst* alloca = builder_->CreateAlloca(
                                get_llvm_type(param.type), nullptr, sanitize_name(param.name));
                            builder_->CreateStore(&arg, alloca);
                            local_vars[param.name] = alloca;
                            arg_idx++;
                        }
                    }
                }

                // Emit instructions
                for (const auto& inst : bb.instructions) {
                    if (!emit_instruction(inst, local_vars, fn_llvm, bb_map)) {
                        Diagnostic d;
                        d.severity = DiagnosticSeverity::Error;
                        d.message = "Failed to emit instruction: opcode " +
                            std::to_string(static_cast<uint8_t>(inst.opcode));
                        d.location = inst.loc;
                        sink_.emit(d);
                        return false;
                    }
                }
            }
        }

        if (const char* dump_path = std::getenv("BFC_DUMP_LLVM")) {
            std::error_code ec;
            llvm::raw_fd_ostream os(dump_path, ec);
            if (!ec) llvm_module.print(os, nullptr);
        }

        return true;
    }

    void declare_runtime_functions(llvm::Module& llvm_module) {
        // Declare runtime functions
        llvm::FunctionType* print_str_type = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            { llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0) }, false
        );
        llvm::Function::Create(print_str_type, llvm::Function::ExternalLinkage, "bf_print_string", &llvm_module);

        llvm::FunctionType* print_int_type = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            { llvm::Type::getInt64Ty(*context_) }, false
        );
        llvm::Function::Create(print_int_type, llvm::Function::ExternalLinkage, "bf_print_int", &llvm_module);

        llvm::FunctionType* print_double_type = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            { llvm::Type::getDoubleTy(*context_) }, false
        );
        llvm::Function::Create(print_double_type, llvm::Function::ExternalLinkage, "bf_print_double", &llvm_module);

        llvm::FunctionType* print_bool_type = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            { llvm::Type::getInt1Ty(*context_) }, false
        );
        llvm::Function::Create(print_bool_type, llvm::Function::ExternalLinkage, "bf_print_bool", &llvm_module);

        // String functions
        llvm::FunctionType* str_concat_type = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
            { llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
              llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0) }, false
        );
        llvm::Function::Create(str_concat_type, llvm::Function::ExternalLinkage, "bf_str_concat", &llvm_module);

        // Type conversion functions
        llvm::FunctionType* cint_type = llvm::FunctionType::get(
            llvm::Type::getInt64Ty(*context_),
            { llvm::Type::getDoubleTy(*context_) }, false
        );
        llvm::Function::Create(cint_type, llvm::Function::ExternalLinkage, "bf_cint", &llvm_module);

        llvm::FunctionType* clng_type = llvm::FunctionType::get(
            llvm::Type::getInt64Ty(*context_),
            { llvm::Type::getDoubleTy(*context_) }, false
        );
        llvm::Function::Create(clng_type, llvm::Function::ExternalLinkage, "bf_clng", &llvm_module);

        llvm::FunctionType* cdbl_type = llvm::FunctionType::get(
            llvm::Type::getDoubleTy(*context_),
            { llvm::Type::getInt64Ty(*context_) }, false
        );
        llvm::Function::Create(cdbl_type, llvm::Function::ExternalLinkage, "bf_cdbl", &llvm_module);

        llvm::FunctionType* cstr_type = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
            { llvm::Type::getInt64Ty(*context_) }, false
        );
        llvm::Function::Create(cstr_type, llvm::Function::ExternalLinkage, "bf_cstr", &llvm_module);

        llvm::FunctionType* cstr_double_type = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
            { llvm::Type::getDoubleTy(*context_) }, false
        );
        llvm::Function::Create(cstr_double_type, llvm::Function::ExternalLinkage, "bf_cstr_double", &llvm_module);

        llvm::FunctionType* cbool_type = llvm::FunctionType::get(
            llvm::Type::getInt1Ty(*context_),
            { llvm::Type::getInt64Ty(*context_) }, false
        );
        llvm::Function::Create(cbool_type, llvm::Function::ExternalLinkage, "bf_cbool", &llvm_module);

        llvm::FunctionType* len_type = llvm::FunctionType::get(
            llvm::Type::getInt64Ty(*context_),
            { llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0) }, false
        );
        llvm::Function::Create(len_type, llvm::Function::ExternalLinkage, "bf_len", &llvm_module);

        llvm::FunctionType* trim_type = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
            { llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0) }, false
        );
        llvm::Function::Create(trim_type, llvm::Function::ExternalLinkage, "bf_trim", &llvm_module);

        llvm::FunctionType* left_type = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
            { llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0), llvm::Type::getInt64Ty(*context_) }, false
        );
        llvm::Function::Create(left_type, llvm::Function::ExternalLinkage, "bf_left", &llvm_module);

        llvm::FunctionType* right_type = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
            { llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0), llvm::Type::getInt64Ty(*context_) }, false
        );
        llvm::Function::Create(right_type, llvm::Function::ExternalLinkage, "bf_right", &llvm_module);

        llvm::FunctionType* mid_type = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
            { llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0), llvm::Type::getInt64Ty(*context_), llvm::Type::getInt64Ty(*context_) }, false
        );
        llvm::Function::Create(mid_type, llvm::Function::ExternalLinkage, "bf_mid", &llvm_module);

        llvm::FunctionType* instr_type = llvm::FunctionType::get(
            llvm::Type::getInt64Ty(*context_),
            { llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
              llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0) }, false
        );
        llvm::Function::Create(instr_type, llvm::Function::ExternalLinkage, "bf_instr", &llvm_module);

        llvm::FunctionType* replace_type = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
            { llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
              llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
              llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0) }, false
        );
        llvm::Function::Create(replace_type, llvm::Function::ExternalLinkage, "bf_replace", &llvm_module);

        // String functions: Format, Split, Join
        llvm::FunctionType* format_type = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
            { llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0) }, true
        );
        llvm::Function::Create(format_type, llvm::Function::ExternalLinkage, "bf_format", &llvm_module);

        llvm::FunctionType* split_type = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0), 0),
            { llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
              llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
              llvm::PointerType::get(llvm::Type::getInt64Ty(*context_), 0) }, false
        );
        llvm::Function::Create(split_type, llvm::Function::ExternalLinkage, "bf_split", &llvm_module);

        llvm::FunctionType* join_type = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
            { llvm::PointerType::get(llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0), 0),
              llvm::Type::getInt64Ty(*context_),
              llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0) }, false
        );
        llvm::Function::Create(join_type, llvm::Function::ExternalLinkage, "bf_join", &llvm_module);

        // File I/O functions
        llvm::FunctionType* fopen_type = llvm::FunctionType::get(
            llvm::Type::getInt64Ty(*context_),
            { llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
              llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0) }, false
        );
        llvm::Function::Create(fopen_type, llvm::Function::ExternalLinkage, "bf_fopen", &llvm_module);

        llvm::FunctionType* fclose_type = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            { llvm::Type::getInt64Ty(*context_) }, false
        );
        llvm::Function::Create(fclose_type, llvm::Function::ExternalLinkage, "bf_fclose", &llvm_module);

        llvm::FunctionType* fprint_str_type = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            { llvm::Type::getInt64Ty(*context_), llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0) }, false
        );
        llvm::Function::Create(fprint_str_type, llvm::Function::ExternalLinkage, "bf_fprint", &llvm_module);

        llvm::FunctionType* fprint_int_type = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            { llvm::Type::getInt64Ty(*context_), llvm::Type::getInt64Ty(*context_) }, false
        );
        llvm::Function::Create(fprint_int_type, llvm::Function::ExternalLinkage, "bf_fprint_int", &llvm_module);

        llvm::FunctionType* fprint_dbl_type = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context_),
            { llvm::Type::getInt64Ty(*context_), llvm::Type::getDoubleTy(*context_) }, false
        );
        llvm::Function::Create(fprint_dbl_type, llvm::Function::ExternalLinkage, "bf_fprint_double", &llvm_module);

        llvm::FunctionType* fgetline_type = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0),
            { llvm::Type::getInt64Ty(*context_) }, false
        );
        llvm::Function::Create(fgetline_type, llvm::Function::ExternalLinkage, "bf_fgetline", &llvm_module);

        llvm::FunctionType* feof_type = llvm::FunctionType::get(
            llvm::Type::getInt64Ty(*context_),
            { llvm::Type::getInt64Ty(*context_) }, false
        );
        llvm::Function::Create(feof_type, llvm::Function::ExternalLinkage, "bf_feof", &llvm_module);
    }

    std::string sanitize_name(const std::string& name) {
        std::string out;
        for (char c : name) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_') {
                out += c;
            } else {
                out += '_';
            }
        }
        if (out.empty() || (out[0] >= '0' && out[0] <= '9')) out = "_" + out;
        return out;
    }

    uint32_t parse_field_offset(const std::string& label) {
        size_t colon = label.find(':');
        if (colon == std::string::npos) return 0;
        try {
            return static_cast<uint32_t>(std::stoul(label.substr(colon + 1)));
        } catch (...) {
            return 0;
        }
    }

    llvm::Value* emit_value(const IrValue& v,
        const std::unordered_map<std::string, llvm::Value*>& local_vars) {
        switch (v.kind) {
            case IrValue::Kind::Constant:
                if (v.type == PrimitiveType::String) {
                    return builder_->CreateGlobalStringPtr(v.str_value);
                }
                if (v.type == PrimitiveType::Integer || v.type == PrimitiveType::Long)
                    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), v.int_value);
                if (v.type == PrimitiveType::Double)
                    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context_), v.double_value);
                if (v.type == PrimitiveType::Boolean)
                    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context_), v.bool_value ? 1 : 0);
                return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), 0);
            case IrValue::Kind::Local:
            case IrValue::Kind::Parameter: {
                auto it = local_vars.find(v.name);
                if (it != local_vars.end()) {
                    llvm::Value* ptr = it->second;
                    return builder_->CreateLoad(get_llvm_type(v.type), ptr);
                }
                return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), 0);
            }
            case IrValue::Kind::VirtualRegister: {
                auto it = local_vars.find(v.name);
                if (it != local_vars.end()) {
                    // Virtual registers hold computed SSA values, not allocas.
                    // Loading from them would treat the value as a pointer.
                    return it->second;
                }
                return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), 0);
            }
        }
        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), 0);
    }

    bool emit_instruction(const IrInstruction& inst,
        std::unordered_map<std::string, llvm::Value*>& local_vars,
        llvm::Function* fn_llvm,
        const std::unordered_map<std::string, llvm::BasicBlock*>& bb_map) {
        std::string result_var = sanitize_name(inst.result.name);
        llvm::Type* result_type = get_llvm_type(inst.result.type);

        switch (inst.opcode) {
            case IrOpcode::ConstInt:
                local_vars[inst.result.name] = llvm::ConstantInt::get(
                    llvm::Type::getInt64Ty(*context_), inst.result.int_value);
                break;
            case IrOpcode::ConstString:
                local_vars[inst.result.name] = builder_->CreateGlobalStringPtr(inst.result.str_value);
                break;
            case IrOpcode::ConstBool:
                local_vars[inst.result.name] = llvm::ConstantInt::get(
                    llvm::Type::getInt1Ty(*context_), inst.result.bool_value ? 1 : 0);
                break;
            case IrOpcode::Local: {
                llvm::AllocaInst* alloca = builder_->CreateAlloca(result_type, nullptr, result_var);
                local_vars[inst.result.name] = alloca;
                break;
            }
            case IrOpcode::Store: {
                llvm::Value* val = emit_value(inst.operands[0], local_vars);
                auto it = local_vars.find(inst.result.name);
                if (it != local_vars.end()) {
                    builder_->CreateStore(val, it->second);
                }
                break;
            }
            case IrOpcode::Add:
                emit_binary_op(inst, local_vars, llvm::Instruction::Add);
                break;
            case IrOpcode::Sub:
                emit_binary_op(inst, local_vars, llvm::Instruction::Sub);
                break;
            case IrOpcode::Mul:
                emit_binary_op(inst, local_vars, llvm::Instruction::Mul);
                break;
            case IrOpcode::Div:
                emit_binary_op(inst, local_vars, inst.result.type == PrimitiveType::Double ||
                    inst.result.type == PrimitiveType::Single ?
                    llvm::Instruction::FDiv : llvm::Instruction::SDiv);
                break;
            case IrOpcode::IntDiv:
                emit_binary_op(inst, local_vars, llvm::Instruction::SDiv);
                break;
            case IrOpcode::Mod:
                emit_binary_op(inst, local_vars, llvm::Instruction::SRem);
                break;
            case IrOpcode::Negate: {
                llvm::Value* v = emit_value(inst.operands[0], local_vars);
                if (inst.result.type == PrimitiveType::Double || inst.result.type == PrimitiveType::Single) {
                    local_vars[inst.result.name] = builder_->CreateFNeg(v, result_var);
                } else {
                    local_vars[inst.result.name] = builder_->CreateNeg(v, result_var);
                }
                break;
            }
            case IrOpcode::Eq:
                emit_cmp(inst, local_vars, inst.result.type == PrimitiveType::Double ||
                    inst.result.type == PrimitiveType::Single ?
                    llvm::CmpInst::FCMP_OEQ : llvm::CmpInst::ICMP_EQ);
                break;
            case IrOpcode::Ne:
                emit_cmp(inst, local_vars, inst.result.type == PrimitiveType::Double ||
                    inst.result.type == PrimitiveType::Single ?
                    llvm::CmpInst::FCMP_ONE : llvm::CmpInst::ICMP_NE);
                break;
            case IrOpcode::Lt:
                emit_cmp(inst, local_vars, inst.result.type == PrimitiveType::Double ||
                    inst.result.type == PrimitiveType::Single ?
                    llvm::CmpInst::FCMP_OLT : llvm::CmpInst::ICMP_SLT);
                break;
            case IrOpcode::Gt:
                emit_cmp(inst, local_vars, inst.result.type == PrimitiveType::Double ||
                    inst.result.type == PrimitiveType::Single ?
                    llvm::CmpInst::FCMP_OGT : llvm::CmpInst::ICMP_SGT);
                break;
            case IrOpcode::Le:
                emit_cmp(inst, local_vars, inst.result.type == PrimitiveType::Double ||
                    inst.result.type == PrimitiveType::Single ?
                    llvm::CmpInst::FCMP_OLE : llvm::CmpInst::ICMP_SLE);
                break;
            case IrOpcode::Ge:
                emit_cmp(inst, local_vars, inst.result.type == PrimitiveType::Double ||
                    inst.result.type == PrimitiveType::Single ?
                    llvm::CmpInst::FCMP_OGE : llvm::CmpInst::ICMP_SGE);
                break;
            case IrOpcode::And:
                emit_logical_op(inst, local_vars, llvm::Instruction::And);
                break;
            case IrOpcode::Or:
                emit_logical_op(inst, local_vars, llvm::Instruction::Or);
                break;
            case IrOpcode::Xor:
                emit_logical_op(inst, local_vars, llvm::Instruction::Xor);
                break;
            case IrOpcode::Not: {
                llvm::Value* v = emit_value(inst.operands[0], local_vars);
                local_vars[inst.result.name] = builder_->CreateNot(v, result_var);
                break;
            }
            case IrOpcode::Concat: {
                llvm::Value* l = emit_value(inst.operands[0], local_vars);
                llvm::Value* r = emit_value(inst.operands[1], local_vars);
                llvm::Function* concat_fn = fn_llvm->getParent()->getFunction("bf_str_concat");
                if (!concat_fn) {
                    return false;
                }
                local_vars[inst.result.name] = builder_->CreateCall(concat_fn, {l, r}, result_var);
                break;
            }
            case IrOpcode::Call: {
                // Handle built-in type conversion and string functions
                std::string funcName = inst.label;
                llvm::Function* callee = nullptr;
                std::vector<llvm::Value*> callArgs;

                // Check for built-in conversions
                if (funcName == "CInt") callee = fn_llvm->getParent()->getFunction("bf_cint");
                else if (funcName == "CLng") callee = fn_llvm->getParent()->getFunction("bf_clng");
                else if (funcName == "CDbl") callee = fn_llvm->getParent()->getFunction("bf_cdbl");
                else if (funcName == "CStr") callee = fn_llvm->getParent()->getFunction("bf_cstr");
                else if (funcName == "CBool") callee = fn_llvm->getParent()->getFunction("bf_cbool");
                else if (funcName == "Len") callee = fn_llvm->getParent()->getFunction("bf_len");
                else if (funcName == "Trim") callee = fn_llvm->getParent()->getFunction("bf_trim");
                else if (funcName == "Left") callee = fn_llvm->getParent()->getFunction("bf_left");
                else if (funcName == "Right") callee = fn_llvm->getParent()->getFunction("bf_right");
                else if (funcName == "Mid") callee = fn_llvm->getParent()->getFunction("bf_mid");
                else if (funcName == "InStr") callee = fn_llvm->getParent()->getFunction("bf_instr");
                else if (funcName == "Replace") callee = fn_llvm->getParent()->getFunction("bf_replace");
                else if (funcName == "Format") callee = fn_llvm->getParent()->getFunction("bf_format");
                else if (funcName == "Split") callee = fn_llvm->getParent()->getFunction("bf_split");
                else if (funcName == "Join") callee = fn_llvm->getParent()->getFunction("bf_join");
                else if (funcName == "FOpen") callee = fn_llvm->getParent()->getFunction("bf_fopen");
                else if (funcName == "FClose") callee = fn_llvm->getParent()->getFunction("bf_fclose");
                else if (funcName == "FPrint") callee = fn_llvm->getParent()->getFunction("bf_fprint");
                else if (funcName == "FPrintInt") callee = fn_llvm->getParent()->getFunction("bf_fprint_int");
                else if (funcName == "FPrintDbl") callee = fn_llvm->getParent()->getFunction("bf_fprint_double");
                else if (funcName == "FGetLine") callee = fn_llvm->getParent()->getFunction("bf_fgetline");
                else if (funcName == "FEof") callee = fn_llvm->getParent()->getFunction("bf_feof");
                else callee = fn_llvm->getParent()->getFunction(sanitize_name(funcName));

                if (!callee) {
                    return false;
                }

                for (const auto& op : inst.operands) {
                    callArgs.push_back(emit_value(op, local_vars));
                }

                llvm::Value* result = builder_->CreateCall(callee, callArgs, result_var);
                if (inst.result.type != PrimitiveType::Void) {
                    local_vars[inst.result.name] = result;
                }
                break;
            }
            case IrOpcode::CallBuiltin: {
                // Handle print builtins
                llvm::Value* arg = emit_value(inst.operands[0], local_vars);
                std::string funcName = "bf_print_string";
                if (inst.operands[0].type == PrimitiveType::Integer || inst.operands[0].type == PrimitiveType::Long)
                    funcName = "bf_print_int";
                else if (inst.operands[0].type == PrimitiveType::Double ||
                         inst.operands[0].type == PrimitiveType::Single)
                    funcName = "bf_print_double";
                else if (inst.operands[0].type == PrimitiveType::Boolean)
                    funcName = "bf_print_bool";

                llvm::Function* fn = fn_llvm->getParent()->getFunction(funcName);
                if (!fn) {
                    return false;
                }
                builder_->CreateCall(fn, {arg});
                break;
            }
            case IrOpcode::CallVirt: {
                std::string funcName = sanitize_name(inst.label);
                llvm::Function* fn = fn_llvm->getParent()->getFunction(funcName);
                if (!fn) {
                    return false;
                }
                if (inst.result.type != PrimitiveType::Void) {
                    local_vars[inst.result.name] = builder_->CreateCall(fn, {}, result_var);
                } else {
                    builder_->CreateCall(fn, {});
                }
                break;
            }
            case IrOpcode::TryStart:
                // TryStart marks beginning of try block - no IR needed for C fallback,
                // LLVM uses landingpad/invoke
                break;
            case IrOpcode::TryEnd:
                // TryEnd marks normal end of try
                break;
            case IrOpcode::CatchStart:
                // CatchStart marks beginning of catch block
                break;
            case IrOpcode::CatchEnd:
                // CatchEnd marks end of catch block
                break;
            case IrOpcode::Throw: {
                // Throw exception - call bf_throw with exception value
                if (!inst.operands.empty()) {
                    llvm::Value* arg = emit_value(inst.operands[0], local_vars);
                    llvm::Function* fn = fn_llvm->getParent()->getFunction("bf_throw");
                    if (!fn) return false;
                    builder_->CreateCall(fn, {arg});
                } else {
                    llvm::Function* fn = fn_llvm->getParent()->getFunction("bf_throw");
                    if (!fn) return false;
                    builder_->CreateCall(fn, {llvm::ConstantPointerNull::get(llvm::PointerType::get(llvm::Type::getInt8Ty(*context_), 0))});
                }
                break;
            }
            case IrOpcode::FinallyStart:
            case IrOpcode::FinallyEnd:
                // Finally markers - no code needed for simple implementation
                break;
            case IrOpcode::Branch: {
                llvm::Value* cond = emit_value(inst.operands[0], local_vars);
                llvm::BasicBlock* true_bb = bb_map.at(inst.label);
                llvm::BasicBlock* false_bb = bb_map.at(inst.label2);
                // Simply create the conditional branch with both targets
                // LLVM will handle fallthrough optimization at the machine code level
                builder_->CreateCondBr(cond, true_bb, false_bb);
                break;
            }
            case IrOpcode::Jump: {
                llvm::BasicBlock* target_bb = bb_map.at(inst.label);
                builder_->CreateBr(target_bb);
                break;
            }
            case IrOpcode::Label:
                // Label is handled by SetInsertPoint before emitting block instructions
                break;
            case IrOpcode::Return: {
                if (inst.operands.empty()) {
                    builder_->CreateRetVoid();
                } else {
                    llvm::Value* val = emit_value(inst.operands[0], local_vars);
                    builder_->CreateRet(val);
                }
                break;
            }
            case IrOpcode::ArrayAlloc: {
                llvm::Value* size = emit_value(inst.operands[0], local_vars);
                llvm::Value* elem_count = builder_->CreateAdd(size, builder_->getInt64(1), "elem_count");
                llvm::Type* elem_type = get_llvm_type(inst.operands[0].type);
                llvm::Value* alloc_size = builder_->CreateMul(elem_count,
                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_),
                        get_type_size(elem_type)), "alloc_size");
                llvm::Value* ptr = builder_->CreateCall(
                    fn_llvm->getParent()->getFunction("malloc"),
                    {alloc_size}, "array_ptr");
                local_vars[inst.result.name] = ptr;
                break;
            }
            case IrOpcode::ArrayStore: {
                llvm::Value* base = local_vars.at(inst.operands[0].name);
                llvm::Value* index = emit_value(inst.operands[1], local_vars);
                llvm::Value* val = emit_value(inst.operands[2], local_vars);
                llvm::Value* adjusted_index = builder_->CreateAdd(index, builder_->getInt64(1), "adj_index");
                llvm::Value* elem_ptr = builder_->CreateGEP(
                    get_llvm_type(inst.operands[2].type), base, adjusted_index, "elem_ptr");
                builder_->CreateStore(val, elem_ptr);
                break;
            }
            case IrOpcode::ArrayLoad: {
                llvm::Value* base = local_vars.at(inst.operands[0].name);
                llvm::Value* index = emit_value(inst.operands[1], local_vars);
                llvm::Value* adjusted_index = builder_->CreateAdd(index, builder_->getInt64(1), "adj_index");
                llvm::Value* elem_ptr = builder_->CreateGEP(
                    get_llvm_type(inst.result.type), base, adjusted_index, "elem_ptr");
                llvm::Value* val = builder_->CreateLoad(get_llvm_type(inst.result.type), elem_ptr, "loaded_val");
                local_vars[inst.result.name] = val;
                break;
            }
            case IrOpcode::ArrayLen: {
                llvm::Value* base = local_vars.at(inst.operands[0].name);
                llvm::Value* len_ptr = builder_->CreateGEP(
                    llvm::Type::getInt64Ty(*context_), base, builder_->getInt64(0), "len_ptr");
                llvm::Value* len = builder_->CreateLoad(llvm::Type::getInt64Ty(*context_), len_ptr, "array_len");
                local_vars[inst.result.name] = len;
                break;
            }
            case IrOpcode::ObjAlloc: {
                llvm::Value* obj_size = builder_->getInt64(64);
                llvm::Value* obj_ptr = builder_->CreateCall(
                    fn_llvm->getParent()->getFunction("malloc"),
                    {obj_size}, "obj_ptr");
                // Object references are represented as i64 in the IR so they
                // can be passed through the integer "Me" parameter.
                local_vars[inst.result.name] = builder_->CreatePtrToInt(
                    obj_ptr, llvm::Type::getInt64Ty(*context_), "obj_addr");
                break;
            }
            case IrOpcode::FieldStore: {
                uint32_t offset = parse_field_offset(inst.label);
                llvm::Value* obj_addr = emit_value(inst.operands[0], local_vars);
                llvm::Value* obj_ptr = builder_->CreateIntToPtr(
                    obj_addr, llvm::Type::getInt8PtrTy(*context_), "obj_ptr");
                llvm::Value* field_ptr = builder_->CreateGEP(
                    llvm::Type::getInt8Ty(*context_), obj_ptr,
                    builder_->getInt64(offset), "field_ptr");
                llvm::Value* val = emit_value(inst.operands[1], local_vars);
                builder_->CreateStore(val, field_ptr);
                break;
            }
            case IrOpcode::FieldLoad: {
                uint32_t offset = parse_field_offset(inst.label);
                llvm::Value* obj_addr = emit_value(inst.operands[0], local_vars);
                llvm::Value* obj_ptr = builder_->CreateIntToPtr(
                    obj_addr, llvm::Type::getInt8PtrTy(*context_), "obj_ptr");
                llvm::Value* field_ptr = builder_->CreateGEP(
                    llvm::Type::getInt8Ty(*context_), obj_ptr,
                    builder_->getInt64(offset), "field_ptr");
                llvm::Value* val = builder_->CreateLoad(
                    get_llvm_type(inst.result.type), field_ptr, "loaded_field");
                local_vars[inst.result.name] = val;
                break;
            }
            default:
                // Unknown opcode - skip
                break;
        }
        return true;
    }

    void emit_binary_op(const IrInstruction& inst,
        std::unordered_map<std::string, llvm::Value*>& local_vars,
        llvm::Instruction::BinaryOps op) {
        llvm::Value* l = emit_value(inst.operands[0], local_vars);
        llvm::Value* r = emit_value(inst.operands[1], local_vars);
        bool isFloat = (inst.result.type == PrimitiveType::Double || inst.result.type == PrimitiveType::Single);

        if (isFloat) {
            switch (op) {
                case llvm::Instruction::Add: local_vars[inst.result.name] = builder_->CreateFAdd(l, r, sanitize_name(inst.result.name)); break;
                case llvm::Instruction::Sub: local_vars[inst.result.name] = builder_->CreateFSub(l, r, sanitize_name(inst.result.name)); break;
                case llvm::Instruction::Mul: local_vars[inst.result.name] = builder_->CreateFMul(l, r, sanitize_name(inst.result.name)); break;
                case llvm::Instruction::FDiv: local_vars[inst.result.name] = builder_->CreateFDiv(l, r, sanitize_name(inst.result.name)); break;
                case llvm::Instruction::SRem: local_vars[inst.result.name] = builder_->CreateFRem(l, r, sanitize_name(inst.result.name)); break;
                default: break;
            }
        } else {
            switch (op) {
                case llvm::Instruction::Add: local_vars[inst.result.name] = builder_->CreateAdd(l, r, sanitize_name(inst.result.name)); break;
                case llvm::Instruction::Sub: local_vars[inst.result.name] = builder_->CreateSub(l, r, sanitize_name(inst.result.name)); break;
                case llvm::Instruction::Mul: local_vars[inst.result.name] = builder_->CreateMul(l, r, sanitize_name(inst.result.name)); break;
                case llvm::Instruction::SDiv: local_vars[inst.result.name] = builder_->CreateSDiv(l, r, sanitize_name(inst.result.name)); break;
                case llvm::Instruction::SRem: local_vars[inst.result.name] = builder_->CreateSRem(l, r, sanitize_name(inst.result.name)); break;
                default: break;
            }
        }
    }

    void emit_cmp(const IrInstruction& inst,
        std::unordered_map<std::string, llvm::Value*>& local_vars,
        llvm::CmpInst::Predicate pred) {
        llvm::Value* l = emit_value(inst.operands[0], local_vars);
        llvm::Value* r = emit_value(inst.operands[1], local_vars);
        local_vars[inst.result.name] = builder_->CreateICmp(pred, l, r, sanitize_name(inst.result.name));
    }

    void emit_logical_op(const IrInstruction& inst,
        std::unordered_map<std::string, llvm::Value*>& local_vars,
        llvm::Instruction::BinaryOps op) {
        llvm::Value* l = emit_value(inst.operands[0], local_vars);
        llvm::Value* r = emit_value(inst.operands[1], local_vars);
        switch (op) {
            case llvm::Instruction::And: local_vars[inst.result.name] = builder_->CreateAnd(l, r, sanitize_name(inst.result.name)); break;
            case llvm::Instruction::Or: local_vars[inst.result.name] = builder_->CreateOr(l, r, sanitize_name(inst.result.name)); break;
            case llvm::Instruction::Xor: local_vars[inst.result.name] = builder_->CreateXor(l, r, sanitize_name(inst.result.name)); break;
            default: break;
        }
    }

    uint64_t get_type_size(llvm::Type* type) {
        if (auto* intTy = llvm::dyn_cast<llvm::IntegerType>(type)) {
            return (intTy->getBitWidth() + 7) / 8;
        }
        if (auto* ptrTy = llvm::dyn_cast<llvm::PointerType>(type)) {
            return 8; // 64-bit pointers
        }
        if (auto* arrTy = llvm::dyn_cast<llvm::ArrayType>(type)) {
            return arrTy->getNumElements() * get_type_size(arrTy->getElementType());
        }
        return 8;
    }

    bool run_optimization_passes(llvm::Module& module) {
        llvm::legacy::PassManager pass_manager;
        llvm::PassManagerBuilder builder;
        builder.OptLevel = 2;
        builder.SizeLevel = 0;
        builder.LoopVectorize = true;
        builder.SLPVectorize = true;
        builder.Inliner = llvm::createFunctionInliningPass(225, 0, false);
        builder.populateModulePassManager(pass_manager);
        pass_manager.run(*module);
        return true;
    }

    bool link_with_lld(const std::vector<std::string>& object_files,
        const std::string& output_path,
        const std::string& entry_point) {
        // Use LLD library API directly
        std::vector<const char*> args;
        args.push_back("lld-link");
        args.push_back("/nologo");
        args.push_back("/debug");
        args.push_back(("/out:" + output_path).c_str());
        args.push_back(("/entry:" + entry_point).c_str());
        args.push_back("/subsystem:console");

        // Library paths
        args.push_back("/libpath:C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207\\lib\\x64");
        args.push_back("/libpath:C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207\\lib\\onecore\\x64");
        args.push_back("/libpath:C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.26100.0\\ucrt\\x64");
        args.push_back("/libpath:C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.26100.0\\um\\x64");

        // Required libraries
        args.push_back("legacy_stdio_definitions.lib");
        
        args.push_back("libvcruntime.lib");
        args.push_back("kernel32.lib");
        
        args.push_back("libvcruntime.lib");
        args.push_back("kernel32.lib");
        args.push_back("user32.lib");
        args.push_back("gdi32.lib");
        args.push_back("shell32.lib");
        args.push_back("comctl32.lib");
        args.push_back("ole32.lib");

        // Object files
        for (const auto& obj : object_files) {
            args.push_back(obj.c_str());
        }
        args.push_back("libvcruntime.lib");
        args.push_back("kernel32.lib");
        args.push_back("user32.lib");
        args.push_back("gdi32.lib");
        args.push_back("shell32.lib");
        args.push_back("comctl32.lib");
        args.push_back("ole32.lib");

        args.push_back(nullptr);

        std::string stdout_str;
        std::string stderr_str;
        llvm::raw_string_ostream stdout_stream(stdout_str);
        llvm::raw_string_ostream stderr_stream(stderr_str);

        bool success = lld::coff::link(llvm::ArrayRef<const char*>(args.data(), args.size() - 1),
            stdout_stream, stderr_stream);

        stdout_stream.flush();
        stderr_stream.flush();

        if (!success) {
            Diagnostic d;
            d.severity = DiagnosticSeverity::Error;
            d.message = "LLD linking failed: " + stderr_str;
            d.location = SourceLocation{};
            sink_.emit(d);
            return false;
        }

        Diagnostic d;
        d.severity = DiagnosticSeverity::Note;
        d.message = "[bfc] Linked successfully: " + output_path;
        d.location = SourceLocation{};
        sink_.emit(d);
        return true;
    }
#endif

    // Fallback implementation using C code emission (compiled when LLVM headers not available)
    bool generate_object_fallback(const IrModule& module, const std::string& output_path) {
        std::string c_path = output_path;
        size_t dot = c_path.rfind('.');
        if (dot != std::string::npos) {
            c_path = c_path.substr(0, dot) + ".c";
        } else {
            c_path += ".c";
        }

        std::string c_code = generate_c(module);

        std::ofstream out(c_path);
        if (!out) {
            Diagnostic d;
            d.severity = DiagnosticSeverity::Error;
            d.message = "Failed to open C output file: " + c_path;
            d.location = SourceLocation{};
            sink_.emit(d);
            return false;
        }
        out << c_code;
        out.close();

        std::string clang = bfc::find_llvm_tool("clang.exe");

        std::vector<std::string> args;
        args.push_back(clang);
        args.push_back("-target");
        args.push_back("x86_64-pc-windows-msvc");
        args.push_back("-c");
        args.push_back(c_path);
        args.push_back("-o");
        args.push_back(output_path);
        args.push_back("-O2");
        // Object references are passed as int64_t and cast back to pointers.
        // Keep malloc opaque so the optimizer cannot mis-handle that round-trip.
        args.push_back("-fno-builtin-malloc");
        args.push_back("-g");  // CodeView debug info for PDB support

        return run_command_bat(args);
    }

    // C code generation (fallback)
    std::string c_type(PrimitiveType t) {
        switch (t) {
            case PrimitiveType::Void:   return "void";
            case PrimitiveType::Integer: return "int64_t";
            case PrimitiveType::Long:   return "int64_t";
            case PrimitiveType::Single: return "float";
            case PrimitiveType::Double: return "double";
            case PrimitiveType::Boolean: return "int";
            case PrimitiveType::String: return "char*";
        }
        return "int64_t";
    }

    std::string sanitize_name_c(const std::string& name) {
        std::string out;
        for (char c : name) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_') {
                out += c;
            } else {
                out += '_';
            }
        }
        if (out.empty() || (out[0] >= '0' && out[0] <= '9')) out = "_" + out;
        return out;
    }

    std::string value_expr_c(const IrValue& v, const std::unordered_map<std::string, std::string>& local_vars) {
        switch (v.kind) {
            case IrValue::Kind::Constant:
                if (v.type == PrimitiveType::String) {
                    return "\"" + escape_c_string(v.str_value) + "\"";
                }
                if (v.type == PrimitiveType::Integer || v.type == PrimitiveType::Long)
                    return std::to_string(v.int_value);
                if (v.type == PrimitiveType::Double)
                    return std::to_string(v.double_value);
                if (v.type == PrimitiveType::Boolean)
                    return v.bool_value ? "1" : "0";
                return "0";
            case IrValue::Kind::Local:
            case IrValue::Kind::Parameter: {
                auto it = local_vars.find(v.name);
                if (it != local_vars.end()) return it->second;
                return "0";
            }
            case IrValue::Kind::VirtualRegister: {
                auto it = local_vars.find(v.name);
                if (it != local_vars.end()) return it->second;
                return sanitize_name_c(v.name);
            }
        }
        return "0";
    }

    std::string escape_c_string(const std::string& s) {
        std::string out;
        for (char c : s) {
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '"':  out += "\\\""; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default: out += c; break;
            }
        }
        return out;
    }

    std::string generate_c(const IrModule& module) {
        std::string out;

        out += "// BasicForge-generated C code\n";
out += "// CODEGEN_V2_MARKER_FINAL\n";
        out += "#include <stdio.h>\n";
        out += "#include <string.h>\n";
        out += "#include <stdint.h>\n";
        out += "#include <ctype.h>\n";
        out += "#include <io.h>\n";
        out += "#include <setjmp.h>\n";
        out += "#include <stdarg.h>\n";
        out += "#include <stdlib.h>\n";
        out += "#include <math.h>\n";
        out += "#ifndef WINAPI_FAMILY_PARTITION\n";
        out += "#define WINAPI_FAMILY_PARTITION(x) 1\n";
        out += "#endif\n";
        out += "\n";

        out += "void bf_print_string(const char* s);\n";
        out += "void bf_print_int(int64_t v);\n";
        out += "void bf_print_double(double v);\n";
        out += "void bf_print_bool(int v);\n";
        out += "char* bf_str_concat(char*, char*);\n";
        out += "int64_t bf_cint(double);\n";
        out += "int64_t bf_clng(double);\n";
        out += "double bf_cdbl(int64_t);\n";
        out += "char* bf_cstr(int64_t);\n";
    out += "char* bf_cstr_double(double);\n";
        out += "int bf_cbool(int64_t);\n";
        out += "int64_t bf_len(const char*);\n";
        out += "char* bf_trim(char*);\n";
        out += "char* bf_left(char*, int64_t);\n";
        out += "char* bf_right(char*, int64_t);\n";
        out += "char* bf_mid(char*, int64_t, int64_t);\n";
        out += "int64_t bf_instr(const char*, const char*);\n";
        out += "char* bf_replace(char*, char*, char*);\n";
        out += "char* bf_format(const char*, ...);\n";
        out += "double bf_sqr(double);\n";
        out += "double bf_abs(double);\n";
        out += "int64_t bf_sgn(double);\n";
        out += "double bf_fix(double);\n";
        out += "double bf_int(double);\n";
        out += "double bf_rnd(void);\n";
        // Form Designer runtime API
        out += "int64_t bf_form_new(const char*, int, int);\n";
        out += "int bf_button_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_label_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_textbox_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_checkbox_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_radiobutton_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_groupbox_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_panel_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_picturebox_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_listbox_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_combobox_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_trackbar_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_progressbar_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_datetimepicker_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_listview_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_treeview_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_richtextbox_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_tabcontrol_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_toolbar_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_statusbar_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_updown_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_monthcalendar_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_scrollbar_new(int64_t, const char*, int, int, int, int);\n";
        out += "int bf_hotkey_new(int64_t, const char*, int, int, int, int);\n";
        out += "void bf_set_click(int, int);\n";
        out += "void bf_set_textchanged(int, int);\n";
        out += "void bf_set_doubleclick(int, int);\n";
        out += "void bf_set_selection(int, int);\n";
        out += "void bf_set_scroll(int, int);\n";
        out += "void bf_set_focus(int, int);\n";
        out += "void bf_set_blur(int, int);\n";
        out += "void bf_set_keydown(int, int);\n";
        out += "void bf_set_keyup(int, int);\n";
        out += "void bf_set_mousemove(int, int);\n";
        out += "void bf_set_mousewheel(int, int);\n";
        out += "void bf_set_paint(int, int);\n";
        out += "void bf_set_drop(int, int);\n";
        out += "char* bf_get_dropped_file(void);\n";
        out += "void bf_form_run(int64_t);\n";
        out += "char* bf_get_text(int);\n";
        out += "void bf_set_text(int, const char*);\n";
        out += "void bf_ui_set_name(int, const char*);\n";
        out += "char* bf_get_text_name(const char*);\n";
        out += "void bf_set_text_name(const char*, const char*);\n";
        out += "int bf_form_test_click(int);\n";
        out += "int bf_form_close_first(void);\n";
        out += "int bf_file_exists(const char*);\n";
        out += "char* bf_read_all_text(const char*);\n";
        out += "int bf_write_all_text(const char*, const char*);\n";
        out += "int bf_append_all_text(const char*, const char*);\n";
        out += "char* bf_get_env(const char*);\n";
        out += "int bf_set_env(const char*, const char*);\n";
        out += "void bf_sleep(int);\n";
        out += "void bf_beep(int, int);\n";
        out += "void bf_clear_screen(void);\n";
        out += "void bf_set_title(const char*);\n";
        out += "char* bf_read_line(void);\n";
        out += "void bf_message_box(const char*, const char*);\n";
        out += "void bf_shell_execute(const char*);\n";
        out += "char* bf_clipboard_get(void);\n";
        out += "void bf_clipboard_set(const char*);\n";
        out += "char* bf_upper(const char*);\n";
        out += "char* bf_lower(const char*);\n";
        out += "char* bf_chr(int);\n";
        out += "int bf_asc(const char*);\n";
        out += "int64_t bf_year(double);\n";
        out += "int64_t bf_month(double);\n";
        out += "int64_t bf_day(double);\n";
        out += "double bf_now(void);\n";
        out += "static char* bf_str_copy(char*, const char*);\n";
        out += "static int bf_str_len(const char*);\n";
        out += "\n";
        out += "void bf_throw(const char* msg);\n";
        out += "int bf_try_begin(void);\n";
        out += "void bf_try_end(void);\n";
        out += "void bf_catch_begin(void);\n";
        out += "void bf_catch_end(void);\n";
        out += "void bf_finally_begin(void);\n";
        out += "void bf_finally_end(void);\n";
        out += "\n";
        // String functions
        out += "char** bf_split(const char* s, const char* delim, int* out_count);\n";
        out += "\n";
        // File I/O functions
        out += "int bf_fopen(const char* filename, const char* mode);\n";
        out += "void bf_fclose(int handle);\n";
        out += "void bf_fprint(int handle, const char* s);\n";
        out += "void bf_fprint_int(int handle, int64_t v);\n";
        out += "void bf_fprint_double(int handle, double v);\n";
        out += "char* bf_fgetline(int handle);\n";
        out += "int bf_feof(int handle);\n";
        out += "\n";

        for (const auto& fn : module.functions) {
            out += c_type(fn.return_type) + " " + sanitize_name_c(fn.name) + "(";
            for (size_t i = 0; i < fn.parameters.size(); ++i) {
                if (i) out += ", ";
                out += (fn.parameters[i].is_array ? "int64_t*" : c_type(fn.parameters[i].type))
                       + " " + sanitize_name_c(fn.parameters[i].name);
            }
            out += ");\n";
        }
        out += "\n";

        int line_counter = 1;
        for (const auto& fn : module.functions) {
            out += "#line " + std::to_string(line_counter) + " \"" + module.name + ".bf\"\n";
            out += c_type(fn.return_type) + " " + sanitize_name_c(fn.name) + "(";
            for (size_t i = 0; i < fn.parameters.size(); ++i) {
                if (i) out += ", ";
                out += (fn.parameters[i].is_array ? "int64_t*" : c_type(fn.parameters[i].type))
                       + " " + sanitize_name_c(fn.parameters[i].name);
            }
            out += ") {\n";

            std::unordered_map<std::string, std::string> local_vars;
            for (const auto& p : fn.parameters) {
                local_vars[p.name] = sanitize_name_c(p.name);
            }

            for (const auto& bb : fn.blocks) {
                if (!bb.name.empty() && bb.name != "entry") {
                    out += bb.name + ":\n";
                }
                for (const auto& inst : bb.instructions) {
                    out += emit_instruction_c(inst, local_vars, fn.return_type);
                    line_counter++;
                }
            }
            out += "}\n\n";
            line_counter++;
        }

        if (!module.forms.empty()) {
            out += "void bf_dispatch_event(int id) {\n";
            for (const auto& form : module.forms) {
                for (const auto& ev : form.event_ids) {
                    out += "  if (id == " + std::to_string(ev.first) + ") { " +
                           ev.second + "(); return; }\n";
                }
            }
            out += "}\n\n";
        }

        return out;
    }

    std::string emit_instruction_c(const IrInstruction& inst,
        std::unordered_map<std::string, std::string>& local_vars,
        PrimitiveType fn_return_type = PrimitiveType::Void) {
        std::string out;
        std::string result_var = sanitize_name_c(inst.result.name);
        std::string result_type = c_type(inst.result.type);

        // Ensure all virtual register operands are declared
        for (const auto& op : inst.operands) {
            if (op.kind == IrValue::Kind::VirtualRegister) {
                if (local_vars.find(op.name) == local_vars.end()) {
                    out += "  int " + sanitize_name_c(op.name) + ";\n";
                    local_vars[op.name] = sanitize_name_c(op.name);
                }
            }
        }

        switch (inst.opcode) {
            case IrOpcode::Local: {
                if (local_vars.find(inst.result.name) == local_vars.end()) {
                    out += "  " + result_type + " " + result_var + ";\n";
                    local_vars[inst.result.name] = result_var;
                }
                break;
            }
            case IrOpcode::Store: {
                std::string val = value_expr_c(inst.operands[0], local_vars);
                auto it = local_vars.find(inst.result.name);
                if (it != local_vars.end()) {
                    out += "  " + it->second + " = (" + result_type + ")(" + val + ");\n";
                }
                break;
            }
            case IrOpcode::Add:
            case IrOpcode::Sub:
            case IrOpcode::Mul:
            case IrOpcode::Div:
            case IrOpcode::IntDiv:
            case IrOpcode::Mod: {
                std::string l = value_expr_c(inst.operands[0], local_vars);
                std::string r = value_expr_c(inst.operands[1], local_vars);
                std::string op;
                switch (inst.opcode) {
                    case IrOpcode::Add: op = "+"; break;
                    case IrOpcode::Sub: op = "-"; break;
                    case IrOpcode::Mul: op = "*"; break;
                    case IrOpcode::Div: op = "/"; break;
                    case IrOpcode::IntDiv: op = "/"; break;
                    case IrOpcode::Mod: op = "%"; break;
                    default: op = "+"; break;
                }
                out += "  " + result_type + " " + result_var + " = (" + result_type + ")(" + l + " " + op + " " + r + ");\n";
                local_vars[inst.result.name] = result_var;
                break;
            }
            case IrOpcode::Eq:
            case IrOpcode::Ne:
            case IrOpcode::Lt:
            case IrOpcode::Gt:
            case IrOpcode::Le:
            case IrOpcode::Ge: {
                std::string l = value_expr_c(inst.operands[0], local_vars);
                std::string r = value_expr_c(inst.operands[1], local_vars);
                std::string op;
                switch (inst.opcode) {
                    case IrOpcode::Eq: op = "=="; break;
                    case IrOpcode::Ne: op = "!="; break;
                    case IrOpcode::Lt: op = "<"; break;
                    case IrOpcode::Gt: op = ">"; break;
                    case IrOpcode::Le: op = "<="; break;
                    case IrOpcode::Ge: op = ">="; break;
                    default: op = "=="; break;
                }
                out += "  int " + result_var + " = (" + l + " " + op + " " + r + ");\n";
                local_vars[inst.result.name] = result_var;
                break;
            }
            case IrOpcode::And:
            case IrOpcode::Or:
            case IrOpcode::Xor: {
                std::string l = value_expr_c(inst.operands[0], local_vars);
                std::string r = value_expr_c(inst.operands[1], local_vars);
                std::string op;
                switch (inst.opcode) {
                    case IrOpcode::And: op = "&&"; break;
                    case IrOpcode::Or:  op = "||"; break;
                    case IrOpcode::Xor: op = "^"; break;
                    default: op = "&&"; break;
                }
                out += "  int " + result_var + " = (" + l + " " + op + " " + r + ");\n";
                local_vars[inst.result.name] = result_var;
                break;
            }
            case IrOpcode::Negate: {
                std::string v = value_expr_c(inst.operands[0], local_vars);
                out += "  " + result_type + " " + result_var + " = -(" + v + ");\n";
                local_vars[inst.result.name] = result_var;
                break;
            }
            case IrOpcode::Not: {
                std::string v = value_expr_c(inst.operands[0], local_vars);
                out += "  int " + result_var + " = !(" + v + ");\n";
                local_vars[inst.result.name] = result_var;
                break;
            }
            case IrOpcode::Concat: {
                std::string l = value_expr_c(inst.operands[0], local_vars);
                std::string r = value_expr_c(inst.operands[1], local_vars);
                out += "  " + result_type + " " + result_var + " = bf_str_concat(" + l + ", " + r + ");\n";
                local_vars[inst.result.name] = result_var;
                break;
            }
            case IrOpcode::CallBuiltin: {
                std::string arg = value_expr_c(inst.operands[0], local_vars);
                std::string func = "bf_print_string";
                if (inst.operands[0].type == PrimitiveType::Integer || inst.operands[0].type == PrimitiveType::Long)
                    func = "bf_print_int";
                else if (inst.operands[0].type == PrimitiveType::Double ||
                         inst.operands[0].type == PrimitiveType::Single)
                    func = "bf_print_double";
                else if (inst.operands[0].type == PrimitiveType::Boolean)
                    func = "bf_print_bool";
                out += "  " + func + "(" + arg + ");\n";
                break;
            }
            case IrOpcode::Call: {
                std::string funcName = inst.label;
                if (funcName == "CInt") funcName = "bf_cint";
                else if (funcName == "CLng") funcName = "bf_clng";
                else if (funcName == "CDbl") funcName = "bf_cdbl";
                else                 if (funcName == "CStr") funcName = "bf_cstr";
                else if (funcName == "CStrDouble") funcName = "bf_cstr_double";
                else if (funcName == "FileExists") funcName = "bf_file_exists";
                else if (funcName == "ReadAllText") funcName = "bf_read_all_text";
                else if (funcName == "WriteAllText") funcName = "bf_write_all_text";
                else if (funcName == "AppendAllText") funcName = "bf_append_all_text";
                else if (funcName == "GetEnv") funcName = "bf_get_env";
                else if (funcName == "SetEnv") funcName = "bf_set_env";
                else if (funcName == "Sleep") funcName = "bf_sleep";
                else if (funcName == "Beep") funcName = "bf_beep";
                else if (funcName == "Clear") funcName = "bf_clear_screen";
                else if (funcName == "SetTitle") funcName = "bf_set_title";
                else if (funcName == "ReadLine") funcName = "bf_read_line";
                else if (funcName == "MessageBox") funcName = "bf_message_box";
                else if (funcName == "ShellExecute") funcName = "bf_shell_execute";
                else if (funcName == "GetClipboard") funcName = "bf_clipboard_get";
                else if (funcName == "SetClipboard") funcName = "bf_clipboard_set";
                else if (funcName == "UCase") funcName = "bf_upper";
                else if (funcName == "LCase") funcName = "bf_lower";
                else if (funcName == "Chr") funcName = "bf_chr";
                else if (funcName == "Asc") funcName = "bf_asc";
                else if (funcName == "CBool") funcName = "bf_cbool";
                else if (funcName == "Len") funcName = "bf_len";
                else if (funcName == "Trim") funcName = "bf_trim";
                else if (funcName == "Left") funcName = "bf_left";
                else if (funcName == "Right") funcName = "bf_right";
                else if (funcName == "Mid") funcName = "bf_mid";
                else if (funcName == "InStr") funcName = "bf_instr";
                else if (funcName == "Replace") funcName = "bf_replace";
                else if (funcName == "Format") funcName = "bf_format";
                else if (funcName == "Split") funcName = "bf_split";
                else if (funcName == "Join") funcName = "bf_join";
                else if (funcName == "Sqr") funcName = "bf_sqr";
                else if (funcName == "Abs") funcName = "bf_abs";
                else if (funcName == "Sgn") funcName = "bf_sgn";
                else if (funcName == "Fix") funcName = "bf_fix";
                else if (funcName == "Int") funcName = "bf_int";
                else if (funcName == "Rnd") funcName = "bf_rnd";
                else if (funcName == "Year") funcName = "bf_year";
                else if (funcName == "Month") funcName = "bf_month";
                else if (funcName == "Day") funcName = "bf_day";
                else if (funcName == "Now") funcName = "bf_now";
                else if (funcName == "FOpen") funcName = "bf_fopen";
                else if (funcName == "FClose") funcName = "bf_fclose";
                else if (funcName == "FPrint") funcName = "bf_fprint";
                else if (funcName == "FPrintInt") funcName = "bf_fprint_int";
                else if (funcName == "FPrintDbl") funcName = "bf_fprint_double";
                else if (funcName == "FGetLine") funcName = "bf_fgetline";
                else if (funcName == "FEof") funcName = "bf_feof";

                std::vector<std::string> args;
                for (const auto& op : inst.operands) {
                    args.push_back(value_expr_c(op, local_vars));
                }

                std::string argList;
                for (size_t i = 0; i < args.size(); ++i) {
                    if (i) argList += ", ";
                    argList += args[i];
                }

                if (inst.result.type != PrimitiveType::Void) {
                    out += "  " + result_type + " " + result_var + " = " + funcName + "(" + argList + ");\n";
                    local_vars[inst.result.name] = result_var;
                } else {
                    out += "  " + funcName + "(" + argList + ");\n";
                }
                break;
            }
            case IrOpcode::CallVirt: {
                std::string funcName = sanitize_name_c(inst.label);
                out += "  " + funcName + "();\n";
                break;
            }
            case IrOpcode::Branch: {
                std::string cond = value_expr_c(inst.operands[0], local_vars);
                // Emit both targets explicitly; the IR does not guarantee that
                // the true block is the next emitted block.
                if (!inst.label.empty()) {
                    out += "  if (" + cond + ") goto " + inst.label + ";\n";
                }
                if (!inst.label2.empty()) {
                    out += "  goto " + inst.label2 + ";\n";
                }
                break;
            }
            case IrOpcode::Jump: {
                out += "  goto " + inst.label + ";\n";
                break;
            }
            case IrOpcode::Label: {
                out += inst.label + ":\n";
                out += ";\n";
                break;
            }
            case IrOpcode::Return: {
                if (inst.operands.empty()) {
                    out += "  return;\n";
                } else {
                    std::string val = value_expr_c(inst.operands[0], local_vars);
                    std::string ret_type = c_type(fn_return_type);
                    if (ret_type == "void") ret_type = "int64_t"; // fallback
                    out += "  return (" + ret_type + ")(" + val + ");\n";
                }
                break;
            }
            case IrOpcode::ArrayAlloc: {
                std::string size = value_expr_c(inst.operands[0], local_vars);
                std::string elem_type = c_type(inst.result.type);
                // Array is a pointer to element type
                out += "  " + elem_type + "* " + result_var + " = (" + elem_type + "*)malloc((" + size + " + 1) * sizeof(" + elem_type + "));\n";
                out += "  " + result_var + "[0] = " + size + ";\n";
                local_vars[inst.result.name] = result_var;
                break;
            }
            case IrOpcode::ArrayStore: {
                std::string base = local_vars[inst.operands[0].name];
                std::string index = value_expr_c(inst.operands[1], local_vars);
                std::string val = value_expr_c(inst.operands[2], local_vars);
                out += "  " + base + "[(" + index + ") + 1] = (" + c_type(inst.operands[2].type) + ")(" + val + ");\n";
                break;
            }
            case IrOpcode::ArrayLoad: {
                std::string base = local_vars[inst.operands[0].name];
                std::string index = value_expr_c(inst.operands[1], local_vars);
                out += "  " + result_type + " " + result_var + " = " + base + "[(" + index + ") + 1];\n";
                local_vars[inst.result.name] = result_var;
                break;
            }
            case IrOpcode::ArrayLen: {
                std::string base = local_vars[inst.operands[0].name];
                out += "  int64_t " + result_var + " = " + base + "[0];\n";
                local_vars[inst.result.name] = result_var;
                break;
            }
            case IrOpcode::ObjAlloc: {
                // Object references are int64_t so they match the "Me" parameter.
                        out += "  int64_t " + result_var + " = (int64_t)(char*)calloc(1, 64);\n";
                local_vars[inst.result.name] = result_var;
                break;
            }
            case IrOpcode::FieldStore: {
                std::string obj = value_expr_c(inst.operands[0], local_vars);
                std::string val = value_expr_c(inst.operands[1], local_vars);
                // Parse offset from label (format: "field:offset")
                uint32_t offset = parse_field_offset(inst.label);
                std::string elem_type = c_type(inst.operands[1].type);
                // Use pointer arithmetic with offset
                out += "  *((" + elem_type + "*)((char*)" + obj + " + " + std::to_string(offset) + ")) = (" + elem_type + ")(" + val + ");\n";
                break;
            }
            case IrOpcode::FieldLoad: {
                std::string obj = value_expr_c(inst.operands[0], local_vars);
                // Parse offset from label (format: "field:offset")
                uint32_t offset = parse_field_offset(inst.label);
                std::string elem_type = c_type(inst.result.type);
                out += "  " + result_type + " " + result_var + " = *((" + elem_type + "*)((char*)" + obj + " + " + std::to_string(offset) + "));\n";
                local_vars[inst.result.name] = result_var;
                break;
            }
            case IrOpcode::TryStart:
                // TryStart: call bf_try_begin(), if non-zero jump to catch
                out += "  if (bf_try_begin()) goto " + inst.label + ";\n";
                break;
            case IrOpcode::TryEnd:
                // TryEnd: call bf_try_end(), jump to finally
                out += "  bf_try_end();\n";
                out += "  goto " + inst.label + ";\n";
                break;
            case IrOpcode::CatchStart:
                // CatchStart: call bf_catch_begin()
                out += "  bf_catch_begin();\n";
                break;
            case IrOpcode::CatchEnd:
                // CatchEnd: call bf_catch_end(), jump to finally
                out += "  bf_catch_end();\n";
                out += "  goto " + inst.label + ";\n";
                break;
            case IrOpcode::Throw: {
                // Throw: call bf_throw(exception_value), longjmp
                std::string val = value_expr_c(inst.operands[0], local_vars);
                out += "  bf_throw(" + val + ");\n";
                break;
            }
            case IrOpcode::FinallyStart:
                out += "  bf_finally_begin();\n";
                break;
            case IrOpcode::FinallyEnd:
                out += "  bf_finally_end();\n";
                break;
            default:
                break;
        }
        return out;
    }

    bool link_with_lld_fallback(const std::vector<std::string>& object_files,
        const std::string& output_path,
        const std::string& entry_point) {
        // Fallback: write command to .bat file and execute (avoids cmd.exe quoting issues)
        std::string lld_link = bfc::find_llvm_tool("lld-link.exe");

        std::vector<std::string> args;
        args.push_back(lld_link);
        args.push_back("/nologo");
        args.push_back("/out:" + output_path);
        args.push_back("/entry:" + entry_point);
        args.push_back("/subsystem:console");
        args.push_back("/libpath:C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207\\lib\\x64");
        args.push_back("/libpath:C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.44.35207\\lib\\onecore\\x64");
        args.push_back("/libpath:C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.26100.0\\ucrt\\x64");
        args.push_back("/libpath:C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.26100.0\\um\\x64");
        args.push_back("legacy_stdio_definitions.lib");
        
        args.push_back("libvcruntime.lib");
        args.push_back("kernel32.lib");
        args.push_back("user32.lib");
        args.push_back("gdi32.lib");
        args.push_back("shell32.lib");
        args.push_back("comctl32.lib");
        args.push_back("ole32.lib");

        for (const auto& obj : object_files) {
            args.push_back(obj);
        }
        args.push_back("libvcruntime.lib");
        args.push_back("kernel32.lib");
        args.push_back("user32.lib");
        args.push_back("gdi32.lib");
        args.push_back("shell32.lib");
        args.push_back("comctl32.lib");
        args.push_back("ole32.lib");

        return run_command_bat(args);
    }
};

LlvmCodeGen::LlvmCodeGen(DiagnosticSink& sink) : impl_(std::make_unique<CodeGenImpl>(sink)) {}

LlvmCodeGen::~LlvmCodeGen() = default;

bool LlvmCodeGen::generate_object(const IrModule& module, const std::string& output_path) {
    return impl_->generate_object(module, output_path);
}

bool LlvmCodeGen::link_executable(const std::vector<std::string>& object_files,
                                  const std::string& output_path,
                                  const std::string& entry_point) {
    return impl_->link_executable(object_files, output_path, entry_point);
}

} // namespace bfc




