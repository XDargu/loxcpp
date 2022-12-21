#include "Vm.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <cmath>
#include <time.h>

#include "Debug.h"
#include "Compiler.h"
#include "Object.h"

bool isFalsey(Value value)
{
    return isNil(value) || (isBoolean(value) && !asBoolean(value));
}

VM::VM()
    : stackTop(nullptr)
    , frames()
    , frameCount(0)
    , openUpvalues(nullptr)
{
    resetStack();
}

InterpretResult VM::interpret(const std::string& source)
{
    Compiler compiler(source);

    ObjFunction* function = compiler.compile();
    if (function == nullptr) return InterpretResult::INTERPRET_COMPILE_ERROR;

    resetStack();

    defineNative("clock", 0, [](int argCount, Value* args)
    {
        return Value((double)clock() / CLOCKS_PER_SEC);
    });
    defineNative("rangeVal", 2, [](int argCount, Value* args)
    {
        if (isRange(args[0]) && isNumber(args[1]))
        {
            ObjRange* range = asRange(args[0]);
            const double idx = asNumber(args[1]);

            if (range->isInBounds(idx))
            {
                return Value(range->getValue(idx));
            }
        }

        return Value();
    });
    defineNative("inRangeBounds", 2, [](int argCount, Value* args)
    {
        if (isRange(args[0]) && isNumber(args[1]))
        {
            ObjRange* range = asRange(args[0]);
            const double idx = asNumber(args[1]);

            return Value(range->isInBounds(idx));
        }

        return Value();
    });
    defineNative("readInput", 0, [](int argCount, Value* args)
    {
        std::string line;
        std::getline(std::cin, line);

        return Value(takeString(line.c_str(), line.length()));
    });
    defineNative("readFile", 1, [](int argCount, Value* args)
    {
        if (isString(args[0]))
        {
            ObjString* fileName = asString(args[0]);
            std::ifstream fileStream(fileName->chars);
            std::stringstream buffer;
            buffer << fileStream.rdbuf();
            fileStream.close();
            return Value(takeString(buffer.str().c_str(), buffer.str().length()));
        }

        return Value(takeString("", 0));
    });
    defineNative("writeFile", 2, [](int argCount, Value* args)
    {
        if (isString(args[0]) && isString(args[1]))
        {
            ObjString* fileName = asString(args[0]);
            ObjString* content = asString(args[1]);

            std::ofstream fileStream(fileName->chars.c_str());
            if (fileStream.is_open())
            {
                fileStream.write(content->chars.c_str(), content->chars.length());
            }
            fileStream.close();
        }

        return Value();
    });

    push(Value(function));
    ObjClosure* closure = newClosure(function);
    pop();
    push(Value(closure));
    call(closure, 0);

    return run();
}

InterpretResult VM::run()
{
    CallFrame* frame = &frames[frameCount - 1];

    auto readByte = [&]() -> uint8_t { return *frame->ip++; };
    auto readShort = [&]() -> uint16_t
    {
        const uint8_t* constantStart = frame->ip;
        frame->ip += 2;
        // Interpret the constant as the next 2 elements in the vector
        return *reinterpret_cast<const uint16_t*>(constantStart);
    };
    auto readDWord = [&]() -> uint32_t {
        const uint8_t* constantStart = frame->ip;
        frame->ip += 4;
        // Interpret the constant as the next 4 elements in the vector
        return *reinterpret_cast<const uint32_t*>(constantStart);
    };
    auto readConstant = [&]() -> Value { return frame->closure->function->chunk.constants.values[readByte()]; };
    auto readLongConstant = [&]() -> Value { return frame->closure->function->chunk.constants.values[readDWord()]; };
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
        disassembleInstruction(frame->closure->function->chunk, 
            static_cast<size_t>(frame->ip - &frame->closure->function->chunk.code[0]));
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
                push(frame->slots[slot]);
                break;
            }
            case OpCode::OP_SET_LOCAL:
            {
                const uint8_t slot = readByte();
                frame->slots[slot] = peek(0);
                break;
            }
            case OpCode::OP_GET_LOCAL_LONG:
            {
                const uint8_t slot = readDWord();
                push(frame->slots[slot]);
                break;
            }
            case OpCode::OP_SET_LOCAL_LONG:
            {
                const uint8_t slot = readDWord();
                frame->slots[slot] = peek(0);
                break;
            }
            case OpCode::OP_GET_GLOBAL:
            {
                ObjString* name = readString();
                Value value;
                if (!globals.get(name, &value))
                {
                    runtimeError("Undefined variable '%s'.", name->chars.c_str());
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OpCode::OP_GET_UPVALUE:
            {
                const uint8_t slot = readByte();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case OpCode::OP_SET_UPVALUE:
            {
                const uint8_t slot = readByte();
                *frame->closure->upvalues[slot]->location = peek(0);
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
                    runtimeError("Undefined variable '%s'.", name->chars.c_str());
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
                    runtimeError("Undefined variable '%s'.", name->chars.c_str());
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
                    runtimeError("Undefined variable '%s'.", name->chars.c_str());
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
            case OpCode::OP_MATCH:
            {
                const Value pattern = pop();
                const Value value = pop();
                if (isRange(pattern) && isNumber(value))
                {
                    push(Value(asRange(pattern)->contains(asNumber(value))));
                }
                else
                {
                    push(Value(value == pattern));
                }
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
                else if (isString(peek(0)))
                {
                    ObjString* a = asString(pop());
                    ObjString* val = valueAsString(pop());  // TODO: This allocates memory!
                    
                    ObjString* result = ::concatenate(val, a);
                    push(Value(result));

                }
                else if (isString(peek(1)))
                {
                    ObjString* val = valueAsString(pop()); // TODO: This allocates memory!
                    ObjString* b = asString(pop());

                    ObjString* result = ::concatenate(b, val);
                    push(Value(result));
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
            case OpCode::OP_INCREMENT:
            {
                if (!isNumber(peek(0)))
                {
                    runtimeError("Can only increment numbers");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                const double a = asNumber(pop());
                push(Value(a + 1));
                break;
            }
            case OpCode::OP_RANGE:
            {
                if (!validateBinaryOperator()) { return InterpretResult::INTERPRET_RUNTIME_ERROR; }
                const double max = asNumber(pop());
                const double min = asNumber(pop());
                push(Value(newRange(min, max)));
                break;
            }
            case OpCode::OP_RANGE_VALUE:
            {
                if (isRange(peek(0)) && isNumber(peek(1)))
                {
                    ObjRange* range = asRange(pop());
                    const double idx = asNumber(pop());
                    if (range->isInBounds(idx))
                    {
                        push(Value(range->getValue(idx)));
                    }
                    else
                    {
                        push(Value());
                    }
                }
                else if (isString(peek(0)) && isNumber(peek(1)))
                {
                    ObjString* string = asString(pop());
                    const double idx = asNumber(pop());
                    if (idx >= 0 && idx < string->length)
                    {
                        ObjString* character = takeString(&string->chars[idx], 1);
                        push(Value(character));
                    }
                    else
                    {
                        push(Value());
                    }
                }
                else
                {
                    runtimeError("Invalid range type.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OpCode::OP_RANGE_IN_BOUNDS:
            {
                if (isRange(peek(0)) && isNumber(peek(1)))
                {
                    ObjRange* range = asRange(pop());
                    const double idx = asNumber(pop());
                    if (range->isInBounds(idx))
                    {
                        push(Value(true));
                    }
                    else
                    {
                        push(Value(false));
                    }
                }
                else if (isString(peek(0)) && isNumber(peek(1)))
                {
                    ObjString* string = asString(pop());
                    const double idx = asNumber(pop());
                    if (idx >= 0 && idx < string->length)
                    {
                        push(Value(true));
                    }
                    else
                    {
                        push(Value(false));
                    }
                }
                else
                {
                    runtimeError("Invalid range type.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
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
                frame->ip += offset;
                break;
            }
            case OpCode::OP_JUMP_IF_FALSE:
            {
                const uint16_t offset = readShort();
                if(isFalsey(peek(0))) frame->ip += offset;
                break;
            }
            case OpCode::OP_LOOP:
            {
                const uint16_t offset = readShort();
                frame->ip -= offset;
                break;
            }
            case OpCode::OP_CALL:
            {
                const uint8_t argCount = readByte();
                if (!callValue(peek(argCount), argCount))
                {
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                frame = &frames[frameCount - 1];
                break;
            }
            case OpCode::OP_CLOSURE:
            {
                ObjFunction* function = asFunction(readConstant());
                ObjClosure* closure = newClosure(function);
                push(Value(closure));
                for (size_t i = 0; i < closure->upvalues.size(); i++)
                {
                    const uint8_t isLocal = readByte();
                    const uint8_t index = readByte();
                    if (isLocal)
                    {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    }
                    else
                    {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OpCode::OP_CLOSURE_LONG:
            {
                ObjFunction* function = asFunction(readLongConstant());
                ObjClosure* closure = newClosure(function);
                push(Value(closure));
                for (size_t i = 0; i < closure->upvalues.size(); i++)
                {
                    const uint8_t isLocal = readByte();
                    const uint8_t index = readByte();
                    if (isLocal)
                    {
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    }
                    else
                    {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OpCode::OP_CLOSE_UPVALUE:
                closeUpvalues(stackTop - 1);
                pop();
                break;
            case OpCode::OP_RETURN:
            {
                Value result = pop();
                closeUpvalues(frame->slots);
                frameCount--;
                if (frameCount == 0)
                {
                    pop();
                    return InterpretResult::INTERPRET_OK;
                }

                stackTop = frame->slots;
                push(result);
                frame = &frames[frameCount - 1];
                break;
            }
        }
        static_assert(static_cast<int>(OpCode::COUNT) == 42, "Missing operations in the VM");
    }
}

void VM::resetStack()
{
    stackTop = &stack[0];
    frameCount = 0;
}

inline void VM::runtimeError(const char* format, ...) 
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = frameCount - 1; i >= 0; i--)
    {
        const CallFrame& frame = frames[i];
        const ObjFunction* function = frame.closure->function;
        const size_t instruction = frame.ip - &function->chunk.code[0] - 1;

        std::cerr << "[line " << function->chunk.lines[instruction] << "] in ";
        if (function->name == nullptr)
        {
            std::cerr << "script" << std::endl;
        }
        else
        {
            std::cerr << function->name->chars << "()" << std::endl;
        }
    }

    resetStack();
}

inline void VM::defineNative(const char* name, uint8_t arity, NativeFn function)
{
    push(Value(copyString(name, (int)strlen(name))));
    push(Value(newNative(arity, function)));
    globals.set(asString(stack[0]), stack[1]);
    pop();
    pop();
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

    push(Value(::concatenate(a, b)));
}

void VM::push(Value value)
{
    *stackTop = value;
    stackTop++;
}

Value VM::pop()
{
    stackTop--;
    return *stackTop;
}

Value VM::peek(int distance)
{
    return stackTop[-1 - distance];
}

bool VM::call(ObjClosure* closure, uint8_t argCount)
{
    if (argCount != closure->function->arity)
    {
        runtimeError("Expected %d arguments but got %d.", closure->function->arity, argCount);
        return false;
    }
    if (frameCount == FRAMES_MAX)
    {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame* frame = &frames[frameCount++];
    frame->closure = closure;
    frame->ip = &closure->function->chunk.code[0];
    frame->slots = stackTop - argCount - 1;
    return true;
}

bool VM::callValue(const Value& callee, uint8_t argCount)
{
    if (isObject(callee))
    {
        switch (getObjType(callee))
        {
        case ObjType::CLOSURE:
            return call(asClosure(callee), argCount);
        case ObjType::NATIVE:
        {
            ObjNative* native = asNative(callee);
            if (argCount != native->arity)
            {
                runtimeError("Expected %d arguments but got %d.", native->arity, argCount);
                return false;
            }

            Value result = native->function(argCount, stackTop - argCount);
            stackTop -= argCount + 1;
            push(result);
            return true;
        }
        default:
            break; // Non-callable object type.
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

ObjUpvalue* VM::captureUpvalue(Value* local)
{
    ObjUpvalue* prevUpvalue = nullptr;
    ObjUpvalue* upvalue = openUpvalues;
    while (upvalue != nullptr && upvalue->location > local)
    {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != nullptr && upvalue->location == local)
    {
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = newUpvalue(local);

    createdUpvalue->next = upvalue;

    if (prevUpvalue == nullptr)
    {
        openUpvalues = createdUpvalue;
    }
    else
    {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

void VM::closeUpvalues(Value* last)
{
    while (openUpvalues != nullptr && openUpvalues->location >= last)
    {
        ObjUpvalue* upvalue = openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        openUpvalues = upvalue->next;
    }
}
