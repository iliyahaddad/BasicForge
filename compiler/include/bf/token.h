#pragma once

#include "bf/bfc.h"
#include <string>
#include <variant>

namespace bfc {

enum class TokenType : uint8_t {
    // End of file
    Eof,

    // Identifiers and keywords
    Identifier,
    Module,
    Form,
    Sub,
    Function,
    Class,
    Structure,
    Property,
    End,
    Dim,
    As,
    Integer,
    Long,
    Single,
    Double,
    Boolean,
    String,
    True,
    False,
    Return,
    Print,
    If,
    Then,
    Else,
    For,
    To,
    Step,
    Next,
    While,
    Select,
    Case,
    Public,
    Private,
    Protected,
    Friend,
    New,
    Nothing,
    Of,
    Rem,
    Const,
    Delegate,
    Namespace,
    Enum,
    Implements,
    Interface,
    Me,
    Each,
    In,
    Do,
    Loop,
    Until,
    Throw,
    Try,
    Catch,
    Finally,
    Optional,
    ByVal,
    ByRef,

    // Literals
    IntegerLiteral,
    LongLiteral,
    DoubleLiteral,
    StringLiteral,
    InterpolatedString,

    // Operators / punctuation
    Ampersand,    // &  (string concat)
    Plus,
    Minus,
    Star,
    Slash,
    Mod,
    Backslash,    // \  integer divide
    Caret,        // ^  power
    Assign,       // =  assignment
    Eq,           // == equality
    Ne,
    Lt,
    Gt,
    Le,
    Ge,
    And,
    Or,
    Not,
    Xor,
    CInt,
    CLng,
    CDbl,
    CStr,
    CBool,
    Len,
    Trim,
    Left,
    Right,
    Mid,
    InStr,
    Replace,
    LParen,
    RParen,
    Comma,
    Colon,
    Dot,
    Bang,
    Underscore,   // line continuation
};

struct Token {
    TokenType type;
    std::string text;
    SourceLocation location;
};

} // namespace bfc
