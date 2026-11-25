#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace bfc {

// Source location for diagnostics and error reporting.
struct SourceLocation {
    std::string file;
    uint32_t line = 0;
    uint32_t column = 0;
    uint32_t offset = 0;

    SourceLocation() = default;
    SourceLocation(std::string f, uint32_t l, uint32_t c, uint32_t o)
        : file(std::move(f)), line(l), column(c), offset(o) {}
};

// A contiguous span of source text.
struct SourceSpan {
    SourceLocation start;
    SourceLocation end;
};

// BasicForge primitive types (language-owned, independent of LLVM).
enum class PrimitiveType : uint8_t {
    Void,
    Integer,   // 32-bit signed
    Long,      // 64-bit signed
    Single,    // 32-bit float
    Double,    // 64-bit float
    Boolean,
    String,
};

std::string primitive_type_name(PrimitiveType t);

} // namespace bfc