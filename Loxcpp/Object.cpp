#include "Object.h"

#include <iostream>

#include "Memory.h"
#include "VM.h"

uint32_t hashString(const char* key, int length)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++)
    {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* allocateString(const char* chars, int length, uint32_t hash)
{
    // TODO: the std::string allocates memory in the heap, can be improved!
    ObjString* string = new ObjString(chars, length);
    VM::getInstance().addObject(string);
    string->hash = hash;

    VM::getInstance().stringTable().set(string, Value());
    return string;
}

ObjString* copyString(const char* chars, int length)
{
    const uint32_t hash = hashString(chars, length);
    ObjString* interned = VM::getInstance().stringTable().findString(chars, length, hash);
    if (interned != nullptr) return interned;

    return allocateString(chars, length, hash);
}

ObjString* takeString(const char* chars, int length)
{
    const uint32_t hash = hashString(chars, length);
    ObjString* interned = VM::getInstance().stringTable().findString(chars, length, hash);
    if (interned != nullptr) return interned;

    return allocateString(chars, length, hash);
}

ObjFunction* newFunction()
{
    ObjFunction* function = new ObjFunction(0, Chunk(), nullptr);
    VM::getInstance().addObject(function);
    return function;
}

ObjNative* newNative(uint8_t arity, NativeFn function)
{
    ObjNative* native = new ObjNative(arity, function);
    VM::getInstance().addObject(native);
    return native;
}

void printFunction(ObjFunction* function)
{
    if (function->name == nullptr)
    {
        std::cout << "<script>";
        return;
    }
    std::cout << "<fn " << function->name->chars << ">";
}

void printObject(Value value)
{
    switch (getObjType(value))
    {
    case ObjType::STRING:
        std::cout << asCString(value);
        break;
    case ObjType::NATIVE:
        printf("<native fn>");
        break;
    case ObjType::FUNCTION:
        printFunction(asFunction(value));
        break;
    }
}