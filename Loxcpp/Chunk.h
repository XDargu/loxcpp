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
    OP_GREATER,
    OP_LESS,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_PRINT,
    OP_RETURN,

    COUNT
};

typedef std::vector<uint8_t> ChunkInstructions;

struct Chunk
{
    Chunk()
    {
        for (int i = 0; i < 300; ++i)
        {
            addConstant(Value((double)i));
        }
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

    void writeConstant(Value value, int line)
    {
        uint32_t constant = addConstant(value);

        if (constant < 256)
        {
            write(OpCode::OP_CONSTANT, line);
            write(static_cast<uint8_t>(constant), line);
        }
        else
        {
            write(OpCode::OP_CONSTANT_LONG, line);

            // Convert constant to an array of 4 uint8_t
            uint8_t* vp = (uint8_t*)&constant;

            write(vp[0], line);
            write(vp[1], line);
            write(vp[2], line);
            write(vp[3], line);
        }
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