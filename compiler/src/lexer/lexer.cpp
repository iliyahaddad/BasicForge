#include "bf/lexer.h"

namespace bfc {

Lexer::Lexer(std::string source, std::string file)
    : source_(std::move(source)), file_(std::move(file)) {}

char Lexer::peek() const {
    if (pos_ >= source_.size()) return '\0';
    return source_[pos_];
}

char Lexer::advance() {
    char c = peek();
    if (c != '\0') {
        ++pos_;
        ++offset_;
        if (c == '\n') {
            ++line_;
            column_ = 1;
        } else {
            ++column_;
        }
    }
    return c;
}

void Lexer::skip_whitespace() {
    while (true) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skip_comment() {
    // BasicForge comment: '
    if (peek() == '\'') {
        while (peek() != '\0' && peek() != '\n') advance();
        // Consume the trailing newline so it doesn't leak as a token.
        if (peek() == '\n') advance();
        return;
    }
    // REM comment: case-insensitive "REM" to end of line
    if (is_identifier_start(peek())) {
        State saved = save_state();
        size_t save = pos_;
        while (is_identifier_part(peek())) advance();
        std::string word = source_.substr(save, pos_ - save);
        if (to_lower(word) == "rem") {
            while (peek() != '\0' && peek() != '\n') advance();
            // Consume the trailing newline so it doesn't leak as a token.
            if (peek() == '\n') advance();
        } else {
            // Not a comment: rewind all position tracking, not just pos_.
            restore_state(saved);
        }
    }
}

Token Lexer::make_token(TokenType type, std::string text) {
    return Token{type, std::move(text), current_loc()};
}

SourceLocation Lexer::current_loc() const {
    return SourceLocation{file_, line_, column_, offset_};
}

Lexer::State Lexer::save_state() const {
    State s;
    s.pos = pos_;
    s.line = line_;
    s.column = column_;
    s.offset = offset_;
    return s;
}

void Lexer::restore_state(const State& s) {
    pos_ = s.pos;
    line_ = s.line;
    column_ = s.column;
    offset_ = s.offset;
}

Token Lexer::peek_token() {
    State saved = save_state();
    Token tok = next_token();
    restore_state(saved);
    return tok;
}

Token Lexer::scan_number() {
    size_t start = pos_;
    bool is_double = false;
    while (peek() >= '0' && peek() <= '9') advance();
    if (peek() == '.') {
        // Peek ahead: if followed by a digit, it's a double.
        // We need a way to peek two ahead; check next char after '.'.
        size_t after_dot = pos_ + 1;
        if (after_dot < source_.size() && source_[after_dot] >= '0' && source_[after_dot] <= '9') {
            is_double = true;
            advance(); // consume '.'
            while (peek() >= '0' && peek() <= '9') advance();
        }
    }
    std::string text = source_.substr(start, pos_ - start);
    return make_token(is_double ? TokenType::DoubleLiteral : TokenType::IntegerLiteral, text);
}

Token Lexer::scan_string() {
    advance(); // consume opening quote
    size_t start = pos_;
    while (peek() != '\0' && peek() != '"') {
        if (peek() == '\\') advance(); // skip escaped char
        advance();
    }
    std::string text = source_.substr(start, pos_ - start);
    if (peek() == '"') advance(); // consume closing quote
    return make_token(TokenType::StringLiteral, text);
}

Token Lexer::scan_interpolated_string() {
    advance(); // '$'
    advance(); // '"'
    size_t start = pos_;
    while (peek() != '\0' && peek() != '"') {
        if (peek() == '\\') advance();
        advance();
    }
    std::string text = source_.substr(start, pos_ - start);
    if (peek() == '"') advance();
    return make_token(TokenType::InterpolatedString, text);
}

Token Lexer::scan_identifier_or_keyword() {
    size_t start = pos_;
    while (is_identifier_part(peek())) advance();
    std::string text = source_.substr(start, pos_ - start);
    auto it = keywords().find(to_lower(text));
    if (it != keywords().end()) {
        return make_token(it->second, text);
    }
    return make_token(TokenType::Identifier, text);
}

Token Lexer::scan_char(char expected, TokenType single, TokenType combined, char combined_char) {
    advance(); // consume first char
    if (peek() == combined_char) {
        advance();
        std::string text(1, expected);
        text.push_back(combined_char);
        return make_token(combined, text);
    }
    return make_token(single, std::string(1, expected));
}

Token Lexer::next_token() {
    // Loop: skip whitespace and comments repeatedly until we reach a real token.
    // This handles REM comments and quote comments that appear consecutively.
    for (;;) {
        skip_whitespace();
        skip_comment();
        skip_whitespace();
        // A comment may be followed by another comment line.
        if (peek() == '\'') continue;

        // Skip UTF-8 BOM if present at start of file
        if (offset_ == 0 && peek() == '\xEF' && source_.size() >= 3 &&
            source_[1] == '\xBB' && source_[2] == '\xBF') {
            advance(); advance(); advance();
            continue;
        }

        if (peek() == '\0') {
            return make_token(TokenType::Eof, "");
        }

        char c = peek();

        if (c == '"') return scan_string();
        if (c == '$') return scan_interpolated_string();
        if (c >= '0' && c <= '9') return scan_number();
        if (is_identifier_start(c)) {
            Token tok = scan_identifier_or_keyword();
            // REM is a comment keyword: skip to end of line and continue.
            if (tok.type == TokenType::Rem) {
                while (peek() != '\0' && peek() != '\n') advance();
                if (peek() == '\n') advance();
                continue;
            }
            return tok;
        }

        switch (c) {
            case '&':  return scan_char('&', TokenType::Ampersand, TokenType::Ampersand, '&');
            case '+':  advance(); return make_token(TokenType::Plus, "+");
            case '-':  return scan_char('-', TokenType::Minus, TokenType::Minus, '-');
            case '*':  advance(); return make_token(TokenType::Star, "*");
            case '/':  return scan_char('/', TokenType::Slash, TokenType::Slash, '/');
            case '\\': advance(); return make_token(TokenType::Backslash, "\\");
            case '^':  advance(); return make_token(TokenType::Caret, "^");
            case '=':  return scan_char('=', TokenType::Assign, TokenType::Eq, '=');
        case '<':
            advance();
            if (peek() == '=') {
                advance();
                return make_token(TokenType::Le, "<=");
            }
            if (peek() == '>') {
                advance();
                return make_token(TokenType::Ne, "<>");
            }
            return make_token(TokenType::Lt, "<");
        case '>':  return scan_char('>', TokenType::Gt, TokenType::Ge, '=');
            case '!':  return scan_char('!', TokenType::Bang, TokenType::Ne, '=');
            case '(':  advance(); return make_token(TokenType::LParen, "(");
            case ')':  advance(); return make_token(TokenType::RParen, ")");
            case ',':  advance(); return make_token(TokenType::Comma, ",");
            case ':':  advance(); return make_token(TokenType::Colon, ":");
            case '.':  advance(); return make_token(TokenType::Dot, ".");
        }

        // Unknown character
        advance();
        return make_token(TokenType::Identifier, std::string(1, c));
    }
}

} // namespace bfc
