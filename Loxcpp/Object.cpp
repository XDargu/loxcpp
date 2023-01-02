#include "Object.h"

#include <iostream>

#include "Memory.h"
#include "VM.h"

template<class T, class... Args>
T* allocate(Args&&... args)
{
    T* obj = new T(std::forward<Args>(args)...);
#ifdef DEBUG_LOG_GC
    std::cout << obj << " allocate " << sizeof(*obj) << " for " << objTypeToString(obj->type) << std::endl;
#endif
#ifdef DEBUG_STRESS_GC
    VM::getInstance().collectGarbage();
#endif
    VM::getInstance().addObject(obj);
    return obj;
}


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
    // TODO: the std::string allocates memory in the heap out of our control, it can be improved!
    ObjString* string = allocate<ObjString>(chars, length);
    string->hash = hash;
    VM::getInstance().push(Value(string));
    VM::getInstance().stringTable().set(string, Value());
    VM::getInstance().pop();
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

ObjString* takeString(std::string&& chars)
{
    const uint32_t hash = hashString(chars.c_str(), chars.length());
    ObjString* interned = VM::getInstance().stringTable().findString(chars.c_str(), chars.length(), hash);
    if (interned != nullptr) return interned;

    ObjString* string = allocate<ObjString>(std::move(chars));
    string->hash = hash;

    VM::getInstance().stringTable().set(string, Value());
    return string;
}

ObjUpvalue* newUpvalue(Value* slot)
{
    return allocate<ObjUpvalue>(slot);
}

ObjInstance* newInstance(ObjClass* klass)
{
    return allocate<ObjInstance>(klass);
}

ObjBoundMethod* newBoundMethod(const Value& receiver, ObjClosure* method)
{
    return allocate<ObjBoundMethod>(receiver, method);
}

ObjClass* newClass(ObjString* name)
{
    return allocate<ObjClass>(name);
}

ObjClosure* newClosure(ObjFunction* function)
{
    return allocate<ObjClosure>(function);
}

ObjFunction* newFunction()
{
    return allocate<ObjFunction>(0, Chunk(), nullptr);
}

ObjNative* newNative(uint8_t arity, NativeFn function)
{
    return allocate<ObjNative>(arity, function);
}

ObjRange* newRange(double min, double max)
{
    return allocate<ObjRange>(min, max);
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

void printRange(ObjRange* range)
{
    std::cout << range->min << ".." << range->max;
}

void printObject(const Value& value)
{
    switch (getObjType(value))
    {
    case ObjType::STRING:
        std::cout << asCString(value);
        break;
    case ObjType::NATIVE:
        std::cout << "<native fn>";
        break;
    case ObjType::UPVALUE:
        printf("upvalue");
        break;
    case ObjType::FUNCTION:
        printFunction(asFunction(value));
        break;
    case ObjType::CLOSURE:
        printFunction(asClosure(value)->function);
        break;
    case ObjType::BOUND_METHOD:
        printFunction(asBoundMethod(value)->method->function);
        break;
    case ObjType::RANGE:
        printRange(asRange(value));
        break;
    case ObjType::CLASS:
        std::cout << asClass(value)->name->chars;
        break;
    case ObjType::INSTANCE:
        std::cout << asInstance(value)->klass->name->chars << " instance";
        break;
    }
    static_assert(static_cast<int>(ObjType::COUNT) == 9, "Missing enum value");
}

ObjString* objectAsString(const Value& value)
{
    switch (getObjType(value))
    {
        case ObjType::STRING: return asString(value);
        case ObjType::NATIVE: return takeString("<native fn>", 11);
        case ObjType::FUNCTION: return takeString("<" + asFunction(value)->name->chars + ">");
        case ObjType::CLOSURE: return takeString("<" + asClosure(value)->function->name->chars + ">");
        case ObjType::BOUND_METHOD: return takeString("<" + asBoundMethod(value)->method->function->name->chars + ">");
        case ObjType::RANGE: return takeString(std::to_string(asRange(value)->min) + ".." + std::to_string(asRange(value)->max));
        case ObjType::CLASS: return takeString("" + asClass(value)->name->chars);
        case ObjType::INSTANCE: return takeString(asInstance(value)->klass->name->chars + " instance");
    }

    static_assert(static_cast<int>(ObjType::COUNT) == 9, "Missing enum value");
    return takeString("<Unknown>", 9);
}

ObjString* concatenate(ObjString* a, ObjString* b)
{
    std::string concat = a->chars + b->chars; // TODO: This allocates memory! We are moving it to the ObjString

    ObjString* result = takeString(std::move(concat));
    return result;
}