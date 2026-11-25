#include "bf/parser.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>

namespace bfc {

static std::string bf_lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static bool bf_iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

static bool bf_is_control_type(const std::string& lower) {
    static const char* types[] = {"button", "label", "textbox", "checkbox",
                                  "radiobutton", "groupbox", "panel",
                                  "picturebox", "listbox", "combobox",
                                  "trackbar", "progressbar", "datetimepicker",
                                  "listview", "treeview", "richtextbox",
                                  "tabcontrol", "toolbar", "statusbar",
                                  "updown", "monthcalendar", "scrollbar",
                                  "hotkey"};
    for (const char* t : types) if (lower == t) return true;
    return false;
}

static std::string bf_normalize_control(const std::string& lower) {
    if (lower == "button") return "Button";
    if (lower == "label") return "Label";
    if (lower == "textbox") return "TextBox";
    if (lower == "checkbox") return "CheckBox";
    if (lower == "radiobutton") return "RadioButton";
    if (lower == "groupbox") return "GroupBox";
    if (lower == "panel") return "Panel";
    if (lower == "picturebox") return "PictureBox";
    if (lower == "listbox") return "ListBox";
    if (lower == "combobox") return "ComboBox";
    if (lower == "trackbar") return "TrackBar";
    if (lower == "progressbar") return "ProgressBar";
    if (lower == "datetimepicker") return "DateTimePicker";
    if (lower == "listview") return "ListView";
    if (lower == "treeview") return "TreeView";
    if (lower == "richtextbox") return "RichTextBox";
    if (lower == "tabcontrol") return "TabControl";
    if (lower == "toolbar") return "ToolBar";
    if (lower == "statusbar") return "StatusBar";
    if (lower == "updown") return "UpDown";
    if (lower == "monthcalendar") return "MonthCalendar";
    if (lower == "scrollbar") return "ScrollBar";
    if (lower == "hotkey") return "HotKey";
    return lower;
}

// ============================================================================
// Construction / helpers
// ============================================================================

Parser::Parser(Lexer& lexer, DiagnosticSink& sink)
    : lexer_(lexer), sink_(sink) {
    advance();
}

void Parser::advance() {
    current_ = lexer_.next_token();
}

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

bool Parser::check(TokenType type) const {
    return current_.type == type;
}

void Parser::consume(TokenType type, const std::string& msg) {
    if (check(type)) { advance(); return; }
    error_at_current(msg);
}

void Parser::consume_end(TokenType keyword) {
    consume(TokenType::End, "Expected 'End'");
    consume(keyword, "Expected keyword after 'End'");
}

void Parser::error_at_current(const std::string& msg) {
    Diagnostic d;
    d.severity = DiagnosticSeverity::Error;
    d.message = msg;
    d.location = current_.location;
    sink_.emit(d);
}

// ============================================================================
// Entry point
// ============================================================================

std::unique_ptr<Program> Parser::parse() {
    return parse_program();
}

std::unique_ptr<Program> Parser::parse_program() {
    auto program = std::make_unique<Program>();

    while (current_.type != TokenType::Eof) {
        if (check(TokenType::Rem) || check(TokenType::Identifier)) {
            // Could be a module declaration or a top-level statement
            auto saved = lexer_.save_state();
            advance();
            if (check(TokenType::Identifier) && current_.text == "Module") {
                // Put back and parse as module
                lexer_.restore_state(saved);
                program->modules.push_back(parse_module());
            } else {
                lexer_.restore_state(saved);
                // Top-level statement - wrap in an implicit module
                auto mod = std::make_unique<ModuleDecl>();
                mod->name = "<main>";
                auto stmt = parse_statement();
                if (stmt) {
                    auto proc = std::make_unique<SubOrFunction>();
                    proc->name = "<implicit>";
                    proc->is_function = false;
                    proc->body.statements.push_back(std::move(stmt));
                    mod->procedures.push_back(std::move(proc));
                }
                program->modules.push_back(std::move(mod));
                break;
            }
        } else if (check(TokenType::Module)) {
            program->modules.push_back(parse_module());
        } else if (check(TokenType::Namespace)) {
            program->namespaces.push_back(std::move(parse_namespace()));
        } else {
            error_at_current("Unexpected token at module level");
            advance();
        }
    }

    return program;
}

// ============================================================================
// Module
// ============================================================================

std::unique_ptr<ModuleDecl> Parser::parse_module() {
    auto module = std::make_unique<ModuleDecl>();

    consume(TokenType::Module, "Expected 'Module'");
    if (check(TokenType::Identifier)) {
        module->name = current_.text;
        advance();
    } else {
        module->name = "<anonymous>";
    }

    while (current_.type != TokenType::Eof &&
           current_.type != TokenType::End) {
        if (check(TokenType::Sub) || check(TokenType::Function)) {
            module->procedures.push_back(std::move(parse_procedure()));
        } else if (check(TokenType::Class)) {
            module->classes.push_back(std::move(parse_class()));
        } else if (check(TokenType::Interface)) {
            module->interfaces.push_back(std::move(parse_interface()));
        } else if (check(TokenType::Enum)) {
            module->enums.push_back(std::move(parse_enum()));
        } else if (check(TokenType::Structure)) {
            module->structures.push_back(std::move(parse_structure()));
        } else if (check(TokenType::Delegate)) {
            module->delegates.push_back(std::move(parse_delegate()));
        } else if (check(TokenType::Form)) {
            module->forms.push_back(std::move(parse_form()));
        } else if (check(TokenType::Namespace)) {
            module->namespaces.push_back(std::move(parse_namespace()));
        } else if (check(TokenType::Dim) || check(TokenType::Const)) {
            module->procedures.push_back(std::move(parse_module_level_decl()));
        } else {
            error_at_current("Unexpected declaration in module");
            advance();
        }
    }

    consume_end(TokenType::Module);

    return module;
}

// Parse module-level Dim/Const declaration
std::unique_ptr<SubOrFunction> Parser::parse_module_level_decl() {
    auto proc = std::make_unique<SubOrFunction>();
    proc->name = "<module_var>";
    proc->is_function = false;
    proc->return_type.kind = PrimitiveType::Void;

    // Check for Const
    bool is_const = match(TokenType::Const);
    if (!is_const) {
        // Must be Dim
        consume(TokenType::Dim, "Expected 'Dim'");
    }

    Block block;
    if (check(TokenType::Identifier)) {
        DimStmt dim;
        dim.name = current_.text;
        advance();
        dim.is_const = is_const;

        // Optional array bounds
        if (match(TokenType::LParen)) {
            dim.array_size = parse_expression();
            consume(TokenType::RParen, "Expected ')'");
        }

        // Optional type
        if (match(TokenType::As)) {
            dim.type = parse_type();
            set_symbol_type(dim.name, dim.type.kind);
        } else {
            dim.type_inferred = true;
            dim.type.kind = PrimitiveType::Integer;
        }

        // Optional initializer
        if (match(TokenType::Assign)) {
            dim.initializer = parse_expression();
        }

        // Wrap in a statement and add to block
        auto stmt = std::make_unique<Statement>();
        stmt->data = DimStmt(std::move(dim));
        block.statements.push_back(std::move(stmt));
    }

    proc->body = std::move(block);
    return proc;
}

// ============================================================================
// Procedures (Sub / Function)
// ============================================================================

std::unique_ptr<SubOrFunction> Parser::parse_procedure() {
    auto proc = std::make_unique<SubOrFunction>();

    bool is_function = check(TokenType::Function);
    advance(); // consume Sub or Function
    proc->is_function = is_function;

    // Optional access modifier
    if (check(TokenType::Public) || check(TokenType::Private) ||
        check(TokenType::Protected) || check(TokenType::Friend)) {
        advance();
    }

    SourceLocation name_loc;
    if (check(TokenType::Identifier)) {
        proc->name = current_.text;
        name_loc = current_.location;
        advance();
    } else {
        error_at_current("Expected procedure name");
        proc->name = "<unknown>";
    }

    // Optional type parameters (generics)
    if (check(TokenType::Of)) {
        proc->type_parameters = parse_type_parameters();
    }

    // Parameter list
    if (match(TokenType::LParen)) {
        proc->parameters = parse_parameters();
        consume(TokenType::RParen, "Expected ')' after parameters");
    }

    // Return type (for Function)
    if (is_function) {
        if (match(TokenType::As)) {
            proc->return_type = parse_type();
        } else {
            error_at_current("Expected 'As' before return type");
            proc->return_type.kind = PrimitiveType::Void;
        }
    } else {
        proc->return_type.kind = PrimitiveType::Void;
    }

    add_symbol(proc->name, is_function ? "Function" : "Sub",
               primitive_type_name(proc->return_type.kind), name_loc);

    // Body
    proc->body = parse_block_until(TokenType::End);
    consume(TokenType::End, "Expected 'End'");
    if (proc->is_function) {
        consume(TokenType::Function, "Expected 'End Function'");
    } else {
        consume(TokenType::Sub, "Expected 'End Sub'");
    }

    return proc;
}

TypeRef Parser::parse_type() {
    TypeRef type;

    if (check(TokenType::Integer)) type.kind = PrimitiveType::Integer;
    else if (check(TokenType::Long)) type.kind = PrimitiveType::Long;
    else if (check(TokenType::Single)) type.kind = PrimitiveType::Single;
    else if (check(TokenType::Double)) type.kind = PrimitiveType::Double;
    else if (check(TokenType::Boolean)) type.kind = PrimitiveType::Boolean;
    else if (check(TokenType::String)) type.kind = PrimitiveType::String;
    else {
        // Could be a user-defined type or generic type
        if (check(TokenType::Identifier)) {
            type.generic_name = current_.text;
            type.is_generic = true;
            type.kind = PrimitiveType::Integer; // Default to Integer for user-defined types
            advance();

            // Check for generic type arguments: Of (Type1, Type2, ...)
            if (match(TokenType::Of)) {
                consume(TokenType::LParen, "Expected '(' after 'Of'");
                while (current_.type != TokenType::RParen && current_.type != TokenType::Eof) {
                    TypeRef arg_type = parse_type();
                    type.type_arguments.push_back(std::move(arg_type));
                    if (!match(TokenType::Comma)) break;
                }
                consume(TokenType::RParen, "Expected ')' after generic type arguments");
            }
            return type;
        } else {
            error_at_current("Expected type name");
            type.kind = PrimitiveType::Integer;
            return type;
        }
    }
    
    // Built-in type matched - advance past the type keyword
    advance();

    // Array suffix
    if (match(TokenType::LParen)) {
        type.is_array = true;
        if (!check(TokenType::RParen)) {
            // Parse array size expression but don't store it in TypeRef
            // The size will be handled during codegen
            parse_expression();
        }
        consume(TokenType::RParen, "Expected ')' after array size");
    }

    return type;
}

std::vector<Parameter> Parser::parse_parameters() {
    std::vector<Parameter> params;

    while (current_.type != TokenType::RParen &&
           current_.type != TokenType::Eof) {
        Parameter p;

        // Optional ByVal/ByRef (skip)
        if (check(TokenType::ByVal) || check(TokenType::ByRef)) {
            advance();
        }

        // Optional Optional keyword
        if (check(TokenType::Optional)) {
            p.is_optional = true;
            advance();
        }

        SourceLocation name_loc;
        if (check(TokenType::Identifier)) {
            p.name = current_.text;
            name_loc = current_.location;
            advance();
        } else {
            error_at_current("Expected parameter name");
            p.name = "<unknown>";
        }

        // Optional array marker: name()
        if (match(TokenType::LParen)) {
            consume(TokenType::RParen, "Expected ')'");
            p.is_array = true;
        }

        consume(TokenType::As, "Expected 'As' after parameter name");
        p.type = parse_type();
        add_symbol(p.name, "Parameter", primitive_type_name(p.type.kind), name_loc);

        // Optional default value for optional parameters
        if (p.is_optional && match(TokenType::Assign)) {
            p.default_value = parse_expression();
        }

        params.push_back(std::move(p));

        if (!match(TokenType::Comma)) break;
    }

    return params;
}

// ============================================================================
// Enum
// ============================================================================

std::unique_ptr<EnumDecl> Parser::parse_enum() {
    auto en = std::make_unique<EnumDecl>();

    consume(TokenType::Enum, "Expected 'Enum'");
    if (check(TokenType::Identifier)) {
        en->name = current_.text;
        advance();
    } else {
        en->name = "<unknown>";
        error_at_current("Expected enum name");
    }

    while (current_.type != TokenType::Eof &&
           current_.type != TokenType::End) {
        if (check(TokenType::Identifier)) {
            EnumDecl::Member m;
            m.name = current_.text;
            m.value = 0;
            advance();

            if (match(TokenType::Assign)) {
                if (check(TokenType::IntegerLiteral)) {
                    m.value = std::stoll(current_.text);
                    advance();
                }
            }

            en->members.push_back(m);
        } else {
            advance();
        }
    }

    consume_end(TokenType::Enum);

    return en;
}

// ============================================================================
// Class
// ============================================================================

std::unique_ptr<ClassDecl> Parser::parse_class() {
    auto cls = std::make_unique<ClassDecl>();

    consume(TokenType::Class, "Expected 'Class'");
    SourceLocation class_loc;
    if (check(TokenType::Identifier)) {
        cls->name = current_.text;
        class_loc = current_.location;
        advance();
    } else {
        cls->name = "<unknown>";
        error_at_current("Expected class name");
    }
    add_symbol(cls->name, "Class", "", class_loc);

    // Optional access modifier
    if (check(TokenType::Public) || check(TokenType::Private) ||
        check(TokenType::Protected) || check(TokenType::Friend)) {
        advance();
    }

    // Optional generic type parameters: (Of T1, T2, ...)
    if (match(TokenType::LParen)) {
        if (match(TokenType::Of)) {
            while (current_.type != TokenType::RParen && current_.type != TokenType::Eof) {
                TypeParameter param;
                if (check(TokenType::Identifier)) {
                    param.name = current_.text;
                    advance();
                }
                cls->type_parameters.parameters.push_back(std::move(param));
                if (!match(TokenType::Comma)) break;
            }
            consume(TokenType::RParen, "Expected ')' after type parameters");
        } else {
            // Not a generic declaration, skip to closing paren
            while (current_.type != TokenType::RParen && current_.type != TokenType::Eof) {
                advance();
            }
            consume(TokenType::RParen, "Expected ')'");
        }
    }

    // Optional Implements
    if (match(TokenType::Implements)) {
        while (check(TokenType::Identifier)) {
            cls->implements.push_back(current_.text);
            advance();
            if (!match(TokenType::Comma)) break;
        }
    }

    while (current_.type != TokenType::Eof &&
           current_.type != TokenType::End) {
        if (check(TokenType::Sub) || check(TokenType::Function)) {
            cls->methods.push_back(std::move(parse_class_method()));
        } else if (check(TokenType::Property)) {
            cls->properties.push_back(std::move(parse_class_property()));
        } else if (check(TokenType::Dim)) {
            advance();
            // Parse property/field declaration
            if (check(TokenType::Identifier)) {
                ClassProperty prop;
                prop.name = current_.text;
                prop.is_public = true;
                advance();
                consume(TokenType::As, "Expected 'As'");
                prop.type = parse_type();
                cls->properties.push_back(std::move(prop));
            }
        } else if (check(TokenType::Identifier)) {
            // Bare field declaration: Name As Type
            ClassProperty prop;
            prop.name = current_.text;
            prop.is_public = true;
            advance();
            consume(TokenType::As, "Expected 'As'");
            prop.type = parse_type();
            cls->properties.push_back(std::move(prop));
        } else if (check(TokenType::Public) ||
                    check(TokenType::Private) ||
                    check(TokenType::Protected) ||
                    check(TokenType::Friend)) {
            // Access modifier followed by member
            advance();
            if (check(TokenType::Sub) || check(TokenType::Function)) {
                cls->methods.push_back(std::move(parse_class_method()));
            } else if (check(TokenType::Property)) {
                cls->properties.push_back(std::move(parse_class_property()));
            } else if (check(TokenType::Identifier)) {
                ClassProperty prop;
                prop.name = current_.text;
                prop.is_public = true;
                advance();
                consume(TokenType::As, "Expected 'As'");
                prop.type = parse_type();
                cls->properties.push_back(std::move(prop));
            }
        } else {
            error_at_current("Unexpected member in class");
            advance();
        }
    }

    consume_end(TokenType::Class);

    return cls;
}

std::unique_ptr<FormDecl> Parser::parse_form() {
    auto form = std::make_unique<FormDecl>();
    consume(TokenType::Form, "Expected 'Form'");

    if (check(TokenType::Identifier)) {
        form->name = current_.text;
        add_symbol(form->name, "Form", "", current_.location);
        advance();
    } else {
        form->name = "<unknown>";
        error_at_current("Expected form name");
    }

    while (!check(TokenType::Eof) && !check(TokenType::End)) {
        if (!check(TokenType::Identifier)) {
            advance();
            continue;
        }

        const std::string word = current_.text;
        const std::string lower = bf_lower(word);
        if (std::getenv("BFC_DEBUG_FORM")) {
            std::cerr << "FORM " << word << " -> " << lower
                      << " ctrl=" << bf_is_control_type(lower) << "\n";
        }

        if (bf_is_control_type(lower)) {
            advance(); // control type
            FormControlDecl ctrl;
            ctrl.type = bf_normalize_control(lower);

            if (check(TokenType::Identifier)) {
                ctrl.name = current_.text;
                add_symbol(ctrl.name, "Control", "", current_.location);
                advance();
            } else {
                ctrl.name = "<unknown>";
                error_at_current("Expected control name");
            }

            while (!check(TokenType::Eof) && !check(TokenType::End)) {
                if (!check(TokenType::Identifier)) {
                    advance();
                    continue;
                }
                const std::string key = current_.text;
                advance();
                if (!match(TokenType::Assign)) continue;

                std::string value;
                if (check(TokenType::StringLiteral) || check(TokenType::IntegerLiteral) ||
                    check(TokenType::DoubleLiteral) || check(TokenType::Identifier) ||
                    check(TokenType::True) || check(TokenType::False)) {
                    value = current_.text;
                    advance();
                }

                if (bf_lower(key).rfind("on", 0) == 0 && key.size() > 2) {
                    ctrl.events[key.substr(2)] = value;
                } else {
                    ctrl.properties[key] = value;
                }
            }

            consume(TokenType::End, "Expected 'End'");
            if (check(TokenType::Identifier)) advance(); // control type
            form->controls.push_back(std::move(ctrl));
            continue;
        }

        // Form-level property: Name = value
        advance(); // property name
        if (match(TokenType::Assign)) {
            std::string value;
            if (check(TokenType::StringLiteral) || check(TokenType::IntegerLiteral) ||
                check(TokenType::Identifier) || check(TokenType::True) ||
                check(TokenType::False)) {
                value = current_.text;
                advance();
            }
            form->properties[word] = value;
        }
    }

    consume(TokenType::End, "Expected 'End Form'");
    consume(TokenType::Form, "Expected 'Form'");
    return form;
}

// Structure
// ============================================================================

std::unique_ptr<StructDecl> Parser::parse_structure() {
    auto st = std::make_unique<StructDecl>();

    consume(TokenType::Structure, "Expected 'Structure'");
    if (check(TokenType::Identifier)) {
        st->name = current_.text;
        advance();
    } else {
        st->name = "<unknown>";
        error_at_current("Expected structure name");
    }

    // Optional access modifier
    if (check(TokenType::Public) || check(TokenType::Private) ||
        check(TokenType::Protected) || check(TokenType::Friend)) {
        advance();
    }

    while (current_.type != TokenType::Eof &&
           current_.type != TokenType::End) {
        if (check(TokenType::Dim)) {
            advance();
            if (check(TokenType::Identifier)) {
                ClassProperty prop;
                prop.name = current_.text;
                prop.is_public = true;
                advance();
                consume(TokenType::As, "Expected 'As'");
                prop.type = parse_type();
                st->fields.push_back(std::move(prop));
            }
        } else if (check(TokenType::Identifier)) {
            // Bare field declaration: Name As Type
            ClassProperty prop;
            prop.name = current_.text;
            prop.is_public = true;
            advance();
            consume(TokenType::As, "Expected 'As'");
            prop.type = parse_type();
            st->fields.push_back(std::move(prop));
        } else if (check(TokenType::Public) ||
                    check(TokenType::Private) ||
                    check(TokenType::Protected) ||
                    check(TokenType::Friend)) {
            advance();
            if (check(TokenType::Dim)) {
                advance();
                if (check(TokenType::Identifier)) {
                    ClassProperty prop;
                    prop.name = current_.text;
                    prop.is_public = true;
                    advance();
                    consume(TokenType::As, "Expected 'As'");
                    prop.type = parse_type();
                    st->fields.push_back(std::move(prop));
                }
            }
        } else {
            error_at_current("Unexpected member in structure");
            advance();
        }
    }

    consume_end(TokenType::Structure);

    return st;
}

// Namespace
// ============================================================================

std::unique_ptr<NamespaceDecl> Parser::parse_namespace() {
    auto ns = std::make_unique<NamespaceDecl>();

    consume(TokenType::Namespace, "Expected 'Namespace'");
    if (check(TokenType::Identifier)) {
        ns->name = current_.text;
        advance();
    } else {
        ns->name = "<unknown>";
        error_at_current("Expected namespace name");
    }

    while (current_.type != TokenType::Eof &&
           current_.type != TokenType::End) {
        if (check(TokenType::Sub) || check(TokenType::Function)) {
            ns->procedures.push_back(std::move(parse_procedure()));
        } else if (check(TokenType::Class)) {
            ns->classes.push_back(std::move(parse_class()));
        } else if (check(TokenType::Interface)) {
            ns->interfaces.push_back(std::move(parse_interface()));
        } else if (check(TokenType::Enum)) {
            ns->enums.push_back(std::move(parse_enum()));
        } else if (check(TokenType::Structure)) {
            ns->structures.push_back(std::move(parse_structure()));
        } else if (check(TokenType::Delegate)) {
            ns->delegates.push_back(std::move(parse_delegate()));
        } else if (check(TokenType::Form)) {
            ns->forms.push_back(std::move(parse_form()));
        } else if (check(TokenType::Namespace)) {
            ns->namespaces.push_back(std::move(parse_namespace()));
        } else if (check(TokenType::Dim) || check(TokenType::Const)) {
            ns->procedures.push_back(std::move(parse_module_level_decl()));
        } else {
            error_at_current("Unexpected declaration in namespace");
            advance();
        }
    }

    consume_end(TokenType::Namespace);

    return ns;
}

// Delegate
// ============================================================================

std::unique_ptr<DelegateDecl> Parser::parse_delegate() {
    auto d = std::make_unique<DelegateDecl>();

    consume(TokenType::Delegate, "Expected 'Delegate'");
    if (check(TokenType::Sub) || check(TokenType::Function)) {
        bool is_function = check(TokenType::Function);
        advance();
        d->is_function = is_function;

        if (check(TokenType::Identifier)) {
            d->name = current_.text;
            advance();
        } else {
            d->name = "<unknown>";
            error_at_current("Expected delegate name");
        }

        if (match(TokenType::LParen)) {
            d->parameters = parse_parameters();
            consume(TokenType::RParen, "Expected ')' after parameters");
        }

        if (is_function) {
            if (match(TokenType::As)) {
                d->return_type = parse_type();
            } else {
                error_at_current("Expected 'As' before return type");
                d->return_type.kind = PrimitiveType::Void;
            }
        } else {
            d->return_type.kind = PrimitiveType::Void;
        }
    } else {
        error_at_current("Expected 'Sub' or 'Function' after 'Delegate'");
    }

    return d;
}

ClassProperty Parser::parse_class_property() {
    ClassProperty prop;
    prop.is_public = true;

    consume(TokenType::Property, "Expected 'Property'");

    // Access modifier
    if (check(TokenType::Public) || check(TokenType::Private) ||
        check(TokenType::Protected) || check(TokenType::Friend)) {
        prop.is_public = (current_.type == TokenType::Public);
        advance();
    }

    SourceLocation prop_loc;
    if (check(TokenType::Identifier)) {
        prop.name = current_.text;
        prop_loc = current_.location;
        advance();
    } else {
        prop.name = "<unknown>";
        error_at_current("Expected property name");
    }

    consume(TokenType::As, "Expected 'As'");
    prop.type = parse_type();
    add_symbol(prop.name, "Property", primitive_type_name(prop.type.kind), prop_loc);

    return prop;
}

ClassMethod Parser::parse_class_method() {
    ClassMethod method;
    method.is_public = true;

    bool is_function = check(TokenType::Function);
    advance(); // Sub or Function

    // Access modifier
    if (check(TokenType::Public) || check(TokenType::Private) ||
        check(TokenType::Protected) || check(TokenType::Friend)) {
        method.is_public = (current_.type == TokenType::Public);
        advance();
    }

    SourceLocation method_loc;
    // Constructor: Sub New(...)
    if (check(TokenType::New)) {
        method.name = "New";
        method.is_constructor = true;
        method_loc = current_.location;
        advance();
    } else if (check(TokenType::Identifier)) {
        method.name = current_.text;
        method_loc = current_.location;
        advance();

        // Constructor detection (name same as class)
        if (method.name == "New") {
            method.is_constructor = true;
        }
    } else {
        method.name = "<unknown>";
        error_at_current("Expected method name");
    }

    if (match(TokenType::LParen)) {
        method.parameters = parse_parameters();
        consume(TokenType::RParen, "Expected ')' after parameters");
    }

    if (is_function) {
        method.is_function = true;
        if (match(TokenType::As)) {
            method.return_type = parse_type();
        }
    } else {
        method.return_type.kind = PrimitiveType::Void;
    }

    method.body = parse_block_until(TokenType::End);
    consume(TokenType::End, "Expected 'End'");
    if (method.is_function) {
        consume(TokenType::Function, "Expected 'End Function'");
    } else {
        consume(TokenType::Sub, "Expected 'End Sub'");
    }

    add_symbol(method.name,
               method.is_constructor ? "Constructor"
                                     : (method.is_function ? "Function" : "Sub"),
               primitive_type_name(method.return_type.kind), method_loc);

    return method;
}

// ============================================================================
// Interface
// ============================================================================

std::unique_ptr<InterfaceDecl> Parser::parse_interface() {
    auto iface = std::make_unique<InterfaceDecl>();

    consume(TokenType::Interface, "Expected 'Interface'");
    if (check(TokenType::Identifier)) {
        iface->name = current_.text;
        advance();
    } else {
        iface->name = "<unknown>";
        error_at_current("Expected interface name");
    }

    while (current_.type != TokenType::Eof &&
           current_.type != TokenType::End) {
        if (check(TokenType::Sub) || check(TokenType::Function)) {
            InterfaceMember member;
            member.is_function = (current_.type == TokenType::Function);
            advance();
            if (check(TokenType::Identifier)) {
                member.name = current_.text;
                advance();
            }
            if (match(TokenType::LParen)) {
                while (current_.type != TokenType::RParen &&
                       current_.type != TokenType::Eof) {
                    Parameter p;
                    if (check(TokenType::Identifier)) {
                        p.name = current_.text;
                        advance();
                    }
                    consume(TokenType::As, "Expected 'As'");
                    p.type = parse_type();
                    member.parameters.push_back(std::move(p));
                    if (!match(TokenType::Comma)) break;
                }
                consume(TokenType::RParen, "Expected ')'");
            }
            if (match(TokenType::As)) {
                member.return_type = parse_type();
            }
            iface->members.push_back(std::move(member));
        } else {
            error_at_current("Unexpected member in interface");
            advance();
        }
    }

    consume_end(TokenType::Interface);

    return iface;
}

TypeParameterList Parser::parse_type_parameters() {
    TypeParameterList list;
    consume(TokenType::Of, "Expected 'Of'");
    consume(TokenType::LParen, "Expected '(' after 'Of'");

    while (current_.type != TokenType::RParen &&
           current_.type != TokenType::Eof) {
        TypeParameter tp;
        if (check(TokenType::Identifier)) {
            tp.name = current_.text;
            advance();
        }
        list.parameters.push_back(tp);
        if (!match(TokenType::Comma)) break;
    }

    consume(TokenType::RParen, "Expected ')' after type parameters");
    return list;
}

// ============================================================================
// Exception handling
// ============================================================================

std::unique_ptr<Statement> Parser::parse_try_catch_finally() {
    auto stmt = std::make_unique<Statement>();
    auto& tcf = stmt->data.emplace<TryCatchFinallyStmt>();

    consume(TokenType::Try, "Expected 'Try'");

    // Try body - parse until Catch, Finally, or End
    tcf.try_body = parse_block_terminators(
        {TokenType::Catch, TokenType::Finally, TokenType::End}, {}).statements;

    // Catch clauses
    while (check(TokenType::Catch)) {
        advance(); // Catch
        CatchClause cc;
        // Optional: Catch [ex [As type]]
        if (check(TokenType::Identifier)) {
            cc.exception_variable = current_.text;
            advance();
            if (match(TokenType::As)) {
                cc.exception_type = parse_type();
            }
        }
        // Catch body - parse until Catch, Finally, or End
        cc.body = parse_block_terminators(
            {TokenType::Catch, TokenType::Finally, TokenType::End}, {}).statements;
        tcf.catch_clauses.push_back(std::move(cc));
    }

    // Finally
    if (match(TokenType::Finally)) {
        tcf.finally_body = parse_block_terminators(
            {TokenType::End}, {}).statements;
    }

    consume_end(TokenType::Try);

    return stmt;
}

std::unique_ptr<Statement> Parser::parse_throw() {
    auto stmt = std::make_unique<Statement>();
    auto& t = stmt->data.emplace<ThrowStmt>();

    consume(TokenType::Throw, "Expected 'Throw'");
    t.expression = parse_expression();

    return stmt;
}

// ============================================================================
// Statements
// ============================================================================

std::unique_ptr<Statement> Parser::parse_statement() {
    switch (current_.type) {
        case TokenType::Dim: {
            auto stmt = std::make_unique<Statement>();
            stmt->data = DimStmt(std::move(*parse_dim()));
            return stmt;
        }
        case TokenType::Const: {
            auto stmt = std::make_unique<Statement>();
            stmt->data = DimStmt(std::move(*parse_const()));
            return stmt;
        }
        case TokenType::Print: {
            auto stmt = std::make_unique<Statement>();
            stmt->data = PrintStmt(std::move(*parse_print()));
            return stmt;
        }
        case TokenType::Return: {
            auto stmt = std::make_unique<Statement>();
            stmt->data = ReturnStmt(std::move(*parse_return()));
            return stmt;
        }
        case TokenType::If: {
            auto stmt = std::make_unique<Statement>();
            stmt->data = IfStmt(std::move(*parse_if()));
            return stmt;
        }
        case TokenType::For: {
            auto stmt = std::make_unique<Statement>();
            stmt->data = ForStmt(std::move(*parse_for()));
            return stmt;
        }
        case TokenType::Each: {
            auto stmt = std::make_unique<Statement>();
            stmt->data = ForEachStmt(std::move(*parse_for_each()));
            return stmt;
        }
        case TokenType::Do: {
            auto stmt = std::make_unique<Statement>();
            stmt->data = DoLoopStmt(std::move(*parse_do_loop()));
            return stmt;
        }
        case TokenType::While: {
            auto stmt = std::make_unique<Statement>();
            stmt->data = WhileStmt(std::move(*parse_while()));
            return stmt;
        }
        case TokenType::Select: {
            auto stmt = std::make_unique<Statement>();
            stmt->data = SelectCaseStmt(std::move(*parse_select_case()));
            return stmt;
        }
        case TokenType::Try:
            return parse_try_catch_finally();
        case TokenType::Throw:
            return parse_throw();
        case TokenType::Identifier: {
            return parse_assignment_lhs_stmt();
        }
        case TokenType::Me: {
            // Me.member = value or Me.member access
            auto stmt = std::make_unique<Statement>();
            advance(); // consume 'Me'
            if (match(TokenType::Dot)) {
                if (check(TokenType::Identifier)) {
                    std::string field_name = current_.text;
                    advance();
                    if (match(TokenType::Assign)) {
                        // Me.field = value
                        auto& fas = stmt->data.emplace<FieldAssignmentStmt>();
                        auto meExpr = std::make_unique<Expression>();
                        meExpr->data = MeExpr{};
                        fas.object = std::move(meExpr);
                        fas.field_name = field_name;
                        fas.expression = parse_expression();
                        return stmt;
                    } else {
                        // Me.field access as expression statement
                        auto& es = stmt->data.emplace<ExpressionStmt>();
                        auto meExpr = std::make_unique<Expression>();
                        meExpr->data = MeExpr{};
                        auto memberExpr = std::make_unique<Expression>();
                        memberExpr->data = MemberAccessExpr{std::move(meExpr), field_name};
                        es.expression = std::move(memberExpr);
                        return stmt;
                    }
                } else {
                    error_at_current("Expected field name after 'Me.'");
                    advance();
                    auto& es = stmt->data.emplace<ExpressionStmt>();
                    auto meExpr = std::make_unique<Expression>();
                    meExpr->data = MeExpr{};
                    es.expression = std::move(meExpr);
                    return stmt;
                }
            } else {
                error_at_current("Expected '.' after 'Me'");
                auto& es = stmt->data.emplace<ExpressionStmt>();
                auto meExpr = std::make_unique<Expression>();
                meExpr->data = MeExpr{};
                es.expression = std::move(meExpr);
                return stmt;
            }
        }
        default: {
            auto inner = parse_expression_stmt();
            auto stmt = std::make_unique<Statement>();
            stmt->data = ExpressionStmt(std::move(*inner));
            if (!inner) {
                error_at_current("Unexpected statement");
                advance();
            }
            return stmt;
        }
    }
}

std::unique_ptr<Expression> Parser::parse_expression_fragment(const std::string& text) {
    Lexer lexer(text);
    Parser parser(lexer, sink_);
    return parser.parse_expression();
}

std::unique_ptr<Expression> Parser::parse_interpolated_text(const std::string& text) {
    std::vector<std::unique_ptr<Expression>> parts;
    std::string literal;
    size_t i = 0;
    while (i < text.size()) {
        if (text[i] == '{') {
            if (i + 1 < text.size() && text[i + 1] == '{') {
                literal += '{';
                i += 2;
                continue;
            }
            if (!parts.empty() || !literal.empty()) {
                auto lit = std::make_unique<Expression>();
                lit->data = StringLiteralExpr{literal};
                parts.push_back(std::move(lit));
                literal.clear();
            }
            size_t end = text.find('}', i + 1);
            if (end == std::string::npos) {
                literal += text.substr(i);
                break;
            }
            std::string expr_text = text.substr(i + 1, end - i - 1);
            parts.push_back(parse_expression_fragment(expr_text));
            i = end + 1;
        } else if (text[i] == '}') {
            if (i + 1 < text.size() && text[i + 1] == '}') {
                literal += '}';
                i += 2;
                continue;
            }
            literal += '}';
            ++i;
        } else {
            literal += text[i];
            ++i;
        }
    }
    if (!parts.empty() || !literal.empty()) {
        auto lit = std::make_unique<Expression>();
        lit->data = StringLiteralExpr{literal};
        parts.push_back(std::move(lit));
    }
    if (parts.empty()) {
        auto lit = std::make_unique<Expression>();
        lit->data = StringLiteralExpr{""};
        return lit;
    }
    auto result = std::move(parts[0]);
    for (size_t k = 1; k < parts.size(); ++k) {
        auto expr = std::make_unique<Expression>();
        expr->data = BinaryExpr{BinaryExpr::Op::Concat, std::move(result),
                                std::move(parts[k])};
        result = std::move(expr);
    }
    return result;
}

std::unique_ptr<Statement> Parser::parse_assignment_lhs_stmt() {
    // Save state and try to parse as assignment
    auto saved = lexer_.save_state();
    Token saved_token = current_;

    // Parse identifier
    if (!check(TokenType::Identifier)) {
        lexer_.restore_state(saved);
        current_ = saved_token;
        {
            auto stmt = std::make_unique<Statement>();
            stmt->data = ExpressionStmt(std::move(*parse_expression_stmt()));
            return stmt;
        }
    }
    std::string name = current_.text;
    advance();

    // Method call statement: obj.Method(args) or obj.Method (no parens).
    // This must be checked before the field-assignment path, otherwise
    // "p.Hi()" is parsed as "p.Hi" followed by an expected '='.
    if (check(TokenType::Dot)) {
        auto dot_state = lexer_.save_state();
        Token dot_token = current_;
        advance(); // consume '.'
        if (check(TokenType::Identifier)) {
            advance(); // consume method name
            if (check(TokenType::LParen)) {
                lexer_.restore_state(saved);
                current_ = saved_token;
                auto inner = parse_expression_stmt();
                auto stmt = std::make_unique<Statement>();
                stmt->data = ExpressionStmt(std::move(*inner));
                return stmt;
            }
        }
        lexer_.restore_state(dot_state);
        current_ = dot_token;
    }

    // Check for array assignment or a call statement: name(...)
    if (check(TokenType::LParen)) {
        // Look ahead to the matching ')' and check for a following '='.
        auto ahead_state = lexer_.save_state();
        int depth = 0;
        bool is_array_assign = false;
        Token t = current_;
        while (t.type != TokenType::Eof) {
            if (t.type == TokenType::LParen) {
                depth++;
            } else if (t.type == TokenType::RParen) {
                depth--;
                if (depth == 0) {
                    t = lexer_.next_token();
                    is_array_assign = (t.type == TokenType::Assign);
                    break;
                }
            }
            t = lexer_.next_token();
        }
        lexer_.restore_state(ahead_state);

        if (is_array_assign) {
            auto stmt = std::make_unique<Statement>();
            auto& asg = stmt->data.emplace<ArrayAssignmentStmt>();
            asg.name = name;
            advance(); // consume (
            asg.index = parse_expression();
            consume(TokenType::RParen, "Expected ')'");
            consume(TokenType::Assign, "Expected '='");
            asg.expression = parse_expression();
            return stmt;
        }

        // Function-call statement (e.g. bf_form_test_click(0)).
        lexer_.restore_state(saved);
        current_ = saved_token;
        auto expr = parse_expression();
        auto stmt = std::make_unique<Statement>();
        auto& es = stmt->data.emplace<ExpressionStmt>();
        es.expression = std::move(expr);
        return stmt;
    }

    // Check for field assignment: obj.field = expr or obj.method() = expr (unlikely)
    if (check(TokenType::Dot)) {
        // Parse member access assignment
        auto stmt = std::make_unique<Statement>();
        auto& fas = stmt->data.emplace<FieldAssignmentStmt>();

        // Parse object expression
        auto obj = std::make_unique<Expression>();
        obj->data = IdentifierExpr{name};
        advance(); // consume dot

        if (check(TokenType::Identifier)) {
            fas.field_name = current_.text;
            advance();
        } else {
            error_at_current("Expected field name");
            fas.field_name = "<unknown>";
        }

consume(TokenType::Assign, "Expected '='");
        fas.object = std::move(obj);
        fas.expression = parse_expression();

        return stmt;
    }

    // Simple assignment: name = expr
    if (check(TokenType::Assign)) {
        auto stmt = std::make_unique<Statement>();
        auto& asg = stmt->data.emplace<AssignmentStmt>();
        asg.name = name;
        advance(); // consume =
        asg.expression = parse_expression();
        return stmt;
    }

    lexer_.restore_state(saved);
    {
        auto stmt = std::make_unique<Statement>();
        stmt->data = ExpressionStmt(std::move(*parse_expression_stmt()));
        return stmt;
    }
}

std::unique_ptr<DimStmt> Parser::parse_dim() {
    auto stmt = std::make_unique<DimStmt>();
    consume(TokenType::Dim, "Expected 'Dim'");

    SourceLocation name_loc;
    if (check(TokenType::Identifier)) {
        stmt->name = current_.text;
        name_loc = current_.location;
        if (std::getenv("BFC_LSP_DEBUG")) {
            std::cerr << "DIM " << stmt->name << " at " << name_loc.line << ":"
                      << name_loc.column << " off=" << name_loc.offset << "\n";
        }
        advance();
    } else {
        error_at_current("Expected variable name");
        stmt->name = "<unknown>";
    }
    add_symbol(stmt->name, "Variable", "", name_loc);

    // Optional array bounds
    if (match(TokenType::LParen)) {
        stmt->array_size = parse_expression();
        stmt->type.is_array = true; // Mark as array
        consume(TokenType::RParen, "Expected ')'");
    }

    // Optional type
    if (match(TokenType::As)) {
        // Check for As New ClassName(...) syntax
        if (match(TokenType::New)) {
            // As New ClassName(...) - create NewExpr as initializer
            std::string class_name;
            if (check(TokenType::Identifier)) {
                class_name = current_.text;
                advance();
            }
            // Check for generic type args: (Of Type1, Type2, ...)
            if (match(TokenType::LParen)) {
                if (match(TokenType::Of)) {
                    // Skip generic type arguments
                    while (current_.type != TokenType::RParen && current_.type != TokenType::Eof) {
                        parse_type();
                        if (!match(TokenType::Comma)) break;
                    }
                    consume(TokenType::RParen, "Expected ')' after type arguments");
                } else {
                    // Regular constructor arguments - need to put back the LParen
                    // We already consumed LParen, so parse args directly
                    std::vector<std::unique_ptr<Expression>> args;
                    while (current_.type != TokenType::RParen && current_.type != TokenType::Eof) {
                        args.push_back(parse_expression());
                        if (!match(TokenType::Comma)) break;
                    }
                    consume(TokenType::RParen, "Expected ')' after constructor arguments");
                    // Create NewExpr as initializer
                    auto newExpr = std::make_unique<Expression>();
                    NewExpr ne;
                    ne.class_name = class_name;
                    ne.arguments = std::move(args);
                    newExpr->data = std::move(ne);
                    stmt->initializer = std::move(newExpr);
                    stmt->type.kind = PrimitiveType::Integer;
                    stmt->type.generic_name = class_name;
                    stmt->type.is_generic = true;
                    return stmt;
                }
                // Optional constructor arguments after generic type args
                std::vector<std::unique_ptr<Expression>> args;
                if (match(TokenType::LParen)) {
                    while (current_.type != TokenType::RParen && current_.type != TokenType::Eof) {
                        args.push_back(parse_expression());
                        if (!match(TokenType::Comma)) break;
                    }
                    consume(TokenType::RParen, "Expected ')' after constructor arguments");
                }
                auto newExpr = std::make_unique<Expression>();
                NewExpr ne;
                ne.class_name = class_name;
                ne.arguments = std::move(args);
                newExpr->data = std::move(ne);
                stmt->initializer = std::move(newExpr);
                stmt->type.kind = PrimitiveType::Integer;
                stmt->type.generic_name = class_name;
                stmt->type.is_generic = true;
                return stmt;
            }
            // No parens - just As New ClassName
            auto newExpr = std::make_unique<Expression>();
            NewExpr ne;
            ne.class_name = class_name;
            newExpr->data = std::move(ne);
            stmt->initializer = std::move(newExpr);
            stmt->type.kind = PrimitiveType::Integer;
            stmt->type.generic_name = class_name;
            stmt->type.is_generic = true;
            return stmt;
        }
        stmt->type = parse_type();
        set_symbol_type(stmt->name, stmt->type.kind);
        // Preserve array flag set before type parsing
        if (stmt->array_size) {
            stmt->type.is_array = true;
        }
    } else {
        stmt->type_inferred = true;
        stmt->type.kind = PrimitiveType::Integer;
    }

    // Optional initializer
    if (match(TokenType::Assign)) {
        stmt->initializer = parse_expression();
    }

    return stmt;
}

std::unique_ptr<DimStmt> Parser::parse_const() {
    auto stmt = std::make_unique<DimStmt>();
    stmt->is_const = true;
    consume(TokenType::Const, "Expected 'Const'");

    if (check(TokenType::Identifier)) {
        stmt->name = current_.text;
        advance();
    } else {
        error_at_current("Expected constant name");
        stmt->name = "<unknown>";
    }

    // Optional type
    if (match(TokenType::As)) {
        stmt->type = parse_type();
    } else {
        stmt->type_inferred = true;
        stmt->type.kind = PrimitiveType::Integer;
    }

    // Required initializer for constants
    if (match(TokenType::Assign)) {
        stmt->initializer = parse_expression();
    } else {
        error_at_current("Constant must have an initializer");
        stmt->initializer = std::make_unique<Expression>();
        stmt->initializer->data = IntegerLiteralExpr{0};
    }

    return stmt;
}

std::unique_ptr<PrintStmt> Parser::parse_print() {
    auto stmt = std::make_unique<PrintStmt>();
    consume(TokenType::Print, "Expected 'Print'");

    // Optional parenthesized argument list: Print(expr, expr, ...)
    if (check(TokenType::LParen)) {
        advance(); // consume (
        while (!check(TokenType::RParen) && current_.type != TokenType::Eof) {
            stmt->arguments.push_back(parse_expression());
            if (!match(TokenType::Comma)) break;
        }
        consume(TokenType::RParen, "Expected ')'");
        return stmt;
    }

    while (current_.type != TokenType::Eof &&
           current_.type != TokenType::Colon &&
           current_.type != TokenType::End &&
           current_.type != TokenType::RParen) {
        stmt->arguments.push_back(parse_expression());
        if (!match(TokenType::Comma)) break;
    }

    return stmt;
}

std::unique_ptr<ReturnStmt> Parser::parse_return() {
    auto stmt = std::make_unique<ReturnStmt>();
    consume(TokenType::Return, "Expected 'Return'");

    if (!check(TokenType::End) &&
        current_.type != TokenType::Eof &&
        current_.type != TokenType::Colon) {
        stmt->expression = parse_expression();
    }

    return stmt;
}

std::unique_ptr<IfStmt> Parser::parse_if() {
    auto stmt = std::make_unique<IfStmt>();
    consume(TokenType::If, "Expected 'If'");

    stmt->condition = parse_expression();
    SourceLocation then_loc = current_.location;
    consume(TokenType::Then, "Expected 'Then'");

    // Single-line If: "If cond Then statement" (no End If).
    if (current_.type != TokenType::Eof && current_.type != TokenType::End &&
        current_.location.line == then_loc.line) {
        stmt->then_branch.push_back(parse_statement());
        return stmt;
    }

    // Then branch
    while (current_.type != TokenType::Eof &&
           current_.type != TokenType::Else &&
           current_.type != TokenType::End) {
        stmt->then_branch.push_back(parse_statement());
    }

    // Else branch
    if (match(TokenType::Else)) {
        while (current_.type != TokenType::Eof &&
               current_.type != TokenType::End) {
            stmt->else_branch.push_back(parse_statement());
        }
    }

    consume_end(TokenType::If);

    return stmt;
}

std::unique_ptr<ForStmt> Parser::parse_for() {
    consume(TokenType::For, "Expected 'For'");

    // Check for For Each: For Each var [As type] In collection
    if (check(TokenType::Each)) {
        // Parse as For Each - but we need to return a ForStmt
        // We'll encode ForEach as a ForStmt with a special marker
        auto stmt = std::make_unique<ForStmt>();
        advance(); // consume 'Each'
        
        if (check(TokenType::Identifier)) {
            stmt->variable = current_.text;
            advance();
        } else {
            error_at_current("Expected loop variable");
            stmt->variable = "<unknown>";
        }
        
        // Optional As type
        if (match(TokenType::As)) {
            stmt->type_hint = parse_type();
        }
        
        consume(TokenType::In, "Expected 'In'");
        stmt->start = parse_expression();
        // For Each uses start as collection, end as null
        stmt->end = nullptr;
        stmt->is_for_each = true;
        
        stmt->body = parse_block_until(TokenType::Next).statements;
        consume(TokenType::Next, "Expected 'Next'");
        
        return stmt;
    }

    auto stmt = std::make_unique<ForStmt>();

    if (check(TokenType::Identifier)) {
        stmt->variable = current_.text;
        advance();
    } else {
        error_at_current("Expected loop variable");
        stmt->variable = "<unknown>";
    }

    if (match(TokenType::As)) {
        stmt->type_hint = parse_type();
    }

    consume(TokenType::Assign, "Expected '='");
    stmt->start = parse_expression();
    consume(TokenType::To, "Expected 'To'");
    stmt->end = parse_expression();

    if (match(TokenType::Step)) {
        stmt->step = parse_expression();
    }

    stmt->body = parse_block_until(TokenType::Next).statements;
    consume(TokenType::Next, "Expected 'Next'");

    return stmt;
}

std::unique_ptr<ForEachStmt> Parser::parse_for_each() {
    auto stmt = std::make_unique<ForEachStmt>();
    consume(TokenType::Each, "Expected 'Each'");

    if (check(TokenType::Identifier)) {
        stmt->variable = current_.text;
        advance();
    } else {
        error_at_current("Expected variable name");
        stmt->variable = "<unknown>";
    }

    consume(TokenType::In, "Expected 'In'");
    stmt->collection = parse_expression();

    stmt->body = parse_block_until(TokenType::Next).statements;
    consume(TokenType::Next, "Expected 'Next'");

    return stmt;
}

std::unique_ptr<DoLoopStmt> Parser::parse_do_loop() {
    auto stmt = std::make_unique<DoLoopStmt>();
    consume(TokenType::Do, "Expected 'Do'");

    // Check for condition on Do (Do While / Do Until)
    if (check(TokenType::While)) {
        stmt->loopType = DoLoopStmt::Type::DoWhile;
        advance();
        stmt->condition = parse_expression();
    } else if (check(TokenType::Until)) {
        stmt->loopType = DoLoopStmt::Type::DoUntil;
        advance();
        stmt->condition = parse_expression();
    } else {
        stmt->loopType = DoLoopStmt::Type::DoWhile;
        stmt->condition = nullptr;
    }

    stmt->body = parse_block_until(TokenType::Loop).statements;

    consume(TokenType::Loop, "Expected 'Loop'");

    // Check for condition on Loop (Loop While / Loop Until)
    if (check(TokenType::While)) {
        if (stmt->loopType == DoLoopStmt::Type::DoWhile &&
            !stmt->condition) {
            stmt->loopType = DoLoopStmt::Type::LoopWhile;
        }
        advance();
        stmt->condition = parse_expression();
    } else if (check(TokenType::Until)) {
        if (stmt->loopType == DoLoopStmt::Type::DoUntil &&
            !stmt->condition) {
            stmt->loopType = DoLoopStmt::Type::LoopUntil;
        }
        advance();
        stmt->condition = parse_expression();
    }

    return stmt;
}

std::unique_ptr<WhileStmt> Parser::parse_while() {
    auto stmt = std::make_unique<WhileStmt>();
    consume(TokenType::While, "Expected 'While'");

    stmt->condition = parse_expression();
    stmt->body = parse_block_until(TokenType::End).statements;
    consume_end(TokenType::While);

    return stmt;
}

std::unique_ptr<SelectCaseStmt> Parser::parse_select_case() {
    auto stmt = std::make_unique<SelectCaseStmt>();
    consume(TokenType::Select, "Expected 'Select'");
    consume(TokenType::Case, "Expected 'Case'");

    stmt->subject = parse_expression();

    while (current_.type != TokenType::Eof &&
           current_.type != TokenType::End) {
        if (check(TokenType::Case)) {
            advance(); // Case
            SelectCaseStmt::Case c;

            if (check(TokenType::Else)) {
                advance(); // Else - empty values means Else
            } else if (check(TokenType::Identifier) && current_.text == "Else") {
                advance(); // Else as identifier
            } else {
                do {
                    c.values.push_back(parse_expression());
                    if (match(TokenType::To)) {
                        c.range_end = parse_expression();
                    }
                } while (match(TokenType::Comma));
            }

            // Parse body until next Case/Else/End Select
            while (current_.type != TokenType::Eof &&
                   current_.type != TokenType::Case &&
                   current_.type != TokenType::End) {
                c.body.push_back(parse_statement());
            }

            stmt->cases.push_back(std::move(c));
        } else {
            error_at_current("Expected 'Case'");
            advance();
        }
    }

    consume_end(TokenType::Select);

    return stmt;
}

std::unique_ptr<ExpressionStmt> Parser::parse_expression_stmt() {
    auto stmt = std::make_unique<ExpressionStmt>();
    stmt->expression = parse_expression();
    return stmt;
}

std::unique_ptr<ArrayAssignmentStmt> Parser::parse_array_assignment() {
    auto stmt = std::make_unique<ArrayAssignmentStmt>();

    if (check(TokenType::Identifier)) {
        stmt->name = current_.text;
        advance();
    } else {
        error_at_current("Expected array name");
        stmt->name = "<unknown>";
    }

    consume(TokenType::LParen, "Expected '('");
    stmt->index = parse_expression();
    consume(TokenType::RParen, "Expected ')'");

    consume(TokenType::Assign, "Expected '='");
    stmt->expression = parse_expression();

    return stmt;
}

Block Parser::parse_block_until(TokenType end_token) {
    Block block;
    while (current_.type != TokenType::Eof && current_.type != end_token &&
           current_.type != TokenType::End) {
        // Skip empty lines / colons
        if (current_.type == TokenType::Colon) {
            advance();
            continue;
        }
        auto stmt = parse_statement();
        if (stmt) block.statements.push_back(std::move(stmt));
    }
    return block;
}

Block Parser::parse_block_terminators(
    std::initializer_list<TokenType> stop_at,
    std::initializer_list<TokenType> consume_after) {
    Block block;
    while (current_.type != TokenType::Eof) {
        bool stop = false;
        for (auto t : stop_at) {
            if (check(t)) { stop = true; break; }
        }
        if (stop) break;

        if (current_.type == TokenType::Colon) {
            advance();
            continue;
        }

        auto stmt = parse_statement();
        if (stmt) block.statements.push_back(std::move(stmt));
    }

    for (auto t : consume_after) {
        match(t);
    }

    return block;
}

// ============================================================================
// Expressions (precedence climbing)
// ============================================================================

std::unique_ptr<Expression> Parser::parse_expression() {
    return parse_or();
}

std::unique_ptr<Expression> Parser::parse_or() {
    auto left = parse_and();
    while (check(TokenType::Or) || check(TokenType::Xor)) {
        BinaryExpr::Op op = check(TokenType::Or) ? BinaryExpr::Op::Or
                                                 : BinaryExpr::Op::Xor;
        advance();
        auto right = parse_and();
        auto expr = std::make_unique<Expression>();
        expr->data = BinaryExpr{op, std::move(left), std::move(right)};
        left = std::move(expr);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parse_and() {
    auto left = parse_equality();
    while (check(TokenType::And)) {
        advance();
        auto right = parse_equality();
        auto expr = std::make_unique<Expression>();
        expr->data = BinaryExpr{BinaryExpr::Op::And, std::move(left), std::move(right)};
        left = std::move(expr);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parse_assignment_lhs() {
    // For LHS of assignment - just parse primary for now
    return parse_primary();
}

std::unique_ptr<Expression> Parser::parse_equality() {
    auto left = parse_comparison();

    // In BasicForge "=" is both assignment and equality; inside an expression
    // it is equality. "==" and "!=" are also accepted.
    while (check(TokenType::Eq) || check(TokenType::Ne) || check(TokenType::Assign)) {
        BinaryExpr::Op op;
        if (check(TokenType::Ne)) op = BinaryExpr::Op::Ne;
        else op = BinaryExpr::Op::Eq;
        advance();
        auto right = parse_comparison();
        auto expr = std::make_unique<Expression>();
        expr->data = BinaryExpr{op, std::move(left), std::move(right)};
        left = std::move(expr);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parse_comparison() {
    auto left = parse_term();

    while (check(TokenType::Lt) || check(TokenType::Gt) ||
           check(TokenType::Le) || check(TokenType::Ge)) {
        BinaryExpr::Op op;
        if (check(TokenType::Lt)) op = BinaryExpr::Op::Lt;
        else if (check(TokenType::Gt)) op = BinaryExpr::Op::Gt;
        else if (check(TokenType::Le)) op = BinaryExpr::Op::Le;
        else op = BinaryExpr::Op::Ge;
        advance();
        auto right = parse_term();
        auto expr = std::make_unique<Expression>();
        expr->data = BinaryExpr{op, std::move(left), std::move(right)};
        left = std::move(expr);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parse_term() {
    auto left = parse_factor();

    while (check(TokenType::Plus) || check(TokenType::Minus) ||
           check(TokenType::Ampersand)) {
        BinaryExpr::Op op;
        if (check(TokenType::Plus)) op = BinaryExpr::Op::Add;
        else if (check(TokenType::Minus)) op = BinaryExpr::Op::Sub;
        else op = BinaryExpr::Op::Concat;
        advance();
        auto right = parse_factor();
        auto expr = std::make_unique<Expression>();
        expr->data = BinaryExpr{op, std::move(left), std::move(right)};
        left = std::move(expr);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parse_factor() {
    auto left = parse_unary();

    while (check(TokenType::Star) || check(TokenType::Slash) ||
           check(TokenType::Backslash) || check(TokenType::Mod) ||
           check(TokenType::Caret)) {
        BinaryExpr::Op op;
        if (check(TokenType::Star)) op = BinaryExpr::Op::Mul;
        else if (check(TokenType::Slash)) op = BinaryExpr::Op::Div;
        else if (check(TokenType::Backslash)) op = BinaryExpr::Op::IntDiv;
        else if (check(TokenType::Mod)) op = BinaryExpr::Op::Mod;
        else op = BinaryExpr::Op::Pow;
        advance();
        auto right = parse_unary();
        auto expr = std::make_unique<Expression>();
        expr->data = BinaryExpr{op, std::move(left), std::move(right)};
        left = std::move(expr);
    }

    return left;
}

std::unique_ptr<Expression> Parser::parse_unary() {
    if (check(TokenType::Minus) || check(TokenType::Not)) {
        UnaryExpr::Op op;
        if (check(TokenType::Minus)) op = UnaryExpr::Op::Negate;
        else op = UnaryExpr::Op::Not;
        advance();
        auto operand = parse_unary();
        auto expr = std::make_unique<Expression>();
        expr->data = UnaryExpr{op, std::move(operand)};
        return expr;
    }

    return parse_postfix(parse_primary());
}

std::unique_ptr<Expression> Parser::parse_primary() {
    auto expr = std::make_unique<Expression>();

    switch (current_.type) {
        case TokenType::IntegerLiteral: {
            expr->data = IntegerLiteralExpr{std::stoll(current_.text)};
            advance();
            break;
        }
        case TokenType::LongLiteral: {
            expr->data = IntegerLiteralExpr{std::stoll(current_.text)};
            advance();
            break;
        }
        case TokenType::DoubleLiteral: {
            expr->data = DoubleLiteralExpr{std::stod(current_.text)};
            advance();
            break;
        }
        case TokenType::StringLiteral: {
            expr->data = StringLiteralExpr{current_.text};
            advance();
            break;
        }
        case TokenType::InterpolatedString: {
            auto text = current_.text;
            advance();
            return parse_interpolated_text(text);
        }
        case TokenType::True: {
            expr->data = BooleanLiteralExpr{true};
            advance();
            break;
        }
        case TokenType::False: {
            expr->data = BooleanLiteralExpr{false};
            advance();
            break;
        }
        case TokenType::Identifier: {
            std::string name = current_.text;
            advance();

            // Function call
            if (check(TokenType::LParen)) {
                auto call = parse_call(name);
                expr->data = std::move(call->data);
                return expr;
            }

            // Array access
            if (check(TokenType::LParen)) {
                auto arr = parse_array_access(name);
                expr->data = std::move(arr->data);
                return expr;
            }

            // Member access (obj.member) and method calls (obj.Method(args))
            // are handled by parse_postfix so that call arguments are parsed.
            expr->data = IdentifierExpr{std::move(name)};
            break;
        }
        case TokenType::Me: {
            expr->data = MeExpr{};
            advance();
            break;
        }
        case TokenType::New: {
            advance();
            if (check(TokenType::Identifier)) {
                NewExpr ne;
                ne.class_name = current_.text;
                advance();
                // Check for generic instantiation: New Class(Of TypeArgs)(...)
                if (match(TokenType::LParen)) {
                    if (match(TokenType::Of)) {
                        // Generic type arguments - parse and discard (type checking later)
                        while (current_.type != TokenType::RParen && current_.type != TokenType::Eof) {
                            parse_type(); // consume type argument
                            if (!match(TokenType::Comma)) break;
                        }
                        consume(TokenType::RParen, "Expected ')' after type arguments");
                        // Optional constructor arguments
                        if (match(TokenType::LParen)) {
                            while (!check(TokenType::RParen) && current_.type != TokenType::Eof) {
                                ne.arguments.push_back(parse_expression());
                                if (!match(TokenType::Comma)) break;
                            }
                            consume(TokenType::RParen, "Expected ')'");
                        }
                    } else {
                        // Regular constructor arguments
                        while (!check(TokenType::RParen) && current_.type != TokenType::Eof) {
                            ne.arguments.push_back(parse_expression());
                            if (!match(TokenType::Comma)) break;
                        }
                        consume(TokenType::RParen, "Expected ')'");
                    }
                }
                expr->data = std::move(ne);
            } else {
                error_at_current("Expected class name after 'New'");
            }
            break;
        }
        case TokenType::LParen: {
            advance();
            expr = parse_expression();
            consume(TokenType::RParen, "Expected ')'");
            return expr;
        }
        case TokenType::CInt:
        case TokenType::CLng:
        case TokenType::CDbl:
        case TokenType::CStr:
        case TokenType::CBool:
        case TokenType::Len:
        case TokenType::Trim:
        case TokenType::Left:
        case TokenType::Right:
        case TokenType::Mid:
        case TokenType::InStr:
        case TokenType::Replace: {
            // Built-in function call
            std::string funcName = current_.text;
            advance();
            auto call = parse_call(funcName);
            expr->data = std::move(call->data);
            break;
        }
        default:
            error_at_current("Unexpected token in expression");
            advance();
            expr->data = IntegerLiteralExpr{0};
            break;
    }

    return expr;
}

std::unique_ptr<Expression> Parser::parse_postfix(std::unique_ptr<Expression> base) {
    while (true) {
        if (check(TokenType::Dot)) {
            advance();
            if (check(TokenType::Identifier)) {
                std::string member_name = current_.text;
                advance();
                
                // Check for method call: obj.Method(args)
                if (check(TokenType::LParen)) {
                    // Parse arguments
                    std::vector<std::unique_ptr<Expression>> args;
                    advance(); // consume (
                    while (!check(TokenType::RParen) && current_.type != TokenType::Eof) {
                        args.push_back(parse_expression());
                        if (!match(TokenType::Comma)) break;
                    }
                    consume(TokenType::RParen, "Expected ')'");
                    
                    // Create call: obj.Method(args)
                    // Save the object before moving base
                    auto objArg = std::make_unique<Expression>();
                    objArg->data = std::move(base->data);
                    // base is now empty, create a new one for the call
                    base = std::make_unique<Expression>();
                    
                    CallExpr ce;
                    ce.callee = member_name;
                    ce.arguments.push_back(std::move(objArg));
                    for (auto& arg : args) {
                        ce.arguments.push_back(std::move(arg));
                    }
                    base->data = std::move(ce);
                    continue;
                }
                
                auto member = std::make_unique<Expression>();
                member->data = MemberAccessExpr{
                    std::move(base),
                    member_name
                };
                base = std::move(member);
                continue;
            }
        }
        if (check(TokenType::LParen)) {
            break;
        }
        if (check(TokenType::Bang)) {
            advance();
            break;
        }
        break;
    }

    auto expr = std::make_unique<Expression>();
    expr->data = std::move(base->data);
    return expr;
}

std::unique_ptr<Expression> Parser::parse_call(const std::string& callee) {
    auto call = std::make_unique<CallExpr>();
    call->callee = callee;

    consume(TokenType::LParen, "Expected '('");

    while (!check(TokenType::RParen) &&
           current_.type != TokenType::Eof) {
        call->arguments.push_back(parse_expression());
        if (!match(TokenType::Comma)) break;
    }

    consume(TokenType::RParen, "Expected ')'");

    auto expr = std::make_unique<Expression>();
    expr->data = std::move(*call);
    return expr;
}

std::unique_ptr<Expression> Parser::parse_array_access(const std::string& name) {
    auto arr = std::make_unique<ArrayAccessExpr>();
    arr->name = name;

    consume(TokenType::LParen, "Expected '('");
    arr->index = parse_expression();
    consume(TokenType::RParen, "Expected ')'");

    auto expr = std::make_unique<Expression>();
    expr->data = std::move(*arr);
    return expr;
}

} // namespace bfc
