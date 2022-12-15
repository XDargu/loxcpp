#ifndef loxcpp_vm_h
#define loxcpp_vm_h

#include <vector>
#include <array>
#include <string>

#include "Chunk.h"
#include "Value.h"
#include "HashTable.h"

enum class InterpretResult 
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
};

class VM
{
public:

    VM();
    VM(VM const&) = delete;
    void operator=(VM const&) = delete;

    static VM& getInstance()
    {
        static VM instance;
        return instance;
    }

    InterpretResult interpret(Chunk* chunk);
    InterpretResult interpret(const std::string& source);

    Table& stringTable() { return strings; }

private:

    InterpretResult run();
    void resetStack();
    void runtimeError(const char* format, ...);

    Chunk* chunk;
    using InstructonPointer = uint8_t*;
    InstructonPointer ip;

    bool validateBinaryOperator();

    void concatenate();

    void push(Value value)
    {
        *stackTop = value;
        stackTop++;
    }

    Value pop()
    {
        stackTop--;
        return *stackTop;
    }

    Value peek(int distance)
    {
        return stackTop[-1 - distance];
    }

    static constexpr size_t STACK_MAX = 256;
    std::array<Value, STACK_MAX> stack;
    Value* stackTop;
    Table strings;
    Table globals;
};

#endif