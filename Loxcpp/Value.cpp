#include "Value.h"

#include <iostream>

#include "Object.h"

void printValue(Value value)
{
    switch (value.type)
    {
    case ValueType::BOOL:
        std::cout << (asBoolean(value) ? "true" : "false");
        break;
    case ValueType::NIL: std::cout << "nil"; break;
    case ValueType::NUMBER: std::cout << asNumber(value); break;
    case ValueType::OBJ: printObject(value); break;
    }
    
}

bool Value::operator==(const Value& other) const
{
    if (type != other.type) return false;
    switch (type)
    {
        case ValueType::BOOL:   return asBoolean(*this) == asBoolean(other);
        case ValueType::NIL:    return true;
        case ValueType::NUMBER: return asNumber(*this) == asNumber(other);
        case ValueType::OBJ:
        {
            return asObject(*this) == asObject(other);
        }
        default:                return false; // Unreachable.
    }
}
