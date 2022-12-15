#ifndef loxcpp_compiler_h
#define loxcpp_compiler_h

#include <string>
#include <array>
#include <set>

#include "Chunk.h"
#include "Scanner.h"

enum class Precedence : uint8_t
{
    NONE = 0,
    ASSIGNMENT,  // =
    OR,          // or
    AND,         // and
    EQUALITY,    // == !=
    COMPARISON,  // < > <= >=
    TERM,        // + -
    FACTOR,      // * /
    UNARY,       // ! -
    CALL,        // . ()
    PRIMARY
};

struct Parser
{
    Token current;
    Token previous;
    bool hadError = false;
    bool panicMode = false;
};

struct Local
{
    Token name;
    int depth = -1;
    bool constant = false;
};

struct CompilerScope
{
    CompilerScope()
        : localCount(0)
        , scopeDepth(0)
    {}

    std::array<Local, UINT8_COUNT> locals;
    int localCount;
    int scopeDepth;
};

class Compiler
{
    using ParseFn = void (Compiler::*)(bool canAssign);

    struct ParseRule
    {
        ParseRule(ParseFn prefix, ParseFn infix, Precedence precedence);

        ParseFn prefix;
        ParseFn infix;
        Precedence precedence;
    };

    using Rules = std::array<ParseRule, static_cast<size_t>(TokenType::EOFILE) + 1>;

    const Rules rules =
    {
      ParseRule(&Compiler::grouping,  nullptr,             Precedence::NONE),        // LEFT_PAREN    
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // RIGHT_PAREN   
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // LEFT_BRACE    
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // RIGHT_BRACE   
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // COMMA         
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // DOT           
      ParseRule(&Compiler::unary,     &Compiler::binary,   Precedence::TERM),        // MINUS         
      ParseRule(nullptr,              &Compiler::binary,   Precedence::TERM),        // PLUS          
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // SEMICOLON     
      ParseRule(nullptr,              &Compiler::binary,   Precedence::FACTOR),      // SLASH         
      ParseRule(nullptr,              &Compiler::binary,   Precedence::FACTOR),      // STAR          
      ParseRule(&Compiler::unary,     nullptr,             Precedence::NONE),        // BANG          
      ParseRule(nullptr,              &Compiler::binary,   Precedence::EQUALITY),    // BANG_EQUAL    
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // EQUAL         
      ParseRule(nullptr,              &Compiler::binary,   Precedence::EQUALITY),    // EQUAL_EQUAL   
      ParseRule(nullptr,              &Compiler::binary,   Precedence::COMPARISON),  // GREATER       
      ParseRule(nullptr,              &Compiler::binary,   Precedence::COMPARISON),  // GREATER_EQUAL 
      ParseRule(nullptr,              &Compiler::binary,   Precedence::COMPARISON),  // LESS          
      ParseRule(nullptr,              &Compiler::binary,   Precedence::COMPARISON),  // LESS_EQUAL    
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // PLUS_PLUS     
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // MINUS_MINUS    // TODO
      ParseRule(&Compiler::variable,  nullptr,             Precedence::NONE),        // IDENTIFIER     // TODO
      ParseRule(&Compiler::string,    nullptr,             Precedence::NONE),        // STRING        
      ParseRule(&Compiler::number,    nullptr,             Precedence::NONE),        // NUMBER        
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // AND           
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // CLASS         
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // ELSE          
      ParseRule(&Compiler::literal,   nullptr,             Precedence::NONE),        // FALSE         
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // FOR           
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // FUN           
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // IF            
      ParseRule(&Compiler::literal,   nullptr,             Precedence::NONE),        // NIL           
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // OR            
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // PRINT         
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // RETURN        
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // SUPER         
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // THIS          
      ParseRule(&Compiler::literal,   nullptr,             Precedence::NONE),        // TRUE          
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // VAR           
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // CONST         
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // WHILE         
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // ERROR         
      ParseRule(nullptr,              nullptr,             Precedence::NONE),        // EOFILE        
    };

public:

    Compiler(const std::string& source);

    void debugScanner();
    bool compile(Chunk* chunk);
    bool check(TokenType type);

    void advance();
    void consume(TokenType type, const char* message);
    bool match(TokenType type);
    void emitDWord(uint32_t byte);
    void emitByte(uint8_t byte);
    void emitBytes(uint8_t byte1, uint8_t byte2);
    void emitOpWithValue(OpCode shortOp, OpCode longOp, uint32_t value);
    void emitReturn();
    uint32_t makeConstant(Value value);
    void emitConstant(Value value);

    void endCompiler();

    void binary(bool canAssign);
    void literal(bool canAssign);
    void grouping(bool canAssign);
    void number(bool canAssign);
    void string(bool canAssign);
    void namedVariable(const Token& name, bool canAssign);
    void variable(bool canAssign);
    void unary(bool canAssign);
    void parsePrecedence(Precedence precedence);
    uint32_t identifierConstant(const Token& name);
    bool identifiersEqual(const Token& a, const Token& b);
    int resolveLocal(const CompilerScope& compilerScope, const Token& name);
    bool isLocalConst(const CompilerScope& compilerScope, int index);
    bool isGlobalConst(uint8_t index);
    void addLocal(const Token& name, bool isConstant);
    void declareVariable(bool isConstant);
    uint32_t parseVariable(const char* errorMessage, bool isConstant);
    void markInitialized();
    void defineVariable(uint32_t global, bool isConstant);
    const ParseRule* getRule(TokenType type) const;
    void expression();
    void block();
    void beginScope();
    void endScope();
    void varDeclaration(bool isConstant);
    void expressionStatement();
    void printStatement();
    void synchronize();
    void declaration();
    void statement();

    void errorAtCurrent(const std::string& message);
    void error(const std::string& message);
    void errorAt(const Token& token, const std::string& message);

    Chunk* currentChunk() { return compilingChunk; }

    Scanner scanner;
    Parser parser;
    CompilerScope compilerData;
    std::set<uint32_t> constGlobals;

    Chunk* compilingChunk;
    CompilerScope* current;
};

#endif
