#pragma once

#include "bf/diagnostic.h"
#include "bf/lexer.h"

#include <string>
#include <vector>

namespace bfc {

// Diagnostic sink that records diagnostics so tests can assert on them.
class TestSink : public DiagnosticSink {
public:
    std::vector<Diagnostic> diagnostics;

    void emit(const Diagnostic& d) override {
        diagnostics.push_back(d);
        if (d.severity == DiagnosticSeverity::Error) {
            had_error_ = true;
        }
    }
};

// Lex a whole source string, including the EOF token.
inline std::vector<Token> lex_all(const std::string& source) {
    Lexer lexer(source);
    std::vector<Token> tokens;
    for (;;) {
        Token token = lexer.next_token();
        tokens.push_back(token);
        if (token.type == TokenType::Eof) break;
    }
    return tokens;
}

} // namespace bfc