// IR builder unit tests.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "test_helpers.h"

#include <bf/ast.h>
#include <bf/ir_builder.h>
#include <bf/parser.h>
#include <bf/semantic.h>
#include <cassert>
#include <iostream>

using namespace bfc;

static IrModule lower(const std::string& source, TestSink& sink) {
    Lexer lexer(source);
    Parser parser(lexer, sink);
    auto ast = parser.parse();
    assert(ast != nullptr);
    SemanticAnalyzer analyzer(sink);
    analyzer.analyze(*ast);
    IrBuilder irb(sink);
    return irb.lower(*ast);
}

int main() {
    {
        TestSink sink;
        auto module = lower("Module Test\nSub Main()\nPrint(\"Hello\")\nEnd Sub\nEnd Module", sink);
        assert(!sink.has_errors());
        assert(module.functions.size() >= 1);
    }

    {
        TestSink sink;
        auto module = lower(
            "Module Test\n"
            "Function Add(a As Integer, b As Integer) As Integer\n"
            "    Return a + b\n"
            "End Function\n"
            "Sub Main()\n"
            "    Dim x As Integer = Add(1, 2)\n"
            "End Sub\n"
            "End Module",
            sink);
        assert(!sink.has_errors());
        assert(module.functions.size() >= 2); // Add + Main
    }

    {
        // Class methods must be lowered as Class_Method functions.
        TestSink sink;
        auto module = lower(
            "Module Test\n"
            "    Class Counter\n"
            "        Public Value As Integer\n"
            "        Public Sub Add(ByVal n As Integer)\n"
            "            Value = Value + n\n"
            "        End Sub\n"
            "    End Class\n"
            "    Sub Main()\n"
            "        Dim c As New Counter()\n"
            "        c.Add(7)\n"
            "    End Sub\n"
            "End Module",
            sink);
        assert(!sink.has_errors());
        bool found_add = false;
        for (const auto& fn : module.functions) {
            if (fn.name == "Counter_Add") found_add = true;
        }
        assert(found_add);
    }

    std::cout << "All IR tests passed!\n";
    return 0;
}