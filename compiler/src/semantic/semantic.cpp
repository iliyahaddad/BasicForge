#include "bf/semantic.h"

namespace bfc {

SemanticAnalyzer::SemanticAnalyzer(DiagnosticSink& sink) : sink_(sink) {}

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

void SemanticAnalyzer::analyze(Program& program) {
    // First pass: register all modules, namespaces, classes, interfaces, structures, delegates, enums
    for (auto& mod : program.modules) {
        analyze_module(*mod);
    }
    for (auto& ns : program.namespaces) {
        analyze_namespace(*ns);
    }
}

void SemanticAnalyzer::analyze_module(ModuleDecl& mod) {
    variables_.clear();
    classMembers_.clear();
    for (const auto& kv : external_classes_) {
        classMembers_[kv.first] = kv.second;
    }
    overloads_.clear();
    function_return_types_.clear();
    currentNamespace_ = "";
    // Register enum members
    for (auto& modEnum : mod.enums) {
        int64_t nextValue = 0;
        for (auto& member : modEnum->members) {
            variables_[member.name] = PrimitiveType::Integer;
            constants_[member.name] = PrimitiveType::Integer;
            constant_values_[member.name] = member.value != 0 ? member.value : nextValue;
            nextValue = (member.value != 0 ? member.value : nextValue) + 1;
        }
    }
    // Validate Form event handlers
    for (auto& form : mod.forms) {
        for (auto& ctrl : form->controls) {
            for (auto& ev : ctrl.events) {
                bool found = false;
                for (auto& proc : mod.procedures) {
                    if (bf_iequals(proc->name, ev.second)) { found = true; break; }
                }
                if (!found) {
                    for (auto& cls : mod.classes) {
                        for (auto& method : cls->methods) {
                            if (bf_iequals(method.name, ev.second)) { found = true; break; }
                        }
                    }
                }
                if (!found) {
                    emit_error("Event handler '" + ev.second + "' not found");
                }
            }
        }
    }
    // Register classes and their members
    for (auto& cls : mod.classes) {
        std::unordered_map<std::string, PrimitiveType> members;
        for (auto& prop : cls->properties) {
            members[prop.name] = prop.type.kind;
        }
        classMembers_[cls->name] = members;
        // Register class name as a type (for Dim x As ClassName)
        variables_[cls->name] = PrimitiveType::Integer; // placeholder - class type
        
        // Register generic type parameters for the class
        if (!cls->type_parameters.parameters.empty()) {
            register_generic_type_parameters(cls->type_parameters);
        }
        
        // Analyze class methods
        for (auto& method : cls->methods) {
            analyze_class_method(*cls, method);
        }
        
        // Unregister generic type parameters for the class
        if (!cls->type_parameters.parameters.empty()) {
            unregister_generic_type_parameters();
        }
    }
    // Register structures
    for (auto& st : mod.structures) {
        analyze_structure(*st);
    }
    // Register interfaces
    for (auto& iface : mod.interfaces) {
        analyze_interface(*iface);
    }
    // Register delegates
    for (auto& d : mod.delegates) {
        analyze_delegate(*d);
    }
    // Register namespaces
    for (auto& ns : mod.namespaces) {
        analyze_namespace(*ns);
    }
    // Pre-register procedure return types so calls to functions declared later
    // in the module have the correct type.
    for (auto& proc : mod.procedures) {
        function_return_types_[proc->name] = proc->return_type.kind;
    }
    // Analyze procedures
    for (auto& proc : mod.procedures) {
        analyze_procedure(*proc);
    }
}

void SemanticAnalyzer::analyze_namespace(NamespaceDecl& ns) {
    std::string savedNamespace = currentNamespace_;
    if (!currentNamespace_.empty()) {
        currentNamespace_ += ".";
    }
    currentNamespace_ += ns.name;
    
    // Pre-register procedure return types in the namespace.
    for (auto& proc : ns.procedures) {
        function_return_types_[proc->name] = proc->return_type.kind;
    }
    // Analyze procedures in namespace
    for (auto& proc : ns.procedures) {
        analyze_procedure(*proc);
    }
    // Analyze classes
    for (auto& cls : ns.classes) {
        std::unordered_map<std::string, PrimitiveType> members;
        for (auto& prop : cls->properties) {
            members[prop.name] = prop.type.kind;
        }
        classMembers_[currentNamespace_ + "." + cls->name] = members;
        variables_[currentNamespace_ + "." + cls->name] = PrimitiveType::Integer;
        
        // Register generic type parameters for the class
        if (!cls->type_parameters.parameters.empty()) {
            register_generic_type_parameters(cls->type_parameters);
        }
        
        for (auto& method : cls->methods) {
            analyze_class_method(*cls, method);
        }
        
        // Unregister generic type parameters for the class
        if (!cls->type_parameters.parameters.empty()) {
            unregister_generic_type_parameters();
        }
    }
    // Analyze structures
    for (auto& st : ns.structures) {
        analyze_structure(*st);
    }
    // Analyze interfaces
    for (auto& iface : ns.interfaces) {
        analyze_interface(*iface);
    }
    // Analyze delegates
    for (auto& d : ns.delegates) {
        analyze_delegate(*d);
    }
    // Analyze nested namespaces
    for (auto& nested_ns : ns.namespaces) {
        analyze_namespace(*nested_ns);
    }
    
    currentNamespace_ = savedNamespace;
}

void SemanticAnalyzer::analyze_structure(StructDecl& st) {
    std::unordered_map<std::string, PrimitiveType> members;
    for (auto& field : st.fields) {
        members[field.name] = field.type.kind;
    }
    std::string fullName = currentNamespace_.empty() ? st.name : (currentNamespace_ + "." + st.name);
    classMembers_[fullName] = members;
    variables_[fullName] = PrimitiveType::Integer;
}

void SemanticAnalyzer::analyze_interface(InterfaceDecl& iface) {
    // Interface members are just declarations, no implementation
    // Register for type checking
    std::string fullName = currentNamespace_.empty() ? iface.name : (currentNamespace_ + "." + iface.name);
    variables_[fullName] = PrimitiveType::Integer;
}

void SemanticAnalyzer::analyze_delegate(DelegateDecl& d) {
    // Delegates are function pointer types
    std::string fullName = currentNamespace_.empty() ? d.name : (currentNamespace_ + "." + d.name);
    variables_[fullName] = PrimitiveType::Integer;
}

void SemanticAnalyzer::analyze_procedure(SubOrFunction& proc) {
        // Save module-level enum members before clearing locals
        std::vector<std::pair<std::string, PrimitiveType>> enumMembers;
        for (auto& kv : variables_) {
            if (constants_.find(kv.first) == constants_.end()) {
                enumMembers.push_back(kv);
            }
        }
        variables_.clear();
        current_return_type_ = proc.return_type.kind;
        current_function_name_ = proc.name;
        
        // Register generic type parameters if this is a generic procedure
        if (!proc.type_parameters.parameters.empty()) {
            register_generic_type_parameters(proc.type_parameters);
        }
        
        // Register for overloading
        std::string mangled = mangle_name(proc.name, proc.parameters);
        overloads_[mangled].push_back(&proc);
        
        for (auto& p : proc.parameters) {
            variables_[p.name] = p.type.kind;
        }
        analyze_block(proc.body);
        variables_.clear();
        
        // Unregister generic type parameters
        if (!proc.type_parameters.parameters.empty()) {
            unregister_generic_type_parameters();
        }
        
        // Restore enum members
        for (auto& kv : enumMembers) {
            variables_[kv.first] = kv.second;
        }
        current_return_type_ = PrimitiveType::Void;
    }

void SemanticAnalyzer::analyze_class_method(ClassDecl& cls, ClassMethod& method) {
    // Save current class and variables
    std::string savedClass = currentClass_;
    currentClass_ = cls.name;
    
    // Register generic type parameters for the class
    if (!cls.type_parameters.parameters.empty()) {
        register_generic_type_parameters(cls.type_parameters);
    }
    
    // Register generic type parameters for the method
    if (!method.type_parameters.parameters.empty()) {
        register_generic_type_parameters(method.type_parameters);
    }
    
    // Add class members (Me.xxx) to variables
    auto classMembersIt = classMembers_.find(cls.name);
    if (classMembersIt != classMembers_.end()) {
        for (auto& kv : classMembersIt->second) {
            variables_[kv.first] = kv.second;
        }
    }
    
    current_return_type_ = method.return_type.kind;
        current_function_name_ = cls.name + "." + method.name;
        for (auto& p : method.parameters) {
            variables_[p.name] = p.type.kind;
            if (!p.type.generic_name.empty() &&
                classMembers_.count(p.type.generic_name)) {
                class_var_types_[p.name] = p.type.generic_name;
            }
        }
    analyze_block(method.body);
    variables_.clear();
    
    // Unregister method generic type parameters
    if (!method.type_parameters.parameters.empty()) {
        unregister_generic_type_parameters();
    }
    
    // Unregister class generic type parameters
    if (!cls.type_parameters.parameters.empty()) {
        unregister_generic_type_parameters();
    }
    
    current_return_type_ = PrimitiveType::Void;
    currentClass_ = savedClass;
}

void SemanticAnalyzer::analyze_block(const Block& block) {
    for (const auto& stmt : block.statements) {
        analyze_statement(*stmt);
    }
}

void SemanticAnalyzer::analyze_statement(const Statement& stmt) {
    std::visit(
        [this](const auto& s) {
            using T = std::decay_t<decltype(s)>;
            if constexpr (std::is_same_v<T, DimStmt>) {
                if (s.is_const) {
                    // Const declaration - must have initializer
                    if (s.initializer) {
                        TypeRef init_type = analyze_expression(*s.initializer);
                        if (!s.type_inferred && s.type.kind != PrimitiveType::Void) {
                            if (s.type.kind != init_type.kind) {
                                emit_error("Type mismatch in Const initializer");
                            }
                        }
                        // Store constant value if initializer is a literal
                        if (std::holds_alternative<IntegerLiteralExpr>(s.initializer->data)) {
                            constant_values_[s.name] = std::get<IntegerLiteralExpr>(s.initializer->data).value;
                        } else if (std::holds_alternative<DoubleLiteralExpr>(s.initializer->data)) {
                            constant_values_[s.name] = static_cast<int64_t>(std::get<DoubleLiteralExpr>(s.initializer->data).value);
                        } else if (std::holds_alternative<BooleanLiteralExpr>(s.initializer->data)) {
                            constant_values_[s.name] = std::get<BooleanLiteralExpr>(s.initializer->data).value ? 1 : 0;
                        }
                    }
                    variables_[s.name] = s.type.kind;
                    constants_[s.name] = s.type.kind;
                } else {
                    // Regular Dim
                    if (s.initializer) {
                        TypeRef init_type = analyze_expression(*s.initializer);
                        if (!s.type_inferred && s.type.kind != PrimitiveType::Void) {
                            if (s.type.kind != init_type.kind &&
                                !(is_numeric(s.type.kind) && is_numeric(init_type.kind))) {
                                emit_error("Type mismatch in Dim initializer");
                            }
                        }
                    }
                    variables_[s.name] = s.type.kind;
                    if (s.type.is_array || s.array_size) array_vars_.insert(s.name);
                    if (!s.type.generic_name.empty() &&
                        classMembers_.count(s.type.generic_name)) {
                        class_var_types_[s.name] = s.type.generic_name;
                    }
                }
            } else if constexpr (std::is_same_v<T, AssignmentStmt>) {
                // Check if trying to assign to a constant
                if (constants_.find(s.name) != constants_.end()) {
                    emit_error("Cannot assign to constant '" + s.name + "'");
                }
                auto it = variables_.find(s.name);
                if (it == variables_.end()) {
                    emit_error("Variable '" + s.name + "' is not declared");
                } else {
                    TypeRef expr_type = analyze_expression(*s.expression);
                    if (it->second != expr_type.kind &&
                        !(is_numeric(it->second) && is_numeric(expr_type.kind))) {
                        emit_error("Type mismatch in assignment to '" + s.name + "'");
                    }
                }
            } else if constexpr (std::is_same_v<T, PrintStmt>) {
                for (const auto& arg : s.arguments) {
                    TypeRef t = analyze_expression(*arg);
                    if (!is_string(t.kind) && !is_numeric(t.kind) && !is_boolean(t.kind)) {
                        emit_error("Print argument must be a primitive type");
                    }
                }
            } else if constexpr (std::is_same_v<T, ReturnStmt>) {
                if (s.expression) {
                    TypeRef expr_type = analyze_expression(*s.expression);
                    if (current_return_type_ == PrimitiveType::Void) {
                        emit_error("Return with value in a Sub");
                    } else if (expr_type.kind != current_return_type_) {
                        emit_error("Return type mismatch in " + current_function_name_);
                    }
                } else {
                    if (current_return_type_ != PrimitiveType::Void) {
                        emit_error("Return requires a value in a Function");
                    }
                }
            } else if constexpr (std::is_same_v<T, IfStmt>) {
                TypeRef cond = analyze_expression(*s.condition);
                if (cond.kind != PrimitiveType::Boolean) {
                    emit_error("If condition must be Boolean");
                }
                for (const auto& st : s.then_branch) analyze_statement(*st);
                for (const auto& st : s.else_branch) analyze_statement(*st);
            } else if constexpr (std::is_same_v<T, ForStmt>) {
                if (s.is_for_each) {
                    // For Each: start is the collection expression
                    analyze_expression(*s.start);
                    variables_[s.variable] = PrimitiveType::Integer;
                } else {
                    analyze_expression(*s.start);
                    if (s.end) analyze_expression(*s.end);
                    if (s.step) analyze_expression(*s.step);
                    variables_[s.variable] = PrimitiveType::Integer;
                }
                for (const auto& st : s.body) analyze_statement(*st);
            } else if constexpr (std::is_same_v<T, WhileStmt>) {
                TypeRef cond = analyze_expression(*s.condition);
                if (cond.kind != PrimitiveType::Boolean) {
                    emit_error("While condition must be Boolean");
                }
                for (const auto& st : s.body) analyze_statement(*st);
            } else if constexpr (std::is_same_v<T, SelectCaseStmt>) {
                analyze_expression(*s.subject);
                for (const auto& c : s.cases) {
                    for (const auto& v : c.values) analyze_expression(*v);
                    for (const auto& st : c.body) analyze_statement(*st);
                }
            } else if constexpr (std::is_same_v<T, TryCatchFinallyStmt>) {
                for (const auto& st : s.try_body) analyze_statement(*st);
                for (const auto& cc : s.catch_clauses) {
                    if (!cc.exception_variable.empty()) {
                        variables_[cc.exception_variable] = PrimitiveType::String;
                    }
                    for (const auto& st : cc.body) analyze_statement(*st);
                }
                for (const auto& st : s.finally_body) analyze_statement(*st);
            } else if constexpr (std::is_same_v<T, ThrowStmt>) {
                if (s.expression) {
                    analyze_expression(*s.expression);
                }
            }
        },
        stmt.data);
}

TypeRef SemanticAnalyzer::analyze_expression(const Expression& expr) {
    TypeRef result{PrimitiveType::Void};
    std::visit([this, &result](const auto& e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, IntegerLiteralExpr>) {
            result.kind = PrimitiveType::Integer;
        } else if constexpr (std::is_same_v<T, StringLiteralExpr>) {
            result.kind = PrimitiveType::String;
        } else if constexpr (std::is_same_v<T, BooleanLiteralExpr>) {
            result.kind = PrimitiveType::Boolean;
        } else if constexpr (std::is_same_v<T, DoubleLiteralExpr>) {
            result.kind = PrimitiveType::Double;
        } else if constexpr (std::is_same_v<T, IdentifierExpr>) {
            auto it = variables_.find(e.name);
            if (it != variables_.end()) {
                result.kind = it->second;
            } else if (external_variables_.count(e.name)) {
                result.kind = external_variables_[e.name];
            } else {
                result.kind = PrimitiveType::Integer;
            }
        } else if constexpr (std::is_same_v<T, ArrayAccessExpr>) {
            auto it = variables_.find(e.name);
            if (it != variables_.end() && it->second != PrimitiveType::Void) {
                result.kind = it->second; // element type
            } else {
                result.kind = PrimitiveType::Integer;
            }
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            if (e.arguments.size() == 1 && array_vars_.count(e.callee)) {
                analyze_expression(*e.arguments[0]);
                auto it = variables_.find(e.callee);
                result.kind = (it != variables_.end()) ? it->second
                                                     : PrimitiveType::Integer;
            } else if (e.callee == "CInt" || e.callee == "CLng") {
                result.kind = PrimitiveType::Integer;
            } else if (e.callee == "CDbl") {
                result.kind = PrimitiveType::Double;
            } else if (e.callee == "CStr") {
                result.kind = PrimitiveType::String;
            } else if (e.callee == "CBool") {
                result.kind = PrimitiveType::Boolean;
            } else if (e.callee == "Len" || e.callee == "InStr" || e.callee == "FileExists" ||
                       e.callee == "Asc") {
                result.kind = PrimitiveType::Integer;
            } else if (e.callee == "Trim" || e.callee == "Left" || e.callee == "Right" || 
                       e.callee == "Mid" || e.callee == "Replace" || e.callee == "Format" ||
                       e.callee == "CStrDouble" || e.callee == "GetText" ||
                       e.callee == "bf_get_dropped_file" ||
                       e.callee == "ReadAllText" || e.callee == "GetEnv" ||
                       e.callee == "ReadLine" || e.callee == "GetClipboard" ||
                       e.callee == "UCase" || e.callee == "LCase" || e.callee == "Chr") {
                result.kind = PrimitiveType::String;
            } else if (e.callee == "Sqr" || e.callee == "Abs" || e.callee == "Sgn" ||
                       e.callee == "Fix" || e.callee == "Int" || e.callee == "Rnd") {
                result.kind = PrimitiveType::Double;
            } else if (e.callee == "Year" || e.callee == "Month" || e.callee == "Day") {
                result.kind = PrimitiveType::Integer;
            } else if (e.callee == "Now") {
                result.kind = PrimitiveType::Double;
            } else if (e.callee == "Split" || e.callee == "Join") {
                result.kind = PrimitiveType::Integer;
            } else if (builtins_.count(e.callee)) {
                // Void-returning builtins (Open, Close, Print, Input, EOF, LOF)
                result.kind = PrimitiveType::Void;
            } else {
                // User function call
                auto fit = function_return_types_.find(e.callee);
                if (fit != function_return_types_.end()) {
                    result.kind = fit->second;
                } else if (external_functions_.count(e.callee)) {
                    result.kind = external_functions_[e.callee];
                } else if (external_method_returns_.count(e.callee)) {
                    result.kind = external_method_returns_[e.callee];
                } else {
                    result.kind = PrimitiveType::Integer;
                }
            }
        } else if constexpr (std::is_same_v<T, NewExpr>) {
            // Object creation returns the class type
            result.kind = PrimitiveType::Integer; // class reference
        } else if constexpr (std::is_same_v<T, MemberAccessExpr>) {
            result.kind = PrimitiveType::Integer;
            std::string obj_class;
            if (currentClass_.empty()) {
                if (const auto* id = std::get_if<IdentifierExpr>(&e.object->data)) {
                    auto cv = class_var_types_.find(id->name);
                    if (cv != class_var_types_.end()) obj_class = cv->second;
                }
            } else {
                obj_class = currentClass_;
            }
            auto classIt = classMembers_.find(obj_class);
            if (classIt != classMembers_.end()) {
                auto memberIt = classIt->second.find(e.member);
                if (memberIt != classIt->second.end()) result.kind = memberIt->second;
            }
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            TypeRef lt = analyze_expression(*e.left);
            TypeRef rt = analyze_expression(*e.right);
            auto op = e.op;
            if (op == BinaryExpr::Op::Concat) {
                // VB-style: & accepts strings and numbers/booleans and
                // converts the non-string side to text.
                auto concat_ok = [this](PrimitiveType t) {
                    return t == PrimitiveType::String || is_numeric(t) ||
                           t == PrimitiveType::Boolean;
                };
                if (!concat_ok(lt.kind) || !concat_ok(rt.kind)) {
                    emit_error("String concatenation requires String or numeric operands");
                }
                result.kind = PrimitiveType::String;
            } else if (op == BinaryExpr::Op::And || op == BinaryExpr::Op::Or ||
                       op == BinaryExpr::Op::Xor) {
                if (lt.kind != PrimitiveType::Boolean || rt.kind != PrimitiveType::Boolean) {
                    emit_error("Logical operators require Boolean operands");
                }
                result.kind = PrimitiveType::Boolean;
            } else if (op == BinaryExpr::Op::Eq || op == BinaryExpr::Op::Ne ||
                       op == BinaryExpr::Op::Lt || op == BinaryExpr::Op::Gt ||
                       op == BinaryExpr::Op::Le || op == BinaryExpr::Op::Ge) {
                if (lt.kind != rt.kind) {
                    emit_error("Comparison operands must have the same type");
                }
                result.kind = PrimitiveType::Boolean;
            } else {
                // Arithmetic: + - * / \ Mod ^
                if (!is_numeric(lt.kind) || !is_numeric(rt.kind)) {
                    emit_error("Arithmetic operands must be numeric");
                }
                if (op == BinaryExpr::Op::Div) {
                    result.kind = PrimitiveType::Double;
                } else if (lt.kind == PrimitiveType::Double || rt.kind == PrimitiveType::Double) {
                    result.kind = PrimitiveType::Double;
                } else if (lt.kind == PrimitiveType::Single || rt.kind == PrimitiveType::Single) {
                    result.kind = PrimitiveType::Single;
                } else if (lt.kind == PrimitiveType::Long || rt.kind == PrimitiveType::Long) {
                    result.kind = PrimitiveType::Long;
                } else {
                    result.kind = PrimitiveType::Integer;
                }
            }
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            TypeRef ot = analyze_expression(*e.operand);
            if (e.op == UnaryExpr::Op::Not) {
                if (ot.kind != PrimitiveType::Boolean) {
                    emit_error("Not requires a Boolean operand");
                }
                result.kind = PrimitiveType::Boolean;
            } else {
                if (!is_numeric(ot.kind)) {
                    emit_error("Unary minus requires a numeric operand");
                }
                result.kind = ot.kind;
            }
        } else if constexpr (std::is_same_v<T, MeExpr>) {
            // Me keyword - reference to current object
            if (!currentClass_.empty()) {
                result.kind = PrimitiveType::Integer; // class reference
            } else {
                result.kind = PrimitiveType::Integer;
            }
        } else if constexpr (std::is_same_v<T, GenericInstantiationExpr>) {
            // Generic type instantiation: resolve the generic type with type arguments
            // For now, return the base type kind
            if (!e.generic_name.empty()) {
                result.kind = PrimitiveType::Integer; // class reference
            } else {
                result.kind = PrimitiveType::Integer;
            }
        }
    }, expr.data);
    return result;
}

void SemanticAnalyzer::check_type_match(TypeRef expected, TypeRef actual,
                                         const SourceLocation& loc,
                                         const std::string& context) {
    (void)loc; (void)context;
    if (expected.kind != actual.kind) {
        emit_error("Type mismatch in " + context);
    }
}

bool SemanticAnalyzer::is_numeric(PrimitiveType t) const {
    return t == PrimitiveType::Integer || t == PrimitiveType::Long ||
           t == PrimitiveType::Single || t == PrimitiveType::Double;
}

bool SemanticAnalyzer::is_string(PrimitiveType t) const {
    return t == PrimitiveType::String;
}

bool SemanticAnalyzer::is_boolean(PrimitiveType t) const {
    return t == PrimitiveType::Boolean;
}

void SemanticAnalyzer::emit_error(const std::string& msg) {
    Diagnostic d;
    d.severity = DiagnosticSeverity::Error;
    d.message = msg;
    d.location = SourceLocation{};
    sink_.emit(d);
}

void SemanticAnalyzer::emit_warning(const std::string& msg) {
    Diagnostic d;
    d.severity = DiagnosticSeverity::Warning;
    d.message = msg;
    d.location = SourceLocation{};
    sink_.emit(d);
}

PrimitiveType SemanticAnalyzer::resolve_type(const TypeRef& type) {
    return type.kind;
}

std::string SemanticAnalyzer::mangle_name(const std::string& name, const std::vector<Parameter>& params) {
    std::string mangled = name;
    for (const auto& p : params) {
        mangled += "_" + std::to_string(static_cast<int>(p.type.kind));
    }
    return mangled;
}

// ============================================================================
// Generic Type Handling
// ============================================================================

TypeRef SemanticAnalyzer::analyze_type(const TypeRef& type, bool allowGenerics) {
    TypeRef result = type;
    
    if (allowGenerics && type.is_generic && !type.generic_name.empty()) {
        // This is a generic type instantiation like List(Of Integer)
        return instantiate_generic_type(type.generic_name, type.type_arguments);
    }
    
    // Substitute any generic type parameters in the type
    return substitute_generic_type(type);
}

TypeRef SemanticAnalyzer::substitute_generic_type(const TypeRef& type) {
    TypeRef result = type;
    
    // If this is a generic type parameter (e.g., T in List(Of T)), substitute it
    if (type.is_generic && type.generic_name.empty() && !type.type_arguments.empty()) {
        // This shouldn't happen in normal usage
        return result;
    }
    
    // Check if this type name matches a generic type parameter
    for (auto it = genericTypeParamStack_.rbegin(); it != genericTypeParamStack_.rend(); ++it) {
        auto it_param = it->find(type.generic_name);
        if (it_param != it->end()) {
            // Found a matching generic type parameter, substitute it
            result.kind = it_param->second;
            result.is_generic = false;
            result.generic_name.clear();
            result.type_arguments.clear();
            return result;
        }
    }
    
    // Substitute in type arguments
    for (auto& arg : result.type_arguments) {
        arg = substitute_generic_type(arg);
    }
    
    return result;
}

void SemanticAnalyzer::register_generic_type_parameters(const TypeParameterList& params) {
    std::unordered_map<std::string, PrimitiveType> paramMap;
    for (const auto& param : params.parameters) {
        // For now, treat type parameters as Integer (placeholder)
        // In a full implementation, we'd track constraints
        paramMap[param.name] = PrimitiveType::Integer;
    }
    genericTypeParamStack_.push_back(std::move(paramMap));
}

void SemanticAnalyzer::unregister_generic_type_parameters() {
    if (!genericTypeParamStack_.empty()) {
        genericTypeParamStack_.pop_back();
    }
}

TypeRef SemanticAnalyzer::instantiate_generic_type(const std::string& genericName, const std::vector<TypeRef>& typeArgs) {
    TypeRef result;
    
    // Analyze each type argument
    std::vector<TypeRef> analyzedArgs;
    for (const auto& arg : typeArgs) {
        analyzedArgs.push_back(analyze_type(arg, false));
    }
    
    // Create a unique name for this instantiation
    std::string instantiationName = genericName;
    for (const auto& arg : analyzedArgs) {
        instantiationName += "_" + std::to_string(static_cast<int>(arg.kind));
    }
    
    // Store the type arguments for this instantiation
    std::unordered_map<std::string, PrimitiveType> argMap;
    for (size_t i = 0; i < typeArgs.size(); ++i) {
        if (i < typeArgs.size()) {
            // For now, use a simple mapping based on parameter name
            std::string paramName = "T" + std::to_string(i);
            argMap[paramName] = analyzedArgs[i].kind;
        }
    }
    genericTypeArguments_[instantiationName] = std::move(argMap);
    
    // Return the instantiated type
    result.kind = PrimitiveType::Integer; // placeholder - class reference
    result.is_generic = false;
    result.generic_name = instantiationName;
    
    return result;
}

} // namespace bfc