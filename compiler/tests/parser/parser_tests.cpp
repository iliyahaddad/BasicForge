// Parser unit tests.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "test_helpers.h"

#include <bf/ast.h>
#include <bf/parser.h>
#include <cassert>
#include <iostream>
#include <memory>

using namespace bfc;

static std::unique_ptr<Program> parse(const std::string& source, TestSink& sink) {
    Lexer lexer(source);
    Parser parser(lexer, sink);
    return parser.parse();
}

int main() {
    {
        TestSink sink;
        auto ast = parse("Module Test\nSub Main()\nPrint(\"Hello\")\nEnd Sub\nEnd Module", sink);
        assert(ast != nullptr);
        assert(!sink.has_errors());
        assert(ast->modules.size() == 1);
        assert(ast->modules[0]->name == "Test");
    }

    {
        TestSink sink;
        auto ast = parse(
            "Module Test\n"
            "Sub Main()\n"
            "    Dim x As Integer\n"
            "    Dim y As String = \"hi\"\n"
            "End Sub\n"
            "End Module",
            sink);
        assert(ast != nullptr);
        assert(!sink.has_errors());
    }

    {
        TestSink sink;
        auto ast = parse(
            "Module Test\n"
            "Sub Main()\n"
            "    If True Then\n"
            "        Print(\"yes\")\n"
            "    End If\n"
            "End Sub\n"
            "End Module",
            sink);
        assert(ast != nullptr);
        assert(!sink.has_errors());
    }

    {
        TestSink sink;
        auto ast = parse(
            "Module Test\n"
            "Function Add(a As Integer, b As Integer) As Integer\n"
            "    Return a + b\n"
            "End Function\n"
            "Sub Main()\n"
            "End Sub\n"
            "End Module",
            sink);
        assert(ast != nullptr);
        assert(!sink.has_errors());
        assert(ast->modules[0]->procedures.size() >= 2);
    }

    {
        // Class with fields, properties, methods and a constructor.
        TestSink sink;
        auto ast = parse(
            "Module Test\n"
            "    Class Counter\n"
            "        Public Value As Integer\n"
            "        Public Sub Add(ByVal n As Integer)\n"
            "            Value = Value + n\n"
            "        End Sub\n"
            "        Public Function Get() As Integer\n"
            "            Return Value\n"
            "        End Function\n"
            "    End Class\n"
            "    Sub Main()\n"
            "        Dim c As New Counter()\n"
            "        c.Add(7)\n"
            "    End Sub\n"
            "End Module",
            sink);
        assert(ast != nullptr);
        assert(!sink.has_errors());
        assert(ast->modules[0]->classes.size() == 1);
        assert(ast->modules[0]->classes[0]->methods.size() == 2);
    }

    {
        // Broken input must produce a diagnostic.
        TestSink sink;
        auto ast = parse("Module Test\nSub Main()\nPrint(\"x\")\n", sink);
        assert(ast != nullptr);
        assert(sink.has_errors());
    }

    std::cout << "All parser tests passed!\n";
    return 0;
}