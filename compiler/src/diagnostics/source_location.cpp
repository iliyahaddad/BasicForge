#include "bf/bfc.h"

namespace bfc {

std::string primitive_type_name(PrimitiveType t) {
    switch (t) {
        case PrimitiveType::Void:    return "Void";
        case PrimitiveType::Integer: return "Integer";
        case PrimitiveType::Long:    return "Long";
        case PrimitiveType::Single:  return "Single";
        case PrimitiveType::Double:  return "Double";
        case PrimitiveType::Boolean: return "Boolean";
        case PrimitiveType::String:  return "String";
    }
    return "Unknown";
}

} // namespace bfc
