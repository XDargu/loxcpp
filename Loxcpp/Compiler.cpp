#include "Compiler.h"

#include <iostream>
#include <iomanip>

#include "Scanner.h"
#include "Debug.h"
#include "Object.h"

uint8_t OpByte(OpCode opCode) { return static_cast<uint8_t>(opCode); }
Precedence nextPrecedence(Precedence precedence) { return static_cast<Precedence>(static_cast<int>(precedence) + 1); }

Compiler::ParseRule::ParseRule(ParseFn prefix, ParseFn infix, Precedence precedence)
    : prefix(prefix)
    , infix(infix)
    , precedence(precedence)
{}

void Compiler::debugScanner()
{
    int line = -1;
    for (;;)
    {
        const Token token = scanner.scanToken();
        if (token.line != line)
        {
            std::cout << std::setfill('0') << std::setw(4) << token.line << " ";
            line = token.line;
        }
        else
        {
            std::cout <<  "   | ";
        }
        std::cout << tokenTypeToString(token.type) << " " << token.toString() << std::endl;

        std::cout << std::endl;

        if (token.type == TokenType::EOFILE) break;
    }
}

bool Compiler::compile(Chunk* chunk)
{
    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    while (!match(TokenType::EOFILE))
    {
        declaration();
    }

    endCompiler();

    return !parser.hadError;
}

bool Compiler::check(TokenType type)
{
    return parser.current.type == type;
}

Compiler::Compiler(const std::string& source)
    : scanner(source)
    , compilingChunk(nullptr)
    , compilerData()
    , current(&compilerData)
{
}

void Compiler::advance()
{
    parser.previous = parser.current;

    for (;;)
    {
        parser.current = scanner.scanToken();
        if (parser.current.type != TokenType::ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

void Compiler::consume(TokenType type, const char* message)
{
    if (parser.current.type == type)
    {
        advance();
        return;
    }

    errorAtCurrent(message);
}

bool Compiler::match(TokenType type)
{
    if (!check(type)) return false;
    advance();
    return true;
}

void Compiler::emitDWord(uint32_t byte)
{
    // Convert constant to an array of 4 uint8_t
    const uint8_t* vp = (uint8_t*)&byte;

    emitByte(vp[0]);
    emitByte(vp[1]);
    emitByte(vp[2]);
    emitByte(vp[3]);
}

void Compiler::emitByte(uint8_t byte)
{
    currentChunk()->write(byte, parser.previous.line);
}

void Compiler::emitBytes(uint8_t byte1, uint8_t byte2)
{
    emitByte(byte1);
    emitByte(byte2);
}

void Compiler::emitOpWithValue(OpCode shortOp, OpCode longOp, uint32_t value)
{
    if (value > UINT8_MAX)
    {
        emitByte(OpByte(longOp));
        emitDWord(value);
    }
    else
    {
        emitBytes(OpByte(shortOp), static_cast<uint8_t>(value));
    }
}

void Compiler::emitReturn()
{
    emitByte(OpByte(OpCode::OP_RETURN));
}

uint32_t Compiler::makeConstant(Value value)
{
    return currentChunk()->addConstant(value);
}

void Compiler::emitConstant(Value value)
{
    const uint32_t constant = makeConstant(value);
    emitOpWithValue(OpCode::OP_CONSTANT, OpCode::OP_CONSTANT_LONG, constant);
}

void Compiler::endCompiler()
{
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError)
    {
        disassembleChunk(*currentChunk(), "code");
    }
#endif
    emitReturn();
}

void Compiler::binary(bool canAssign)
{
    const TokenType operatorType = parser.previous.type;
    const ParseRule* rule = getRule(operatorType);
    parsePrecedence(nextPrecedence(rule->precedence));

    switch (operatorType)
    {
        case TokenType::BANG_EQUAL:    emitBytes(OpByte(OpCode::OP_EQUAL), OpByte(OpCode::OP_NOT)); break;
        case TokenType::EQUAL_EQUAL:   emitByte(OpByte(OpCode::OP_EQUAL)); break;
        case TokenType::GREATER:       emitByte(OpByte(OpCode::OP_GREATER)); break;
        case TokenType::GREATER_EQUAL: emitBytes(OpByte(OpCode::OP_LESS), OpByte(OpCode::OP_NOT)); break;
        case TokenType::LESS:          emitByte(OpByte(OpCode::OP_LESS)); break;
        case TokenType::LESS_EQUAL:    emitBytes(OpByte(OpCode::OP_GREATER), OpByte(OpCode::OP_NOT)); break;
        case TokenType::PLUS:          emitByte(OpByte(OpCode::OP_ADD)); break;
        case TokenType::MINUS:         emitByte(OpByte(OpCode::OP_SUBTRACT)); break;
        case TokenType::STAR:          emitByte(OpByte(OpCode::OP_MULTIPLY)); break;
        case TokenType::SLASH:         emitByte(OpByte(OpCode::OP_DIVIDE)); break;
        default: return; // Unreachable.
    }
}

void Compiler::literal(bool canAssign)
{
    switch (parser.previous.type)
    {
        case TokenType::FALSE:         emitByte(OpByte(OpCode::OP_FALSE)); break;
        case TokenType::NIL:           emitByte(OpByte(OpCode::OP_NIL)); break;
        case TokenType::TRUE:          emitByte(OpByte(OpCode::OP_TRUE)); break;
        default: return; // Unreachable.
    }
}

void Compiler::grouping(bool canAssign)
{
    expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
}

void Compiler::number(bool canAssign)
{
    const double value = strtod(parser.previous.start, nullptr);
    emitConstant(Value(value));
}

void Compiler::string(bool canAssign)
{
    emitConstant(Value(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

void Compiler::namedVariable(const Token& name, bool canAssign)
{
    OpCode getOp;
    OpCode getOpLong;
    OpCode setOp;
    OpCode setOpLong;
    int arg = resolveLocal(*current, name);

    const bool isLocal = arg != -1;

    if (isLocal)
    {
        getOp = OpCode::OP_GET_LOCAL;
        getOpLong = OpCode::OP_GET_LOCAL_LONG;
        setOp = OpCode::OP_SET_LOCAL;
        setOpLong = OpCode::OP_SET_LOCAL_LONG;
    }
    else
    {
        arg = identifierConstant(name);
        getOp = OpCode::OP_GET_GLOBAL;
        getOpLong = OpCode::OP_GET_GLOBAL_LONG;
        setOp = OpCode::OP_SET_GLOBAL;
        setOpLong = OpCode::OP_SET_GLOBAL_LONG;
    }

    if (canAssign && match(TokenType::EQUAL))
    {
        if (isLocal)
        {
            if (isLocalConst(*current, arg))
                error("Can't reassign a const variable");
        }
        else
        {
            if (isGlobalConst(arg))
                error("Can't reassign a const variable");
        }

        expression();
        emitOpWithValue(setOp, setOpLong, arg);
    }
    else
    {
        emitOpWithValue(getOp, getOpLong, arg);
    }
}

void Compiler::variable(bool canAssign)
{
    namedVariable(parser.previous, canAssign);
}

void Compiler::unary(bool canAssign)
{
    const TokenType operatorType = parser.previous.type;

    // Compile the operand.
    parsePrecedence(Precedence::UNARY);

    // Emit the operator instruction.
    switch (operatorType)
    {
        case TokenType::MINUS: emitByte(OpByte(OpCode::OP_NEGATE)); break;
        case TokenType::BANG: emitByte(OpByte(OpCode::OP_NOT)); break;
        default: return; // Unreachable.
    }
}

void Compiler::parsePrecedence(Precedence precedence)
{
    advance();
    const ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == nullptr)
    {
        error("Expect expression.");
        return;
    }

    const bool canAssign = precedence <= Precedence::ASSIGNMENT;
    (this->*prefixRule)(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence)
    {
        advance();
        const ParseFn infixRule = getRule(parser.previous.type)->infix;
        (this->*infixRule)(canAssign);
    }

    if (canAssign && match(TokenType::EQUAL))
    {
        error("Invalid assignment target.");
    }
}

// TODO: This atm can return constants of 32 bits! But nothing else is taking that into account
uint32_t Compiler::identifierConstant(const Token& name)
{
    return makeConstant(Value(copyString(name.start, name.length)));
}

bool Compiler::identifiersEqual(const Token& a, const Token& b)
{
    if (a.length != b.length) return false;
    return memcmp(a.start, b.start, a.length) == 0;
}

int Compiler::resolveLocal(const CompilerScope& compilerScope, const Token& name)
{
    for (int i = compilerScope.localCount - 1; i >= 0; i--)
    {
        const Local& local = compilerScope.locals[i];
        if (identifiersEqual(name, local.name))
        {
            const bool isInOwnInitializer = local.depth == -1;
            if (isInOwnInitializer)
            {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

bool Compiler::isLocalConst(const CompilerScope& compilerScope, int index)
{
    const Local& local = compilerScope.locals[index];
    return local.constant;
}

bool Compiler::isGlobalConst(uint8_t index)
{
    return constGlobals.find(index) != constGlobals.end();
}

void Compiler::addLocal(const Token& name, bool isConstant)
{
    if (current->localCount == UINT8_COUNT)
    {
        error("Too many local variables in function.");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->constant = isConstant;
}

void Compiler::declareVariable(bool isConstant)
{
    if (current->scopeDepth == 0) return;

    const Token& name = parser.previous;

    for (int i = current->localCount - 1; i >= 0; i--)
    {
        const Local& local = current->locals[i];
        if (local.depth != -1 && local.depth < current->scopeDepth)
        {
            break;
        }

        if (identifiersEqual(name, local.name))
        {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(name, isConstant);
}

 uint32_t Compiler::parseVariable(const char* errorMessage, bool isConstant)
{
    consume(TokenType::IDENTIFIER, errorMessage);

    declareVariable(isConstant);
    if (current->scopeDepth > 0) return 0;

    const uint32_t constant = identifierConstant(parser.previous);
    if (isConstant)
    {
        constGlobals.insert(constant);
    }

    return constant;
}

 void Compiler::markInitialized()
 {
     current->locals[current->localCount - 1].depth = current->scopeDepth;
 }

void Compiler::defineVariable(uint32_t global, bool isConstant)
{
    if (current->scopeDepth > 0)
    {
        markInitialized();
        return;
    }

    emitOpWithValue(OpCode::OP_DEFINE_GLOBAL, OpCode::OP_DEFINE_GLOBAL_LONG, global);
}

const Compiler::ParseRule* Compiler::getRule(TokenType type) const
{
    return &rules[static_cast<int>(type)];
}

void Compiler::expression()
{
    parsePrecedence(Precedence::ASSIGNMENT);
}

void Compiler::block()
{
    while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::EOFILE))
    {
        declaration();
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");
}

void Compiler::beginScope()
{
    current->scopeDepth++;
}

void Compiler::endScope()
{
    current->scopeDepth--;

    while (current->localCount > 0 &&
        current->locals[current->localCount - 1].depth > current->scopeDepth)
    {
        emitByte(OpByte(OpCode::OP_POP));
        current->localCount--;
    }
}

void Compiler::varDeclaration(bool isConstant)
{
    const uint32_t global = parseVariable("Expect variable name.", isConstant);

    if (match(TokenType::EQUAL))
    {
        expression();
    }
    else {
        emitByte(OpByte(OpCode::OP_NIL));
    }
    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global, isConstant);
}

void Compiler::expressionStatement()
{
    expression();
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    emitByte(OpByte(OpCode::OP_POP));
}

void Compiler::printStatement()
{
    expression();
    consume(TokenType::SEMICOLON, "Expect ';' after value.");
    emitByte(OpByte(OpCode::OP_PRINT));
}

void Compiler::synchronize()
{
    parser.panicMode = false;

    while (parser.current.type != TokenType::EOFILE)
    {
        if (parser.previous.type == TokenType::SEMICOLON) return;
        switch (parser.current.type)
        {
            case TokenType::CLASS:
            case TokenType::FUN:
            case TokenType::VAR:
            case TokenType::CONST:
            case TokenType::FOR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::PRINT:
            case TokenType::RETURN:
                return;

            default:
                ; // Do nothing.
        }

        advance();
    }
}

void Compiler::declaration()
{
    if (match(TokenType::VAR))
    {
        varDeclaration(false);
    }
    else if (match(TokenType::CONST))
    {
        varDeclaration(true);
    }
    else
    {
        statement();
    }

    if (parser.panicMode) synchronize();
}

void Compiler::statement()
{
    if (match(TokenType::PRINT))
    {
        printStatement();
    }
    else if (match(TokenType::LEFT_BRACE))
    {
        beginScope();
        block();
        endScope();
    }
    else
    {
        expressionStatement();
    }
}

void Compiler::errorAtCurrent(const std::string& message)
{
    errorAt(parser.current, message);
}

void Compiler::error(const std::string& message)
{
    errorAt(parser.previous, message);
}

void Compiler::errorAt(const Token& token, const std::string& message)
{
    if (parser.panicMode) return;
    parser.panicMode = true;

    std::cerr << "[line " << token.line << "] Error";

    if (token.type == TokenType::EOFILE)
    {
        std::cerr << " at end";
    }
    else if (token.type == TokenType::ERROR)
    {
        // Nothing.
    }
    else
    {
        std::cerr << " at " << token.toString();
    }

    std::cerr << ": " << message << std::endl;
    parser.hadError = true;
}
