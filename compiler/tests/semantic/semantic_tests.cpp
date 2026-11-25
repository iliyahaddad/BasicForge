// Semantic analysis unit tests.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "test_helpers.h"

#include <bf/ast.h>
#include <bf/parser.h>
#include <bf/semantic.h>
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
        assert(ast != nullptr && !sink.has_errors());
        SemanticAnalyzer analyzer(sink);
        analyzer.analyze(*ast);
        assert(!sink.has_errors());
    }

    {
        // Class members must be visible inside methods.
        TestSink sink;
        auto ast = parse(
            "Module Test\n"
            "    Class Person\n"
            "        Public Name As String\n"
            "        Public Age As Integer\n"
            "        Public Sub Show()\n"
            "            Print(Name & \": \" & CStr(Age))\n"
            "        End Sub\n"
            "    End Class\n"
            "    Sub Main()\n"
            "    End Sub\n"
            "End Module",
            sink);
        assert(ast != nullptr && !sink.has_errors());
        SemanticAnalyzer analyzer(sink);
        analyzer.analyze(*ast);
        assert(!sink.has_errors());
    }

    {
        // Invalid syntax must be reported.
        TestSink sink;
        auto ast = parse("Module Test\nSub Main()\nIf True Then\n", sink);
        assert(ast != nullptr);
        assert(sink.has_errors());
    }

    std::cout << "All semantic tests passed!\n";
    return 0;
}