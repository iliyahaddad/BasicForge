// Diagnostic sink tests.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "test_helpers.h"

#include <bf/diagnostic.h>
#include <bf/parser.h>
#include <cassert>
#include <iostream>

using namespace bfc;

int main() {
    {
        // A clean program produces no errors.
        TestSink sink;
        Lexer lexer("Module Test\nSub Main()\nPrint(\"Hello\")\nEnd Sub\nEnd Module");
        Parser parser(lexer, sink);
        auto ast = parser.parse();
        assert(ast != nullptr);
        assert(!sink.has_errors());
        assert(sink.diagnostics.empty());
    }

    {
        // Missing End Sub must produce an error diagnostic.
        TestSink sink;
        Lexer lexer("Module Test\nSub Main()\nPrint(\"x\")\n");
        Parser parser(lexer, sink);
        auto ast = parser.parse();
        assert(ast != nullptr);
        assert(sink.has_errors());
        assert(!sink.diagnostics.empty());
    }

    {
        // TestSink records severity and message.
        TestSink sink;
        Diagnostic d;
        d.severity = DiagnosticSeverity::Error;
        d.message = "boom";
        sink.emit(d);
        assert(sink.has_errors());
        assert(sink.diagnostics.size() == 1);
        assert(sink.diagnostics[0].message == "boom");
    }

    std::cout << "All diagnostic tests passed!\n";
    return 0;
}