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

constexpr int GC_HEAP_GROW_FACTOR = 2;

bool isFalsey(Value value)
{
    return isNil(value) || (isBoolean(value) && !asBoolean(value));
}

bool isCallable(Value& value)
{
    return isClosure(value);
}

bool isIterable(Value& value)
{
    return isList(value) || isString(value) || isRange(value);
}

VM::VM()
    : stackTop(nullptr)
    , frames()
    , frameCount(0)
    , openUpvalues(nullptr)
    , compiler()
{
    resetStack();
}

InterpretResult VM::interpret(const std::string& source)
{
    if (!nativesDefined)
    {
        nativesDefined = true;
        defineNative("clock", 0, [](int argCount, Value* args, VM* vm)
        {
            return Value((double)clock() / CLOCKS_PER_SEC);
        });
        defineNative("inBounds", 2, [](int argCount, Value* args, VM* vm)
        {
            if (!isNumber(args[1]))
                return Value();

            const int idx = static_cast<int>(asNumber(args[1]));

            if (isRange(args[0]))
            {
                ObjRange* range = asRange(args[0]);
                return Value(range->isInBounds(idx));
            }
            else if (isList(args[0]))
            {
                ObjList* list = asList(args[0]);
                return Value(list->isInBounds(idx));
            }
            else if (isString(args[0]))
            {
                ObjString* str = asString(args[0]);
                return Value(idx >= 0 && idx < str->chars.length());
            }

            return Value();
        });
        defineNative("readInput", 0, [](int argCount, Value* args, VM* vm)
        {
            std::string line;
            std::getline(std::cin, line);

            return Value(takeString(line.c_str(), line.length()));
        });
        defineNative("readFile", 1, [](int argCount, Value* args, VM* vm)
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
        defineNative("writeFile", 2, [](int argCount, Value* args, VM* vm)
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
        defineNative("sizeof", 1, [](int argCount, Value* args, VM* vm)
        {
            return Value((double)sizeOf(args[0]));
        });
        defineNative("push", 2, [](int argCount, Value* args, VM* vm)
        {
            if (!isList(args[0]))
            {
                return Value();
            }
            ObjList* list = asList(args[0]);
            Value item = args[1];
            list->append(item);
            return Value(static_cast<double>(list->items.size()));
        });
        defineNative("pop", 1, [](int argCount, Value* args, VM* vm)
        {
            if (!isList(args[0]))
            {
                return Value();
            }
            ObjList* list = asList(args[0]);

            if (list->items.size() == 0)
            {
                return Value();
            }

            Value value = *list->items.end();
            list->items.pop_back();
            return value;
        });
        defineNative("erase", 2, [](int argCount, Value* args, VM* vm)
        {
            if (!isList(args[0]) || !isNumber(args[1]))
            {
                return Value();
            }

            ObjList* list = asList(args[0]);
            const int index = static_cast<int>(asNumber(args[1]));

            if (index < 0 || index >= list->items.size())
            {
                return Value();
            }

            list->deleteValue(index);
            return Value();
        });
        defineNative("map", 2, [](int argCount, Value* args, VM* vm)
        {
            if (!isIterable(args[0]) || !isCallable(args[1]))
            {
                return Value();
            }
            ObjList* mappedList = newList();

            if (isRange(args[0]))
            {
                ObjRange* range = asRange(args[0]);

                for (int idx=0; range->isInBounds(idx); ++idx)
                {
                    vm->push(args[1]);
                    vm->push(Value(range->getValue(idx)));
                    vm->callValue(args[1], 1);
                    vm->run(vm->frameCount - 1);
                    mappedList->append(vm->pop());
                }
            }
            else if (isList(args[0]))
            {
                ObjList* list = asList(args[0]);
                mappedList->items.reserve(list->items.size());
                for (int idx = 0; list->isInBounds(idx); ++idx)
                {
                    vm->push(args[1]);
                    vm->push(list->getValue(idx));
                    vm->callValue(args[1], 1);
                    vm->run(vm->frameCount - 1);
                    mappedList->append(vm->pop());
                }
            }
            else if (isString(args[0]))
            {
                ObjString* str = asString(args[0]);
                mappedList->items.reserve(str->chars.length());

                for (char c : str->chars)
                {
                    vm->push(args[1]);
                    vm->push(Value(takeString(&c, 1)));
                    vm->callValue(args[1], 1);
                    vm->run(vm->frameCount - 1);
                    mappedList->append(vm->pop());
                }
            }

            return Value(mappedList);
        });
        defineNative("filter", 2, [](int argCount, Value* args, VM* vm)
        {
            if (!isIterable(args[0]) || !isCallable(args[1]))
            {
                return Value();
            }
            ObjList* mappedList = newList();

            if (isRange(args[0]))
            {
                ObjRange* range = asRange(args[0]);

                for (int idx = 0; range->isInBounds(idx); ++idx)
                {
                    vm->push(args[1]);
                    vm->push(Value(range->getValue(idx)));
                    vm->callValue(args[1], 1);
                    vm->run(vm->frameCount - 1);
                    if (!isFalsey(vm->pop()))
                        mappedList->append(Value(range->getValue(idx)));
                }
            }
            else if (isList(args[0]))
            {
                ObjList* list = asList(args[0]);
                for (int idx = 0; list->isInBounds(idx); ++idx)
                {
                    vm->push(args[1]);
                    vm->push(list->getValue(idx));
                    vm->callValue(args[1], 1);
                    vm->run(vm->frameCount - 1);
                    if (!isFalsey(vm->pop()))
                        mappedList->append(list->getValue(idx));
                }
            }
            else if (isString(args[0]))
            {
                ObjString* str = asString(args[0]);
                for (char c : str->chars)
                {
                    vm->push(args[1]);
                    vm->push(Value(takeString(&c, 1)));
                    vm->callValue(args[1], 1);
                    vm->run(vm->frameCount - 1);
                    if (!isFalsey(vm->pop()))
                        mappedList->append(Value(takeString(&c, 1)));
                }
            }

            return Value(mappedList);
        });
        defineNative("reduce", 3, [](int argCount, Value* args, VM* vm)
        {
            if (!isIterable(args[0]) || !isCallable(args[1]))
            {
                return Value();
            }

            Value accum = args[2];

            if (isRange(args[0]))
            {
                ObjRange* range = asRange(args[0]);

                for (int idx = 0; range->isInBounds(idx); ++idx)
                {
                    vm->push(args[1]);
                    vm->push(Value(range->getValue(idx)));
                    vm->push(accum);
                    vm->callValue(args[1], 2);
                    vm->run(vm->frameCount - 1);
                    accum = vm->pop();
                }
            }
            else if (isList(args[0]))
            {
                ObjList* list = asList(args[0]);
                for (int idx = 0; list->isInBounds(idx); ++idx)
                {
                    vm->push(args[1]);
                    vm->push(list->getValue(idx));
                    vm->push(accum);
                    vm->callValue(args[1], 2);
                    vm->run(vm->frameCount - 1);
                    accum = vm->pop();
                }
            }
            else if (isString(args[0]))
            {
                ObjString* str = asString(args[0]);
                for (char c : str->chars)
                {
                    vm->push(args[1]);
                    vm->push(Value(takeString(&c, 1)));
                    vm->push(accum);
                    vm->callValue(args[1], 2);
                    vm->run(vm->frameCount - 1);
                    accum = vm->pop();
                }
            }

            return accum;
        });

        struct NativeMethodDef
        {
            const char* name;
            int arity;
            NativeFn function;
        };

        auto defineNativeClass = [this](const char* name, std::vector<NativeMethodDef>&& methods)
        {
            // Native Class Test
            push(Value(copyString(name, (int)strlen(name))));
            push(Value(newClass(asString(stack[0]))));

            for (const NativeMethodDef& method : methods)
            {
                push(Value(copyString(method.name, (int)strlen(method.name))));
                push(Value(newNative(method.arity, method.function, true)));

                if (strcmp(method.name, "init") == 0)
                    asClass(stack[1])->initializer = peek(0);
                else
                    asClass(stack[1])->methods.set(asString(peek(1)), peek(0));

                pop();
                pop();
            }

            globals.set(asString(peek(1)), peek(0));
            pop();
            pop();
        };

        // TODO: Add support for static functions and properties
        // TODO: Add support for defining properties in a class
        // TODO: Maybe remove the ability to add/remove fields randomly to classes?
        defineNativeClass("Math",
        {
            { "init", 0, [](int argCount, Value* args, VM* vm)
                {
                    ObjInstance* this_ = asInstance(args[0]);
                    this_->fields.set(copyString("PI", 2), Value(3.14159265358979323846));
                    return Value(this_);
                }
            },
            { "abs", 1, [](int argCount, Value* args, VM* vm)
                {
                    ObjInstance* this_ = asInstance(args[0]);
                    if (isNumber(args[1]))
                    {
                        return Value(fabs(asNumber(args[1])));
                    }
                    return Value();
                }
            },
            { "min", 2, [](int argCount, Value* args, VM* vm)
                {
                    ObjInstance* this_ = asInstance(args[0]);
                    if (isNumber(args[1]) && isNumber(args[2]))
                    {
                        return Value(std::min(asNumber(args[1]), asNumber(args[2])));
                    }
                    return Value();
                }
            }
        });
    
}

    ObjFunction* function = compiler.compile(source);
    if (function == nullptr) return InterpretResult::INTERPRET_COMPILE_ERROR;

    push(Value(function));
    ObjClosure* closure = newClosure(function);
    pop();
    push(Value(closure));
    call(closure, 0);

    return run(0);
}

void VM::addObject(Obj* obj)
{
    bytesAllocated += sizeof(*obj);
    if (bytesAllocated > nextGC)
    {
        collectGarbage();
    }

    objects.push_back(obj);
}

void VM::freeAllObjects()
{
#ifdef DEBUG_LOG_GC
    const size_t before = bytesAllocated;
#endif

    for (Obj* obj : objects)
    {
        bytesAllocated -= sizeof(*obj);
        delete obj;
    }

    objects.clear();

#ifdef DEBUG_LOG_GC
    std::cout << "   collected " << (before - bytesAllocated) <<
        " bytes (from " << before << " to " << bytesAllocated << ") next at " << nextGC << std::endl;
#endif
}

void VM::collectGarbage()
{
#ifdef DEBUG_LOG_GC
    std::cout << "-- gc begin" << std::endl;
    const size_t before = bytesAllocated;
#endif

    markRoots();
    traceReferences();
    strings.removeWhite();
    sweep();

    nextGC = bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    std::cout << "-- gc end" << std::endl;
    std::cout << "   collected " << (before - bytesAllocated) <<
        " bytes (from " << before << " to " << bytesAllocated << ") next at " << nextGC << std::endl;
#endif
}

void VM::markRoots()
{
    for (Value* slot = &stack[0]; slot < stackTop; slot++)
    {
        markValue(*slot);
    }

    for (int i = 0; i < frameCount; i++)
    {
        markObject(frames[i].closure);
    }

    for (ObjUpvalue* upvalue = openUpvalues;
        upvalue != nullptr;
        upvalue = upvalue->next)
    {
        markObject(upvalue);
    }

    globals.mark();
    markCompilerRoots();
}

void VM::traceReferences()
{
    while (grayNodes.size() > 0)
    {
        Obj* object = grayNodes.back();
        grayNodes.pop_back();
        blackenObject(object);
    }
}

void VM::sweep()
{
    GCObjList::iterator it = objects.begin();
    while (it != objects.end())
    {
        Obj* object = *it;
        if (object->isMarked)
        {
            object->isMarked = false;
            ++it;
        }
        else
        {
            bytesAllocated -= sizeof(*object);
            delete object;
            it = objects.erase(it);
        }
    }
}

void VM::markObject(Obj* object)
{
    if (object == nullptr) return;
    if (object->isMarked) return;

#ifdef DEBUG_LOG_GC
    std::cout << object << " mark ";
    printValue(Value(object));
    std::cout << std::endl;
#endif

    object->isMarked = true;

    grayNodes.push_back(object);
}

void VM::markValue(Value& value)
{
    if (isObject(value)) markObject(asObject(value));
}

void VM::markArray(ValueArray& valArray)
{
    for (int i = 0; i < valArray.values.size(); i++)
    {
        markValue(valArray.values[i]);
    }
}

void VM::markCompilerRoots()
{
    for (const CompilerScope* compilerScope = compiler.current;
        compilerScope != nullptr;
        compilerScope = compilerScope->enclosing)
    {
        markObject(compilerScope->function);
    }
}

void VM::blackenObject(Obj* object)
{
#ifdef DEBUG_LOG_GC
    std::cout << object << " blacken ";
    printValue(Value(object));
    std::cout << std::endl;
#endif

    switch (object->type) {
    case ObjType::NATIVE:
    case ObjType::STRING:
    case ObjType::RANGE:
        break;
    case ObjType::LIST:
    {
        ObjList* list = static_cast<ObjList*>(object);
        for (int i = 0; i < list->items.size(); ++i)
        {
            markValue(list->items[i]);
        }
        break;
    }
    case ObjType::UPVALUE:
        markValue((static_cast<ObjUpvalue*>(object)->closed));
        break;
    case ObjType::FUNCTION:
    {
        ObjFunction* function = static_cast<ObjFunction*>(object);
        markObject(function->name);
        markArray(function->chunk.constants);
        break;
    }
    case ObjType::CLOSURE:
    {
        ObjClosure* closure = static_cast<ObjClosure*>(object);

        markObject(closure->function);
        for (int i = 0; i < closure->upvalues.size(); i++)
        {
            markObject(closure->upvalues[i]);
        }
        break;
    }
    case ObjType::BOUND_METHOD:
    {
        ObjBoundMethod* bound = static_cast<ObjBoundMethod*>(object);
        markValue(bound->receiver);
        markValue(bound->method);
        break;
    }
    case ObjType::CLASS:
    {
        ObjClass* klass = static_cast<ObjClass*>(object);
        markObject(klass->name);
        markValue(klass->initializer);
        klass->methods.mark();
        break;
    }
    case ObjType::INSTANCE:
    {
        ObjInstance* instance = static_cast<ObjInstance*>(object);
        markObject(instance->klass);
        instance->fields.mark();
        break;
    }
    }

    static_assert(static_cast<int>(ObjType::COUNT) == 10, "Missing enum value");
}

InterpretResult VM::run(int depth)
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
            case OpCode::OP_GET_PROPERTY:
            {
                if (!isInstance(peek(0)))
                {
                    runtimeError("Only instances have properties.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = asInstance(peek(0));
                ObjString* name = readString();

                pop(); // Instance.

                Value value;
                if (instance->fields.get(name, &value))
                {
                    push(value);
                    break;
                }

                if (bindMethod(instance, name))
                {
                    break;
                }

                push(Value()); // Nil
                break;
            }
            case OpCode::OP_GET_PROPERTY_LONG:
            {
                if (!isInstance(peek(0)))
                {
                    runtimeError("Only instances have properties.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = asInstance(peek(0));
                ObjString* name = readStringLong();

                pop(); // Instance.

                Value value;
                if (instance->fields.get(name, &value))
                {
                    push(value);
                    break;
                }

                if (bindMethod(instance, name))
                {
                    break;
                }

                push(Value()); // Nil
                break;
            }
            case OpCode::OP_SET_PROPERTY:
            {
                if (!isInstance(peek(1)))
                {
                    runtimeError("Only instances have fields.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = asInstance(peek(1));
                instance->fields.set(readString(), peek(0));
                const Value value = pop();
                pop();
                push(value);
                break;
            }
            case OpCode::OP_SET_PROPERTY_LONG:
            {
                if (!isInstance(peek(1)))
                {
                    runtimeError("Only instances have fields.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance* instance = asInstance(peek(1));
                instance->fields.set(readStringLong(), peek(0));
                const Value value = pop();
                pop();
                push(value);
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
                    ObjString* a = asString(peek(0));
                    ObjString* val = valueAsString(peek(1));  // TODO: This allocates memory!
                    
                    ObjString* result = ::concatenate(val, a);

                    pop();
                    pop();
                    push(Value(result));

                }
                else if (isString(peek(1)))
                {
                    ObjString* val = valueAsString(peek(0)); // TODO: This allocates memory!
                    ObjString* b = asString(peek(1));

                    ObjString* result = ::concatenate(b, val);

                    pop();
                    pop();
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
            case OpCode::OP_BUILD_RANGE:
            {
                if (!validateBinaryOperator()) { return InterpretResult::INTERPRET_RUNTIME_ERROR; }
                const double max = asNumber(pop());
                const double min = asNumber(pop());
                push(Value(newRange(min, max)));
                break;
            }
            case OpCode::OP_BUILD_LIST:
            {
                // Stack before: [item1, item2, ..., itemN] and after: [list]
                ObjList* list = newList();
                uint8_t itemCount = readByte();

                // Add items to list
                push(Value(list)); // So list isn't sweeped by GC in appendToList
                for (int i = itemCount; i > 0; --i)
                {
                    list->append(peek(i));
                }
                pop();

                // Pop items from stack
                while (itemCount-- > 0)
                {
                    pop();
                }

                push(Value(list));
                break;
            }
            case OpCode::OP_INDEX_SUBSCR:
            {
                // stack is: [...,source,index] and after: [item]
                Value index = pop();
                Value source = pop();

                if (isInstance(source))
                {
                    if (!isString(index))
                    {
                        runtimeError("Fields can only be accessed by strings.");
                        return InterpretResult::INTERPRET_RUNTIME_ERROR;
                    }

                    ObjInstance* instance = asInstance(source);
                    ObjString* name = asString(index);

                    Value value;
                    if (instance->fields.get(name, &value))
                    {
                        push(value);
                        break;
                    }

                    push(index); // bind method will pop this
                    if (bindMethod(instance, name))
                    {
                        break;
                    }

                    push(Value()); // Nil
                    break;
                }
                if (!isNumber(index))
                {
                    runtimeError("Index is not a number.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }

                const int idx = static_cast<int>(asNumber(index));

                if (isList(source))
                {
                    ObjList* list = asList(source);
                    if (list->isInBounds(idx))
                    {
                        push(Value(list->getValue(idx)));
                    }
                    else
                    {
                        push(Value());
                    }
                }
                else if (isRange(source))
                {
                    ObjRange* range = asRange(source);
                    if (range->isInBounds(idx))
                    {
                        push(Value(range->getValue(idx)));
                    }
                    else
                    {
                        push(Value());
                    }
                }
                else if (isString(source))
                {
                    ObjString* string = asString(source);
                    if (idx >= 0 && idx < string->length)
                    {
                        const char c = string->chars[idx];
                        ObjString* character = takeString(&c, 1);
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
            case OpCode::OP_STORE_SUBSCR:
            {
                // stack is: [...,source,index,item] and after: [item]
                // We can have: instance and string, or range|list|string and number
                Value item = pop();
                Value index = pop();
                Value source = pop();

                if (isInstance(source))
                {
                    if (!isString(index))
                    {
                        runtimeError("Fields can only be accessed by strings.");
                        return InterpretResult::INTERPRET_RUNTIME_ERROR;
                    }

                    ObjInstance* instance = asInstance(source);
                    ObjString* name = asString(index);

                    instance->fields.set(name, item);
                    push(item);
                }
                else
                {
                    if (!isNumber(index))
                    {
                        runtimeError("List index is not a number.");
                        return InterpretResult::INTERPRET_RUNTIME_ERROR;
                    }

                    const int idx = static_cast<int>(asNumber(index));

                    if (isList(source))
                    {
                        ObjList* list = asList(source);

                        // TODO: Maybe just reserve more space in the list?
                        if (!list->isInBounds(idx))
                        {
                            runtimeError("Invalid list index.");
                            return InterpretResult::INTERPRET_RUNTIME_ERROR;
                        }

                        list->setValue(idx, item);
                        push(item);
                    }
                    else if (isString(source))
                    {
                        if(!isString(item))
                        {
                            runtimeError("You can only assign characters.");
                            return InterpretResult::INTERPRET_RUNTIME_ERROR;
                        }

                        ObjString* str = asString(source);
                        ObjString* character = asString(item);

                        if (character->chars.length() != 1)
                        {
                            runtimeError("Invalid string length.");
                            return InterpretResult::INTERPRET_RUNTIME_ERROR;
                        }

                        if (idx >= 0 && idx < str->length)
                        {
                            str->chars[idx] = character->chars[0];
                        }
                        else
                        {
                            runtimeError("Invalid string index.");
                            return InterpretResult::INTERPRET_RUNTIME_ERROR;
                        }
                    }
                    else
                    {
                        runtimeError("Cannot store value.");
                        return InterpretResult::INTERPRET_RUNTIME_ERROR;
                    }
                }
                break;
            }
            case OpCode::OP_RANGE_IN_BOUNDS:
            {
                // stack is: [...,source,index] and after: [true|false]
                if (!isNumber(peek(0)))
                {
                    runtimeError("List index is not a number.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }

                const int idx = static_cast<int>(asNumber(pop()));
                const Value item = pop();

                if (isRange(item))
                {
                    ObjRange* range = asRange(item);
                    push(Value(range->isInBounds(idx)));
                }
                else if (isList(item))
                {
                    ObjList* list = asList(item);
                    push(Value(list->isInBounds(idx)));
                }
                else if (isString(item))
                {
                    ObjString* string = asString(item);
                    const bool isInRange = idx >= 0 && idx < string->length;
                    push(Value(isInRange));
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
                push(Value(isFalsey(pop())));
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
            case OpCode::OP_INVOKE:
            {
                ObjString* method = readString();
                const uint8_t argCount = readByte();
                if (!invoke(method, argCount))
                {
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }
                frame = &frames[frameCount - 1];
                break;
            }
            case OpCode::OP_INVOKE_LONG:
            {
                ObjString* method = readStringLong();
                const uint8_t argCount = readByte();
                if (!invoke(method, argCount))
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
                const Value result = pop();
                closeUpvalues(frame->slots);
                frameCount--;
                if (frameCount == 0)
                {
                    //pop();
                    return InterpretResult::INTERPRET_OK;
                }

                stackTop = frame->slots;
                push(result);
                frame = &frames[frameCount - 1];

                if (frameCount == depth)
                {
                    return InterpretResult::INTERPRET_OK;
                }
                break;
            }
            case OpCode::OP_CLASS:
                push(Value(newClass(readString())));
                break;
            case OpCode::OP_CLASS_LONG:
                push(Value(newClass(readStringLong())));
                break;
            case OpCode::OP_METHOD:
                defineMethod(readString());
                break;
            case OpCode::OP_METHOD_LONG:
                defineMethod(readStringLong());
                break;
        }
        static_assert(static_cast<int>(OpCode::COUNT) == 54, "Missing operations in the VM");
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
    push(Value(newNative(arity, function, false)));
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
    ObjString* b = asString(peek(0));
    ObjString* a = asString(peek(1));

    ObjString* concat = ::concatenate(a, b);

    pop();
    pop();
    push(Value(concat));
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
        case ObjType::BOUND_METHOD:
        {
            ObjBoundMethod* bound = asBoundMethod(callee);
            stackTop[-argCount - 1] = bound->receiver;
            return callValue(bound->method, argCount);
        }
        case ObjType::CLASS:
        {
            ObjClass* klass = asClass(callee);
            stackTop[-argCount - 1] = Value(newInstance(klass));

            if (!isNil(klass->initializer))
            {
                return callValue(klass->initializer, argCount);
            }
            else if (argCount != 0)
            {
                runtimeError("Expected 0 arguments but got %d.", argCount);
                return false;
            }

            return true;
        }
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

            const Value result = native->function(argCount, stackTop - (native->isMethod ? argCount + 1 : argCount), this);
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

bool VM::invokeFromClass(ObjClass* klass, ObjString* name, uint8_t argCount)
{
    Value method;
    if (!klass->methods.get(name, &method))
    {
        runtimeError("Undefined property '%s'.", name->chars.c_str());
        return false;
    }
    return callValue(method, argCount);
}

bool VM::invoke(ObjString* name, uint8_t argCount)
{
    const Value receiver = peek(argCount);

    if (!isInstance(receiver))
    {
        runtimeError("Only instances have methods.");
        return false;
    }

    ObjInstance* instance = asInstance(receiver);

    Value value;
    if (instance->fields.get(name, &value))
    {
        stackTop[-argCount - 1] = value;
        return callValue(value, argCount);
    }

    return invokeFromClass(instance->klass, name, argCount);
}

bool VM::bindMethod(ObjInstance* instance, ObjString* name)
{
    Value method;
    if (!instance->klass->methods.get(name, &method))
    {
        pop(); // Method name
        push(Value()); // Nil
        return false;
    }

    ObjBoundMethod* bound = newBoundMethod(Value(instance), method);
    pop(); // Method name
    push(Value(bound));
    return true;
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

void VM::defineMethod(ObjString* name)
{
    const Value method = peek(0);
    ObjClass* klass = asClass(peek(1));
    if (name->length == 4 && name->chars == "init")
    {
        klass->initializer = method;
    }
    else
    {
        klass->methods.set(name, method);
    }
    pop();
}
