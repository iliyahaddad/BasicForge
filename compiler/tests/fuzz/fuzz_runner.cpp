// Standalone fuzz runner (for use without libFuzzer)
#include "../test_helpers.h"
#include <bf/lexer.h>
#include <bf/parser.h>
#include <bf/diagnostic.h>
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <vector>

std::string generate_random_input(size_t length) {
    static const char charset[] = 
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
        "{}[]()<>+-*/=.,;:'\" \t\n\r"
        "Sub Function Class Module Dim If Then Else For While Do Loop Select Case Try Catch Throw Return Print";
    
    static std::mt19937 rng(12345); // Fixed seed for reproducibility
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += charset[dist(rng)];
    }
    return result;
}

int fuzz_lexer(const std::string& input) {
    try {
        bfc::Lexer lexer(input);
        for (;;) {
            bfc::Token token = lexer.next_token();
            if (token.type == bfc::TokenType::Eof) break;
        }
        return 0;
    } catch (...) {
        std::cerr << "Lexer crash on input: " << input.substr(0, 100) << "...\n";
        return 1;
    }
}

int fuzz_parser(const std::string& input) {
    try {
        bfc::Lexer lexer(input);
        bfc::TestSink sink;
        bfc::Parser parser(lexer, sink);
        auto ast = parser.parse();
        (void)ast;
        return 0;
    } catch (...) {
        std::cerr << "Parser crash on input: " << input.substr(0, 100) << "...\n";
        return 1;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: fuzz_runner <mode> [iterations] [max_length]\n";
        std::cerr << "  mode: lexer, parser, both\n";
        return 1;
    }
    
    std::string mode = argv[1];
    int iterations = (argc > 2) ? std::stoi(argv[2]) : 1000;
    size_t max_length = (argc > 3) ? std::stoi(argv[3]) : 200;
    
    std::cout << "Fuzzing " << mode << " with " << iterations << " iterations, max length " << max_length << "\n";
    
    int failures = 0;
    for (int i = 0; i < iterations; ++i) {
        std::string input = generate_random_input(1 + (i % max_length));
        
        if (mode == "lexer" || mode == "both") {
            failures += fuzz_lexer(input);
        }
        if (mode == "parser" || mode == "both") {
            failures += fuzz_parser(input);
        }
        
        if (failures > 0) {
            std::cerr << "Fuzzing failed at iteration " << i << "\n";
            break;
        }
    }
    
    if (failures == 0) {
        std::cout << "All fuzz iterations passed!\n";
        return 0;
    } else {
        std::cerr << failures << " failures detected\n";
        return 1;
    }
}