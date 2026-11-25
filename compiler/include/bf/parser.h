#pragma once

#include "bf/ast.h"
#include "bf/lexer.h"
#include "bf/diagnostic.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace bfc {

// Declaration info collected while parsing, used by the LSP server for
// hover/definition without a separate symbol pass.
struct SymbolInfo {
    std::string name;
    std::string kind;      // "Sub", "Function", "Class", "Variable", "Const", ...
    std::string type_name; // e.g. "Integer" (may be empty)
    SourceLocation location;
};

class Parser {
public:
    Parser(Lexer& lexer, DiagnosticSink& sink);

    std::unique_ptr<Program> parse();

    const std::vector<SymbolInfo>& symbols() const { return symbols_; }

private:
    Lexer& lexer_;
    DiagnosticSink& sink_;
    Token current_;
    std::unordered_map<std::string, TypeRef> variables_;
    std::vector<SymbolInfo> symbols_;

    void add_symbol(const std::string& name, const std::string& kind,
                    const std::string& type_name, const SourceLocation& loc) {
        symbols_.push_back(SymbolInfo{name, kind, type_name, loc});
    }

    void set_symbol_type(const std::string& name, PrimitiveType type) {
        std::string type_name = primitive_type_name(type);
        for (auto& sym : symbols_) {
            if (sym.name == name && sym.type_name.empty()) {
                sym.type_name = type_name;
                break;
            }
        }
    }

    void advance();
    bool match(TokenType type);
    bool check(TokenType type) const;
    void consume(TokenType type, const std::string& msg);
    void consume_end(TokenType keyword);
    void error_at_current(const std::string& msg);

    // Declarations
    std::unique_ptr<Program> parse_program();
    std::unique_ptr<ModuleDecl> parse_module();
    std::unique_ptr<SubOrFunction> parse_procedure();
    std::unique_ptr<SubOrFunction> parse_module_level_decl();
    TypeRef parse_type();
    std::vector<Parameter> parse_parameters();
    std::unique_ptr<EnumDecl> parse_enum();
    std::unique_ptr<ClassDecl> parse_class();
    std::unique_ptr<FormDecl> parse_form();
    std::unique_ptr<StructDecl> parse_structure();
    std::unique_ptr<NamespaceDecl> parse_namespace();
    std::unique_ptr<DelegateDecl> parse_delegate();
    std::unique_ptr<InterfaceDecl> parse_interface();
    TypeParameterList parse_type_parameters();
    ClassProperty parse_class_property();
    ClassMethod parse_class_method();

    // Exception handling
    std::unique_ptr<Statement> parse_try_catch_finally();
    std::unique_ptr<Statement> parse_throw();

    // Statements
    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<Statement> parse_assignment_lhs_stmt();
    std::unique_ptr<DimStmt> parse_dim();
    std::unique_ptr<DimStmt> parse_const();
    std::unique_ptr<PrintStmt> parse_print();
    std::unique_ptr<ReturnStmt> parse_return();
    std::unique_ptr<IfStmt> parse_if();
    std::unique_ptr<ForStmt> parse_for();
    std::unique_ptr<ForEachStmt> parse_for_each();
    std::unique_ptr<DoLoopStmt> parse_do_loop();
    std::unique_ptr<WhileStmt> parse_while();
    std::unique_ptr<SelectCaseStmt> parse_select_case();
    std::unique_ptr<ExpressionStmt> parse_expression_stmt();
    std::unique_ptr<ArrayAssignmentStmt> parse_array_assignment();
    Block parse_block_until(TokenType end_token);
    Block parse_block_terminators(std::initializer_list<TokenType> stop_at,
                                  std::initializer_list<TokenType> consume_after = {});

    // Expressions
    std::unique_ptr<Expression> parse_expression();
    std::unique_ptr<Expression> parse_interpolated_text(const std::string& text);
    std::unique_ptr<Expression> parse_expression_fragment(const std::string& text);
    std::unique_ptr<Expression> parse_or();
    std::unique_ptr<Expression> parse_and();
    std::unique_ptr<Expression> parse_assignment_lhs();
    std::unique_ptr<Expression> parse_equality();
    std::unique_ptr<Expression> parse_comparison();
    std::unique_ptr<Expression> parse_term();
    std::unique_ptr<Expression> parse_factor();
    std::unique_ptr<Expression> parse_unary();
    std::unique_ptr<Expression> parse_primary();
    std::unique_ptr<Expression> parse_postfix(std::unique_ptr<Expression> base);
    std::unique_ptr<Expression> parse_call(const std::string& callee);
    std::unique_ptr<Expression> parse_array_access(const std::string& name);
};

} // namespace bfc
