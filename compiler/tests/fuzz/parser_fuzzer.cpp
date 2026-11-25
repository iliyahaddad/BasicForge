// Fuzz test for BasicForge parser
#include <bf/lexer.h>
#include <bf/parser.h>
#include <bf/diagnostic.h>
#include <cstdint>
#include <cstddef>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) return 0;
    
    // Convert input to string
    std::string input(reinterpret_cast<const char*>(data), size);
    
    try {
        // Run lexer
        bfc::Lexer lexer(input);
        
        // Run parser
        bfc::DiagnosticSink sink;
        bfc::Parser parser(lexer, sink);
        auto ast = parser.parse();
        
        // If we got here without crashing, the parser handled the input
        (void)ast; // Suppress unused warning
    } catch (...) {
        // Parser threw an exception - that's a bug
        return 1;
    }
    
    return 0;
}