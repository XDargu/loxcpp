#include <iostream>
#include <iomanip>

#include "Debug.h"
#include "Value.h"

size_t simpleInstruction(const std::string& name, size_t offset)
{
    std::cout << name << std::endl;
    return offset + 1;
}

size_t byteInstruction(const char* name, const Chunk& chunk, size_t offset)
{
    const uint8_t slot = chunk.code[offset + 1];
    std::cout << name << " " << +slot << std::endl;
    return offset + 2;
}

size_t dwordInstruction(const char* name, const Chunk& chunk, size_t offset)
{
    const uint8_t* slotStart = &chunk.code[offset + 1];
    // Interpret the constant as the next 4 elements in the vector
    const uint32_t slot = *reinterpret_cast<const uint32_t*>(slotStart);

    std::cout << name << " " << +slot << std::endl;
    return offset + 2;
}

size_t constantInstruction(const std::string& name, const Chunk& chunk, size_t offset)
{
    const uint8_t constant = chunk.code[offset + 1];

    std::cout << name << "  " << +constant << "  ";
    const Value value = chunk.constants.values[constant];
    printValue(value);

    std::cout << std::endl;
    return offset + 2;
}

size_t constantLongInstruction(const std::string& name, const Chunk& chunk, size_t offset)
{
    const uint8_t* constantStart = &chunk.code[offset + 1];
    // Interpret the constant as the next 4 elements in the vector
    const uint32_t constant = *reinterpret_cast<const uint32_t*>(constantStart);

    std::cout << name << "  " << +constant << "  ";
    printValue(chunk.constants.values[constant]);

    std::cout << std::endl;
    return offset + 5;
}

void disassembleChunk(const Chunk& chunk, const char* name)
{
    std::cout << "==" << name << "==" << std::endl;

    for (size_t offset = 0; offset < chunk.code.size();)
    {
        offset = disassembleInstruction(chunk, offset);
    }
}

size_t disassembleInstruction(const Chunk& chunk, size_t offset)
{
    std::cout << std::setfill('0') << std::setw(4) << offset << " ";

    if (offset > 0 && chunk.lines[offset] == chunk.lines[offset - 1])
    {
        std::cout << "   | ";
    }
    else
    {
        std::cout << std::setfill('0') << std::setw(4) << chunk.lines[offset] << " ";
    }

    const OpCode instruction = static_cast<OpCode>(chunk.code[offset]);
    switch (instruction)
    {
    case OpCode::OP_CONSTANT:
        return constantInstruction("OP_CONSTANT", chunk, offset);
    case OpCode::OP_CONSTANT_LONG:
        return constantLongInstruction("OP_CONSTANT_LONG", chunk, offset);
    case OpCode::OP_NIL:
        return simpleInstruction("OP_NIL", offset);
    case OpCode::OP_TRUE:
        return simpleInstruction("OP_TRUE", offset);
    case OpCode::OP_FALSE:
        return simpleInstruction("OP_FALSE", offset);
    case OpCode::OP_POP:
        return simpleInstruction("OP_POP", offset);
    case OpCode::OP_GET_LOCAL:
        return byteInstruction("OP_GET_LOCAL", chunk, offset);
    case OpCode::OP_SET_LOCAL:
        return byteInstruction("OP_SET_LOCAL", chunk, offset);
    case OpCode::OP_GET_LOCAL_LONG:
        return dwordInstruction("OP_GET_LOCAL_LONG", chunk, offset);
    case OpCode::OP_SET_LOCAL_LONG:
        return dwordInstruction("OP_SET_LOCAL_LONG", chunk, offset);
    case OpCode::OP_GET_GLOBAL:
        return constantInstruction("OP_GET_GLOBAL", chunk, offset);
    case OpCode::OP_DEFINE_GLOBAL:
        return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OpCode::OP_SET_GLOBAL:
        return constantInstruction("OP_SET_GLOBAL", chunk, offset);
    case OpCode::OP_GET_GLOBAL_LONG:
        return constantLongInstruction("OP_GET_GLOBAL_LONG", chunk, offset);
    case OpCode::OP_DEFINE_GLOBAL_LONG:
        return constantLongInstruction("OP_DEFINE_GLOBAL_LONG", chunk, offset);
    case OpCode::OP_SET_GLOBAL_LONG:
        return constantLongInstruction("OP_SET_GLOBAL_LONG", chunk, offset);
    case OpCode::OP_EQUAL:
        return simpleInstruction("OP_EQUAL", offset);
    case OpCode::OP_GREATER:
        return simpleInstruction("OP_GREATER", offset);
    case OpCode::OP_LESS:
        return simpleInstruction("OP_LESS", offset);
    case OpCode::OP_NEGATE:
        return simpleInstruction("OP_NEGATE", offset);
    case OpCode::OP_ADD:
        return simpleInstruction("OP_ADD", offset);
    case OpCode::OP_SUBTRACT:
        return simpleInstruction("OP_SUBTRACT", offset);
    case OpCode::OP_MULTIPLY:
        return simpleInstruction("OP_MULTIPLY", offset);
    case OpCode::OP_DIVIDE:
        return simpleInstruction("OP_DIVIDE", offset);
    case OpCode::OP_NOT:
        return simpleInstruction("OP_NOT", offset);
    case OpCode::OP_PRINT:
        return simpleInstruction("OP_PRINT", offset);
    case OpCode::OP_RETURN:
        return simpleInstruction("OP_RETURN", offset);
    default:
        std::cout << "Unknown opcode " << static_cast<uint8_t>(instruction) << std::endl;
        return offset + 1;
    }

    static_assert(static_cast<int>(OpCode::COUNT) == 27, "Missing operations in the Debug");
}