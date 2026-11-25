#pragma once

#include "bf/token.h"
#include <unordered_map>

namespace bfc {

static const std::unordered_map<std::string, TokenType>& keywords() {
    static const std::unordered_map<std::string, TokenType> k = {
        {"module", TokenType::Module},
        {"form", TokenType::Form},
        {"sub", TokenType::Sub},
        {"function", TokenType::Function},
        {"class", TokenType::Class},
        {"property", TokenType::Property},
        {"end", TokenType::End},
        {"dim", TokenType::Dim},
        {"as", TokenType::As},
        {"integer", TokenType::Integer},
        {"long", TokenType::Long},
        {"single", TokenType::Single},
        {"double", TokenType::Double},
        {"boolean", TokenType::Boolean},
        {"string", TokenType::String},
        {"true", TokenType::True},
        {"false", TokenType::False},
        {"return", TokenType::Return},
        {"print", TokenType::Print},
        {"if", TokenType::If},
        {"then", TokenType::Then},
        {"else", TokenType::Else},
        {"for", TokenType::For},
        {"to", TokenType::To},
        {"step", TokenType::Step},
        {"next", TokenType::Next},
        {"while", TokenType::While},
        {"select", TokenType::Select},
        {"case", TokenType::Case},
        {"public", TokenType::Public},
        {"private", TokenType::Private},
        {"protected", TokenType::Protected},
        {"friend", TokenType::Friend},
        {"new", TokenType::New},
        {"nothing", TokenType::Nothing},
        {"of", TokenType::Of},
        {"rem", TokenType::Rem},
        {"and", TokenType::And},
        {"or", TokenType::Or},
        {"not", TokenType::Not},
        {"mod", TokenType::Mod},
        {"xor", TokenType::Xor},
        {"cint", TokenType::CInt},
        {"clng", TokenType::CLng},
        {"cdbl", TokenType::CDbl},
        {"cstr", TokenType::CStr},
        {"cbool", TokenType::CBool},
        {"len", TokenType::Len},
        {"trim", TokenType::Trim},
        {"left", TokenType::Left},
        {"right", TokenType::Right},
        {"mid", TokenType::Mid},
        {"instr", TokenType::InStr},
        {"replace", TokenType::Replace},
        {"each", TokenType::Each},
        {"in", TokenType::In},
        {"do", TokenType::Do},
        {"loop", TokenType::Loop},
        {"until", TokenType::Until},
        {"enum", TokenType::Enum},
        {"try", TokenType::Try},
        {"catch", TokenType::Catch},
        {"finally", TokenType::Finally},
        {"throw", TokenType::Throw},
        {"implements", TokenType::Implements},
        {"interface", TokenType::Interface},
        {"me", TokenType::Me},
        {"structure", TokenType::Structure},
        {"namespace", TokenType::Namespace},
        {"delegate", TokenType::Delegate},
        {"const", TokenType::Const},
        {"optional", TokenType::Optional},
        {"byval", TokenType::ByVal},
        {"byref", TokenType::ByRef},
    };
    return k;
}

static std::string to_lower(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c >= 'A' && c <= 'Z') out.push_back(static_cast<char>(c + 32));
        else out.push_back(c);
    }
    return out;
}

static bool is_identifier_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
static bool is_identifier_part(char c) {
    return is_identifier_start(c) || (c >= '0' && c <= '9');
}

class Lexer {
public:
    explicit Lexer(std::string source, std::string file = "<input>");

    Token next_token();

    // Save / restore lexer position for limited lookahead (assignment detection).
    struct State {
        size_t pos = 0;
        uint32_t line = 1;
        uint32_t column = 1;
        uint32_t offset = 0;
    };
    State save_state() const;
    void restore_state(const State& s);

    // Peek at the next token without consuming it
    Token peek_token();

private:
    std::string source_;
    std::string file_;
    size_t pos_ = 0;
    uint32_t line_ = 1;
    uint32_t column_ = 1;
    uint32_t offset_ = 0;

    char peek() const;
    char advance();
    void skip_whitespace();
    void skip_comment();
    Token make_token(TokenType type, std::string text);
    SourceLocation current_loc() const;
    Token scan_number();
    Token scan_string();
    Token scan_interpolated_string();
    Token scan_identifier_or_keyword();
    Token scan_char(char expected, TokenType single, TokenType combined, char combined_char);
};

} // namespace bfc
