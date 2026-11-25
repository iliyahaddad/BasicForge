#pragma once

#include "bf/ast.h"

namespace bfc {

// BasicForge intermediate representation (language-owned, independent of LLVM).
// This is the IR that the optimizer works on before lowering to LLVM IR.

enum class IrOpcode : uint8_t {
    // Values
    ConstInt,
    ConstString,
    ConstBool,
    Local,
    Param,

    // Arithmetic
    Add, Sub, Mul, Div, IntDiv, Mod, Negate,

    // Compare
    Eq, Ne, Lt, Gt, Le, Ge,

    // Logic / string
    And, Or, Xor, Not, Concat,

    // Control
    Branch, Jump, Label, Return,

    // Memory
    Store,
    ArrayAlloc,
    ArrayStore,
    ArrayLoad,
    ArrayLen,
    ObjAlloc,
    FieldStore,
    FieldLoad,

    // Calls
    Call, CallBuiltin, CallVirt,

    // Exception handling
    TryStart,
    TryEnd,
    CatchStart,
    CatchEnd,
    Throw,
    FinallyStart,
    FinallyEnd,
};

struct IrValue {
    enum class Kind : uint8_t { VirtualRegister, Constant, Local, Parameter };
    Kind kind;
    PrimitiveType type;
    std::string name;      // for Local/Parameter
    int64_t int_value;     // for Constant
    double double_value;   // for Constant double
    std::string str_value; // for Constant string
    bool bool_value;       // for Constant bool
    bool is_array = false; // parameter is passed by reference (array)
};

struct IrInstruction {
    IrOpcode opcode;
    std::vector<IrValue> operands;
    IrValue result;
    std::string label;     // for Label/Branch targets (true target for Branch)
    std::string label2;    // for Branch false target
    SourceLocation loc;
};

struct IrBasicBlock {
    std::string name;
    std::vector<IrInstruction> instructions;
};

struct IrFunction {
    std::string name;
    std::vector<IrValue> parameters;
    PrimitiveType return_type;
    std::vector<IrBasicBlock> blocks;
};

struct IrFormControl {
    std::string name;
    std::string type;
    std::string text;
    int left = 0;
    int top = 0;
    int width = 100;
    int height = 30;
};

struct IrForm {
    std::string name;
    std::string title;
    int width = 640;
    int height = 480;
    std::vector<IrFormControl> controls;
    std::vector<std::pair<int, std::string>> event_ids; // event id -> handler name
};

struct IrModule {
    std::string name;
    std::vector<IrFunction> functions;
    std::vector<IrForm> forms;
};

} // namespace bfc
