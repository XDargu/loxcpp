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
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
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
    OP_INCREMENT,
    OP_RANGE,
    OP_RANGE_VALUE,
    OP_RANGE_IN_BOUNDS,
    OP_NOT,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_CLOSURE,
    OP_CLOSURE_LONG,
    OP_CLOSE_UPVALUE,
    OP_RETURN,

    COUNT
};

typedef std::vector<uint8_t> ChunkInstructions;

struct Chunk
{
    Chunk();

    void write(OpCode byte, int line);
    void write(uint8_t byte, int line);

    uint32_t addConstant(Value value);

    ChunkInstructions code;
    std::vector<int> lines;
    ValueArray constants;
};

#endif