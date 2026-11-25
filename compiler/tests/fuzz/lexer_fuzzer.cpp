// Fuzz test for BasicForge lexer
#include <bf/lexer.h>
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
        auto tokens = lexer.tokenize();
        
        // If we got here without crashing, the lexer handled the input
    } catch (...) {
        // Lexer threw an exception - that's a bug
        return 1;
    }
    
    return 0;
}