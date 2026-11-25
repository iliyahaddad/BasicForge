#pragma once

#include "bf/ast.h"
#include "bf/diagnostic.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>

namespace bfc {

class SemanticAnalyzer {
public:
    SemanticAnalyzer(DiagnosticSink& sink);

    void analyze(Program& program);

    void register_external(const std::string& name, PrimitiveType type,
                           bool is_function) {
        if (is_function) {
            external_functions_[name] = type;
        } else {
            external_variables_[name] = type;
        }
    }

    void register_external_class(
        const std::string& name,
        const std::unordered_map<std::string, PrimitiveType>& members) {
        external_classes_[name] = members;
    }

    void register_external_method(const std::string& name, PrimitiveType ret) {
        external_method_returns_[name] = ret;
    }

private:
    DiagnosticSink& sink_;
    std::unordered_map<std::string, PrimitiveType> variables_;
    std::unordered_map<std::string, PrimitiveType> constants_; // For Const declarations
    std::unordered_map<std::string, int64_t> constant_values_; // For Const and Enum values
    std::unordered_map<std::string, std::unordered_map<std::string, PrimitiveType>> classMembers_;
    std::unordered_map<std::string, std::vector<const SubOrFunction*>> overloads_; // For overloading resolution
    
    // Generic type support
    std::unordered_map<std::string, TypeParameterList> genericTypeParameters_; // class/interface name -> type parameters
    std::unordered_map<std::string, std::unordered_map<std::string, PrimitiveType>> genericTypeArguments_; // instantiation name -> type arguments
    std::vector<std::unordered_map<std::string, PrimitiveType>> genericTypeParamStack_; // Stack for nested generic contexts
    
    std::string currentClass_;
    std::string currentNamespace_;
    std::unordered_set<std::string> builtins_ = {"Print", "Len", "Trim", "Left", "Right", "Mid", "InStr", "Replace", 
        "CInt", "CLng", "CDbl", "CStr", "CBool", "Sqr", "Abs", "Sgn", "Fix", "Int", "Rnd",
        "Format", "Split", "Join", "Year", "Month", "Day", "Now",
        "Open", "Close", "Print", "Input", "EOF", "LOF", "CStrDouble",
        "GetText", "SetText",
        "FileExists", "ReadAllText", "WriteAllText", "AppendAllText",
        "GetEnv", "SetEnv", "Sleep", "Beep", "Clear", "SetTitle", "ReadLine",
        "MessageBox", "ShellExecute", "GetClipboard", "SetClipboard",
        "UCase", "LCase", "Chr", "Asc"};
    std::unordered_set<std::string> array_vars_;
    std::unordered_map<std::string, PrimitiveType> function_return_types_;
    std::unordered_map<std::string, PrimitiveType> external_functions_;
    std::unordered_map<std::string, PrimitiveType> external_variables_;
    std::unordered_map<std::string, std::unordered_map<std::string, PrimitiveType>> external_classes_;
    std::unordered_map<std::string, PrimitiveType> external_method_returns_;
    std::unordered_map<std::string, std::string> class_var_types_;
    PrimitiveType current_return_type_ = PrimitiveType::Void;
    std::string current_function_name_;

    void analyze_module(ModuleDecl& mod);
    void analyze_namespace(NamespaceDecl& ns);
    void analyze_procedure(SubOrFunction& proc);
    void analyze_class_method(ClassDecl& cls, ClassMethod& method);
    void analyze_structure(StructDecl& st);
    void analyze_interface(InterfaceDecl& iface);
    void analyze_delegate(DelegateDecl& d);
    void analyze_block(const Block& block);
    void analyze_statement(const Statement& stmt);
    TypeRef analyze_expression(const Expression& expr);
    
    // Generic type handling
    TypeRef analyze_type(const TypeRef& type, bool allowGenerics = true);
    TypeRef substitute_generic_type(const TypeRef& type);
    void register_generic_type_parameters(const TypeParameterList& params);
    void unregister_generic_type_parameters();
    TypeRef instantiate_generic_type(const std::string& genericName, const std::vector<TypeRef>& typeArgs);
    
    void check_type_match(TypeRef expected, TypeRef actual, const SourceLocation& loc,
                          const std::string& context);
    bool is_numeric(PrimitiveType t) const;
    bool is_string(PrimitiveType t) const;
    bool is_boolean(PrimitiveType t) const;
    void emit_error(const std::string& msg);
    void emit_warning(const std::string& msg);
    PrimitiveType resolve_type(const TypeRef& type);
    std::string mangle_name(const std::string& name, const std::vector<Parameter>& params);
};

} // namespace bfc
