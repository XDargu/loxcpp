#ifndef loxcpp_chunk_h
#define loxcpp_chunk_h

#include <vector>

#include "Common.h"
#include "Value.h"

enum class OpCode : uint8_t
{
    OP_CONSTANT = 0,
    OP_CONSTANT_LONG,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_LOCAL_LONG,
    OP_SET_LOCAL_LONG,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_GLOBAL_LONG,
    OP_DEFINE_GLOBAL_LONG,
    OP_SET_GLOBAL_LONG,
    OP_EQUAL,
    OP_MATCH,
    OP_GREATER,
    OP_LESS,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MODULO,
    OP_RANGE,
    OP_NOT,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_RETURN,

    COUNT
};

typedef std::vector<uint8_t> ChunkInstructions;

struct Chunk
{
    Chunk()
    {
#ifdef FORCE_LONG_OPS
        for (int i = 0; i < 300; ++i)
        {
            addConstant(Value((double)i));
        }
#endif
    }

    void write(OpCode byte, int line)
    {
        code.push_back(static_cast<uint8_t>(byte));
        lines.push_back(line);
    }

    void write(uint8_t byte, int line)
    {
        code.push_back(byte);
        lines.push_back(line);
    }

    uint32_t addConstant(Value value)
    {
        auto result = std::find(constants.values.begin(), constants.values.end(), value);
        if (result != constants.values.end())
            return static_cast<uint32_t>(std::distance(constants.values.begin(), result));

        constants.values.push_back(value);
        return static_cast<uint32_t>(constants.values.size() - 1);
    }

    ChunkInstructions code;
    std::vector<int> lines;
    ValueArray constants;
};

#endif