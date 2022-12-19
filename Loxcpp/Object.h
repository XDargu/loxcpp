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
    UPVALUE,
    FUNCTION,
    CLOSURE,
    RANGE,
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
    ObjString(std::string&& str)
        : Obj(ObjType::STRING)
        , length(str.length())
        , chars(std::move(str))
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
        , upvalueCount(0)
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
    int upvalueCount;
    Chunk chunk;
    ObjString* name;
};

struct ObjUpvalue : Obj
{
    ObjUpvalue(Value* location)
        : Obj(ObjType::UPVALUE)
        , location(location)
        , closed()
        , next(nullptr)
    {
        //std::cout << "UPVALUE created: " << this->chars << std::endl;
    }
    ~ObjUpvalue()
    {
        //std::cout << "UPVALUE destroyed: " << this->chars << std::endl;
    }

    Value* location;
    Value closed;
    ObjUpvalue* next;
};

struct ObjClosure : Obj
{
    ObjClosure(ObjFunction* function)
        : Obj(ObjType::CLOSURE)
        , function(function)
        , upvalues(function->upvalueCount, nullptr)
    {
        //std::cout << "CLOSURE created: " << this->chars << std::endl;
    }
    ~ObjClosure()
    {
        //std::cout << "CLOSURE destroyed: " << this->chars << std::endl;
    }

    ObjFunction* function;
    std::vector<ObjUpvalue*> upvalues;
};

using NativeFn = Value(*)(int argCount, Value* args);

struct ObjNative : Obj
{
    ObjNative(uint8_t arity, NativeFn function)
        : Obj(ObjType::NATIVE)
        , function(function)
        , arity(arity)
    {
        //std::cout << "NATIVE created: " << this->chars << std::endl;
    }
    ~ObjNative()
    {
        //std::cout << "NATIVE destroyed: " << this->chars << std::endl;
    }

    NativeFn function;
    uint8_t arity;
};

struct ObjRange : Obj
{
    ObjRange(double min, double max)
        : Obj(ObjType::RANGE)
        , min(min)
        , max(max)
    {
        //std::cout << "RANGE created: " << this->chars << std::endl;
    }
    ~ObjRange()
    {
        //std::cout << "RANGE destroyed: " << this->chars << std::endl;
    }

    bool contains(double value)
    {
        if (min < max) return value >= min && value<= max; // 1..5
        return value <= min && value >= max; // 5..1
    }

    bool isInBounds(double idx)
    {
        if (min < max) return idx >= 0 && idx <= max - min; // 1..5
        return idx >= 0 && idx <= min - max; // 5..1
    }

    double getValue(double idx)
    {
        if (min < max) return min + idx; // 1..5
        return min - idx; // 5..1
    }

    double min;
    double max;
};

inline ObjType getObjType(const Value& value) { return asObject(value)->type; }
inline bool isObjType(const Value& value, const ObjType type)
{
    return isObject(value) && asObject(value)->type == type;
}
inline bool isString(const Value& value) { return isObjType(value, ObjType::STRING); }
inline bool isClosure(const Value& value) { return isObjType(value, ObjType::CLOSURE); }
inline bool isFunction(const Value& value) { return isObjType(value, ObjType::FUNCTION); }
inline bool isNative(const Value& value) { return isObjType(value, ObjType::NATIVE); }
inline bool isRange(const Value& value) { return isObjType(value, ObjType::RANGE); }

inline const char* asCString(const Value& value) { return static_cast<ObjString*>(asObject(value))->chars.c_str(); }

inline ObjString* asString(const Value& value) { return static_cast<ObjString*>(asObject(value)); }
inline ObjClosure* asClosure(const Value& value) { return static_cast<ObjClosure*>(asObject(value)); }
inline ObjFunction* asFunction(const Value& value) { return static_cast<ObjFunction*>(asObject(value)); }
inline ObjNative* asNative(const Value& value) { return static_cast<ObjNative*>(asObject(value)); }
inline ObjRange* asRange(const Value& value) { return static_cast<ObjRange*>(asObject(value)); }

ObjString* copyString(const char* chars, int length);
ObjString* takeString(const char* chars, int length);
ObjString* takeString(std::string&& chars);

ObjUpvalue* newUpvalue(Value* slot);
ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjNative* newNative(uint8_t arity, NativeFn function);

ObjRange* newRange(double min, double max);

void printObject(const Value& value);
ObjString* objectAsString(const Value& value);
ObjString* concatenate(ObjString* a, ObjString* b);

#endif