#pragma once

#include <bf/lexer.h>
#include <bf/parser.h>
#include <bf/ast.h>
#include <bf/semantic.h>
#include <bf/ir_builder.h>
#include <bf/diagnostic.h>
#include <string>
#include <vector>
#include <memory>

namespace bf {
namespace test {

// Helper to create a source location for testing
inline SourceLocation makeLocation(const std::string& file = "<test>", int line = 1, int column = 1) {
    return SourceLocation(file, line, column);
}

// Helper to run lexer on source and return tokens
inline std::vector<Token> lexSource(const std::string& source) {
    Lexer lexer(source);
    return lexer.tokenize();
}

// Helper to run parser on source and return AST
inline std::unique_ptr<ModuleDecl> parseSource(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(lexer, *(new DiagnosticSink()));
    return parser.parse();
}

// Helper to run semantic analysis on source
inline bool analyzeSource(const std::string& source, DiagnosticSink& sink) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(lexer, sink);
    auto ast = parser.parse();
    
    if (!ast || sink.has_errors()) {
        return false;
    }
    
    SemanticAnalyzer analyzer(sink);
    analyzer.analyze(*ast);
    
    return !sink.has_errors();
}

// Test source files
inline const char* kHelloWorldSource = R"(
Module Test
Sub Main()
    Print("Hello World")
End Sub
End Module
)";

inline const char* kArithmeticSource = R"(
Module Test
Sub Main()
    Dim a As Integer = 10
    Dim b As Integer = 5
    Dim sum As Integer = a + b
    Print(sum)
End Sub
End Module
)";

inline const char* kControlFlowSource = R"(
Module Test
Sub Main()
    Dim i As Integer = 0
    While i < 10
        i = i + 1
    End While
    Print(i)
End Sub
End Module
)";

inline const char* kFunctionSource = R"(
Module Test
Function Add(a As Integer, b As Integer) As Integer
    Return a + b
End Function
Sub Main()
    Print(Add(3, 4))
End Sub
End Module
)";

} // namespace test
} // namespace bf