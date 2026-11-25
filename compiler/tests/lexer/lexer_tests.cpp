// Lexer unit tests.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "test_helpers.h"

#include <bf/lexer.h>
#include <cassert>
#include <iostream>

using namespace bfc;

int main() {
    {
        auto tokens = lex_all("Module Sub Function End");
        assert(tokens.size() >= 5);
        assert(tokens[0].type == TokenType::Module);
        assert(tokens[1].type == TokenType::Sub);
        assert(tokens[2].type == TokenType::Function);
        assert(tokens[3].type == TokenType::End);
    }

    {
        auto tokens = lex_all("MyVar my_var _private");
        assert(tokens.size() >= 4);
        assert(tokens[0].type == TokenType::Identifier);
        assert(tokens[1].type == TokenType::Identifier);
        assert(tokens[2].type == TokenType::Identifier);
    }

    {
        auto tokens = lex_all("123 456 789");
        assert(tokens.size() >= 4);
        assert(tokens[0].type == TokenType::IntegerLiteral);
        assert(tokens[1].type == TokenType::IntegerLiteral);
        assert(tokens[2].type == TokenType::IntegerLiteral);
    }

    {
        auto tokens = lex_all("\"hello\" \"world\"");
        assert(tokens.size() >= 3);
        assert(tokens[0].type == TokenType::StringLiteral);
        assert(tokens[1].type == TokenType::StringLiteral);
    }

    {
        // Keywords are case-insensitive.
        auto tokens = lex_all("module sub function");
        assert(tokens[0].type == TokenType::Module);
        assert(tokens[1].type == TokenType::Sub);
        assert(tokens[2].type == TokenType::Function);
    }

    {
        auto tokens = lex_all("Dim x As Integer = 42");
        assert(tokens[0].type == TokenType::Dim);
        assert(tokens[1].type == TokenType::Identifier);
        assert(tokens[2].type == TokenType::As);
        assert(tokens[3].type == TokenType::Integer);
        assert(tokens[4].type == TokenType::Assign);
        assert(tokens[5].type == TokenType::IntegerLiteral);
    }

    std::cout << "All lexer tests passed!\n";
    return 0;
}