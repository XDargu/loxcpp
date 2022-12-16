#include "Vm.h"

#include <iostream>
#include <cstdarg>
#include <cmath>

#include "Debug.h"
#include "Compiler.h"
#include "Object.h"

bool isFalsey(Value value)
{
    return isNil(value) || (isBoolean(value) && !asBoolean(value));
}

VM::VM()
    : stackTop(nullptr)
    , chunk(nullptr)
    , ip(nullptr)
{}

InterpretResult VM::interpret(Chunk* chunk)
{
    this->chunk = chunk;
    //this->ip = &chunk->code[0];
    ip = &chunk->code[0];

    stackTop = &stack[0];

    return run();
}

InterpretResult VM::interpret(const std::string& source)
{
    Chunk chunk;
    Compiler compiler(source);

    if (!compiler.compile(&chunk))
    {
        return InterpretResult::INTERPRET_COMPILE_ERROR;
    }

    return interpret(&chunk);
}

InterpretResult VM::run()
{
    auto readByte = [&]() -> uint8_t { return *ip++; };
    auto readShort = [&]() -> uint16_t
    {
        const uint8_t* constantStart = ip;
        ip += 2;
        // Interpret the constant as the next 2 elements in the vector
        return *reinterpret_cast<const uint16_t*>(constantStart);
    };
    auto readDWord = [&]() -> uint32_t {
        const uint8_t* constantStart = ip;
        ip += 4;
        // Interpret the constant as the next 4 elements in the vector
        return *reinterpret_cast<const uint32_t*>(constantStart);
    };
    auto readConstant = [&]() -> Value { return chunk->constants.values[readByte()]; };
    auto readLongConstant = [&]() -> Value { return chunk->constants.values[readDWord()]; };
    auto readString = [&]() -> ObjString* { return asString(readConstant()); };
    auto readStringLong = [&]() -> ObjString* { return asString(readLongConstant()); };

    for (;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        std::cout << "          ";
        for (Value* slot = &stack[0]; slot < stackTop; slot++)
        {
            std::cout <<  "[ ";
            printValue(*slot);
            std::cout << " ]";
        }
        std::cout << std::endl;
        disassembleInstruction(*chunk, static_cast<int>(ip - &chunk->code[0]));
#endif

        const OpCode instruction = static_cast<OpCode>(readByte());
        switch (instruction )
        {
            case OpCode::OP_CONSTANT:
            {
                const Value constant = readConstant();
                push(constant);
                break;
            }
            case OpCode::OP_CONSTANT_LONG:
            {
                const Value constant = readLongConstant();
                push(constant);
                break;
            }
            case OpCode::OP_NIL: push(Value()); break;
            case OpCode::OP_TRUE: push(Value(true)); break;
            case OpCode::OP_FALSE: push(Value(false)); break;
            case OpCode::OP_POP: pop(); break;
            case OpCode::OP_GET_LOCAL:
            {
                const uint8_t slot = readByte();
                push(stack[slot]);
                break;
            }
            case OpCode::OP_SET_LOCAL:
            {
                const uint8_t slot = readByte();
                stack[slot] = peek(0);
                break;
            }
            case OpCode::OP_GET_LOCAL_LONG:
            {
                const uint8_t slot = readDWord();
                push(stack[slot]);
                break;
            }
            case OpCode::OP_SET_LOCAL_LONG:
            {
                const uint8_t slot = readDWord();
                stack[slot] = peek(0);
                break;
            }
            case OpCode::OP_GET_GLOBAL:
            {
                ObjString* name = readString();
                Value value;
                if (!globals.get(name, &value))
                {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OpCode::OP_DEFINE_GLOBAL:
            {
                ObjString* name = readString();
                globals.set(name, peek(0));
                pop();
                break;
            }
            case OpCode::OP_SET_GLOBAL:
            {
                ObjString* name = readString();
                if (globals.set(name, peek(0)))
                {
                    globals.remove(name);
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OpCode::OP_GET_GLOBAL_LONG:
            {
                ObjString* name = readStringLong();
                Value value;
                if (!globals.get(name, &value))
                {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OpCode::OP_DEFINE_GLOBAL_LONG:
            {
                ObjString* name = readStringLong();
                globals.set(name, peek(0));
                pop();
                break;
            }
            case OpCode::OP_SET_GLOBAL_LONG:
            {
                ObjString* name = readStringLong();
                if (globals.set(name, peek(0)))
                {
                    globals.remove(name);
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OpCode::OP_EQUAL:
            {
                const Value b = pop();
                const Value a = pop();
                push(Value(a == b));
                break;
            }
            case OpCode::OP_GREATER:
            {
                if (!validateBinaryOperator()) { return InterpretResult::INTERPRET_RUNTIME_ERROR; }
                const double b = asNumber(pop());
                const double a = asNumber(pop());
                push(Value(a > b));
                break;
            }
            case OpCode::OP_LESS:
            {
                if (!validateBinaryOperator()) { return InterpretResult::INTERPRET_RUNTIME_ERROR; }
                const double b = asNumber(pop());
                const double a = asNumber(pop());
                push(Value(a < b));
                break;
            }
            case OpCode::OP_NEGATE:
            {
                if (!isNumber(peek(0)))
                {
                    runtimeError("Operand must be a number");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                push(Value(-asNumber(pop()))); break;
            }
            case OpCode::OP_ADD:
            {
                if (isString(peek(0)) && isString(peek(1)))
                {
                    concatenate();
                }
                else if (isNumber(peek(0)) && isNumber(peek(1)))
                {
                    const double b = asNumber(pop());
                    const double a = asNumber(pop());
                    push(Value(a + b));
                }
                else
                {
                    runtimeError("Operands must be two numbers or two strings.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OpCode::OP_SUBTRACT:
            {
                if (!validateBinaryOperator()) { return InterpretResult::INTERPRET_RUNTIME_ERROR; }
                const double b = asNumber(pop());
                const double a = asNumber(pop());
                push(Value(a - b));
                break;
            }
            case OpCode::OP_MULTIPLY:
            {
                if (!validateBinaryOperator()) { return InterpretResult::INTERPRET_RUNTIME_ERROR; }
                const double b = asNumber(pop());
                const double a = asNumber(pop());
                push(Value(a * b));
                break;
            }
            case OpCode::OP_DIVIDE:
            {
                if (!validateBinaryOperator()) { return InterpretResult::INTERPRET_RUNTIME_ERROR; }
                const double b = asNumber(pop());
                const double a = asNumber(pop());
                push(Value(a / b));
                break;
            }
            case OpCode::OP_MODULO:
            {
                if (!validateBinaryOperator()) { return InterpretResult::INTERPRET_RUNTIME_ERROR; }
                const double b = asNumber(pop());
                const double a = asNumber(pop());
                push(Value(std::fmod(a, b)));
                break;
            }
            case OpCode::OP_NOT:
            {
                push(isFalsey(pop()));
                break;
            }
            case OpCode::OP_PRINT:
            {
                printValue(pop());
                printf("\n");
                break;
            }
            case OpCode::OP_JUMP:
            {
                const uint16_t offset = readShort();
                ip += offset;
                break;
            }
            case OpCode::OP_JUMP_IF_FALSE:
            {
                const uint16_t offset = readShort();
                if (isFalsey(peek(0))) ip += offset;
                break;
            }
            case OpCode::OP_LOOP:
            {
                const uint16_t offset = readShort();
                ip -= offset;
                break;
            }
            case OpCode::OP_RETURN:
            {
                return InterpretResult::INTERPRET_OK;
            }
        }
        static_assert(static_cast<int>(OpCode::COUNT) == 31, "Missing operations in the VM");
    }
}

void VM::resetStack()
{
    stackTop = 0;
}

inline void VM::runtimeError(const char* format, ...) 
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    const size_t instruction = ip - &chunk->code[0] - 1;
    const int line = chunk->lines[instruction];
    std::cerr << "[line " << line << "] in script" << std::endl;
    resetStack();
}

bool VM::validateBinaryOperator()
{
    if (!isNumber(peek(0)) || !isNumber(peek(1)))
    {
        runtimeError("Operands must be numbers.");
        return false;
    }

    return true;
}

void VM::concatenate()
{
    ObjString* b = asString(pop());
    ObjString* a = asString(pop());

    std::string concat = a->chars + b->chars; // TODO: This allocates memory!

    ObjString* result = takeString(concat.c_str(), concat.length());
    push(Value(result));
}
