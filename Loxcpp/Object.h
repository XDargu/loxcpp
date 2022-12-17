#ifndef loxcpp_object_h
#define loxcpp_object_h

#include <string>
#include <iostream>

#include "Common.h"
#include "Chunk.h"
#include "Value.h"

enum class ObjType
{
    STRING,
    NATIVE,
    FUNCTION,
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
        //std::cout << "STRING created: " << this->chars << std::endl;
    }
    ~ObjString()
    {
        //std::cout << "STRING destroyed: " << this->chars << std::endl;
    }
    int length;
    std::string chars;
};

struct ObjFunction : Obj
{
    ObjFunction(int arity, const Chunk& chunk, ObjString* name)
        : Obj(ObjType::FUNCTION)
        , arity(arity)
        , chunk(chunk)
        , name(name)
    {
        //std::cout << "FUNCTION created: " << this->chars << std::endl;
    }
    ~ObjFunction()
    {
        //std::cout << "FUNCTION destroyed: " << this->chars << std::endl;
    }

    int arity;
    Chunk chunk;
    ObjString* name;
};

using NativeFn = Value(*)(int argCount, Value* args);

struct ObjNative : Obj
{
    ObjNative(uint8_t arity, NativeFn function)
        : Obj(ObjType::NATIVE)
        , function(function)
        , arity(arity)
    {
        //std::cout << "FUNCTION created: " << this->chars << std::endl;
    }
    ~ObjNative()
    {
        //std::cout << "FUNCTION destroyed: " << this->chars << std::endl;
    }

    NativeFn function;
    uint8_t arity;
};

inline ObjType getObjType(const Value& value) { return asObject(value)->type; }
inline bool isObjType(const Value& value, const ObjType type)
{
    return isObject(value) && asObject(value)->type == type;
}
inline bool isString(const Value& value) { return isObjType(value, ObjType::STRING); }
inline bool isFunction(const Value& value) { return isObjType(value, ObjType::FUNCTION); }
inline bool isNative(const Value& value) { return isObjType(value, ObjType::NATIVE); }

inline const char* asCString(const Value& value) { return static_cast<ObjString*>(asObject(value))->chars.c_str(); }

inline ObjString* asString(const Value& value) { return static_cast<ObjString*>(asObject(value)); }
inline ObjFunction* asFunction(const Value& value) { return static_cast<ObjFunction*>(asObject(value)); }
inline ObjNative* asNative(const Value& value) { return static_cast<ObjNative*>(asObject(value)); }

ObjString* copyString(const char* chars, int length);
ObjString* takeString(const char* chars, int length);

ObjFunction* newFunction();
ObjNative* newNative(uint8_t arity, NativeFn function);

void printObject(Value value);

#endif