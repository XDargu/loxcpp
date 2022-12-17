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

enum class InterpretResult 
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
};

using InstructonPointer = uint8_t*;

struct CallFrame
{
    ObjFunction* function = nullptr;
    InstructonPointer ip = nullptr;
    Value* slots = nullptr;
};


class VM
{
public:

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

    void addObject(Obj* obj)
    {
        objects.push_back(obj);
    }

    void freeObject(Obj* obj)
    {
        // Very inefficient way of freeing objects
        // Improvement: each object points to the next one, no need for list in VM
        objects.remove(obj);
        delete obj;
    }

    void freeAllObjects()
    {
        for (Obj* obj : objects)
        {
            delete obj;
        }

        objects.clear();
    }

private:

    InterpretResult run();
    void resetStack();
    void runtimeError(const char* format, ...);
    void defineNative(const char* name, uint8_t arity, NativeFn function);

    bool validateBinaryOperator();

    void concatenate();

    void push(Value value);
    Value pop();
    Value peek(int distance);

    bool call(ObjFunction* function, uint8_t argCount);
    bool callValue(Value callee, uint8_t argCount);

    static constexpr size_t STACK_MAX = 256;
    static constexpr size_t FRAMES_MAX = 255;

    std::array<CallFrame, FRAMES_MAX> frames;
    size_t frameCount;
    std::array<Value, STACK_MAX> stack;
    std::list<Obj*> objects;
    Value* stackTop;
    Table strings;
    Table globals;
};

#endif