#pragma once

#include "bf/ast.h"
#include "bf/ir.h"
#include "bf/diagnostic.h"
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

namespace bfc {

// Lowers the AST to BasicForge IR. The IR is the single input to codegen,
// so the IR must completely represent the program.
class IrBuilder {
public:
    IrBuilder(DiagnosticSink& sink);

    IrModule lower(Program& program);

private:
    DiagnosticSink& sink_;
    IrModule module_;
    IrFunction* current_fn_ = nullptr;
    IrBasicBlock* current_bb_ = nullptr;
    uint32_t next_reg_ = 0;
    uint32_t next_label_ = 0;
    std::unordered_map<std::string, PrimitiveType> var_types_;
    std::unordered_map<std::string, std::unordered_map<std::string, PrimitiveType>> classMembers_;
    std::unordered_map<std::string, int64_t> constant_values_; // For Const and Enum values
    std::unordered_map<std::string, std::string> struct_var_types_; // variable name -> struct type name

    // Method name -> lowered function name (e.g. "Hi" -> "Person_Hi").
    // Built before any function body is lowered so forward calls resolve.
    std::unordered_map<std::string, std::string> method_functions_;
    // Names declared with Dim x(n) As T (used to disambiguate a(i) from f(i)).
    std::unordered_set<std::string> array_vars_;
    
    // Class layout: class name -> field name -> (offset, type)
    std::unordered_map<std::string, std::unordered_map<std::string, std::pair<uint32_t, PrimitiveType>>> classLayouts_;
    std::string currentClass_ = "";

    IrValue new_vreg(PrimitiveType type);
    IrValue make_const_int(int64_t v);
    IrValue make_const_string(const std::string& v);
    IrValue make_const_bool(bool v);
    IrValue make_const_double(double v);
    IrValue make_local(const std::string& name, PrimitiveType type);
    IrValue make_param(const std::string& name, PrimitiveType type, bool is_array = false);
    std::string new_label();

    IrBasicBlock& new_block(const std::string& name);
    void emit(IrOpcode op, std::vector<IrValue> ops, IrValue res = {},
              std::string label = "");
    void emit(IrOpcode op, std::vector<IrValue> ops, IrValue res,
              std::string label, std::string label2);

    void lower_module(ModuleDecl& mod);
    void lower_namespace(NamespaceDecl& ns);
    void collect_method_functions(Program& program);
    void collect_class_methods(const std::vector<std::unique_ptr<ClassDecl>>& classes);
    void lower_structure(StructDecl& st);
    void lower_class(ClassDecl& cls);
    void lower_form(const FormDecl& form);
    void build_entry_main();
    void lower_procedure(SubOrFunction& proc);
    void lower_block(const Block& block);
    void lower_statement(const Statement& stmt);
    IrValue lower_expression(const Expression& expr);

    void lower_if(const Expression& cond,
                  const std::vector<std::unique_ptr<Statement>>& then_branch,
                  const std::vector<std::unique_ptr<Statement>>& else_branch);
    void lower_for(const ForStmt& s);
    void lower_for_each(const ForEachStmt& s);
    void lower_do_loop(const DoLoopStmt& s);
    void lower_while(const Expression& cond,
                     const std::vector<std::unique_ptr<Statement>>& body);
    void lower_select_case(const SelectCaseStmt& s);
    
    void lower_try_catch_finally(const TryCatchFinallyStmt& tcf);
    void lower_throw(const ThrowStmt& stmt);
    
    void compute_class_layout(ClassDecl& cls);
    void compute_struct_layout(StructDecl& st);
    uint32_t get_primitive_size(PrimitiveType t);
};

} // namespace bfc
