#ifndef loxcpp_object_h
#define loxcpp_object_h

#include <string>
#include <iostream>

#include "Common.h"
#include "Value.h"

enum class ObjType
{
    STRING,
};

struct Obj
{
    Obj(ObjType type)
        : type(type)
        , hash(0)
    {}

    virtual ~Obj() {}

    ObjType type;
    uint32_t hash;
};

struct ObjString : Obj
{
    ObjString(const char* chars, int length)
        : Obj(ObjType::STRING)
        , length(length)
        , chars(chars, length)
    {
        //std::cout << "String created: " << this->chars << std::endl;
    }
    ~ObjString()
    {
        //std::cout << "String destroyed: " << this->chars << std::endl;
    }
    int length;
    std::string chars;
};

inline ObjType getObjType(const Value& value) { return asObject(value)->type; }
inline bool isObjType(const Value& value, const ObjType type)
{
    return isObject(value) && asObject(value)->type == type;
}
inline bool isString(const Value& value) { return isObjType(value, ObjType::STRING); }
inline ObjString* asString(const Value& value) { return static_cast<ObjString*>(asObject(value)); }
inline const char* asCString(const Value& value) { return static_cast<ObjString*>(asObject(value))->chars.c_str(); }

ObjString* copyString(const char* chars, int length);
ObjString* takeString(const char* chars, int length);

void printObject(Value value);

#endif