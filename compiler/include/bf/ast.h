#pragma once

#include "bf/bfc.h"
#include "bf/token.h"
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <cstdint>
#include <unordered_map>

namespace bfc {

// Forward declarations
struct Expression;
struct Statement;

// ---------- Types ----------

struct TypeParameter {
    std::string name;
    // Optional constraint (e.g., class, interface, new())
    std::vector<std::string> constraints; 
};

struct TypeRef {
    PrimitiveType kind = PrimitiveType::Void;
    bool is_array = false;
    // Generics support
    std::vector<TypeRef> type_arguments; // for generic types like List<Integer>
    std::string generic_name; // name of generic type (e.g., "List")
    bool is_generic = false;
};

struct TypeParameterList {
    std::vector<TypeParameter> parameters;
};

// ---------- Declarations ----------

struct Parameter {
    std::string name;
    TypeRef type;
    bool is_array = false;
    bool is_byref = false;
    bool is_optional = false;
    std::unique_ptr<Expression> default_value;
};

struct ProcedureDecl {
    std::string name;
    std::vector<Parameter> parameters;
    TypeRef return_type; // Void for Sub
    bool is_function = false;
};

// ---------- Expressions ----------

struct IntegerLiteralExpr {
    int64_t value = 0;
};

struct StringLiteralExpr {
    std::string value;
};

struct BooleanLiteralExpr {
    bool value = false;
};

struct DoubleLiteralExpr {
    double value = 0.0;
};

struct IdentifierExpr {
    std::string name;
};

struct ArrayAccessExpr {
    std::string name;
    std::unique_ptr<Expression> index;
};

struct CallExpr {
    std::string callee;
    std::vector<std::unique_ptr<Expression>> arguments;
};

struct BinaryExpr {
    enum class Op : uint8_t {
        Add, Sub, Mul, Div, IntDiv, Mod, Pow,
        And, Or, Xor, Concat,
        Eq, Ne, Lt, Gt, Le, Ge,
    };
    Op op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
};

struct UnaryExpr {
    enum class Op : uint8_t { Not, Negate };
    Op op;
    std::unique_ptr<Expression> operand;
};

// ---------- Class Member Access ----------

struct NewExpr {
    std::string class_name;
    std::vector<std::unique_ptr<Expression>> arguments;
};

struct GenericInstantiationExpr {
    std::string generic_name;
    std::vector<TypeRef> type_arguments;
};

struct MemberAccessExpr {
    std::unique_ptr<Expression> object;
    std::string member;
};

struct MeExpr {
    // Empty - represents the current instance
};

struct Expression {
    std::variant<
        std::monostate,
        IntegerLiteralExpr,
        StringLiteralExpr,
        BooleanLiteralExpr,
        DoubleLiteralExpr,
        IdentifierExpr,
        ArrayAccessExpr,
        CallExpr,
        NewExpr,
        GenericInstantiationExpr,
        MemberAccessExpr,
        MeExpr,
        BinaryExpr,
        UnaryExpr
    > data;
    TypeRef inferred_type;
};

// ---------- Statements ----------

struct DimStmt {
    std::string name;
    TypeRef type;
    bool type_inferred = false;
    std::unique_ptr<Expression> initializer;
    std::unique_ptr<Expression> array_size; // null if not array
    bool is_const = false; // For Const declarations
};

struct PrintStmt {
    std::vector<std::unique_ptr<Expression>> arguments;
};

struct ReturnStmt {
    std::unique_ptr<Expression> expression;
};

struct ThrowStmt {
    std::unique_ptr<Expression> expression;
};

struct IfStmt {
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Statement>> then_branch;
    std::vector<std::unique_ptr<Statement>> else_branch;
};

struct ForStmt {
    std::string variable;
    TypeRef type_hint;
    std::unique_ptr<Expression> start;
    std::unique_ptr<Expression> end;
    std::unique_ptr<Expression> step;
    std::vector<std::unique_ptr<Statement>> body;
    bool is_for_each = false; // true if this is a For Each loop
};

struct DoLoopStmt {
    enum class Type : uint8_t { DoWhile, DoUntil, LoopWhile, LoopUntil };
    Type loopType;
    std::unique_ptr<Expression> condition; // null for LoopWhile/LoopUntil (checked at bottom)
    std::vector<std::unique_ptr<Statement>> body;
};

struct ForEachStmt {
    std::string variable;
    std::unique_ptr<Expression> collection;
    std::vector<std::unique_ptr<Statement>> body;
};

struct WhileStmt {
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Statement>> body;
};

struct SelectCaseStmt {
    std::unique_ptr<Expression> subject;
    struct Case {
        std::vector<std::unique_ptr<Expression>> values; // empty = Else
        std::unique_ptr<Expression> range_end;           // for "Case a To b"
        std::vector<std::unique_ptr<Statement>> body;
    };
    std::vector<Case> cases;
};

struct CatchClause {
    std::string exception_variable;
    TypeRef exception_type; // optional
    std::vector<std::unique_ptr<Statement>> body;
};

struct TryCatchFinallyStmt {
    std::vector<std::unique_ptr<Statement>> try_body;
    std::vector<CatchClause> catch_clauses;
    std::vector<std::unique_ptr<Statement>> finally_body;
};

struct AssignmentStmt {
    std::string name;
    std::unique_ptr<Expression> expression;
};

struct ArrayAssignmentStmt {
    std::string name;
    std::unique_ptr<Expression> index;
    std::unique_ptr<Expression> expression;
};

struct FieldAssignmentStmt {
    std::unique_ptr<Expression> object;  // MemberAccessExpr or IdentifierExpr
    std::string field_name;
    std::unique_ptr<Expression> expression;
};

struct ExpressionStmt {
    std::unique_ptr<Expression> expression;
};

struct Statement {
    std::variant<
        std::monostate,
        DimStmt,
        PrintStmt,
        ReturnStmt,
        ThrowStmt,
        AssignmentStmt,
        ArrayAssignmentStmt,
        FieldAssignmentStmt,
        IfStmt,
        ForStmt,
        ForEachStmt,
        DoLoopStmt,
        WhileStmt,
        SelectCaseStmt,
        TryCatchFinallyStmt,
        ExpressionStmt
    > data;
};

// ---------- Blocks ----------

struct Block {
    std::vector<std::unique_ptr<Statement>> statements;
};

// ---------- Sub / Function / Class / Interface / Namespace / Delegate / Enum / Structure ----------

struct SubOrFunction {
    std::string name;
    std::vector<Parameter> parameters;
    TypeRef return_type;
    bool is_function;
    Block body;
    // Generics support
    TypeParameterList type_parameters;
};

struct ClassProperty {
    std::string name;
    TypeRef type;
    bool is_public = true;
    bool is_readonly = false;
};

struct ClassMethod {
    std::string name;
    std::vector<Parameter> parameters;
    TypeRef return_type; // Void for Sub
    bool is_function = false;
    bool is_public = true;
    bool is_constructor = false;
    Block body;
    // Generics support
    TypeParameterList type_parameters;
};

struct ClassDecl {
    std::string name;
    std::vector<ClassProperty> properties;
    std::vector<ClassMethod> methods;
    bool is_public = true;
    // Generics support
    TypeParameterList type_parameters;
    // Implements clause
    std::vector<std::string> implements;
};

struct EnumDecl {
    std::string name;
    struct Member {
        std::string name;
        int64_t value;
    };
    std::vector<Member> members;
};

struct InterfaceMember {
    std::string name;
    TypeParameterList type_parameters;
    std::vector<Parameter> parameters;
    TypeRef return_type;
    bool is_function;
};

struct InterfaceDecl {
    std::string name;
    std::vector<InterfaceMember> members;
    bool is_public = true;
};

struct DelegateDecl {
    std::string name;
    std::vector<Parameter> parameters;
    TypeRef return_type;
    bool is_function;
};

struct StructDecl {
    std::string name;
    std::vector<ClassProperty> fields; // same as properties but value semantics
    bool is_public = true;
};

struct FormControlDecl {
    std::string name;
    std::string type; // "Button", "Label", "TextBox", "CheckBox", ...
    std::unordered_map<std::string, std::string> properties; // Text, Left, Top, ...
    std::unordered_map<std::string, std::string> events;     // Click -> handler
};

struct FormDecl {
    std::string name;
    std::unordered_map<std::string, std::string> properties; // Width, Height, Text
    std::vector<FormControlDecl> controls;
};

struct NamespaceDecl {
    std::string name;
    std::vector<std::unique_ptr<SubOrFunction>> procedures;
    std::vector<std::unique_ptr<ClassDecl>> classes;
    std::vector<std::unique_ptr<InterfaceDecl>> interfaces;
    std::vector<std::unique_ptr<EnumDecl>> enums;
    std::vector<std::unique_ptr<StructDecl>> structures;
    std::vector<std::unique_ptr<DelegateDecl>> delegates;
    std::vector<std::unique_ptr<FormDecl>> forms;
    std::vector<std::unique_ptr<NamespaceDecl>> namespaces;
};

struct ModuleDecl {
    std::string name;
    std::vector<std::unique_ptr<SubOrFunction>> procedures;
    std::vector<std::unique_ptr<ClassDecl>> classes;
    std::vector<std::unique_ptr<InterfaceDecl>> interfaces;
    std::vector<std::unique_ptr<EnumDecl>> enums;
    std::vector<std::unique_ptr<StructDecl>> structures;
    std::vector<std::unique_ptr<DelegateDecl>> delegates;
    std::vector<std::unique_ptr<FormDecl>> forms;
    std::vector<std::unique_ptr<NamespaceDecl>> namespaces;
};

// ---------- Program ----------

struct Program {
    std::vector<std::unique_ptr<ModuleDecl>> modules;
    std::vector<std::unique_ptr<NamespaceDecl>> namespaces; // top-level namespaces
};

} // namespace bfc
