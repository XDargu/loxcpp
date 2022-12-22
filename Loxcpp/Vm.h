#ifndef loxcpp_vm_h
#define loxcpp_vm_h

#include <vector>
#include <list>
#include <array>
#include <string>

#include "Chunk.h"
#include "Value.h"
#include "HashTable.h"
#include "Object.h"
#include "Compiler.h"

class Compiler;

enum class InterpretResult 
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
};

using InstructonPointer = uint8_t*;

struct CallFrame
{
    ObjClosure* closure = nullptr;
    InstructonPointer ip = nullptr;
    Value* slots = nullptr;
};


class VM
{
public:

    using Table = ::TableCpp;
    using ObjList = std::list<Obj*>;

    VM();
    VM(VM const&) = delete;
    void operator=(VM const&) = delete;

    ~VM()
    {
        freeAllObjects();
    }

    static VM& getInstance()
    {
        static VM instance;
        return instance;
    }

    InterpretResult interpret(const std::string& source);

    Table& stringTable() { return strings; }

    // Memory. TODO: Separate from the VM
    void addObject(Obj* obj);
    void freeAllObjects();
    void collectGarbage();
    void markRoots();
    void traceReferences();
    void sweep();
    void markObject(Obj* object);
    void markValue(Value& value);
    void markArray(ValueArray& valArray);
    void markCompilerRoots();
    void blackenObject(Obj* object);

    void push(Value value);
    Value pop();
    Value peek(int distance);

private:

    InterpretResult run();
    void resetStack();
    void runtimeError(const char* format, ...);
    void defineNative(const char* name, uint8_t arity, NativeFn function);

    bool validateBinaryOperator();

    void concatenate();

    bool call(ObjClosure* closure, uint8_t argCount);
    bool callValue(const Value& callee, uint8_t argCount);
    ObjUpvalue* captureUpvalue(Value* local);
    void closeUpvalues(Value* last);

    static constexpr size_t STACK_MAX = 256;
    static constexpr size_t FRAMES_MAX = 255;

    std::array<CallFrame, FRAMES_MAX> frames;
    size_t frameCount;
    std::array<Value, STACK_MAX> stack;
    ObjList objects;
    ObjUpvalue* openUpvalues; // Maybe this could also be a list?
    Value* stackTop;
    Table strings;
    Table globals;
    Compiler compiler;
    bool nativesDefined = false;

    std::vector<Obj*> grayNodes;
    size_t bytesAllocated = 0;
    size_t nextGC = 256;
};

#endif