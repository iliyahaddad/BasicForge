#include "bf/ir_builder.h"
#include <iostream>
#include <functional>

namespace bfc {

static std::string bf_lower_ascii(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return out;
}

static std::string form_prop(
    const std::unordered_map<std::string, std::string>& props,
    const std::string& key, const std::string& fallback) {
    for (const auto& kv : props) {
        if (bf_lower_ascii(kv.first) == bf_lower_ascii(key)) return kv.second;
    }
    return fallback;
}

static int form_int(const std::unordered_map<std::string, std::string>& props,
                    const std::string& key, int fallback) {
    std::string v = form_prop(props, key, "");
    if (v.empty()) return fallback;
    try {
        return std::stoi(v);
    } catch (...) {
        return fallback;
    }
}

IrBuilder::IrBuilder(DiagnosticSink& sink) : sink_(sink) {}

IrValue IrBuilder::new_vreg(PrimitiveType type) {
    IrValue v;
    v.kind = IrValue::Kind::VirtualRegister;
    v.type = type;
    v.name = "%r" + std::to_string(next_reg_++);
    return v;
}

IrValue IrBuilder::make_const_int(int64_t v) {
    IrValue c;
    c.kind = IrValue::Kind::Constant;
    c.type = PrimitiveType::Integer;
    c.int_value = v;
    return c;
}

IrValue IrBuilder::make_const_string(const std::string& v) {
    IrValue c;
    c.kind = IrValue::Kind::Constant;
    c.type = PrimitiveType::String;
    c.str_value = v;
    return c;
}

IrValue IrBuilder::make_const_bool(bool v) {
    IrValue c;
    c.kind = IrValue::Kind::Constant;
    c.type = PrimitiveType::Boolean;
    c.bool_value = v;
    return c;
}

IrValue IrBuilder::make_const_double(double v) {
    IrValue c;
    c.kind = IrValue::Kind::Constant;
    c.type = PrimitiveType::Double;
    c.double_value = v;
    return c;
}

IrValue IrBuilder::make_local(const std::string& name, PrimitiveType type) {
    IrValue l;
    l.kind = IrValue::Kind::Local;
    l.type = type;
    l.name = name;
    return l;
}

IrValue IrBuilder::make_param(const std::string& name, PrimitiveType type, bool is_array) {
    IrValue v;
    v.kind = IrValue::Kind::Parameter;
    v.type = type;
    v.name = name;
    v.is_array = is_array;
    return v;
}

std::string IrBuilder::new_label() {
    return "L" + std::to_string(next_label_++);
}

IrBasicBlock& IrBuilder::new_block(const std::string& name) {
    module_.functions.back().blocks.push_back(IrBasicBlock{name, {}});
    current_bb_ = &module_.functions.back().blocks.back();
    return *current_bb_;
}

void IrBuilder::emit(IrOpcode op, std::vector<IrValue> ops, IrValue res, std::string label, std::string label2) {
    current_bb_->instructions.push_back(IrInstruction{op, std::move(ops), std::move(res), std::move(label), std::move(label2), {}});
}

void IrBuilder::emit(IrOpcode op, std::vector<IrValue> ops, IrValue res, std::string label) {
    emit(op, std::move(ops), std::move(res), std::move(label), "");
}

IrModule IrBuilder::lower(Program& program) {
    // Build the method -> function map before lowering any body, so that a
    // method can call another method that is declared later in the class.
    collect_method_functions(program);
    for (auto& mod : program.modules) {
        lower_module(*mod);
    }
    for (auto& ns : program.namespaces) {
        lower_namespace(*ns);
    }
    if (!module_.forms.empty()) {
        build_entry_main();
    }
    return module_;
}

void IrBuilder::collect_class_methods(const std::vector<std::unique_ptr<ClassDecl>>& classes) {
    for (auto& cls : classes) {
        if (!cls) continue;
        for (auto& method : cls->methods) {
            std::string fn = cls->name + "_" + (method.is_constructor ? "New" : method.name);
            method_functions_[method.name] = fn;
        }
    }
}

void IrBuilder::collect_method_functions(Program& program) {
    for (auto& mod : program.modules) {
        if (mod) collect_class_methods(mod->classes);
    }
    std::function<void(const NamespaceDecl&)> walk = [&](const NamespaceDecl& ns) {
        collect_class_methods(ns.classes);
        for (auto& nested : ns.namespaces) {
            if (nested) walk(*nested);
        }
    };
    for (auto& ns : program.namespaces) {
        if (ns) walk(*ns);
    }
}

void IrBuilder::lower_namespace(NamespaceDecl& ns) {
    // Register enum member values
    for (auto& nsEnum : ns.enums) {
        int64_t nextValue = 0;
        for (auto& member : nsEnum->members) {
            constant_values_[ns.name + "." + member.name] = (member.value != 0) ? member.value : nextValue;
            constant_values_[member.name] = (member.value != 0) ? member.value : nextValue;
            nextValue = (member.value != 0) ? member.value + 1 : nextValue + 1;
        }
    }
    // Lower procedures
    for (auto& proc : ns.procedures) {
        lower_procedure(*proc);
    }
    // Lower classes
    for (auto& cls : ns.classes) {
        compute_class_layout(*cls);
        lower_class(*cls);
    }
    // Lower structures
    for (auto& st : ns.structures) {
        compute_struct_layout(*st);
    }
    // Lower forms
    for (auto& form : ns.forms) {
        lower_form(*form);
    }
    // Lower nested namespaces
    for (auto& nested : ns.namespaces) {
        lower_namespace(*nested);
    }
}

void IrBuilder::lower_module(ModuleDecl& mod) {
    // Register enum member values as compile-time constants
    for (auto& modEnum : mod.enums) {
        int64_t nextValue = 0;
        for (auto& member : modEnum->members) {
            constant_values_[member.name] = (member.value != 0) ? member.value : nextValue;
            nextValue = (member.value != 0) ? member.value + 1 : nextValue + 1;
        }
    }
    // First compute class layouts
    for (auto& cls : mod.classes) {
        compute_class_layout(*cls);
    }
    // Lower structures
    for (auto& st : mod.structures) {
        compute_struct_layout(*st);
    }
    // Lower classes (create method functions)
    for (auto& cls : mod.classes) {
        lower_class(*cls);
    }
    // Lower forms (create Form_<Name>_Create functions)
    for (auto& form : mod.forms) {
        lower_form(*form);
    }
    // Lower procedures
    for (auto& proc : mod.procedures) {
        lower_procedure(*proc);
    }
}

void IrBuilder::compute_struct_layout(StructDecl& st) {
    std::unordered_map<std::string, std::pair<uint32_t, PrimitiveType>> layout;
    uint32_t offset = 0;
    
    for (auto& field : st.fields) {
        layout[field.name] = {offset, field.type.kind};
        offset += get_primitive_size(field.type.kind);
    }
    
    classLayouts_[st.name] = layout;
}

void IrBuilder::compute_class_layout(ClassDecl& cls) {
    std::unordered_map<std::string, std::pair<uint32_t, PrimitiveType>> layout;
    uint32_t offset = 0;
    
    for (auto& prop : cls.properties) {
        layout[prop.name] = {offset, prop.type.kind};
        offset += get_primitive_size(prop.type.kind);
    }
    
    classLayouts_[cls.name] = layout;
}

uint32_t IrBuilder::get_primitive_size(PrimitiveType t) {
    switch (t) {
        case PrimitiveType::Integer:
        case PrimitiveType::Long:
        case PrimitiveType::Double:
        case PrimitiveType::String: // pointer
        case PrimitiveType::Boolean:
            return 8; // 64-bit
        case PrimitiveType::Single:
            return 4;
        default:
            return 8;
    }
}

void IrBuilder::lower_class(ClassDecl& cls) {
    // Lower each method as a separate function
    for (auto& method : cls.methods) {
        std::string funcName = cls.name + "_" + method.name;
        if (method.is_constructor) {
            funcName = cls.name + "_New";
        }
        
        module_.functions.push_back(IrFunction{});
        IrFunction& fn = module_.functions.back();
        fn.name = funcName;
        fn.return_type = method.return_type.kind;
        
        // First parameter is "Me" (this pointer)
        fn.parameters.push_back(make_param("Me", PrimitiveType::Integer));
        
        var_types_.clear();
        var_types_["Me"] = PrimitiveType::Integer;
        
        // Class fields are not locals: they live in the object referred to by
        // "Me". Identifier lookup falls back to the class layout.
        
        // Add method parameters
        for (auto& p : method.parameters) {
            PrimitiveType pt = p.type.kind == PrimitiveType::Void ? PrimitiveType::Integer : p.type.kind;
            var_types_[p.name] = pt;
            fn.parameters.push_back(make_param(p.name, pt, p.is_array));
            if (p.is_array) array_vars_.insert(p.name);
        }
        
        current_fn_ = &fn;
        new_block("entry");
        
        // Save current class context
        std::string savedClass = currentClass_;
        currentClass_ = cls.name;
        
        lower_block(method.body);
        
        currentClass_ = savedClass;
        
        if (current_bb_->instructions.empty() ||
            current_bb_->instructions.back().opcode != IrOpcode::Return) {
            emit(IrOpcode::Return, {});
        }
        current_fn_ = nullptr;
        current_bb_ = nullptr;
    }
}

void IrBuilder::lower_form(const FormDecl& form) {
    module_.forms.push_back(IrForm{});
    IrForm& iform = module_.forms.back();
    iform.name = form.name;
    iform.title = form_prop(form.properties, "Text", form.name);
    iform.width = form_int(form.properties, "Width", 640);
    iform.height = form_int(form.properties, "Height", 480);

    module_.functions.push_back(IrFunction{});
    IrFunction& fn = module_.functions.back();
    fn.name = "Form_" + form.name + "_Create";
    fn.return_type = PrimitiveType::Integer;
    current_fn_ = &fn;
    new_block("entry");

    IrValue form_val = new_vreg(PrimitiveType::Integer);
    emit(IrOpcode::Call,
         {make_const_string(iform.title), make_const_int(iform.width),
          make_const_int(iform.height)},
         form_val, "bf_form_new");

    for (const auto& ctrl : form.controls) {
        IrFormControl ic;
        ic.name = ctrl.name;
        ic.type = ctrl.type;
        ic.text = form_prop(ctrl.properties, "Text", "");
        ic.left = form_int(ctrl.properties, "Left", 10);
        ic.top = form_int(ctrl.properties, "Top", 10);
        ic.width = form_int(ctrl.properties, "Width", 100);
        ic.height = form_int(ctrl.properties, "Height", 30);

        IrValue ctrl_val = new_vreg(PrimitiveType::Integer);
        std::string create_fn = "bf_" + bf_lower_ascii(ctrl.type) + "_new";
        emit(IrOpcode::Call,
             {form_val, make_const_string(ic.text), make_const_int(ic.left),
              make_const_int(ic.top), make_const_int(ic.width),
              make_const_int(ic.height)},
             ctrl_val, create_fn);

        emit(IrOpcode::Call, {ctrl_val, make_const_string(ic.name)}, {},
             "bf_ui_set_name");

        for (const auto& ev : ctrl.events) {
            const int id = static_cast<int>(iform.event_ids.size());
            iform.event_ids.push_back({id, ev.second});
            const std::string ev_lower = bf_lower_ascii(ev.first);
            if (ev_lower == "textchanged") {
                emit(IrOpcode::Call, {ctrl_val, make_const_int(id)}, {},
                     "bf_set_textchanged");
            } else if (ev_lower == "doubleclick") {
                emit(IrOpcode::Call, {ctrl_val, make_const_int(id)}, {},
                     "bf_set_doubleclick");
            } else if (ev_lower == "selectedindexchanged" ||
                       ev_lower == "afterselect" ||
                       ev_lower == "valuechanged") {
                emit(IrOpcode::Call, {ctrl_val, make_const_int(id)}, {},
                     "bf_set_selection");
            } else if (ev_lower == "scroll") {
                emit(IrOpcode::Call, {ctrl_val, make_const_int(id)}, {},
                     "bf_set_scroll");
            } else if (ev_lower == "gotfocus") {
                emit(IrOpcode::Call, {ctrl_val, make_const_int(id)}, {},
                     "bf_set_focus");
            } else if (ev_lower == "lostfocus") {
                emit(IrOpcode::Call, {ctrl_val, make_const_int(id)}, {},
                     "bf_set_blur");
            } else if (ev_lower == "keydown") {
                emit(IrOpcode::Call, {ctrl_val, make_const_int(id)}, {},
                     "bf_set_keydown");
            } else if (ev_lower == "keyup") {
                emit(IrOpcode::Call, {ctrl_val, make_const_int(id)}, {},
                     "bf_set_keyup");
            } else if (ev_lower == "mousemove" || ev_lower == "mouseenter" ||
                       ev_lower == "mouseleave") {
                emit(IrOpcode::Call, {ctrl_val, make_const_int(id)}, {},
                     "bf_set_mousemove");
            } else if (ev_lower == "mousewheel") {
                emit(IrOpcode::Call, {ctrl_val, make_const_int(id)}, {},
                     "bf_set_mousewheel");
            } else if (ev_lower == "paint") {
                emit(IrOpcode::Call, {ctrl_val, make_const_int(id)}, {},
                     "bf_set_paint");
            } else if (ev_lower == "dragdrop" || ev_lower == "drop") {
                emit(IrOpcode::Call, {ctrl_val, make_const_int(id)}, {},
                     "bf_set_drop");
            } else {
                // Click / CheckedChanged / others map to a click event.
                emit(IrOpcode::Call, {ctrl_val, make_const_int(id)}, {},
                     "bf_set_click");
            }
        }

        iform.controls.push_back(ic);
    }

    emit(IrOpcode::Return, {form_val});
    current_fn_ = nullptr;
    current_bb_ = nullptr;
}

void IrBuilder::build_entry_main() {
    bool has_main = false;
    for (auto& fn : module_.functions) {
        if (fn.name == "Main") {
            fn.name = "Main_impl";
            has_main = true;
            break;
        }
    }

    module_.functions.push_back(IrFunction{});
    IrFunction& main_fn = module_.functions.back();
    main_fn.name = "Main";
    main_fn.return_type = PrimitiveType::Void;
    current_fn_ = &main_fn;
    new_block("entry");

    std::vector<IrValue> form_values;
    for (const auto& form : module_.forms) {
        IrValue form_val = new_vreg(PrimitiveType::Integer);
        emit(IrOpcode::Call, {}, form_val, "Form_" + form.name + "_Create");
        form_values.push_back(form_val);
    }
    if (has_main) {
        emit(IrOpcode::Call, {}, {}, "Main_impl");
    }
    for (const auto& form_val : form_values) {
        emit(IrOpcode::Call, {form_val}, {}, "bf_form_run");
    }
    emit(IrOpcode::Return, {});

    current_fn_ = nullptr;
    current_bb_ = nullptr;
}

void IrBuilder::lower_procedure(SubOrFunction& proc) {
    module_.functions.push_back(IrFunction{});
    IrFunction& fn = module_.functions.back();
    fn.name = proc.name;
    fn.return_type = proc.return_type.kind;
    var_types_.clear();
    for (auto& p : proc.parameters) {
        PrimitiveType pt = p.type.kind == PrimitiveType::Void ? PrimitiveType::Integer : p.type.kind;
        var_types_[p.name] = pt;
        fn.parameters.push_back(make_param(p.name, pt, p.is_array));
        if (p.is_array) array_vars_.insert(p.name);
    }
    current_fn_ = &fn;
    new_block("entry");
    lower_block(proc.body);
    if (current_bb_->instructions.empty() ||
        current_bb_->instructions.back().opcode != IrOpcode::Return) {
        emit(IrOpcode::Return, {});
    }
    current_fn_ = nullptr;
    current_bb_ = nullptr;
}

void IrBuilder::lower_block(const Block& block) {
    for (const auto& stmt : block.statements) {
        lower_statement(*stmt);
    }
}

void IrBuilder::lower_statement(const Statement& stmt) {
    std::visit([this](const auto& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, DimStmt>) {
            PrimitiveType t = s.type.kind == PrimitiveType::Void ? PrimitiveType::Integer : s.type.kind;
            var_types_[s.name] = t;
            if (s.is_const) {
                // Const declarations: evaluate initializer at compile time
                if (s.initializer) {
                    IrValue init = lower_expression(*s.initializer);
                    // If the initializer is a constant, store its value for compile-time resolution
                    if (init.kind == IrValue::Kind::Constant) {
                        constant_values_[s.name] = init.int_value;
                    }
                    // Also emit as a local for runtime use
                    IrValue local = make_local(s.name, t);
                    emit(IrOpcode::Local, {}, local);
                    emit(IrOpcode::Store, {init}, local);
                }
            } else {
                if (s.type.is_array && s.array_size) {
                    // Array declaration: allocate array memory (ArrayAlloc emits alloca)
                    IrValue size_val = lower_expression(*s.array_size);
                    IrValue array_base = make_local(s.name, t);
                    emit(IrOpcode::ArrayAlloc, {size_val}, array_base);
                    array_vars_.insert(s.name);
                } else if (s.type.is_generic && !s.type.generic_name.empty() &&
                       classLayouts_.count(s.type.generic_name)) {
                    // Class / structure declaration. A class variable holds a
                    // reference to a heap object; "As New Class(...)" already
                    // allocates and runs the constructor, otherwise allocate
                    // here and run the parameterless constructor if present.
                    IrValue local = make_local(s.name, t);
                    emit(IrOpcode::Local, {}, local);
                    struct_var_types_[s.name] = s.type.generic_name;
                    if (s.initializer) {
                        IrValue init = lower_expression(*s.initializer);
                        emit(IrOpcode::Store, {init}, local);
                    } else {
                        IrValue obj = new_vreg(t);
                        emit(IrOpcode::ObjAlloc, {}, obj, s.type.generic_name);
                        std::string ctor = s.type.generic_name + "_New";
                        bool hasCtor = false;
                        for (const auto& f : module_.functions) {
                            if (f.name == ctor) { hasCtor = true; break; }
                        }
                        if (hasCtor) emit(IrOpcode::Call, {obj}, {}, ctor);
                        emit(IrOpcode::Store, {obj}, local);
                    }
                } else {
                    // Scalar declaration
                    IrValue local = make_local(s.name, t);
                    emit(IrOpcode::Local, {}, local);
                    if (s.initializer) {
                        IrValue init = lower_expression(*s.initializer);
                        emit(IrOpcode::Store, {init}, local);
                    } else {
                        emit(IrOpcode::Store, {make_const_int(0)}, local);
                    }
                }
            }
        } else if constexpr (std::is_same_v<T, AssignmentStmt>) {
            auto vit = var_types_.find(s.name);
            bool stored_field = false;
            if (vit == var_types_.end() && !currentClass_.empty()) {
                // Bare field assignment inside a method: Name = value.
                auto cit = classLayouts_.find(currentClass_);
                if (cit != classLayouts_.end()) {
                    auto fit = cit->second.find(s.name);
                    if (fit != cit->second.end()) {
                        IrValue me = make_local("Me", PrimitiveType::Integer);
                        IrValue val = lower_expression(*s.expression);
                        emit(IrOpcode::FieldStore, {me, val}, {},
                             s.name + ":" + std::to_string(fit->second.first));
                        stored_field = true;
                    }
                }
            }
            if (!stored_field) {
                PrimitiveType t = (vit != var_types_.end()) ? vit->second : PrimitiveType::Integer;
                IrValue local = make_local(s.name, t);
                IrValue val = lower_expression(*s.expression);
                emit(IrOpcode::Store, {val}, local);
            }
        } else if constexpr (std::is_same_v<T, ArrayAssignmentStmt>) {
            auto vit = var_types_.find(s.name);
            PrimitiveType t = (vit != var_types_.end()) ? vit->second : PrimitiveType::Integer;
            IrValue array_base = make_local(s.name, t);
            IrValue index_val = lower_expression(*s.index);
            IrValue value_val = lower_expression(*s.expression);
            emit(IrOpcode::ArrayStore, {array_base, index_val, value_val}, {});
        } else if constexpr (std::is_same_v<T, FieldAssignmentStmt>) {
            // obj.field = value
            IrValue obj = lower_expression(*s.object);
            IrValue val = lower_expression(*s.expression);
            // Compute field offset from class/struct layout
            uint32_t offset = 0;
            std::string objTypeName;
            // Determine the object's type name
            if (std::holds_alternative<IdentifierExpr>(s.object->data)) {
                auto& objId = std::get<IdentifierExpr>(s.object->data);
                auto structIt = struct_var_types_.find(objId.name);
                if (structIt != struct_var_types_.end()) {
                    objTypeName = structIt->second;
                }
            }
            if (objTypeName.empty() && !currentClass_.empty()) {
                objTypeName = currentClass_;
            }
            if (!objTypeName.empty()) {
                auto layoutIt = classLayouts_.find(objTypeName);
                if (layoutIt != classLayouts_.end()) {
                    auto fieldIt = layoutIt->second.find(s.field_name);
                    if (fieldIt != layoutIt->second.end()) {
                        offset = fieldIt->second.first;
                    }
                }
            }
            emit(IrOpcode::FieldStore, {obj, val}, {}, s.field_name + ":" + std::to_string(offset));
        } else if constexpr (std::is_same_v<T, PrintStmt>) {
            for (const auto& arg : s.arguments) {
                IrValue v = lower_expression(*arg);
                emit(IrOpcode::CallBuiltin, {v}, {}, "Print");
            }
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            if (s.expression) {
                IrValue v = lower_expression(*s.expression);
                emit(IrOpcode::Return, {v});
            } else {
                emit(IrOpcode::Return, {});
            }
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            lower_if(*s.condition, s.then_branch, s.else_branch);
        } else if constexpr (std::is_same_v<T, ForStmt>) {
            lower_for(s);
        } else if constexpr (std::is_same_v<T, ForEachStmt>) {
            lower_for_each(s);
        } else if constexpr (std::is_same_v<T, DoLoopStmt>) {
            lower_do_loop(s);
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            lower_while(*s.condition, s.body);
        } else if constexpr (std::is_same_v<T, SelectCaseStmt>) {
            lower_select_case(s);
        } else if constexpr (std::is_same_v<T, ExpressionStmt>) {
            (void)lower_expression(*s.expression);
        } else if constexpr (std::is_same_v<T, TryCatchFinallyStmt>) {
            lower_try_catch_finally(s);
        } else if constexpr (std::is_same_v<T, ThrowStmt>) {
            lower_throw(s);
        }
    }, stmt.data);
}

IrValue IrBuilder::lower_expression(const Expression& expr) {
    IrValue result = make_const_int(0);
    std::visit([this, &result](const auto& e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, IntegerLiteralExpr>) {
            result = make_const_int(e.value);
        } else if constexpr (std::is_same_v<T, StringLiteralExpr>) {
            result = make_const_string(e.value);
        } else if constexpr (std::is_same_v<T, BooleanLiteralExpr>) {
            result = make_const_bool(e.value);
        } else if constexpr (std::is_same_v<T, DoubleLiteralExpr>) {
            result = make_const_double(e.value);
        } else if constexpr (std::is_same_v<T, IdentifierExpr>) {
            // Check if this is a compile-time constant (Const or Enum member)
            auto cit = constant_values_.find(e.name);
            if (cit != constant_values_.end()) {
                result = make_const_int(cit->second);
            } else {
                auto vit = var_types_.find(e.name);
                if (vit == var_types_.end() && !currentClass_.empty()) {
                    // Bare field read inside a method: Name -> Me.Name.
                    auto cit = classLayouts_.find(currentClass_);
                    if (cit != classLayouts_.end()) {
                        auto fit = cit->second.find(e.name);
                        if (fit != cit->second.end()) {
                            IrValue me = make_local("Me", PrimitiveType::Integer);
                            result = new_vreg(fit->second.second);
                            emit(IrOpcode::FieldLoad, {me}, result,
                                 e.name + ":" + std::to_string(fit->second.first));
                            return;
                        }
                    }
                }
                PrimitiveType itype = (vit != var_types_.end()) ? vit->second : PrimitiveType::Integer;
                result = make_local(e.name, itype);
            }
        } else if constexpr (std::is_same_v<T, ArrayAccessExpr>) {
            auto vit = var_types_.find(e.name);
            PrimitiveType etype = (vit != var_types_.end()) ? vit->second : PrimitiveType::Integer;
            IrValue array_base = make_local(e.name, etype);
            IrValue index_val = lower_expression(*e.index);
            result = new_vreg(etype);
            emit(IrOpcode::ArrayLoad, {array_base, index_val}, result);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            if (e.arguments.size() == 1 && array_vars_.count(e.callee)) {
                // Array element read: a(i) (the parser emits a CallExpr for
                // both calls and indexed reads).
                auto vit = var_types_.find(e.callee);
                PrimitiveType etype = (vit != var_types_.end()) ? vit->second
                                                                : PrimitiveType::Integer;
                IrValue idx = lower_expression(*e.arguments[0]);
                IrValue arr = make_local(e.callee, etype);
                result = new_vreg(etype);
                emit(IrOpcode::ArrayLoad, {arr, idx}, result);
            } else if (e.callee == "Print") {
                if (!e.arguments.empty()) {
                    result = lower_expression(*e.arguments[0]);
                }
            } else if (e.callee == "CInt" || e.callee == "CLng" ||
                       e.callee == "CDbl" || e.callee == "CStr" ||
                       e.callee == "CBool") {
                // Conversion function: single argument
                if (e.arguments.size() == 1) {
                    IrValue arg = lower_expression(*e.arguments[0]);
                    PrimitiveType retType = PrimitiveType::Integer;
                    if (e.callee == "CDbl") retType = PrimitiveType::Double;
                    else if (e.callee == "CStr") retType = PrimitiveType::String;
                    else if (e.callee == "CBool") retType = PrimitiveType::Boolean;
                    result = new_vreg(retType);
                    emit(IrOpcode::Call, {arg}, result, e.callee);
                }
            } else if (e.callee == "GetText" || e.callee == "SetText") {
                std::vector<IrValue> args;
                for (auto& arg : e.arguments) args.push_back(lower_expression(*arg));
                if (e.callee == "GetText") {
                    result = new_vreg(PrimitiveType::String);
                    emit(IrOpcode::Call, args, result, "bf_get_text_name");
                } else {
                    emit(IrOpcode::Call, args, {}, "bf_set_text_name");
                }
            } else if (e.callee == "Len" || e.callee == "Trim" ||
                   e.callee == "Left" || e.callee == "Right" ||
                   e.callee == "Mid" || e.callee == "InStr" ||
                   e.callee == "Replace" || e.callee == "Format" ||
                   e.callee == "Split" || e.callee == "Join") {
                // String builtin function
                PrimitiveType retType = PrimitiveType::String;
                if (e.callee == "Len" || e.callee == "InStr") retType = PrimitiveType::Integer;
                std::vector<IrValue> args;
                for (auto& arg : e.arguments) {
                    args.push_back(lower_expression(*arg));
                }
                result = new_vreg(retType);
                emit(IrOpcode::Call, args, result, e.callee);
            } else if (e.callee == "Sqr" || e.callee == "Abs" ||
                       e.callee == "Sgn" || e.callee == "Fix" ||
                       e.callee == "Int" || e.callee == "Rnd") {
                // Math builtin function
                PrimitiveType retType = PrimitiveType::Double;
                if (e.callee == "Sgn") retType = PrimitiveType::Integer;
                std::vector<IrValue> args;
                if (!e.arguments.empty()) {
                    args.push_back(lower_expression(*e.arguments[0]));
                }
                result = new_vreg(retType);
                emit(IrOpcode::Call, args, result, e.callee);
            } else if (e.callee == "Year" || e.callee == "Month" ||
                       e.callee == "Day" || e.callee == "Now") {
                // Date builtin function
                PrimitiveType retType = PrimitiveType::Integer;
                if (e.callee == "Now") retType = PrimitiveType::Double;
                std::vector<IrValue> args;
                if (!e.arguments.empty()) {
                    args.push_back(lower_expression(*e.arguments[0]));
                }
                result = new_vreg(retType);
                emit(IrOpcode::Call, args, result, e.callee);
            } else if (e.callee == "Open" || e.callee == "Close" ||
                       e.callee == "EOF" || e.callee == "LOF") {
                // File I/O builtin - mostly void
                result = new_vreg(PrimitiveType::Void);
                emit(IrOpcode::Call, {}, result, e.callee);
            } else {
                // User function or class method call.
                std::string callee = e.callee;
                PrimitiveType retType = PrimitiveType::Integer;
                bool found = false;

                // Prefer a method of the current class (Me is argument 0).
                if (!currentClass_.empty()) {
                    std::string preferred = currentClass_ + "_" + e.callee;
                    for (const auto& f : module_.functions) {
                        if (f.name == preferred) {
                            callee = preferred;
                            retType = f.return_type;
                            found = true;
                            break;
                        }
                    }
                }
                // Then a plain module-level function.
                if (!found) {
                    for (const auto& f : module_.functions) {
                        if (f.name == e.callee) {
                            callee = e.callee;
                            retType = f.return_type;
                            found = true;
                            break;
                        }
                    }
                }
                // Then any class method. The parser already put the object as
                // the first argument, which becomes the "Me" parameter.
                if (!found && method_functions_.count(e.callee)) {
                    callee = method_functions_[e.callee];
                    for (const auto& f : module_.functions) {
                        if (f.name == callee) {
                            retType = f.return_type;
                            found = true;
                            break;
                        }
                    }
                }
                if (found) {
                    result = new_vreg(retType);
                    std::vector<IrValue> args;
                    for (auto& arg : e.arguments) {
                        args.push_back(lower_expression(*arg));
                    }
                    emit(IrOpcode::Call, args, result, callee);
            } else if (e.callee == "UCase" || e.callee == "LCase" ||
                       e.callee == "Chr" || e.callee == "bf_get_dropped_file") {
                    std::vector<IrValue> args;
                    for (auto& arg : e.arguments) args.push_back(lower_expression(*arg));
                    result = new_vreg(PrimitiveType::String);
                    emit(IrOpcode::Call, args, result, e.callee);
                } else if (e.callee == "Asc") {
                    std::vector<IrValue> args;
                    for (auto& arg : e.arguments) args.push_back(lower_expression(*arg));
                    result = new_vreg(PrimitiveType::Integer);
                    emit(IrOpcode::Call, args, result, e.callee);
                } else if (e.callee == "Sleep" || e.callee == "Beep" ||
                           e.callee == "Clear" || e.callee == "SetTitle" ||
                           e.callee == "SetEnv" || e.callee == "WriteAllText" ||
                           e.callee == "AppendAllText" || e.callee == "MessageBox" ||
                           e.callee == "ShellExecute" || e.callee == "SetClipboard") {
                    std::vector<IrValue> args;
                    for (auto& arg : e.arguments) args.push_back(lower_expression(*arg));
                    emit(IrOpcode::Call, args, {}, e.callee);
                } else if (e.callee == "ReadAllText" || e.callee == "GetEnv" ||
                           e.callee == "ReadLine" || e.callee == "GetClipboard") {
                    std::vector<IrValue> args;
                    for (auto& arg : e.arguments) args.push_back(lower_expression(*arg));
                    result = new_vreg(PrimitiveType::String);
                    emit(IrOpcode::Call, args, result, e.callee);
                } else if (e.callee.rfind("bf_", 0) == 0) {
                    // External runtime helper (e.g. bf_form_test_click).
                    std::vector<IrValue> args;
                    for (auto& arg : e.arguments) {
                        args.push_back(lower_expression(*arg));
                    }
                    result = new_vreg(PrimitiveType::Integer);
                    emit(IrOpcode::Call, args, result, e.callee);
                }
            }
        } else if constexpr (std::is_same_v<T, NewExpr>) {
            // Object creation: allocate object memory and call constructor
            PrimitiveType etype = PrimitiveType::Integer; // class reference
            result = new_vreg(etype);
            emit(IrOpcode::ObjAlloc, {}, result, e.class_name);
            
            // Call constructor if it exists (ClassName_New)
            std::string ctorName = e.class_name + "_New";
            bool hasCtor = false;
            for (const auto& f : module_.functions) {
                if (f.name == ctorName) {
                    hasCtor = true;
                    break;
                }
            }
            if (hasCtor) {
                // Pass the object as the "Me" parameter, followed by the
                // explicit constructor arguments.
                std::vector<IrValue> args;
                args.push_back(result);
                for (auto& arg : e.arguments) {
                    args.push_back(lower_expression(*arg));
                }
                emit(IrOpcode::Call, args, {}, ctorName);
            }
        } else if constexpr (std::is_same_v<T, MemberAccessExpr>) {
            // Check for enum access: EnumName.Member
            bool isEnumAccess = false;
            if (std::holds_alternative<IdentifierExpr>(e.object->data)) {
                auto& objId = std::get<IdentifierExpr>(e.object->data);
                // If the object is an enum type name, resolve the member as a constant
                auto cit = constant_values_.find(e.member);
                if (cit != constant_values_.end() && (objId.name == "Color" || objId.name == "Enum")) {
                    result = make_const_int(cit->second);
                    isEnumAccess = true;
                }
            }
            if (!isEnumAccess) {
                // Object field access - use class layout for offset
                IrValue obj = lower_expression(*e.object);
                
                // Determine field type and offset from class/struct layout
                PrimitiveType etype = PrimitiveType::Integer;
                uint32_t offset = 0;
                std::string objTypeName;
                if (std::holds_alternative<IdentifierExpr>(e.object->data)) {
                    auto& objId = std::get<IdentifierExpr>(e.object->data);
                    auto structIt = struct_var_types_.find(objId.name);
                    if (structIt != struct_var_types_.end()) {
                        objTypeName = structIt->second;
                    }
                }
                if (objTypeName.empty() && !currentClass_.empty()) {
                    objTypeName = currentClass_;
                }
                if (!objTypeName.empty()) {
                    auto layoutIt = classLayouts_.find(objTypeName);
                    if (layoutIt != classLayouts_.end()) {
                        auto fieldIt = layoutIt->second.find(e.member);
                        if (fieldIt != layoutIt->second.end()) {
                            etype = fieldIt->second.second;
                            offset = fieldIt->second.first;
                        }
                    }
                }
                
                result = new_vreg(etype);
                emit(IrOpcode::FieldLoad, {obj}, result, e.member + ":" + std::to_string(offset));
            }
        } else if constexpr (std::is_same_v<T, MeExpr>) {
            // Me keyword: reference to current object
            auto vit = var_types_.find("Me");
            if (vit != var_types_.end()) {
                result = make_local("Me", vit->second);
            } else {
                result = make_const_int(0);
            }
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            IrValue l = lower_expression(*e.left);
            IrValue r = lower_expression(*e.right);
            PrimitiveType resultType = PrimitiveType::Integer;
            IrOpcode op = IrOpcode::Add;
            switch (e.op) {
                case BinaryExpr::Op::Add:
                case BinaryExpr::Op::Sub:
                case BinaryExpr::Op::Mul:
                    op = (e.op == BinaryExpr::Op::Add) ? IrOpcode::Add :
                         (e.op == BinaryExpr::Op::Sub) ? IrOpcode::Sub : IrOpcode::Mul;
                    resultType = (l.type == PrimitiveType::Double || r.type == PrimitiveType::Double) ?
                                 PrimitiveType::Double : PrimitiveType::Integer;
                    break;
                case BinaryExpr::Op::Div:
                    op = IrOpcode::Div;
                    resultType = (l.type == PrimitiveType::Double || r.type == PrimitiveType::Double) ?
                                 PrimitiveType::Double : PrimitiveType::Integer;
                    break;
                case BinaryExpr::Op::IntDiv:
                    op = IrOpcode::IntDiv;
                    resultType = PrimitiveType::Integer;
                    break;
                case BinaryExpr::Op::Mod:
                    op = IrOpcode::Mod;
                    resultType = PrimitiveType::Integer;
                    break;
                case BinaryExpr::Op::Concat: {
                    op = IrOpcode::Concat;
                    resultType = PrimitiveType::String;
                    // Convert non-string operands to text (VB-style &).
                    auto to_string = [this](IrValue v) -> IrValue {
                        if (v.type == PrimitiveType::String) return v;
                        if (v.type == PrimitiveType::Double ||
                            v.type == PrimitiveType::Single) {
                            IrValue s = new_vreg(PrimitiveType::String);
                            emit(IrOpcode::Call, {v}, s, "CStrDouble");
                            return s;
                        }
                        IrValue s = new_vreg(PrimitiveType::String);
                        emit(IrOpcode::Call, {v}, s, "CStr");
                        return s;
                    };
                    l = to_string(l);
                    r = to_string(r);
                    break;
                }
                case BinaryExpr::Op::Eq:
                case BinaryExpr::Op::Ne:
                case BinaryExpr::Op::Lt:
                case BinaryExpr::Op::Gt:
                case BinaryExpr::Op::Le:
                case BinaryExpr::Op::Ge:
                    op = (e.op == BinaryExpr::Op::Eq) ? IrOpcode::Eq :
                         (e.op == BinaryExpr::Op::Ne) ? IrOpcode::Ne :
                         (e.op == BinaryExpr::Op::Lt) ? IrOpcode::Lt :
                         (e.op == BinaryExpr::Op::Gt) ? IrOpcode::Gt :
                         (e.op == BinaryExpr::Op::Le) ? IrOpcode::Le : IrOpcode::Ge;
                    resultType = PrimitiveType::Boolean;
                    break;
                case BinaryExpr::Op::And:
                case BinaryExpr::Op::Or:
                case BinaryExpr::Op::Xor:
                    op = (e.op == BinaryExpr::Op::And) ? IrOpcode::And :
                         (e.op == BinaryExpr::Op::Or) ? IrOpcode::Or : IrOpcode::Xor;
                    resultType = PrimitiveType::Boolean;
                    break;
                case BinaryExpr::Op::Pow:
                    op = IrOpcode::Add;
                    resultType = PrimitiveType::Double;
                    break;
            }
            result = new_vreg(resultType);
            emit(op, {l, r}, result);
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            IrValue v = lower_expression(*e.operand);
            result = new_vreg((e.op == UnaryExpr::Op::Negate) ? v.type : PrimitiveType::Boolean);
            IrOpcode op = (e.op == UnaryExpr::Op::Negate) ? IrOpcode::Negate : IrOpcode::Not;
            emit(op, {v}, result);
        }
    }, expr.data);
    return result;
}

// ---------- Control flow ----------

void IrBuilder::lower_if(const Expression& cond,
                          const std::vector<std::unique_ptr<Statement>>& then_branch,
                          const std::vector<std::unique_ptr<Statement>>& else_branch) {
    IrValue cond_val = lower_expression(cond);
    std::string then_label = new_label();
    std::string end_label = new_label();
    std::string else_label = new_label();
    bool has_else = !else_branch.empty();

    // If cond true → then_label; cond false → end_label (or else_label)
    std::string false_target = has_else ? else_label : end_label;
    emit(IrOpcode::Branch, {cond_val}, {}, then_label, false_target);

    // then block
    emit(IrOpcode::Label, {}, {}, then_label);
    for (const auto& st : then_branch) lower_statement(*st);
    emit(IrOpcode::Jump, {}, {}, end_label);

    // else block
    if (has_else) {
        emit(IrOpcode::Label, {}, {}, else_label);
        for (const auto& st : else_branch) lower_statement(*st);
        emit(IrOpcode::Jump, {}, {}, end_label);
    }
    emit(IrOpcode::Label, {}, {}, end_label);
}

void IrBuilder::lower_while(const Expression& cond,
                            const std::vector<std::unique_ptr<Statement>>& body) {
    std::string loop_label = new_label();
    std::string body_label = new_label();
    std::string end_label = new_label();

    emit(IrOpcode::Label, {}, {}, loop_label);
    IrValue cond_val = lower_expression(cond);
    emit(IrOpcode::Branch, {cond_val}, {}, body_label, end_label);

    emit(IrOpcode::Label, {}, {}, body_label);
    for (const auto& st : body) lower_statement(*st);
    emit(IrOpcode::Jump, {}, {}, loop_label);

    emit(IrOpcode::Label, {}, {}, end_label);
}

void IrBuilder::lower_for(const ForStmt& s) {
    // For Each: iterate over array indices 0..len-1, load element into variable
    if (s.is_for_each) {
        std::string loop_label = new_label();
        std::string body_label = new_label();
        std::string end_label = new_label();

        // Get array length
        IrValue collection_val = lower_expression(*s.start);
        IrValue len_val = new_vreg(PrimitiveType::Integer);
        emit(IrOpcode::ArrayLen, {collection_val}, len_val);

        // Create internal index variable
        std::string index_var_name = s.variable + "_index";
        IrValue index_var = make_local(index_var_name, PrimitiveType::Integer);
        var_types_[index_var_name] = PrimitiveType::Integer;
        emit(IrOpcode::Local, {}, index_var);
        emit(IrOpcode::Store, {make_const_int(0)}, index_var);

        // Loop variable (holds array element)
        IrValue loop_var = make_local(s.variable, PrimitiveType::Integer);
        var_types_[s.variable] = PrimitiveType::Integer;
        emit(IrOpcode::Local, {}, loop_var);

        emit(IrOpcode::Label, {}, {}, loop_label);
        IrValue cond = new_vreg(PrimitiveType::Boolean);
        emit(IrOpcode::Lt, {index_var, len_val}, cond);
        emit(IrOpcode::Branch, {cond}, {}, body_label, end_label);

        emit(IrOpcode::Label, {}, {}, body_label);
        // Load array element into loop variable
        IrValue elem = new_vreg(PrimitiveType::Integer);
        emit(IrOpcode::ArrayLoad, {collection_val, index_var}, elem);
        emit(IrOpcode::Store, {elem}, loop_var);
        for (const auto& st : s.body) lower_statement(*st);

        IrValue next_val = new_vreg(PrimitiveType::Integer);
        emit(IrOpcode::Add, {index_var, make_const_int(1)}, next_val);
        emit(IrOpcode::Store, {next_val}, index_var);

        emit(IrOpcode::Jump, {}, {}, loop_label);
        emit(IrOpcode::Label, {}, {}, end_label);
        return;
    }

    std::string loop_label = new_label();
    std::string body_label = new_label();
    std::string end_label = new_label();

    // counter = start
    IrValue start_val = lower_expression(*s.start);
    IrValue counter = make_local(s.variable, PrimitiveType::Integer);
    emit(IrOpcode::Local, {}, counter);
    emit(IrOpcode::Store, {start_val}, counter);

    emit(IrOpcode::Label, {}, {}, loop_label);
    IrValue end_val = lower_expression(*s.end);
    IrValue cond = new_vreg(PrimitiveType::Boolean);
    bool negative_step = false;
    if (s.step) {
        if (const auto* u = std::get_if<UnaryExpr>(&s.step->data)) {
            if (u->op == UnaryExpr::Op::Negate) negative_step = true;
        } else if (const auto* lit = std::get_if<IntegerLiteralExpr>(&s.step->data)) {
            if (lit->value < 0) negative_step = true;
        }
    }
    emit(negative_step ? IrOpcode::Ge : IrOpcode::Le, {counter, end_val}, cond);
    emit(IrOpcode::Branch, {cond}, {}, body_label, end_label);

    emit(IrOpcode::Label, {}, {}, body_label);
    for (const auto& st : s.body) lower_statement(*st);

    IrValue step_val = s.step ? lower_expression(*s.step) : make_const_int(1);
    IrValue next_val = new_vreg(PrimitiveType::Integer);
    emit(IrOpcode::Add, {counter, step_val}, next_val);
    emit(IrOpcode::Store, {next_val}, counter);

    emit(IrOpcode::Jump, {}, {}, loop_label);
    emit(IrOpcode::Label, {}, {}, end_label);
}

void IrBuilder::lower_select_case(const SelectCaseStmt& s) {
    IrValue subject = lower_expression(*s.subject);

    size_t else_index = s.cases.size();
    for (size_t i = 0; i < s.cases.size(); ++i) {
        if (s.cases[i].values.empty()) { else_index = i; break; }
    }

    struct Comparison {
        size_t case_idx;
        const Expression* val_ptr;
        const Expression* range_end_ptr;
    };
    std::vector<Comparison> comparisons;
    for (size_t i = 0; i < s.cases.size(); ++i) {
        auto& c = s.cases[i];
        if (c.values.empty()) continue;
        for (size_t vi = 0; vi < c.values.size(); ++vi) {
            comparisons.push_back({i, c.values[vi].get(), c.range_end.get()});
        }
    }

    std::string end_label = new_label();
    std::string else_label = new_label();
    std::vector<std::string> body_labels(s.cases.size());
    for (size_t i = 0; i < s.cases.size(); ++i) {
        body_labels[i] = new_label();
    }

    if (comparisons.empty()) {
        if (else_index != s.cases.size()) {
            for (const auto& st : s.cases[else_index].body) lower_statement(*st);
        }
        return;
    }

    std::vector<std::string> check_labels;
    for (size_t i = 0; i < comparisons.size(); ++i) {
        check_labels.push_back(new_label());
    }

    // Straight-line comparison chain: each check branches to its case body,
    // otherwise falls through to the next check (or the Else body).
    for (size_t ci = 0; ci < comparisons.size(); ++ci) {
        auto& cmp = comparisons[ci];
        emit(IrOpcode::Label, {}, {}, check_labels[ci]);

        IrValue val = lower_expression(*cmp.val_ptr);
        IrValue cond = new_vreg(PrimitiveType::Boolean);
        emit(IrOpcode::Eq, {subject, val}, cond);

        if (cmp.range_end_ptr) {
            IrValue end_val = lower_expression(*cmp.range_end_ptr);
            IrValue ge = new_vreg(PrimitiveType::Boolean);
            IrValue le = new_vreg(PrimitiveType::Boolean);
            IrValue both = new_vreg(PrimitiveType::Boolean);
            emit(IrOpcode::Ge, {subject, val}, ge);
            emit(IrOpcode::Le, {subject, end_val}, le);
            emit(IrOpcode::And, {ge, le}, both);
            cond = both;
        }

        std::string next = (ci + 1 < comparisons.size()) ? check_labels[ci + 1]
                                                         : else_label;
        emit(IrOpcode::Branch, {cond}, {}, body_labels[cmp.case_idx], next);
    }

    // Else body (fall-through target of the last failed check).
    emit(IrOpcode::Label, {}, {}, else_label);
    if (else_index != s.cases.size()) {
        for (const auto& st : s.cases[else_index].body) lower_statement(*st);
    }
    emit(IrOpcode::Jump, {}, {}, end_label);

    // Case bodies.
    for (size_t i = 0; i < s.cases.size(); ++i) {
        if (s.cases[i].values.empty()) continue;
        emit(IrOpcode::Label, {}, {}, body_labels[i]);
        for (const auto& st : s.cases[i].body) lower_statement(*st);
        emit(IrOpcode::Jump, {}, {}, end_label);
    }

    emit(IrOpcode::Label, {}, {}, end_label);
}

void IrBuilder::lower_do_loop(const DoLoopStmt& s) {
    std::string loop_label = new_label();
    std::string body_label = new_label();
    std::string end_label = new_label();
    std::string bottom_label = new_label();

    if (s.loopType == DoLoopStmt::Type::DoWhile || s.loopType == DoLoopStmt::Type::DoUntil) {
        // Condition at top
        emit(IrOpcode::Label, {}, {}, loop_label);
        IrValue cond_val = lower_expression(*s.condition);
        bool jumpToBodyIfTrue = (s.loopType == DoLoopStmt::Type::DoWhile);
        // Branch: true -> body_label, false -> end_label
        std::string true_target = jumpToBodyIfTrue ? body_label : end_label;
        emit(IrOpcode::Branch, {cond_val}, {}, true_target, end_label);
    }

    // Body
    emit(IrOpcode::Label, {}, {}, body_label);
    for (const auto& st : s.body) lower_statement(*st);

    if (s.loopType == DoLoopStmt::Type::LoopWhile || s.loopType == DoLoopStmt::Type::LoopUntil) {
        // Condition at bottom
        emit(IrOpcode::Label, {}, {}, bottom_label);
        IrValue cond_val = lower_expression(*s.condition);
        bool jumpToBodyIfTrue = (s.loopType == DoLoopStmt::Type::LoopWhile);
        // Branch: true -> continue loop (body_label), false -> end_label
        emit(IrOpcode::Branch, {cond_val}, {}, body_label, end_label);
        
        // Emit end_label AFTER bottom_label so it's the next block for fallthrough
        emit(IrOpcode::Label, {}, {}, end_label);
        // Emit a dummy instruction to prevent empty block
        IrValue dummy = make_const_int(0);
        IrValue dummy_var = make_local("__dummy_end", PrimitiveType::Integer);
        emit(IrOpcode::Local, {}, dummy_var);
        emit(IrOpcode::Store, {dummy}, dummy_var);
    } else {
        // Jump back to top for DoWhile/DoUntil
        emit(IrOpcode::Jump, {}, {}, loop_label);
        
        // Emit end_label for DoWhile/DoUntil
        emit(IrOpcode::Label, {}, {}, end_label);
        // Emit a dummy instruction to prevent empty block
        IrValue dummy = make_const_int(0);
        IrValue dummy_var = make_local("__dummy_end", PrimitiveType::Integer);
        emit(IrOpcode::Local, {}, dummy_var);
        emit(IrOpcode::Store, {dummy}, dummy_var);
    }
}

void IrBuilder::lower_for_each(const ForEachStmt& s) {
    // For Each over array: iterate indices 0 to len-1
    // At each iteration, load the array element into the loop variable
    std::string loop_label = new_label();
    std::string body_label = new_label();
    std::string end_label = new_label();

    // Get array length
    IrValue collection_val = lower_expression(*s.collection);
    IrValue len_val = new_vreg(PrimitiveType::Integer);
    emit(IrOpcode::ArrayLen, {collection_val}, len_val);

    // Create internal index variable
    std::string index_var_name = s.variable + "_index";
    IrValue index_var = make_local(index_var_name, PrimitiveType::Integer);
    var_types_[index_var_name] = PrimitiveType::Integer;
    emit(IrOpcode::Local, {}, index_var);
    emit(IrOpcode::Store, {make_const_int(0)}, index_var);

    // Create user's loop variable (will hold array element)
    IrValue loop_var = make_local(s.variable, PrimitiveType::Integer);
    var_types_[s.variable] = PrimitiveType::Integer;
    emit(IrOpcode::Local, {}, loop_var);

    emit(IrOpcode::Label, {}, {}, loop_label);

    // Check if index < len
    IrValue cmp_result = new_vreg(PrimitiveType::Boolean);
    emit(IrOpcode::Lt, {index_var, len_val}, cmp_result);

    emit(IrOpcode::Branch, {cmp_result}, {}, body_label, end_label);

    emit(IrOpcode::Label, {}, {}, body_label);

    // Load array element at current index into user's loop variable
    IrValue elem_val = new_vreg(PrimitiveType::Integer);
    emit(IrOpcode::ArrayLoad, {collection_val, index_var}, elem_val);
    emit(IrOpcode::Store, {elem_val}, loop_var);

    // Execute body
    for (const auto& st : s.body) lower_statement(*st);

    // Increment index
    IrValue inc_result = new_vreg(PrimitiveType::Integer);
    emit(IrOpcode::Add, {index_var, make_const_int(1)}, inc_result);
    emit(IrOpcode::Store, {inc_result}, index_var);

    emit(IrOpcode::Jump, {}, {}, loop_label);

    emit(IrOpcode::Label, {}, {}, end_label);
}

void IrBuilder::lower_try_catch_finally(const TryCatchFinallyStmt& tcf) {
    std::string try_start_label = new_label();
    std::string try_end_label = new_label();
    std::string catch_start_label = new_label();
    std::string catch_end_label = new_label();
    std::string finally_start_label = new_label();
    std::string finally_end_label = new_label();

    // Mark try start - jump to catch label on exception
    emit(IrOpcode::TryStart, {}, {}, catch_start_label, try_end_label);

    // Try body
    emit(IrOpcode::Label, {}, {}, try_start_label);
    for (const auto& st : tcf.try_body) lower_statement(*st);
    // Normal completion of try - skip catch/finally
    emit(IrOpcode::TryEnd, {}, {}, finally_start_label);
    emit(IrOpcode::Jump, {}, {}, finally_start_label);

    // Catch clauses
    emit(IrOpcode::Label, {}, {}, catch_start_label);
    for (size_t i = 0; i < tcf.catch_clauses.size(); ++i) {
        const auto& cc = tcf.catch_clauses[i];
        // Store exception variable if provided
        if (!cc.exception_variable.empty()) {
            IrValue exc_var = make_local(cc.exception_variable, PrimitiveType::String);
            // Load current exception value into exception variable
            emit(IrOpcode::Store, {}, exc_var);
        }
        emit(IrOpcode::CatchStart, {}, {});
        for (const auto& st : cc.body) lower_statement(*st);
        emit(IrOpcode::CatchEnd, {}, {}, finally_start_label);
        if (i < tcf.catch_clauses.size() - 1) {
            emit(IrOpcode::Jump, {}, {}, finally_start_label);
        }
    }

    // Finally block
    emit(IrOpcode::Label, {}, {}, finally_start_label);
    emit(IrOpcode::FinallyStart, {}, {});
    for (const auto& st : tcf.finally_body) lower_statement(*st);
    emit(IrOpcode::FinallyEnd, {}, {});
    emit(IrOpcode::Label, {}, {}, finally_end_label);
}

void IrBuilder::lower_throw(const ThrowStmt& stmt) {
    if (stmt.expression) {
        IrValue val = lower_expression(*stmt.expression);
        emit(IrOpcode::Throw, {val}, {});
    } else {
        emit(IrOpcode::Throw, {}, {});
    }
}

} // namespace bfc
